#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Constants
const int SCREEN_WIDTH = 1440;
const int SCREEN_HEIGHT = 810;
const float PLAYER_SPEED = 28.0f;     
const float PLAYER_ACCEL = 140.0f;    
const float PLAYER_FRICTION = 1.8f;   
const float SPRINT_MULT = 1.4f;
const float ROLL_SPEED = 60.0f;       
const float ROLL_DURATION = 0.35f;
const float ROLL_COST = 30.0f;
const float BULLET_SPEED_BASE = 30.0f; 
const float ENEMY_BULLET_SPEED = 22.0f; 
const float PARRY_WINDOW = 0.25f;
const float PARRY_RANGE = 8.0f;
const float PARRY_COST = 35.0f;
const float CAMERA_HEIGHT = 35.0f;
const float CAMERA_DISTANCE = 25.0f;

// Game State
enum GameState { TITLE, PLAYING, SANCTUARY_MENU, PAUSED, DEAD, SHOP_MENU, WEAPON_SELECT, VICTORY };
enum EquipmentSlot { WEAPON, ARMOR, TALISMAN, RING };
enum EquipmentRarity { COMMON, RARE, EPIC, LEGENDARY };
enum EnemyType { HOLLOWED, ASHBOUND, WATCHER, WHISPERER, SPIRAL, BOSS_KEEPER, GLITCH_SPECTRE };

enum WeaponType {
    WEAPON_PISTOL, WEAPON_REVOLVER, WEAPON_BURST_RIFLE, WEAPON_SHOTGUN, WEAPON_RIFLE, 
    WEAPON_SMG, WEAPON_DUAL_PISTOLS, WEAPON_FLAMETHROWER, WEAPON_RAILGUN, WEAPON_LAUNCHER, 
    WEAPON_LIGHTNING_GUN, WEAPON_DIVINE_BEAM  
};

struct WeaponData {
    WeaponType type;
    std::string name;
    std::string description;
    float fireRate;
    float damage;
    float bulletSpeed;
    int projectileCount;
    float bulletSize;
    Color bulletColor;
    bool piercing;
    bool explosive;
    bool homing;
    int unlockCost;      
    int unlockWave;      
    bool unlocked;
};

struct Equipment {
    std::string name;
    EquipmentSlot slot;
    EquipmentRarity rarity;
    int bonusHealth, bonusStamina, bonusDamage;
    float bonusFireRate, bonusSpeed;
    int bonusGraceFind;
    bool hasLifesteal, hasExplosiveShots, hasDoubleDamage, hasPiercing, hasRapidFire, hasHomingShots;
    int sellValue, buyValue;
    bool equipped;
};

struct Token { Vector3 pos; int value; Color color; float lifetime; };
struct EquipmentDrop { Vector3 pos; Equipment equipment; float lifetime; bool collected; };
struct Bullet { Vector3 pos, vel; Color color; float lifetime; bool playerBullet, reflected; float size, damage; };
struct Particle { Vector3 pos, vel; Color color; float lifetime, maxLifetime, size; };
struct Ghost { Vector3 pos; float rotation, lifetime; };
struct GraceOrb { Vector3 pos; float timer; int value; };
struct Item { Vector3 pos; int type; bool collected; std::string name; };

struct Player {
    Vector3 pos, vel; float rotation;
    int vigor, will, faith, strength, level;
    int health, maxHealth; float stamina; int maxStamina;
    float lampFaith, maxLampFaith; int flasks, maxFlasks, grace, lightTokens; 
    float syncMeter, maxSyncMeter;
    WeaponType currentWeapon;
    Equipment *equippedArmor, *equippedTalisman, *equippedRing;
    std::vector<Equipment> inventory;
    bool needsReboot, isRolling, isParrying, lampActive, isHealing;
    float rollTimer, parryTimer, invulnTimer, healTimer, shootCooldown, shootRate, bulletSpeed, baseDamage, moveSpeed;
    Vector3 rollDir, deathPos; int graceAtDeath, kills, combo, score;
    Vector2 mobileMoveDir; bool mobileFireHeld, mobileDashPressed;
};

struct Enemy {
    EnemyType type; Vector3 pos, vel, startPos; float rotation;
    int health, maxHealth; bool isAlive; float defeatTimer, shootTimer, shootCooldown, moveSpeed, scale;
    Color color; int graceReward; bool isBoss; int bossPhase; float patternAngle, teleportTimer; 
    WeaponType weaponDrop; bool hasWeaponDrop;
    float stamina, maxStamina, actionTimer, abilityCooldown; int aiState; Vector3 chargeDir; bool hasDropped;
};

struct Sanctuary { Vector3 pos; std::string name; bool discovered; float radius; };
struct Structure { Vector3 pos, size; Color color; };

struct GameWorld {
    std::vector<Bullet> bullets; std::vector<Enemy> enemies; std::vector<Particle> particles;
    std::vector<GraceOrb> graceOrbs; std::vector<Item> items; std::vector<Sanctuary> sanctuaries;
    std::vector<Token> tokens; std::vector<EquipmentDrop> equipmentDrops; std::vector<Equipment> shopInventory;
    std::vector<WeaponData> weaponArsenal; std::vector<Ghost> ghosts; std::vector<Structure> ruins;
    Sanctuary* currentSanctuary; bool hasGraceToRecover, waveJustCompleted; int wave;
    std::string message; float messageTimer;
    int selectedShopItem, selectedInventoryItem, selectedWeapon, selectedSanctuaryOption;
    std::vector<WeaponType> availableUnlocks; 
};

// Globals
GameState state = TITLE; Player player; GameWorld world; Camera3D camera;
Mesh particleMesh, bulletMesh; Material instanceMaterial; Shader bloomShader;
RenderTexture2D target, minimapTarget; Texture2D floorTexture;
bool debugMode = false; float screenShake = 0.0f;

// Shaders
const char* bloomVs = R"(
#version 330
in vec3 vertexPosition; in vec2 vertexTexCoord; in vec4 vertexColor;
out vec2 fragTexCoord; out vec4 fragColor;
uniform mat4 mvp;
void main() { fragTexCoord = vertexTexCoord; fragColor = vertexColor; gl_Position = mvp*vec4(vertexPosition, 1.0); }
)";
const char* bloomFs = R"(
#version 330
in vec2 fragTexCoord; in vec4 fragColor; out vec4 finalColor;
uniform sampler2D texture0; uniform float threshold = 0.8;
void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    float brightness = dot(texelColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > threshold) finalColor = texelColor; else finalColor = vec4(0.0, 0.0, 0.0, 1.0);
}
)";

// Forward Declarations
void UpdateDrawFrame(); void InitGame(); void InitPlayer(); void SpawnWave(int wave);
void UpdateGame(float dt); void UpdatePlayer(float dt); void UpdateEnemies(float dt); void UpdateBullets(float dt); void UpdateParticles(float dt);
void CollectGrace(float dt); void UpdateCamera(); Vector3 GetAimPoint();
void FireBullet(Vector3 pos, Vector3 vel, Color col, bool playerBullet, float damage, float size);
void SpawnParticles(Vector3 pos, Color col, int count, float speed);
void DropGrace(Vector3 pos, int amount); void DropTokens(Vector3 pos, int amount); void DropEquipment(Vector3 pos, int waveNum);
Equipment GenerateRandomEquipment(int waveNum); void EquipItem(Equipment* item); void UnequipSlot(EquipmentSlot slot);
void RecalculatePlayerStats(); void InitializeShop(); void InitializeWeaponArsenal();
void UnlockAndEquipWeapon(WeaponType type); void SwitchWeapon(WeaponType type);
WeaponData* GetCurrentWeaponData(); Color GetRarityColor(EquipmentRarity rarity);
Color GetWaveColor() { 
    if (world.wave <= 5) return (Color){ 0, 255, 255, 255 }; 
    if (world.wave <= 10) return (Color){ 255, 80, 0, 255 };  
    return GOLD; 
}
void DamagePlayer(int damage); void PlayerNeedsReboot(); void RebootSystem();
int GetUpgradeCost(int level); void DrawGame3D(); void DrawHUD(); void DrawMinimap(); void DrawMobileControls();
void RefreshMinimapRuins(); void DrawCrosshair(); void DrawSanctuaryMenu(); void DrawShopMenu();
void DrawWeaponSelectMenu(); void DrawDeathScreen(); void DrawVictoryScreen(); void ShakeScreen(float intensity);

// Main Implementation
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "THE LAST LIGHT: DIVINE RECKONING");
    InitGame();
#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60); while(!WindowShouldClose()) UpdateDrawFrame();
#endif
    CloseWindow(); return 0;
}

void UpdateDrawFrame() {
    float dt = GetFrameTime();
    player.mobileMoveDir = {0, 0}; player.mobileFireHeld = false;
    int touchCount = GetTouchPointCount();
    if (touchCount > 0) {
        for (int i = 0; i < touchCount; i++) {
            Vector2 touch = GetTouchPosition(i);
            if (touch.x < 400 && touch.y > SCREEN_HEIGHT - 400) {
                Vector2 center = { 200, (float)SCREEN_HEIGHT - 200 };
                Vector2 dir = Vector2Subtract(touch, center);
                if (Vector2Length(dir) > 20.0f) player.mobileMoveDir = Vector2Scale(Vector2Normalize(dir), 1.0f);
            }
            if (touch.x > SCREEN_WIDTH - 400 && touch.y > SCREEN_HEIGHT - 400) {
                if (Vector2Distance(touch, {(float)SCREEN_WIDTH - 100, (float)SCREEN_HEIGHT - 100}) < 60.0f) player.mobileDashPressed = true;
                else player.mobileFireHeld = true;
            }
        }
    }

    if(IsKeyPressed(KEY_ESCAPE)) { if(state == PLAYING) state = PAUSED; else if(state == PAUSED || state == SANCTUARY_MENU) state = PLAYING; }
    if(IsKeyPressed(KEY_F1)) { debugMode = !debugMode; world.message = debugMode ? "DEBUG: ON" : "DEBUG: OFF"; world.messageTimer = 1.0f; }

    switch(state) {
        case TITLE: if(IsKeyPressed(KEY_ENTER) || (touchCount > 0 && GetTouchPosition(0).y > 400)) { state = PLAYING; world.wave = 1; SpawnWave(1); } break;
        case PLAYING: UpdateGame(dt); break;
        case SANCTUARY_MENU:
            if(IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) world.selectedSanctuaryOption = (world.selectedSanctuaryOption - 1 + 5) % 5;
            if(IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) world.selectedSanctuaryOption = (world.selectedSanctuaryOption + 1) % 5;
            if(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (world.selectedSanctuaryOption == 4) state = PLAYING;
                else {
                    int* stats[] = {&player.vigor, &player.will, &player.faith, &player.strength};
                    if (player.grace >= GetUpgradeCost(*stats[world.selectedSanctuaryOption])) {
                        player.grace -= GetUpgradeCost(*stats[world.selectedSanctuaryOption]);
                        (*stats[world.selectedSanctuaryOption])++; RecalculatePlayerStats();
                    }
                }
            }
            if(IsKeyPressed(KEY_ESCAPE)) state = PLAYING;
            break;
        case DEAD: if(IsKeyPressed(KEY_R)) { RebootSystem(); state = PLAYING; } break;
        default: break;
    }

    BeginTextureMode(target);
    ClearBackground(BLACK);
    if(state == PLAYING || state == PAUSED || state == SANCTUARY_MENU || state == DEAD) {
        BeginMode3D(camera); DrawGame3D(); EndMode3D();
        DrawCrosshair(); DrawHUD();
    }
    if(state == TITLE) {
        DrawText("VOID LIGHT", SCREEN_WIDTH/2 - 200, 200, 60, SKYBLUE);
        DrawText("PRESS ENTER TO START", SCREEN_WIDTH/2 - 150, 400, 20, RAYWHITE);
    }
    if(state == DEAD) { DrawText("SYSTEM REBOOT REQUIRED", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2, 30, RED); DrawText("PRESS R TO REBOOT", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50, 20, RAYWHITE); }
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    BeginShaderMode(bloomShader);
    DrawTextureRec(target.texture, {0,0,(float)target.texture.width, (float)-target.texture.height}, {0,0}, WHITE);
    EndShaderMode();
    DrawFPS(10, 10);
    EndDrawing();
}

void InitGame() {
    particleMesh = GenMeshSphere(1.0f, 4, 4); bulletMesh = GenMeshSphere(1.0f, 6, 6);
    instanceMaterial = LoadMaterialDefault(); bloomShader = LoadShaderFromMemory(bloomVs, bloomFs);
    target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    minimapTarget = LoadRenderTexture(200, 200);
    camera = {{0, CAMERA_HEIGHT, -CAMERA_DISTANCE}, {0,0,0}, {0,1,0}, 60, CAMERA_PERSPECTIVE};
    world = {}; world.wave = 0; InitializeWeaponArsenal();
    world.sanctuaries.push_back({{0,0,0}, "Core", true, 5.0f});
    world.currentSanctuary = &world.sanctuaries[0];
    InitPlayer();
}

void InitPlayer() {
    player = {}; player.pos = world.currentSanctuary->pos;
    player.vigor = 10; player.will = 10; player.faith = 10; player.strength = 10;
    player.health = 100; player.maxHealth = 100; player.stamina = 120; player.maxStamina = 120;
    player.currentWeapon = WEAPON_PISTOL; RecalculatePlayerStats();
}

void SpawnWave(int wave) {
    world.enemies.clear(); world.bullets.clear();
    for(int i=0; i<10 + wave*5; i++) {
        Enemy e = {HOLLOWED, {(float)GetRandomValue(-200, 200), 0, (float)GetRandomValue(-200, 200)}};
        e.isAlive = true; e.health = 50; e.maxHealth = 50; e.scale = 1.0f; e.color = VIOLET; e.moveSpeed = 5.0f;
        e.hasWeaponDrop = (GetRandomValue(0, 100) < 20); e.weaponDrop = WEAPON_RAILGUN;
        world.enemies.push_back(e);
    }
}

void UpdateGame(float dt) {
    UpdateCamera(); UpdatePlayer(dt); UpdateEnemies(dt); UpdateBullets(dt); UpdateParticles(dt);
    if(world.messageTimer > 0) world.messageTimer -= dt;
}

void UpdatePlayer(float dt) {
    player.invulnTimer = std::max(0.0f, player.invulnTimer - dt);
    player.shootCooldown = std::max(0.0f, player.shootCooldown - dt);
    player.stamina = std::min(player.stamina + 25.0f * dt, (float)player.maxStamina);
    Vector3 toAim = Vector3Subtract(GetAimPoint(), player.pos); toAim.y = 0;
    if(Vector3Length(toAim) > 0.1f) player.rotation = atan2f(toAim.x, toAim.z);
    Vector3 input = {0,0,0};
    if(IsKeyDown(KEY_W)) input.z += 1; if(IsKeyDown(KEY_S)) input.z -= 1;
    if(IsKeyDown(KEY_D)) input.x += 1; if(IsKeyDown(KEY_A)) input.x -= 1;
    input.x += player.mobileMoveDir.x; input.z -= player.mobileMoveDir.y;
    Vector3 camDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position)); camDir.y = 0;
    Vector3 moveDir = Vector3Add(Vector3Scale(camDir, input.z), Vector3Scale(Vector3CrossProduct(camDir, {0,1,0}), input.x));
    if(Vector3Length(moveDir) > 0.1f) {
        player.vel = Vector3Add(player.vel, Vector3Scale(Vector3Normalize(moveDir), PLAYER_ACCEL * dt));
    }
    player.vel = Vector3Scale(player.vel, 1.0f / (1.0f + PLAYER_FRICTION * dt));
    player.pos = Vector3Add(player.pos, Vector3Scale(player.vel, dt));
    if((IsMouseButtonDown(MOUSE_LEFT_BUTTON) || player.mobileFireHeld) && player.shootCooldown <= 0) {
        FireBullet(Vector3Add(player.pos, Vector3Scale(Vector3Normalize(toAim), 2.0f)), Vector3Scale(Vector3Normalize(toAim), player.bulletSpeed), GetCurrentWeaponData()->bulletColor, true, player.baseDamage, 0.4f);
        player.shootCooldown = player.shootRate;
    }
    if(IsKeyPressed(KEY_Q) && player.stamina >= PARRY_COST) { player.isParrying = true; player.parryTimer = PARRY_WINDOW; player.stamina -= PARRY_COST; }
    if(player.isParrying) { player.parryTimer -= dt; if(player.parryTimer <= 0) player.isParrying = false; }
}

void UpdateEnemies(float dt) {
    const float GRID_SIZE = 50.0f; std::vector<Bullet*> bulletGrid[20][20];
    for (auto& b : world.bullets) {
        if (!b.playerBullet) continue;
        int gx = (int)((b.pos.x + 500.0f) / GRID_SIZE); int gz = (int)((b.pos.z + 500.0f) / GRID_SIZE);
        if (gx >= 0 && gx < 20 && gz >= 0 && gz < 20) bulletGrid[gx][gz].push_back(&b);
    }
    for(auto& e : world.enemies) {
        if(!e.isAlive) continue;
        if (e.aiState == 5) {
            if (!e.hasDropped) {
                e.hasDropped = true;
                if (e.hasWeaponDrop) {
                    for (auto& w : world.weaponArsenal) if (w.type == e.weaponDrop) { w.unlocked = true; world.message = "Protocol Unlocked: " + w.name; world.messageTimer = 3.0f; break; }
                }
            }
            e.defeatTimer -= dt; if (e.defeatTimer <= 0) e.isAlive = false; continue;
        }
        Vector3 toPlayer = Vector3Subtract(player.pos, e.pos); toPlayer.y = 0;
        e.pos = Vector3Add(e.pos, Vector3Scale(Vector3Normalize(toPlayer), e.moveSpeed * dt));
    }
}

void UpdateBullets(float dt) {
    for (auto it = world.bullets.begin(); it != world.bullets.end();) {
        it->lifetime -= dt;
        if (!it->playerBullet && player.isParrying && Vector3Distance(it->pos, player.pos) < PARRY_RANGE) {
            it->vel = Vector3Scale(Vector3Negate(it->vel), 2.0f); it->playerBullet = true; it->color = GOLD; it->damage *= 2;
        }
        it->pos = Vector3Add(it->pos, Vector3Scale(it->vel, dt));
        bool destroyed = it->lifetime <= 0;
        if (!destroyed && it->playerBullet) {
            for (auto& e : world.enemies) {
                if (e.isAlive && e.aiState != 5 && Vector3Distance(it->pos, e.pos) < 3.0f) {
                    e.health -= (int)it->damage; destroyed = true; if (e.health <= 0) { e.aiState = 5; e.defeatTimer = 1.0f; } break;
                }
            }
        } else if (!destroyed && Vector3Distance(it->pos, player.pos) < 2.0f && player.invulnTimer <= 0) {
            DamagePlayer((int)it->damage); destroyed = true;
        }
        if (destroyed) it = world.bullets.erase(it); else ++it;
    }
}

void UpdateParticles(float dt) {
    for (auto it = world.particles.begin(); it != world.particles.end();) {
        it->lifetime -= dt; it->pos = Vector3Add(it->pos, Vector3Scale(it->vel, dt));
        if (it->lifetime <= 0) it = world.particles.erase(it); else ++it;
    }
}

void UpdateCamera() { camera.target = player.pos; camera.position = Vector3Add(player.pos, {0, CAMERA_HEIGHT, -CAMERA_DISTANCE}); }
Vector3 GetAimPoint() {
    Ray ray = GetMouseRay(GetMousePosition(), camera);
    if(ray.direction.y != 0) { float t = -ray.position.y / ray.direction.y; if(t > 0) return Vector3Add(ray.position, Vector3Scale(ray.direction, t)); }
    return player.pos;
}
void FireBullet(Vector3 pos, Vector3 vel, Color col, bool playerBullet, float damage, float size) { world.bullets.push_back({pos, vel, col, 6.0f, playerBullet, false, size, damage}); }
void SpawnParticles(Vector3 pos, Color col, int count, float speed) { for(int i=0; i<count; i++) world.particles.push_back({pos, {(float)GetRandomValue(-10,10)/10.0f*speed, (float)GetRandomValue(-10,10)/10.0f*speed, (float)GetRandomValue(-10,10)/10.0f*speed}, col, 1.0f, 1.0f, 0.2f}); }
void DamagePlayer(int d) { player.health -= d; player.invulnTimer = 0.5f; if(player.health <= 0) state = DEAD; }
void RebootSystem() { InitPlayer(); state = PLAYING; }
void RecalculatePlayerStats() { WeaponData* w = GetCurrentWeaponData(); player.baseDamage = w->damage; player.bulletSpeed = w->bulletSpeed; player.shootRate = w->fireRate; }
void InitializeWeaponArsenal() {
    world.weaponArsenal.push_back({WEAPON_PISTOL, "Pistol", "Basic", 0.15f, 25.0f, 40.0f, 1, 0.4f, SKYBLUE, false, false, false, 0, 0, true});
    world.weaponArsenal.push_back({WEAPON_RAILGUN, "Railgun", "Heavy", 0.5f, 100.0f, 80.0f, 1, 0.5f, PURPLE, true, false, false, 1000, 5, false});
}
WeaponData* GetCurrentWeaponData() { for(auto& w : world.weaponArsenal) if(w.type == player.currentWeapon) return &w; return &world.weaponArsenal[0]; }
void UnlockAndEquipWeapon(WeaponType t) { for(auto& w : world.weaponArsenal) if(w.type == t) { w.unlocked = true; player.currentWeapon = t; RecalculatePlayerStats(); } }
void SwitchWeapon(WeaponType t) { for(auto& w : world.weaponArsenal) if(w.type == t && w.unlocked) { player.currentWeapon = t; RecalculatePlayerStats(); } }
void InitializeShop() {} void RefreshMinimapRuins() {} void DrawCrosshair() {} void DrawHUD() { DrawText(world.message.c_str(), 10, 100, 20, GREEN); }
void DrawGame3D() {
    DrawPlane({0,0,0}, {1000,1000}, DARKGRAY);
    for(auto& e : world.enemies) if(e.isAlive) DrawSphere(e.pos, e.scale, e.color);
    for(auto& b : world.bullets) DrawSphere(b.pos, b.size, b.color);
    DrawSphere(player.pos, 1.5f, SKYBLUE);
}
void DrawSanctuaryMenu() {} void DrawShopMenu() {} void DrawWeaponSelectMenu() {} void DrawDeathScreen() {} void DrawVictoryScreen() {}
void ShakeScreen(float i) {} int GetUpgradeCost(int l) { return 100 + l * 50; }
void DropGrace(Vector3 p, int a) {} void DropTokens(Vector3 p, int a) {} void DropEquipment(Vector3 p, int w) {}
void EquipItem(Equipment* e) {} void UnequipSlot(EquipmentSlot s) {}