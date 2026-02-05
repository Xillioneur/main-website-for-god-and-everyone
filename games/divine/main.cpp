/*
================================================================================
THE LAST LIGHT: DIVINE RECKONING
================================================================================
A third-person bullet hell Soulslike shooter inspired by:
- Dark Souls & Elden Ring (brutal difficulty, stamina, dodge rolls, sanctuaries)
- God & Divine Light (theological themes, grace, redemption)
- Bullet Hell (massive visible bullets, patterns, parrying)

Features:
✓ Third-person shooter with crosshair aiming
✓ Stamina-based dodge rolling with i-frames
✓ Grace (souls) system - lose on renewal, recover from corpse
✓ Sanctuaries (bonfires) - rest, level up, respawn
✓ Parry system - reflect bullets back
✓ Massive visible bullets
✓ Lamp of Light mechanic
✓ Stats: Vigor, Will, Faith, Strength
✓ Enemy variety with bullet patterns
✓ Boss battles with multiple phases
✓ Item pickups
✓ Flask (Estus) system

Compile:
MacOS:
g++ -std=c++17 divine_reckoning.cpp -o divine_reckoning \
    -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

Linux:
g++ -std=c++17 divine_reckoning.cpp -o divine_reckoning \
    -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
================================================================================
*/

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <future>
#include <thread>
#include <execution>

// Constants
const int SCREEN_WIDTH = 1440;
const int SCREEN_HEIGHT = 810;
const float PLAYER_SPEED = 18.0f;     // Slower, heavier top speed
const float PLAYER_ACCEL = 100.0f;    // Takes longer to hit top speed
const float PLAYER_FRICTION = 1.2f;   // Extreme inertia (drifting)
const float SPRINT_MULT = 1.3f;
const float ROLL_SPEED = 45.0f;       // Weighty dodge burst
const float ROLL_DURATION = 0.35f;
const float ROLL_COST = 30.0f;
const float BULLET_SPEED_BASE = 30.0f; // Fast and responsive
const float ENEMY_BULLET_SPEED = 22.0f; // High-velocity threat
const float PRAYER_WINDOW = 0.25f; // Was PRAYER_WINDOW
const float PRAYER_RANGE = 8.0f;  // Was PRAYER_RANGE
const float PRAYER_COST = 35.0f;   // Was PRAYER_COST
const float CAMERA_HEIGHT = 35.0f;
const float CAMERA_DISTANCE = 25.0f;

// Game State
enum GameState { TITLE, PLAYING, SANCTUARY_MENU, PAUSED, RENEWAL, SHOP_MENU, WEAPON_SELECT, VICTORY };

// Equipment types
enum EquipmentSlot { WEAPON, ARMOR, TALISMAN, RING };
enum EquipmentRarity { COMMON, RARE, EPIC, LEGENDARY };

// Weapon Types - Permanent unlocks with unique mechanics
enum WeaponType {
    WEAPON_PISTOL,      // Default - Balanced
    WEAPON_REVOLVER,    // Slow, high damage
    WEAPON_BURST_RIFLE, // 3-round burst
    WEAPON_SHOTGUN,     // Close range, spread shots
    WEAPON_RIFLE,       // High damage, slower fire
    WEAPON_SMG,         // Rapid fire, lower damage
    WEAPON_DUAL_PISTOLS, // Two shots at once
    WEAPON_FLAMETHROWER, // Short range fire
    WEAPON_RAILGUN,     // Piercing shots, charge mechanic
    WEAPON_LAUNCHER,    // Explosive projectiles
    WEAPON_LIGHTNING_GUN, // Chain lightning
    WEAPON_DIVINE_BEAM  // Legendary - Continuous beam
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
    int unlockCost;      // Light tokens to unlock
    int unlockWave;      // Wave required to unlock
    bool unlocked;
    bool blueprintFound; // New: Allows purchase before wave requirement
};

struct Equipment {
    std::string name;
    EquipmentSlot slot;
    EquipmentRarity rarity;
    
    // Stats bonuses
    int bonusHealth;
    int bonusStamina;
    int bonusDamage;
    float bonusFireRate;
    float bonusSpeed;
    int bonusGraceFind;
    
    // Special effects (for armor/talismans/rings)
    bool hasLifesteal;
    bool hasExplosiveShots;
    bool hasDoubleDamage;
    bool hasPiercing;
    bool hasRapidFire;
    bool hasHomingShots;
    
    int sellValue;
    int buyValue;
    bool equipped;
};

struct Token {
    Vector3 pos;
    int value; // Light Tokens worth different amounts
    Color color;
    float lifetime;
};

struct EquipmentDrop {
    Vector3 pos;
    Equipment equipment;
    float lifetime;
    bool collected;
};
enum EnemyType { HOLLOWED, ASHBOUND, WATCHER, WHISPERER, SPIRAL, BOSS_KEEPER, GLITCH_SPECTRE };

struct Bullet {
    Vector3 pos;
    Vector3 vel;
    Color color;
    float lifetime;
    bool playerBullet;
    bool reflected;
    float size;
    float damage;
};

struct Particle {
    Vector3 pos;
    Vector3 vel;
    Color color;
    float lifetime;
    float maxLifetime;
    float size;
};

struct Ghost {
    Vector3 pos;
    float rotation;
    float lifetime;
};

struct GraceOrb {
    Vector3 pos;
    float timer;
    int value;
};

struct Item {
    Vector3 pos;
    int type; // 0=health, 1=stamina, 2=faith, 3=flask
    bool collected;
    std::string name;
};

struct Player {
    Vector3 pos;
    Vector3 vel; // Physics velocity
    float rotation;
    
    // Stats
    int vigor;
    int will;
    int faith;
    int strength;
    int level;
    
    // Resources
    int health;
    int maxHealth;
    float stamina;
    int maxStamina;
    float lampFaith;
    float maxLampFaith;
    int flasks;
    int maxFlasks;
    int grace;
    int lightTokens; // New currency for shop
    float syncMeter;
    float maxSyncMeter;
    
    // Weapons - Permanent unlocks
    WeaponType currentWeapon;
    
    // Equipment - Random drops
    Equipment* equippedArmor;
    Equipment* equippedTalisman;
    Equipment* equippedRing;
    std::vector<Equipment> inventory;
    
    // State
    bool needsReboot; // Was isRenewing
    bool isRolling;
    bool isParrying;
    bool lampActive;
    bool isHealing;
    float rollTimer;
    float parryTimer;
    float invulnTimer;
    float healTimer;
    float shootCooldown;
    float shootRate;
    float bulletSpeed;
    float baseDamage;
    float moveSpeed;
    Vector3 rollDir;
    
    // Death
    Vector3 renewalPos;
    int graceAtDeath;
    
    // Score
    int kills;
    int combo;
    int score;
};

struct Enemy {
    EnemyType type;
    Vector3 pos;
    Vector3 vel;
    Vector3 startPos;
    float rotation;
    int health;
    int maxHealth;
    bool isAlive;
    float defeatTimer; // New: for defeat animation
    float shootTimer;
    float shootCooldown;
    float moveSpeed;
    float scale;
    Color color;
    int graceReward;
    bool isBoss;
    int bossPhase;
    float patternAngle;
    float teleportTimer; // For Glitch Spectre
    WeaponType weaponDrop; // Weapon this enemy can drop
    bool hasWeaponDrop;
    
    // Advanced AI
    float stamina;
    float maxStamina;
    float actionTimer; // For dodges/dashes duration
    float abilityCooldown; // Time until next special move
    int aiState; // 0=Neutral, 1=Chasing, 2=Dodging, 3=Charging, 4=Stunned, 5=Defeated
    Vector3 chargeDir;
};

struct Sanctuary {
    Vector3 pos;
    std::string name;
    bool discovered;
    float radius;
};

struct Structure {
    Vector3 pos;
    Vector3 size;
    Color color;
};

struct GameWorld {
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<Particle> particles;
    std::vector<GraceOrb> graceOrbs;
    std::vector<Item> items;
    std::vector<Sanctuary> sanctuaries;
    std::vector<Token> tokens;
    std::vector<EquipmentDrop> equipmentDrops;
    std::vector<Equipment> shopInventory;
    std::vector<WeaponData> weaponArsenal; // All available weapons
    std::vector<Ghost> ghosts;
    std::vector<Structure> ruins;
    Sanctuary* currentSanctuary;
    bool hasGraceToRecover;
    int wave;
    std::string message;
    float messageTimer;
    int selectedShopItem;
    int selectedInventoryItem;
    int selectedWeapon; // For weapon selection menu
    int selectedSanctuaryOption; // For sanctuary navigation
    bool waveJustCompleted; // Flag for post-wave weapon unlock
    std::vector<WeaponType> availableUnlocks; // Weapons ready to unlock after wave
};

// Globals
GameState state = TITLE;
Player player;
GameWorld world;
Camera3D camera;
Mesh particleMesh;
Mesh bulletMesh;
Material instanceMaterial;
Shader bloomShader;
RenderTexture2D target;
Texture2D floorTexture;
bool debugMode = false;
float screenShake = 0.0f;

// Instancing Buffers
const int MAX_INSTANCES = 5000;
Matrix bulletMatrices[MAX_INSTANCES];
Color bulletColors[MAX_INSTANCES];
Matrix particleMatrices[MAX_INSTANCES];
Color particleColors[MAX_INSTANCES];

// Simple Bloom Shader
const char* bloomVs = R"(
#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
out vec2 fragTexCoord;
out vec4 fragColor;
uniform mat4 mvp;
void main() {
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
)";

const char* bloomFs = R"(
#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;
uniform sampler2D texture0;
uniform float threshold = 0.8;
void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    float brightness = dot(texelColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > threshold) finalColor = texelColor;
    else finalColor = vec4(0.0, 0.0, 0.0, 1.0);
}
)";

// Function declarations
void InitGame();
void InitPlayer();
void SpawnWave(int wave);
void UpdateGame(float dt);
void UpdatePlayer(float dt);
void UpdateEnemies(float dt);
void UpdateBullets(float dt);
void UpdateParticles(float dt);
void CollectGrace(float dt);
void UpdateCamera();
Vector3 GetAimPoint();
void FireBullet(Vector3 pos, Vector3 vel, Color col, bool playerBullet, float damage, float size);
void SpawnParticles(Vector3 pos, Color col, int count, float speed);
void DropGrace(Vector3 pos, int amount);
void DropTokens(Vector3 pos, int amount);
void DropEquipment(Vector3 pos, int waveNum);
Equipment GenerateRandomEquipment(int waveNum);
void DropWeaponUnlock(Vector3 pos, int waveNum);
void EquipItem(Equipment* item);
void UnequipSlot(EquipmentSlot slot);
void RecalculatePlayerStats();
void InitializeShop();
void InitializeWeaponArsenal();
void UnlockWeapon(WeaponType type);
void UnlockAndEquipWeapon(WeaponType type);
void SwitchWeapon(WeaponType type);
WeaponData* GetCurrentWeaponData();
Color GetRarityColor(EquipmentRarity rarity);

// Theme helper
Color GetWaveColor() {
    if (world.wave <= 5) return (Color){ 0, 255, 255, 255 }; // Cyan
    if (world.wave <= 10) return (Color){ 255, 80, 0, 255 };  // Neon Orange/Red
    if (world.wave <= 15) return (Color){ 180, 50, 255, 255 }; // Deep Purple
    return GOLD; // Legendary Void
}

void DamagePlayer(int damage);
void PlayerNeedsReboot();
void RebootSystem();
int GetUpgradeCost(int level);
void DrawGame3D();
void DrawPlayer();
void DrawEnemy(const Enemy& e);
void DrawBullet(const Bullet& b);
void DrawHUD();
void DrawMinimap();
void DrawCrosshair();
void DrawSanctuaryMenu();
void DrawShopMenu();
void DrawWeaponSelectMenu();
void DrawDeathScreen();
void DrawVictoryScreen();
void ShakeScreen(float intensity);

// ============================================================================
// MAIN
// ============================================================================
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "THE LAST LIGHT: DIVINE RECKONING");
    SetTargetFPS(60);
    HideCursor();
    
    InitGame();
    
    while(!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Input
        if(IsKeyPressed(KEY_ESCAPE)) {
            if(state == PLAYING) state = PAUSED;
            else if(state == PAUSED || state == SANCTUARY_MENU) state = PLAYING;
        }
        
        // Debug Toggle
        if(IsKeyPressed(KEY_F1)) {
            debugMode = !debugMode;
            world.message = debugMode ? "DEBUG MODE: ENABLED" : "DEBUG MODE: DISABLED";
            world.messageTimer = 2.0f;
        }
        
        // State machine
        switch(state) {
            case TITLE:
                if(IsKeyPressed(KEY_ENTER)) {
                    state = PLAYING;
                    world.wave = 1;
                    SpawnWave(1);
                }
                break;
                
            case PLAYING:
                UpdateGame(dt);
                break;
                
            case SANCTUARY_MENU:
                // Modern Navigation System
                if(IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                    world.selectedSanctuaryOption = (world.selectedSanctuaryOption - 1 + 5) % 5;
                }
                if(IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                    world.selectedSanctuaryOption = (world.selectedSanctuaryOption + 1) % 5;
                }
                
                if(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                    if (world.selectedSanctuaryOption == 4) {
                        state = PLAYING;
                    } else {
                        int* stats[] = {&player.vigor, &player.will, &player.faith, &player.strength};
                        if (player.grace >= GetUpgradeCost(*stats[world.selectedSanctuaryOption])) {
                            player.grace -= GetUpgradeCost(*stats[world.selectedSanctuaryOption]);
                            (*stats[world.selectedSanctuaryOption])++;
                            RecalculatePlayerStats();
                            world.message = "UPGRADE SUCCESSFUL";
                            world.messageTimer = 1.0f;
                        } else {
                            world.message = "INSUFFICIENT FRAGMENTS";
                            world.messageTimer = 1.0f;
                        }
                    }
                }

                if(IsKeyPressed(KEY_T)) { // T for Trade/Shop
                    state = SHOP_MENU;
                    InitializeShop();
                }
                if(IsKeyPressed(KEY_ESCAPE)) {
                    state = PLAYING;
                }
                break;
                
            case SHOP_MENU:
                // Weapon switching with number keys
                for(int i = 0; i < 9; i++) {
                    if(IsKeyPressed(KEY_ONE + i)) {
                        // Find the i-th unlocked weapon
                        int unlockedIndex = 0;
                        for(auto& w : world.weaponArsenal) {
                            if(w.unlocked) {
                                if(unlockedIndex == i) {
                                    SwitchWeapon(w.type);
                                    break;
                                }
                                unlockedIndex++;
                            }
                        }
                    }
                }
                
                // Navigate equipment shop
                if(IsKeyPressed(KEY_DOWN)) {
                    if(world.shopInventory.size() > 0) {
                        world.selectedShopItem = (world.selectedShopItem + 1) % world.shopInventory.size();
                    }
                }
                if(IsKeyPressed(KEY_UP)) {
                    if(world.shopInventory.size() > 0) {
                        world.selectedShopItem = (world.selectedShopItem - 1 + world.shopInventory.size()) % world.shopInventory.size();
                    }
                }
                if(IsKeyPressed(KEY_ENTER)) {
                    // Buy equipment item
                    if(world.selectedShopItem < world.shopInventory.size()) {
                        Equipment& item = world.shopInventory[world.selectedShopItem];
                        if(player.lightTokens >= item.buyValue) {
                            player.lightTokens -= item.buyValue;
                            player.inventory.push_back(item);
                            world.message = "Purchased: " + item.name;
                            world.messageTimer = 2.0f;
                        }
                    }
                }
                if(IsKeyPressed(KEY_W)) {
                    // BUY WEAPON with tokens
                    for(auto& w : world.weaponArsenal) {
                        if(!w.unlocked && (world.wave >= w.unlockWave || w.blueprintFound) && player.lightTokens >= w.unlockCost) {
                            player.lightTokens -= w.unlockCost;
                            UnlockAndEquipWeapon(w.type);
                            break;
                        }
                    }
                }
                if(IsKeyPressed(KEY_S)) {
                    // Sell selected inventory item
                    if(world.selectedInventoryItem < player.inventory.size()) {
                        Equipment& item = player.inventory[world.selectedInventoryItem];
                        if(!item.equipped) {
                            player.lightTokens += item.sellValue;
                            player.inventory.erase(player.inventory.begin() + world.selectedInventoryItem);
                            world.message = "Sold for " + std::to_string(item.sellValue) + " tokens";
                            world.messageTimer = 2.0f;
                            if(world.selectedInventoryItem >= player.inventory.size() && world.selectedInventoryItem > 0) {
                                world.selectedInventoryItem--;
                            }
                        }
                    }
                }
                if(IsKeyPressed(KEY_E)) {
                    // Equip selected inventory item
                    if(world.selectedInventoryItem < player.inventory.size()) {
                        EquipItem(&player.inventory[world.selectedInventoryItem]);
                    }
                }
                if(IsKeyPressed(KEY_LEFT)) {
                    if(player.inventory.size() > 0) {
                        world.selectedInventoryItem = (world.selectedInventoryItem - 1 + player.inventory.size()) % player.inventory.size();
                    }
                }
                if(IsKeyPressed(KEY_RIGHT)) {
                    if(player.inventory.size() > 0) {
                        world.selectedInventoryItem = (world.selectedInventoryItem + 1) % player.inventory.size();
                    }
                }
                if(IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_TAB)) {
                    state = SANCTUARY_MENU;
                }
                break;
                
            case WEAPON_SELECT:
                // Post-wave weapon unlock selection
                if(IsKeyPressed(KEY_DOWN)) {
                    if(world.availableUnlocks.size() > 0) {
                        world.selectedWeapon = (world.selectedWeapon + 1) % world.availableUnlocks.size();
                    }
                }
                if(IsKeyPressed(KEY_UP)) {
                    if(world.availableUnlocks.size() > 0) {
                        world.selectedWeapon = (world.selectedWeapon - 1 + world.availableUnlocks.size()) % 
                                              world.availableUnlocks.size();
                    }
                }
                if(IsKeyPressed(KEY_ENTER) && !world.availableUnlocks.empty()) {
                    // Unlock AND equip selected weapon for FREE
                    WeaponType selected = world.availableUnlocks[world.selectedWeapon];
                    UnlockAndEquipWeapon(selected);
                    
                    // Continue to next wave
                    world.wave++;
                    SpawnWave(world.wave);
                    player.health = std::min(player.health + 30, player.maxHealth);
                    state = PLAYING;
                }
                if(IsKeyPressed(KEY_SPACE)) {
                    // Skip weapon unlock
                    world.wave++;
                    SpawnWave(world.wave);
                    player.health = std::min(player.health + 30, player.maxHealth);
                    state = PLAYING;
                }
                break;
                
            case RENEWAL:
                if(IsKeyPressed(KEY_R)) {
                    RebootSystem();
                    state = PLAYING;
                }
                break;
                
            case PAUSED:
                // Just pause, no update
                break;

            case VICTORY:
                if(IsKeyPressed(KEY_ENTER)) {
                    InitGame();
                    state = TITLE;
                }
                break;
        }
        
        // Draw
        BeginTextureMode(target);
        ClearBackground(BLACK);
        
        // Draw 3D world (only when playing or in sanctuary)
        if(state == PLAYING || state == PAUSED || state == SANCTUARY_MENU || state == RENEWAL) {
            BeginMode3D(camera);
            DrawGame3D();
            EndMode3D();
            
            DrawCrosshair();
            DrawHUD();
        }
        
        // Draw UI overlays
        if(state == SANCTUARY_MENU) DrawSanctuaryMenu();
        if(state == SHOP_MENU) DrawShopMenu();
        if(state == WEAPON_SELECT) DrawWeaponSelectMenu();
        if(state == RENEWAL) DrawDeathScreen();
        if(state == PAUSED) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
            DrawText("PAUSED", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2, 60, GOLD);
        } else if(state == TITLE) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.95f));
            DrawRectangleLines(100, 100, SCREEN_WIDTH - 200, SCREEN_HEIGHT - 200, GOLD);
            DrawText("THE LAST LIGHT", SCREEN_WIDTH/2 - 280, 150, 90, GOLD);
            DrawText("DIVINE ASCENSION", SCREEN_WIDTH/2 - 260, 250, 60, WHITE);
            DrawText("A Pilgrimage through the Void", SCREEN_WIDTH/2 - 220, 340, 30, LIGHTGRAY);
            DrawText("WASD: WALK | MOUSE: AIM | LMB: LIGHT | SPACE: DASH | Q: PARRY | L: SENSOR", 
                     150, 450, 22, SKYBLUE);
            DrawText("Faith is your shield. Light is your path. Grace is eternal.", 
                     SCREEN_WIDTH/2 - 350, 550, 26, GOLD);
            DrawText("PRESS ENTER TO BEGIN PILGRIMAGE", SCREEN_WIDTH/2 - 250, 650, 28, WHITE);
            
            DrawText("INSPIRED BY THE DIVINE | DEVELOPED WITH GEMINI", 30, SCREEN_HEIGHT - 40, 20, DARKGRAY);
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        
        // Draw scene with bloom
        BeginShaderMode(bloomShader);
        DrawTextureRec(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2){ 0, 0 }, WHITE);
        EndShaderMode();
        
        // Draw original scene on top (additive)
        BeginBlendMode(BLEND_ADDITIVE);
        DrawTextureRec(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2){ 0, 0 }, WHITE);
        EndBlendMode();
        
        if (state == VICTORY) DrawVictoryScreen();
        
        DrawFPS(10, 10);
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}

// ============================================================================
// INITIALIZATION
// ============================================================================
void InitGame() {
    // Meshes for instancing
    particleMesh = GenMeshSphere(1.0f, 4, 4); // Low poly for performance
    bulletMesh = GenMeshSphere(1.0f, 6, 6);
    instanceMaterial = LoadMaterialDefault();
    bloomShader = LoadShaderFromMemory(bloomVs, bloomFs);
    target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Generate Realistic Floor Texture
    Image noiseImg = GenImagePerlinNoise(1024, 1024, 0, 0, 10.0f);
    ImageColorContrast(&noiseImg, -10);
    ImageColorBrightness(&noiseImg, -180);
    floorTexture = LoadTextureFromImage(noiseImg);
    UnloadImage(noiseImg);
    
    // Camera setup
    camera.position = {0, CAMERA_HEIGHT, -CAMERA_DISTANCE};
    camera.target = {0, 1, 0};
    camera.up = {0, 1, 0};
    camera.fovy = 60;
    camera.projection = CAMERA_PERSPECTIVE;
    
    // World setup
    world = {};
    world.wave = 0;
    world.hasGraceToRecover = false;
    world.messageTimer = 0;
    world.waveJustCompleted = false;
    
    // Initialize weapon arsenal
    InitializeWeaponArsenal();
    
    // Sanctuaries (Neural Beacons)
    Sanctuary s1 = {{0, 0, 0}, "Central Core", true, 5.0f};
    world.sanctuaries.push_back(s1);
    Sanctuary s2 = {{350, 0, 350}, "Hidden Uplink", false, 5.0f};
    world.sanctuaries.push_back(s2);
    
    world.currentSanctuary = &world.sanctuaries[0];
    
    // Generate Neural Ruins
    for (int i = 0; i < 25; i++) {
        float angle = i * (PI * 2.0f / 25.0f);
        float dist = 150.0f + GetRandomValue(0, 300);
        Vector3 clusterPos = { cosf(angle) * dist, 0, sinf(angle) * dist };
        
        // Create a small cluster of pillars
        int pieces = GetRandomValue(3, 6);
        for (int p = 0; p < pieces; p++) {
            Structure s;
            s.pos = {
                clusterPos.x + GetRandomValue(-15, 15),
                0,
                clusterPos.z + GetRandomValue(-15, 15)
            };
            s.size = {
                (float)GetRandomValue(4, 10),
                (float)GetRandomValue(10, 40),
                (float)GetRandomValue(4, 10)
            };
            s.pos.y = s.size.y / 2.0f; // Grounded
            s.color = (Color){15, 15, 25, 255};
            world.ruins.push_back(s);
        }
    }
    
    // Items - Add more caches near ruins
    Item i1 = {{-10, 0, 8}, 0, false, "Health Shard"};
    world.items.push_back(i1);
    Item i2 = {{15, 0, -12}, 1, false, "Stamina Shard"};
    world.items.push_back(i2);
    Item i3 = {{-18, 0, -18}, 2, false, "Faith Shard"};
    world.items.push_back(i3);
    Item i4 = {{22, 0, 22}, 3, false, "Estus Flask"};
    world.items.push_back(i4);
    
    InitPlayer();
}

void InitPlayer() {
    player = {};
    player.pos = world.currentSanctuary->pos;
    player.vel = {0, 0, 0};
    player.rotation = 0;
    
    player.vigor = 10;
    player.will = 10;
    player.faith = 10;
    player.strength = 10;
    player.level = 1;
    
    player.maxHealth = 100;
    player.health = player.maxHealth;
    player.maxStamina = 120;
    player.stamina = player.maxStamina;
    player.maxLampFaith = 100;
    player.lampFaith = player.maxLampFaith;
    player.maxFlasks = 3;
    player.flasks = player.maxFlasks;
    player.grace = 0;
    player.lightTokens = 100; // Start with some tokens
    player.syncMeter = 0;
    player.maxSyncMeter = 100.0f;
    
    // Start with pistol
    player.currentWeapon = WEAPON_PISTOL;
    
    player.shootRate = 0.15f;
    player.bulletSpeed = BULLET_SPEED_BASE; // Set to 12.0f
    player.baseDamage = 25.0f;
    player.moveSpeed = PLAYER_SPEED;
    
    // No equipment at start (armor/talisman/ring only)
    player.equippedArmor = nullptr;
    player.equippedTalisman = nullptr;
    player.equippedRing = nullptr;
    player.inventory.clear();
    
    player.needsReboot = false;
    player.isRolling = false;
    player.isParrying = false;
    player.lampActive = false;
    player.isHealing = false;
    
    player.kills = 0;
    player.combo = 0;
    player.score = 0;
    
    RecalculatePlayerStats();
}

void SpawnWave(int wave) {
    world.enemies.clear();
    world.bullets.clear();
    
    auto spawnEnemy = [](EnemyType type, Vector3 pos, int hp, int grace) {
        Enemy e;
        e.type = type;
        e.pos = pos;
        e.vel = {0, 0, 0};
        e.startPos = pos;
        e.rotation = 0;
        e.maxHealth = hp;
        e.health = hp;
        e.isAlive = true;
        e.defeatTimer = 0;
        e.shootTimer = GetRandomValue(0, 100) / 100.0f;
        e.graceReward = grace;
        e.isBoss = (type == BOSS_KEEPER);
        e.bossPhase = 1;
        e.patternAngle = 0;
        
        // AI Init
        e.stamina = 100.0f;
        e.maxStamina = 100.0f;
        e.actionTimer = 0;
        e.abilityCooldown = GetRandomValue(20, 50) / 10.0f;
        e.aiState = 0;
        e.chargeDir = {0,0,0};
        
        if (e.isBoss) {
            e.maxStamina = 500.0f;
            e.stamina = 500.0f;
            e.abilityCooldown = 3.0f;
        }
        
        switch(type) {
            case HOLLOWED:
                e.moveSpeed = 2.0f;
                e.shootCooldown = 2.0f;
                e.scale = 1.0f;
                e.color = (Color){200, 200, 255, 255}; // Pale Blue (Lost Soul)
                break;
            case ASHBOUND:
                e.moveSpeed = 2.5f;
                e.shootCooldown = 1.5f;
                e.scale = 1.0f;
                e.color = (Color){255, 200, 150, 255}; // Amber (Whisper of Doubt)
                break;
            case WATCHER:
                e.moveSpeed = 1.8f;
                e.shootCooldown = 2.5f;
                e.scale = 1.2f;
                e.color = (Color){255, 255, 200, 255}; // Light Gold (Sentinel of Pride)
                break;
            case WHISPERER:
                e.moveSpeed = 1.5f;
                e.shootCooldown = 1.0f;
                e.scale = 1.0f;
                e.color = (Color){200, 150, 255, 200}; // Lavender (Echo of Grief)
                break;
            case SPIRAL:
                e.moveSpeed = 1.0f;
                e.shootCooldown = 0.8f;
                e.scale = 1.1f;
                e.color = (Color){255, 150, 255, 255}; // Rose (Vortex of Vanity)
                break;
            case BOSS_KEEPER:
                e.moveSpeed = 1.8f;
                e.shootCooldown = 0.4f;
                e.scale = 5.0f; // Meat tank scale
                e.color = GOLD; // Master Node (The Arch-Architect)
                break;
            case GLITCH_SPECTRE:
                e.moveSpeed = 4.0f;
                e.shootCooldown = 1.2f;
                e.scale = 0.8f;
                e.color = WHITE; // Ghost of Despair
                e.teleportTimer = 2.0f;
                break;
        }
        
        // Assign weapon drops (25% chance for regular enemies, 100% for bosses)
        e.hasWeaponDrop = e.isBoss ? true : (GetRandomValue(0, 100) < 25);
        if(e.hasWeaponDrop) {
            // Assign weapon based on enemy type
            if(type == HOLLOWED) {
                e.weaponDrop = GetRandomValue(0, 1) == 0 ? WEAPON_REVOLVER : WEAPON_BURST_RIFLE;
            } else if(type == ASHBOUND) {
                e.weaponDrop = GetRandomValue(0, 1) == 0 ? WEAPON_BURST_RIFLE : WEAPON_DUAL_PISTOLS;
            } else if(type == WATCHER) {
                int roll = GetRandomValue(0, 2);
                if(roll == 0) e.weaponDrop = WEAPON_SHOTGUN;
                else if(roll == 1) e.weaponDrop = WEAPON_DUAL_PISTOLS;
                else e.weaponDrop = WEAPON_FLAMETHROWER;
            } else if(type == WHISPERER) {
                int roll = GetRandomValue(0, 2);
                if(roll == 0) e.weaponDrop = WEAPON_SMG;
                else if(roll == 1) e.weaponDrop = WEAPON_FLAMETHROWER;
                else e.weaponDrop = WEAPON_LIGHTNING_GUN;
            } else if(type == SPIRAL) {
                e.weaponDrop = GetRandomValue(0, 1) == 0 ? WEAPON_LIGHTNING_GUN : WEAPON_RAILGUN;
            } else if(type == BOSS_KEEPER) {
                int roll = GetRandomValue(0, 2);
                if(roll == 0) e.weaponDrop = WEAPON_RAILGUN;
                else if(roll == 1) e.weaponDrop = WEAPON_LAUNCHER;
                else e.weaponDrop = WEAPON_DIVINE_BEAM;
            }
        }
        
        world.enemies.push_back(e);
    };
    
    // ENDLESS WAVE SYSTEM - Progressively harder and more varied
    int baseEnemies = 25 + wave * 10;  // Dramatically more bots
    float hpScale = 1.0f + wave * 0.15f;  // Enemies get tougher
    int graceScale = 50 + wave * 25;  // Better rewards
    
    // Every 5th wave is a boss wave
    if(wave % 5 == 0) {
        // BOSS WAVE - Multiple bosses at higher waves!
        int bossCount = 1 + (wave / 10);  // More bosses every 10 waves
        for(int i = 0; i < bossCount; i++) {
            float angle = i * (2 * PI / bossCount);
            Vector3 pos = {cosf(angle) * 150, 0, sinf(angle) * 150};
            spawnEnemy(BOSS_KEEPER, pos, (int)(1500 * hpScale), graceScale * 40);
        }
        
        // Add support enemies
        for(int i = 0; i < 15; i++) { // More support
            float angle = i * (2 * PI / 15);
            Vector3 pos = {cosf(angle) * 100, 0, sinf(angle) * 100};
            EnemyType type = (EnemyType)(GetRandomValue(0, 4));
            spawnEnemy(type, pos, (int)(80 * hpScale), graceScale);
        }
    }
    // Every 3rd wave has spirals
    else if(wave % 3 == 0) {
        // SPIRAL HELL WAVE
        int spiralCount = 6 + wave / 3;
        for(int i = 0; i < spiralCount; i++) {
            float angle = i * (2 * PI / spiralCount);
            Vector3 pos = {cosf(angle) * 120, 0, sinf(angle) * 120};
            spawnEnemy(SPIRAL, pos, (int)(120 * hpScale), graceScale * 4);
        }
        
        // Fill with mixed enemies
        for(int i = 0; i < baseEnemies; i++) {
            float angle = GetRandomValue(0, 360) * DEG2RAD;
            float radius = 60 + GetRandomValue(0, 100);
            Vector3 pos = {cosf(angle) * radius, 0, sinf(angle) * radius};
            EnemyType type = (EnemyType)(GetRandomValue(0, 2));  // Hollowed, Ashbound, Watcher
            spawnEnemy(type, pos, (int)(60 * hpScale), graceScale);
        }
    }
    // Regular waves with Encampment-based spawning
    else {
        int encampments = 8 + wave; // Double the encampments
        
        for (int enc = 0; encampments > 0 && enc < encampments; enc++) {
            // Pick a random ruin as an anchor
            if (world.ruins.empty()) break;
            int ruinIdx = GetRandomValue(0, world.ruins.size() - 1);
            Vector3 anchor = world.ruins[ruinIdx].pos;
            anchor.y = 0;
            
            int groupSize = baseEnemies / encampments;
            for (int i = 0; i < groupSize; i++) {
                float angle = GetRandomValue(0, 360) * DEG2RAD;
                float radius = GetRandomValue(5, 25);
                Vector3 pos = { anchor.x + cosf(angle) * radius, 0, anchor.z + sinf(angle) * radius };
                
                // Type distribution logic
                EnemyType type;
                int roll = GetRandomValue(0, 100);
                if(wave < 3) {
                    if(roll < 60) type = HOLLOWED; else if(roll < 90) type = ASHBOUND; else type = WATCHER;
                } else if(wave < 7) {
                    if(roll < 20) type = HOLLOWED; else if(roll < 40) type = ASHBOUND; else if(roll < 60) type = WATCHER;
                    else if(roll < 80) type = WHISPERER; else if(roll < 95) type = SPIRAL; else type = GLITCH_SPECTRE;
                } else {
                    if(roll < 10) type = HOLLOWED; else if(roll < 20) type = ASHBOUND; else if(roll < 40) type = WATCHER;
                    else if(roll < 60) type = WHISPERER; else if(roll < 85) type = SPIRAL; else type = GLITCH_SPECTRE;
                }
                
                int hp = (int)( (type == SPIRAL ? 120 : type == GLITCH_SPECTRE ? 100 : 60) * hpScale );
                spawnEnemy(type, pos, hp, graceScale);
            }
        }
        
        // Always spawn a 'Hunter' group near the player to keep the action going
        for (int i = 0; i < 10; i++) {
            float angle = GetRandomValue(0, 360) * DEG2RAD;
            float radius = 60.0f + GetRandomValue(0, 40);
            Vector3 pos = { player.pos.x + cosf(angle) * radius, 0, player.pos.z + sinf(angle) * radius };
            spawnEnemy(ASHBOUND, pos, (int)(80 * hpScale), graceScale);
        }
        
        // Add elite mini-boss every 4 waves (but not on boss waves)
        if(wave >= 8 && wave % 4 == 0 && wave % 5 != 0) {
            spawnEnemy(BOSS_KEEPER, {0, 0, -300}, (int)(1500 * hpScale), graceScale * 30);
        }
    }

    // Apply speed multiplier to all spawned enemies for this wave
    for (auto& e : world.enemies) {
        e.moveSpeed *= 1.3f;
    }
}

int GetUpgradeCost(int level) {
    return 200 + level * 150;
}

// ============================================================================
// EQUIPMENT & SHOP SYSTEMS
// ============================================================================

Color GetRarityColor(EquipmentRarity rarity) {
    switch(rarity) {
        case COMMON: return LIGHTGRAY;
        case RARE: return SKYBLUE;
        case EPIC: return PURPLE;
        case LEGENDARY: return GOLD;
        default: return WHITE;
    }
}

Equipment GenerateRandomEquipment(int waveNum) {
    Equipment eq;
    eq.equipped = false;
    
    // Determine rarity based on wave (higher waves = better loot)
    int rarityRoll = GetRandomValue(0, 100);
    if(waveNum < 5) {
        if(rarityRoll < 70) eq.rarity = COMMON;
        else if(rarityRoll < 95) eq.rarity = RARE;
        else eq.rarity = EPIC;
    } else if(waveNum < 10) {
        if(rarityRoll < 50) eq.rarity = COMMON;
        else if(rarityRoll < 80) eq.rarity = RARE;
        else if(rarityRoll < 95) eq.rarity = EPIC;
        else eq.rarity = LEGENDARY;
    } else {
        if(rarityRoll < 30) eq.rarity = RARE;
        else if(rarityRoll < 70) eq.rarity = EPIC;
        else eq.rarity = LEGENDARY;
    }
    
    // Only generate armor, talisman, ring (no weapons - they're now unlocks)
    eq.slot = (EquipmentSlot)(GetRandomValue(1, 3)); // 1=ARMOR, 2=TALISMAN, 3=RING
    
    // Base stats (scale with rarity)
    float rarityMult = 1.0f + (int)eq.rarity * 0.5f;
    eq.bonusHealth = (int)(GetRandomValue(0, 20) * rarityMult);
    eq.bonusStamina = (int)(GetRandomValue(0, 15) * rarityMult);
    eq.bonusDamage = (int)(GetRandomValue(5, 15) * rarityMult);
    eq.bonusFireRate = GetRandomValue(0, 20) / 100.0f * rarityMult;
    eq.bonusSpeed = GetRandomValue(0, 10) / 10.0f * rarityMult;
    eq.bonusGraceFind = (int)(GetRandomValue(0, 25) * rarityMult);
    
    // Special effects (only for higher rarities)
    eq.hasLifesteal = eq.rarity >= EPIC && GetRandomValue(0, 100) < 30;
    eq.hasExplosiveShots = eq.rarity >= RARE && GetRandomValue(0, 100) < 25;
    eq.hasDoubleDamage = eq.rarity >= EPIC && GetRandomValue(0, 100) < 20;
    eq.hasPiercing = eq.rarity >= EPIC && GetRandomValue(0, 100) < 25;
    eq.hasRapidFire = eq.rarity >= RARE && GetRandomValue(0, 100) < 30;
    eq.hasHomingShots = eq.rarity == LEGENDARY && GetRandomValue(0, 100) < 25;
    
    // Generate name based on slot and rarity
    const char* armorNames[] = {"Tunic", "Plate", "Sacred Armor", "Divine Raiment"};
    const char* talismanNames[] = {"Charm", "Amulet", "Talisman", "Relic"};
    const char* ringNames[] = {"Band", "Ring", "Signet", "Crown Ring"};
    
    switch(eq.slot) {
        case ARMOR:
            eq.name = std::string(armorNames[eq.rarity]) + " of Grace";
            break;
        case TALISMAN:
            eq.name = std::string(talismanNames[eq.rarity]) + " of Faith";
            break;
        case RING:
            eq.name = std::string(ringNames[eq.rarity]) + " of Will";
            break;
        default:
            eq.name = "Unknown";
            break;
    }
    
    eq.sellValue = 10 + (int)eq.rarity * 50 + waveNum * 10;
    eq.buyValue = eq.sellValue * 3;
    
    return eq;
}

void DropEquipment(Vector3 pos, int waveNum) {
    // 20% chance to drop equipment
    if(GetRandomValue(0, 100) < 20) {
        EquipmentDrop drop;
        drop.pos = Vector3Add(pos, {0, 2, 0});
        drop.equipment = GenerateRandomEquipment(waveNum);
        drop.lifetime = 30.0f;
        drop.collected = false;
        world.equipmentDrops.push_back(drop);
    }
}

void DropWeaponUnlock(Vector3 pos, int waveNum) {
    // 10% chance to drop weapon unlock from enemies
    if(GetRandomValue(0, 100) < 10) {
        // Find a locked weapon that meets wave requirement
        for(auto& w : world.weaponArsenal) {
            if(!w.unlocked && waveNum >= w.unlockWave) {
                // Unlock this weapon immediately
                w.unlocked = true;
                world.message = "WEAPON UNLOCKED: " + w.name + "!";
                world.messageTimer = 4.0f;
                SpawnParticles(pos, GOLD, 50, 25.0f);
                return;
            }
        }
    }
}

void DropTokens(Vector3 pos, int amount) {
    int tokenCount = amount / 10 + 1;
    for(int i = 0; i < tokenCount; i++) {
        Token t;
        t.pos = Vector3Add(pos, {
            GetRandomValue(-20, 20) / 10.0f,
            GetRandomValue(10, 30) / 10.0f,
            GetRandomValue(-20, 20) / 10.0f
        });
        t.value = 10;
        t.color = YELLOW;
        t.lifetime = 15.0f;
        world.tokens.push_back(t);
    }
}

void EquipItem(Equipment* item) {
    if(!item || item->equipped) return;
    
    // Only handle armor, talisman, ring (no weapons)
    if(item->slot == WEAPON) return;
    
    // Unequip current item in that slot
    UnequipSlot(item->slot);
    
    // Equip new item
    item->equipped = true;
    switch(item->slot) {
        case ARMOR: player.equippedArmor = item; break;
        case TALISMAN: player.equippedTalisman = item; break;
        case RING: player.equippedRing = item; break;
        default: break;
    }
    
    RecalculatePlayerStats();
    world.message = "Equipped: " + item->name;
    world.messageTimer = 2.0f;
}

void UnequipSlot(EquipmentSlot slot) {
    Equipment* current = nullptr;
    switch(slot) {
        case ARMOR: current = player.equippedArmor; player.equippedArmor = nullptr; break;
        case TALISMAN: current = player.equippedTalisman; player.equippedTalisman = nullptr; break;
        case RING: current = player.equippedRing; player.equippedRing = nullptr; break;
        default: break;
    }
    if(current) current->equipped = false;
}

void RecalculatePlayerStats() {
    // Base stats from levels
    player.maxHealth = 100 + player.vigor * 10;
    player.maxStamina = 120 + player.will * 12;
    player.maxLampFaith = 100 + player.faith * 15;
    player.moveSpeed = PLAYER_SPEED;
    
    // Get weapon stats
    WeaponData* weapon = GetCurrentWeaponData();
    if(weapon) {
        player.baseDamage = weapon->damage + player.strength * 2;
        player.bulletSpeed = weapon->bulletSpeed + player.strength * 0.5f;
        player.shootRate = weapon->fireRate;
    } else {
        player.baseDamage = 25.0f + player.strength * 2;
        player.bulletSpeed = BULLET_SPEED_BASE + player.strength * 0.5f;
        player.shootRate = 0.15f;
    }
    
    // Add equipment bonuses (armor, talisman, ring)
    Equipment* gear[] = {player.equippedArmor, player.equippedTalisman, player.equippedRing};
    for(Equipment* eq : gear) {
        if(eq) {
            player.maxHealth += eq->bonusHealth;
            player.maxStamina += eq->bonusStamina;
            player.baseDamage += eq->bonusDamage;
            player.bulletSpeed += eq->bonusSpeed * 2;
            player.moveSpeed += eq->bonusSpeed;
            player.shootRate = std::max(0.05f, player.shootRate - eq->bonusFireRate);
        }
    }
    
    // Clamp health and stamina to new maxes
    player.health = std::min(player.health, player.maxHealth);
    player.stamina = std::min(player.stamina, (float)player.maxStamina);
}

void InitializeShop() {
    world.shopInventory.clear();
    
    // Generate random shop inventory (6 items)
    for(int i = 0; i < 6; i++) {
        Equipment item = GenerateRandomEquipment(world.wave);
        world.shopInventory.push_back(item);
    }
    
    world.selectedShopItem = 0;
    world.selectedInventoryItem = 0;
}

void InitializeWeaponArsenal() {
    world.weaponArsenal.clear();
    
    // TIER 1 - Starting & Common Drops
    
    // Pistol - Starting weapon
    WeaponData pistol;
    pistol.type = WEAPON_PISTOL;
    pistol.name = "Glimmer of Faith";
    pistol.description = "A humble light against the dark.";
    pistol.fireRate = 0.10f; // Faster
    pistol.damage = 25.0f;
    pistol.bulletSpeed = 32.0f; 
    pistol.projectileCount = 1;
    pistol.bulletSize = 0.35f;
    pistol.bulletColor = SKYBLUE;
    pistol.piercing = false;
    pistol.explosive = false;
    pistol.homing = false;
    pistol.unlockCost = 0;
    pistol.unlockWave = 0;
    pistol.unlocked = true;
    pistol.blueprintFound = true;
    world.weaponArsenal.push_back(pistol);
    
    // Revolver - Heavy hits
    WeaponData revolver;
    revolver.type = WEAPON_REVOLVER;
    revolver.name = "Hammer of Justice";
    revolver.description = "Heavy algorithmic correction.";
    revolver.fireRate = 0.25f; // Faster
    revolver.damage = 45.0f;
    revolver.bulletSpeed = 35.0f; 
    revolver.projectileCount = 1;
    revolver.bulletSize = 0.4f;
    revolver.bulletColor = (Color){180, 180, 200, 255}; 
    revolver.piercing = false;
    revolver.explosive = false;
    revolver.homing = false;
    revolver.unlockCost = 600;
    revolver.unlockWave = 2;
    revolver.unlocked = false;
    revolver.blueprintFound = false;
    world.weaponArsenal.push_back(revolver);
    
    // Burst Rifle - 3-round burst
    WeaponData burst;
    burst.type = WEAPON_BURST_RIFLE;
    burst.name = "Trinity Scepter";
    burst.description = "Fires three sacred pulses.";
    burst.fireRate = 0.04f; 
    burst.damage = 20.0f;
    burst.bulletSpeed = 38.0f; 
    burst.projectileCount = 3; 
    burst.bulletSize = 0.3f;
    burst.bulletColor = (Color){100, 200, 255, 255}; 
    burst.piercing = false;
    burst.explosive = false;
    burst.homing = false;
    burst.unlockCost = 850;
    burst.unlockWave = 3;
    burst.unlocked = false;
    burst.blueprintFound = false;
    world.weaponArsenal.push_back(burst);
    
    // TIER 2 - Mid-game weapons
    
    // Shotgun - Scatter
    WeaponData shotgun;
    shotgun.type = WEAPON_SHOTGUN;
    shotgun.name = "Scepter of Radiance";
    shotgun.description = "Diffusion of light. Close range focus.";
    shotgun.fireRate = 0.40f; // Faster
    shotgun.damage = 18.0f;
    shotgun.bulletSpeed = 28.0f; 
    shotgun.projectileCount = 5;
    shotgun.bulletSize = 0.25f;
    shotgun.bulletColor = VIOLET;
    shotgun.piercing = false;
    shotgun.explosive = false;
    shotgun.homing = false;
    shotgun.unlockCost = 1500;
    shotgun.unlockWave = 5;
    shotgun.unlocked = false;
    shotgun.blueprintFound = false;
    world.weaponArsenal.push_back(shotgun);
    
    // Dual Pistols - Fire 2 bullets per shot
    WeaponData dual;
    dual.type = WEAPON_DUAL_PISTOLS;
    dual.name = "Seraphim Wings";
    dual.description = "Twin emitters of pure intent.";
    dual.fireRate = 0.08f; // Fast
    dual.damage = 18.0f;
    dual.bulletSpeed = 34.0f; 
    dual.projectileCount = 2; 
    dual.bulletSize = 0.32f;
    dual.bulletColor = (Color){100, 255, 255, 255}; 
    dual.piercing = false;
    dual.explosive = false;
    dual.homing = false;
    dual.unlockCost = 1800;
    dual.unlockWave = 6;
    dual.unlocked = false;
    dual.blueprintFound = false;
    world.weaponArsenal.push_back(dual);
    
    // Rifle - High Damage
    WeaponData rifle;
    rifle.type = WEAPON_RIFLE;
    rifle.name = "Staff of Truth";
    rifle.description = "Precision projection of divine will.";
    rifle.fireRate = 0.30f; // Faster
    rifle.damage = 85.0f;
    rifle.bulletSpeed = 55.0f; 
    rifle.projectileCount = 1;
    rifle.bulletSize = 0.45f;
    rifle.bulletColor = ORANGE;
    rifle.piercing = false;
    rifle.explosive = false;
    rifle.homing = false;
    rifle.unlockCost = 2500;
    rifle.unlockWave = 8;
    rifle.unlocked = false;
    rifle.blueprintFound = false;
    world.weaponArsenal.push_back(rifle);
    
    // TIER 3 - Advanced weapons
    
    // SMG - High Speed
    WeaponData smg;
    smg.type = WEAPON_SMG;
    smg.name = "Zealot's Whisper";
    smg.description = "Rapid frequency fragmenter.";
    smg.fireRate = 0.04f; // Extremely fast
    smg.damage = 12.0f;
    smg.bulletSpeed = 40.0f; 
    smg.projectileCount = 1;
    smg.bulletSize = 0.22f;
    smg.bulletColor = YELLOW;
    smg.piercing = false;
    smg.explosive = false;
    smg.homing = false;
    smg.unlockCost = 3500;
    smg.unlockWave = 10;
    smg.unlocked = false;
    smg.blueprintFound = false;
    world.weaponArsenal.push_back(smg);
    
    // Flamethrower
    WeaponData flame;
    flame.type = WEAPON_FLAMETHROWER;
    flame.name = "Pillar of Fire";
    flame.description = "Continuous cleansing of the void.";
    flame.fireRate = 0.03f; // Blazing fast
    flame.damage = 9.0f;
    flame.bulletSpeed = 20.0f; 
    flame.projectileCount = 1;
    flame.bulletSize = 0.5f; 
    flame.bulletColor = (Color){255, 140, 0, 255}; 
    flame.piercing = false;
    flame.explosive = false;
    flame.homing = false;
    flame.unlockCost = 4000;
    flame.unlockWave = 12;
    flame.unlocked = false;
    flame.blueprintFound = false;
    world.weaponArsenal.push_back(flame);
    
    // TIER 4 - Elite weapons
    
    // Railgun
    WeaponData railgun;
    railgun.type = WEAPON_RAILGUN;
    railgun.name = "Light of Eden";
    railgun.description = "Hyper-velocity ray. Purest form.";
    railgun.fireRate = 0.50f; // Faster
    railgun.damage = 130.0f;
    railgun.bulletSpeed = 75.0f; 
    railgun.projectileCount = 1;
    railgun.bulletSize = 0.35f;
    railgun.bulletColor = PURPLE;
    railgun.piercing = true;
    railgun.explosive = false;
    railgun.homing = false;
    railgun.unlockCost = 6000;
    railgun.unlockWave = 15;
    railgun.unlocked = false;
    railgun.blueprintFound = false;
    world.weaponArsenal.push_back(railgun);
    
    // Lightning Gun
    WeaponData lightning;
    lightning.type = WEAPON_LIGHTNING_GUN;
    lightning.name = "Thunder of Sinai";
    lightning.description = "Chains grace between shadows.";
    lightning.fireRate = 0.15f; // Faster
    lightning.damage = 30.0f;
    lightning.bulletSpeed = 45.0f; 
    lightning.projectileCount = 1;
    lightning.bulletSize = 0.35f;
    lightning.bulletColor = (Color){200, 200, 255, 255}; 
    lightning.piercing = false;
    lightning.explosive = false;
    lightning.homing = true; 
    lightning.unlockCost = 7500;
    lightning.unlockWave = 18;
    lightning.unlocked = false;
    lightning.blueprintFound = false;
    world.weaponArsenal.push_back(lightning);
    
    // TIER 5 - Legendary weapons
    
    // Launcher
    WeaponData launcher;
    launcher.type = WEAPON_LAUNCHER;
    launcher.name = "Revelations Bomb";
    launcher.description = "Massive AOE cleansing.";
    launcher.fireRate = 0.70f; // Faster
    launcher.damage = 40.0f;
    launcher.bulletSpeed = 25.0f; 
    launcher.projectileCount = 1;
    launcher.bulletSize = 0.7f;
    launcher.bulletColor = (Color){255, 100, 0, 255};
    launcher.piercing = false;
    launcher.explosive = true;
    launcher.homing = false;
    launcher.unlockCost = 10000;
    launcher.unlockWave = 20;
    launcher.unlocked = false;
    launcher.blueprintFound = false;
    world.weaponArsenal.push_back(launcher);
    
    // Divine Beam
    WeaponData beam;
    beam.type = WEAPON_DIVINE_BEAM;
    beam.name = "Omega Zero";
    beam.description = "The Final Decree. Grace is absolute.";
    beam.fireRate = 0.08f; // Blistering
    beam.damage = 50.0f;
    beam.bulletSpeed = 50.0f; 
    beam.projectileCount = 1;
    beam.bulletSize = 0.5f;
    beam.bulletColor = GOLD;
    beam.piercing = false;
    beam.explosive = false;
    beam.homing = true;
    beam.unlockCost = 15000;
    beam.unlockWave = 25;
    beam.unlocked = false;
    beam.blueprintFound = false;
    world.weaponArsenal.push_back(beam);
    
    world.selectedWeapon = 0;
}

WeaponData* GetCurrentWeaponData() {
    for(auto& w : world.weaponArsenal) {
        if(w.type == player.currentWeapon) {
            return &w;
        }
    }
    return nullptr;
}

void UnlockWeapon(WeaponType type) {
    for(auto& w : world.weaponArsenal) {
        if(w.type == type) {
            if(!w.unlocked) {
                w.unlocked = true;
                world.message = "Unlocked: " + w.name + "!";
                world.messageTimer = 3.0f;
                SpawnParticles(player.pos, GOLD, 50, 20.0f);
            }
            return;
        }
    }
}

void UnlockAndEquipWeapon(WeaponType type) {
    for(auto& w : world.weaponArsenal) {
        if(w.type == type) {
            w.unlocked = true;
            player.currentWeapon = type;
            world.message = "UNLOCKED & EQUIPPED: " + w.name + "!";
            world.messageTimer = 3.0f;
            SpawnParticles(player.pos, w.bulletColor, 60, 25.0f);
            RecalculatePlayerStats();
            return;
        }
    }
}

void SwitchWeapon(WeaponType type) {
    for(auto& w : world.weaponArsenal) {
        if(w.type == type) {
            if(w.unlocked) {
                player.currentWeapon = type;
                world.message = "Equipped: " + w.name;
                world.messageTimer = 2.0f;
                RecalculatePlayerStats();
            } else {
                world.message = "Weapon is locked!";
                world.messageTimer = 2.0f;
            }
            return;
        }
    }
}

// ============================================================================
// UPDATE FUNCTIONS
// ============================================================================
Vector3 GetAimPoint() {
    Ray ray = GetMouseRay(GetMousePosition(), camera);
    if(ray.direction.y != 0) {
        float t = -ray.position.y / ray.direction.y;
        if(t > 0) return Vector3Add(ray.position, Vector3Scale(ray.direction, t));
    }
    return player.pos;
}

void UpdatePlayer(float dt) {
    if(player.needsReboot) return;
    
    if (debugMode) {
        player.syncMeter = player.maxSyncMeter;
        if (IsKeyPressed(KEY_K)) {
            player.lightTokens += 1000;
            world.message = "+1000 CREDITS";
            world.messageTimer = 1.5f;
        }
    }
    
    // Timers
    player.invulnTimer = std::max(0.0f, player.invulnTimer - dt);
    player.shootCooldown = std::max(0.0f, player.shootCooldown - dt);
    player.rollTimer = std::max(0.0f, player.rollTimer - dt);
    player.parryTimer = std::max(0.0f, player.parryTimer - dt);
    player.healTimer = std::max(0.0f, player.healTimer - dt);
    
    player.isRolling = player.rollTimer > 0;
    player.isParrying = player.parryTimer > 0;
    player.isHealing = player.healTimer > 0;
    
    // Stamina regen
    if(!player.isRolling && !player.isHealing) {
        player.stamina = std::min(player.stamina + 25.0f * dt, (float)player.maxStamina);
    }
    
    // Lamp
    if(player.lampActive && player.lampFaith > 0) {
        player.lampFaith -= 8.0f * dt;
        if(player.lampFaith <= 0) {
            player.lampFaith = 0;
            player.lampActive = false;
        }
    } else if(!player.lampActive && player.lampFaith < player.maxLampFaith) {
        player.lampFaith += 5.0f * dt;
    }
    
    // Aim
    Vector3 aimPoint = GetAimPoint();
    Vector3 toAim = Vector3Subtract(aimPoint, player.pos);
    toAim.y = 0;
    if(Vector3Length(toAim) > 0.1f) {
        player.rotation = atan2f(toAim.x, toAim.z);
    }
    
    // Movement
    Vector3 input = {0, 0, 0};
    if(IsKeyDown(KEY_W)) input.z += 1;
    if(IsKeyDown(KEY_S)) input.z -= 1;
    if(IsKeyDown(KEY_D)) input.x += 1;
    if(IsKeyDown(KEY_A)) input.x -= 1;
    
    bool moving = Vector3Length(input) > 0.1f;
    
    Vector3 camDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    camDir.y = 0;
    camDir = Vector3Normalize(camDir);
    Vector3 camRight = Vector3CrossProduct(camDir, {0, 1, 0});
    
    Vector3 moveDir = Vector3Add(Vector3Scale(camDir, input.z), Vector3Scale(camRight, input.x));
    if(moving) moveDir = Vector3Normalize(moveDir);
    
    float maxSpeed = player.moveSpeed;
    if(IsKeyDown(KEY_LEFT_SHIFT) && moving && player.stamina > 10) {
        maxSpeed *= SPRINT_MULT;
        player.stamina -= 12.0f * dt;
    }
    
    // Dodge roll trigger
    if(IsKeyPressed(KEY_SPACE) && !player.isRolling && player.stamina >= ROLL_COST && moving) {
        player.isRolling = true;
        player.rollTimer = ROLL_DURATION;
        player.rollDir = moveDir;
        player.stamina -= ROLL_COST;
        player.invulnTimer = ROLL_DURATION;
        // Immediate velocity burst
        player.vel = Vector3Scale(player.rollDir, ROLL_SPEED);
    }
    
    // Physics-based Movement
    if (player.isRolling) {
        // Roll overrides normal physics with fixed velocity
        player.vel = Vector3Scale(player.rollDir, ROLL_SPEED);
        
        // Spawn ghost trail
        if (GetRandomValue(0, 10) > 7) {
            Ghost g;
            g.pos = player.pos;
            g.rotation = player.rotation;
            g.lifetime = 0.4f;
            world.ghosts.push_back(g);
        }
    } 
    else if (!player.isHealing) {
        // Apply Acceleration
        if (moving) {
            float accelMult = 1.0f;
            // Active Counter-Thrust: If pushing opposite to velocity, apply more force to stop/turn
            if (Vector3DotProduct(player.vel, moveDir) < 0) {
                accelMult = 1.5f; // Reduced braking power for heavier feel
            }
            player.vel = Vector3Add(player.vel, Vector3Scale(moveDir, PLAYER_ACCEL * accelMult * dt));
        }
        
        // Apply Friction (Heavy Inertia) - Linear decay
        player.vel = Vector3Scale(player.vel, 1.0f / (1.0f + PLAYER_FRICTION * dt));
        
        // Clamp speed
        if (Vector3Length(player.vel) > maxSpeed) {
            player.vel = Vector3Scale(Vector3Normalize(player.vel), maxSpeed);
        }
    }
    
    // Apply Velocity
    Vector3 nextPos = Vector3Add(player.pos, Vector3Scale(player.vel, dt));
    
    // Engine Exhaust Particles
    if (Vector3Length(player.vel) > 5.0f && GetRandomValue(0, 5) > 3) {
        Vector3 exhaustPos = Vector3Subtract(player.pos, Vector3Scale(Vector3Normalize(player.vel), 1.0f));
        SpawnParticles(exhaustPos, Fade(GetWaveColor(), 0.3f), 1, 2.0f);
    }

    // Ruin Collisions (Simple AABB)
    bool collidedX = false;
    bool collidedZ = false;
    float playerRadius = 1.8f; // Increased for larger hull

    for (auto& r : world.ruins) {
        // Horizontal Collision
        if (nextPos.x + playerRadius > r.pos.x - r.size.x/2.0f && 
            nextPos.x - playerRadius < r.pos.x + r.size.x/2.0f &&
            player.pos.z + playerRadius > r.pos.z - r.size.z/2.0f &&
            player.pos.z - playerRadius < r.pos.z + r.size.z/2.0f) {
            collidedX = true;
        }
        // Vertical Collision
        if (player.pos.x + playerRadius > r.pos.x - r.size.x/2.0f && 
            player.pos.x - playerRadius < r.pos.x + r.size.x/2.0f &&
            nextPos.z + playerRadius > r.pos.z - r.size.z/2.0f &&
            nextPos.z - playerRadius < r.pos.z + r.size.z/2.0f) {
            collidedZ = true;
        }
    }

    if (collidedX) {
        player.vel.x *= -0.2f; // Slight bounce
        ShakeScreen(0.2f);
    } else {
        player.pos.x = nextPos.x;
    }

    if (collidedZ) {
        player.vel.z *= -0.2f; // Slight bounce
        ShakeScreen(0.2f);
    } else {
        player.pos.z = nextPos.z;
    }
    
    // Bounds - Final Field Scale
    player.pos.x = Clamp(player.pos.x, -500, 500);
    player.pos.z = Clamp(player.pos.z, -500, 500);
    player.pos.y = 1;
    
    // Shooting
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON) && player.shootCooldown <= 0 && !player.isHealing) {
        Vector3 shootDir = Vector3Normalize(toAim);
        Vector3 muzzle = Vector3Add(player.pos, Vector3Scale(shootDir, 2.0f));
        muzzle.y = 1.5f;
        
        // Get current weapon data
        WeaponData* weapon = GetCurrentWeaponData();
        if(!weapon) return; // No weapon somehow
        
        float damage = player.baseDamage;
        Color bulletCol = weapon->bulletColor;
        float bulletSize = weapon->bulletSize;
        int projectileCount = weapon->projectileCount;
        
        // Apply equipment bonuses (Double Damage)
        bool doubleDamageActive = false;
        if(player.equippedTalisman && player.equippedTalisman->hasDoubleDamage && GetRandomValue(0, 100) < 20) {
            damage *= 2.0f;
            doubleDamageActive = true;
            SpawnParticles(muzzle, GOLD, 15, 15.0f);
        }
        if(player.equippedRing && player.equippedRing->hasDoubleDamage && GetRandomValue(0, 100) < 20) {
            damage *= 2.0f;
            doubleDamageActive = true;
        }
        
        // Handle different weapon fire patterns
        if(weapon->type == WEAPON_SHOTGUN) {
            // Shotgun: Fire 5 pellets with spread
            for(int i = 0; i < 5; i++) {
                float spread = (i - 2) * 0.2f;
                float angle = atan2f(shootDir.z, shootDir.x) + spread;
                Vector3 finalDir = Vector3Normalize((Vector3){cosf(angle), 0, sinf(angle)});
                FireBullet(muzzle, Vector3Scale(finalDir, player.bulletSpeed), 
                          bulletCol, true, damage, bulletSize);
            }
        } else if(weapon->type == WEAPON_DUAL_PISTOLS) {
            // Dual pistols: Fire 2 bullets with slight spread
            for(int i = 0; i < 2; i++) {
                float spread = (i == 0) ? -0.08f : 0.08f;
                float angle = atan2f(shootDir.z, shootDir.x) + spread;
                Vector3 finalDir = Vector3Normalize((Vector3){cosf(angle), 0, sinf(angle)});
                FireBullet(muzzle, Vector3Scale(finalDir, player.bulletSpeed), 
                          bulletCol, true, damage, bulletSize);
            }
        } else if(weapon->type == WEAPON_BURST_RIFLE) {
            // Burst rifle: Fire 1 bullet now (will fire 3 total with fire rate)
            FireBullet(muzzle, Vector3Scale(shootDir, player.bulletSpeed), 
                      bulletCol, true, damage, bulletSize);
        } else {
            // All other weapons: Single shot
            FireBullet(muzzle, Vector3Scale(shootDir, player.bulletSpeed), 
                      bulletCol, true, damage, bulletSize);
        }
        
        player.shootCooldown = player.shootRate;
        
        // Weapon-specific visual effects
        switch(weapon->type) {
            case WEAPON_PISTOL:
                SpawnParticles(muzzle, SKYBLUE, 8, 10.0f);
                break;
            case WEAPON_REVOLVER:
                SpawnParticles(muzzle, WHITE, 15, 18.0f);
                SpawnParticles(muzzle, GRAY, 10, 15.0f);
                break;
            case WEAPON_BURST_RIFLE:
                SpawnParticles(muzzle, (Color){100, 200, 255, 255}, 10, 12.0f);
                break;
            case WEAPON_SHOTGUN:
                SpawnParticles(muzzle, VIOLET, 25, 18.0f);
                SpawnParticles(muzzle, ORANGE, 20, 15.0f);
                break;
            case WEAPON_DUAL_PISTOLS:
                SpawnParticles(muzzle, (Color){100, 255, 255, 255}, 16, 14.0f);
                break;
            case WEAPON_RIFLE:
                SpawnParticles(muzzle, ORANGE, 15, 22.0f);
                SpawnParticles(muzzle, YELLOW, 12, 20.0f);
                break;
            case WEAPON_SMG:
                SpawnParticles(muzzle, YELLOW, 6, 8.0f);
                break;
            case WEAPON_FLAMETHROWER:
                SpawnParticles(muzzle, (Color){255, 140, 0, 255}, 18, 10.0f);
                SpawnParticles(muzzle, VIOLET, 15, 8.0f);
                break;
            case WEAPON_RAILGUN:
                SpawnParticles(muzzle, PURPLE, 20, 28.0f);
                SpawnParticles(muzzle, VIOLET, 15, 25.0f);
                break;
            case WEAPON_LIGHTNING_GUN:
                SpawnParticles(muzzle, (Color){200, 200, 255, 255}, 25, 20.0f);
                SpawnParticles(muzzle, WHITE, 18, 18.0f);
                break;
            case WEAPON_LAUNCHER:
                SpawnParticles(muzzle, ORANGE, 30, 22.0f);
                SpawnParticles(muzzle, VIOLET, 25, 20.0f);
                SpawnParticles(muzzle, YELLOW, 20, 18.0f);
                break;
            case WEAPON_DIVINE_BEAM:
                SpawnParticles(muzzle, GOLD, 25, 15.0f);
                SpawnParticles(muzzle, YELLOW, 20, 12.0f);
                break;
            default:
                SpawnParticles(muzzle, YELLOW, 8, 10.0f);
                break;
        }
        
        // Additional explosive effect from equipment
        if(player.equippedTalisman && player.equippedTalisman->hasExplosiveShots) {
            SpawnParticles(muzzle, ORANGE, 12, 12.0f);
        }
    }
    
    // Weapon Switching
    for(int i = 0; i < 9; i++) {
        if(IsKeyPressed(KEY_ONE + i)) {
            // Find the i-th unlocked weapon
            int unlockedIndex = 0;
            for(auto& w : world.weaponArsenal) {
                if(w.unlocked) {
                    if(unlockedIndex == i) {
                        SwitchWeapon(w.type);
                        break;
                    }
                    unlockedIndex++;
                }
            }
        }
    }
    
    // Parry
    if(IsKeyPressed(KEY_Q) && player.stamina >= PRAYER_COST && !player.isParrying) {
        player.isParrying = true;
        player.parryTimer = PRAYER_WINDOW;
        player.stamina -= PRAYER_COST;
        SpawnParticles(player.pos, GOLD, 15, 8.0f);
    }
    
    // Lamp toggle
    if(IsKeyPressed(KEY_L)) {
        player.lampActive = !player.lampActive;
    }
    
    // Flask
    if(IsKeyPressed(KEY_E) && player.flasks > 0 && !player.isHealing && player.health < player.maxHealth) {
        player.isHealing = true;
        player.healTimer = 1.2f;
        player.flasks--;
    }
    if(player.healTimer <= 0.5f && player.healTimer > 0) {
        int healAmount = 40 + player.faith * 2;
        player.health = std::min(player.health + healAmount, player.maxHealth);
    }
    
    // Protocol Override (Ultimate)
    if (IsKeyPressed(KEY_R) && player.syncMeter >= player.maxSyncMeter) {
        player.syncMeter = 0;
        ShakeScreen(2.5f);
        world.message = "PROTOCOL OVERRIDE: RE-CODING...";
        world.messageTimer = 2.0f;
        
        // Shockwave effect
        SpawnParticles(player.pos, GOLD, 100, 30.0f);
        
        // Turn all nearby enemy bullets into tokens
        for (auto it = world.bullets.begin(); it != world.bullets.end();) {
            if (!it->playerBullet && Vector3Distance(player.pos, it->pos) < 40.0f) {
                // Drop a token at bullet position
                DropTokens(it->pos, 10);
                SpawnParticles(it->pos, GOLD, 5, 5.0f);
                it = world.bullets.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Sanctuary check
    for(auto& s : world.sanctuaries) {
        if(Vector3Distance(player.pos, s.pos) < s.radius) {
            if (!s.discovered) {
                s.discovered = true;
                world.message = "NEURAL LINK ESTABLISHED: " + s.name;
                world.messageTimer = 3.0f;
                SpawnParticles(s.pos, GOLD, 50, 15.0f);
            }
            if(IsKeyPressed(KEY_F)) {
                world.currentSanctuary = &s;
                player.health = player.maxHealth;
                player.stamina = player.maxStamina;
                player.lampFaith = player.maxLampFaith;
                player.flasks = player.maxFlasks;
                world.selectedSanctuaryOption = 0; // Reset selection
                state = SANCTUARY_MENU;
            }
        }
    }
    
    // Item collection
    for(auto& item : world.items) {
        if(!item.collected && Vector3Distance(player.pos, item.pos) < 2.0f) {
            item.collected = true;
            SpawnParticles(item.pos, GOLD, 25, 12.0f);
            
            switch(item.type) {
                case 0: player.vigor += 2; player.maxHealth += 10; player.health = player.maxHealth; break;
                case 1: player.will += 2; player.maxStamina += 12; player.stamina = player.maxStamina; break;
                case 2: player.faith += 2; player.maxLampFaith += 15; player.lampFaith = player.maxLampFaith; break;
                case 3: player.maxFlasks++; player.flasks = player.maxFlasks; break;
            }
            
            world.message = item.name + " acquired!";
            world.messageTimer = 3.0f;
        }
    }
    
    // Grace recovery
    if(world.hasGraceToRecover && Vector3Distance(player.pos, player.renewalPos) < 3.0f) {
        player.grace += player.graceAtDeath;
        player.graceAtDeath = 0;
        world.hasGraceToRecover = false;
        world.message = "Grace recovered!";
        world.messageTimer = 3.0f;
        SpawnParticles(player.renewalPos, GOLD, 40, 15.0f);
    }
    
    // Collect tokens
    for(auto it = world.tokens.begin(); it != world.tokens.end();) {
        it->lifetime -= dt;
        
        Vector3 toPlayer = Vector3Subtract(player.pos, it->pos);
        float dist = Vector3Length(toPlayer);
        
        if(dist < 2.0f || it->lifetime <= 0) {
            if(dist < 2.0f) {
                player.lightTokens += it->value;
                SpawnParticles(it->pos, YELLOW, 5, 8.0f);
            }
            it = world.tokens.erase(it);
        } else if(dist < 8.0f) {
            // Magnetically pull tokens to player
            it->pos = Vector3Add(it->pos, Vector3Scale(Vector3Normalize(toPlayer), 12.0f * dt));
            ++it;
        } else {
            ++it;
        }
    }
    
    // Collect equipment drops
    for(auto it = world.equipmentDrops.begin(); it != world.equipmentDrops.end();) {
        it->lifetime -= dt;
        
        if(Vector3Distance(player.pos, it->pos) < 2.0f && !it->collected) {
            it->collected = true;
            player.inventory.push_back(it->equipment);
            world.message = "Found: " + it->equipment.name + "!";
            world.messageTimer = 3.0f;
            SpawnParticles(it->pos, GetRarityColor(it->equipment.rarity), 25, 15.0f);
        }
        
        if(it->collected || it->lifetime <= 0) {
            it = world.equipmentDrops.erase(it);
        } else {
            ++it;
        }
    }
    
    if(world.messageTimer > 0) world.messageTimer -= dt;
}

void UpdateEnemies(float dt) {
    for(auto& e : world.enemies) {
        if(!e.isAlive) continue;
        
                    // Defeat Animation Logic
                    if (e.aiState == 5) {
                        e.defeatTimer -= dt;
                        e.pos.y += 30.0f * dt; // Rapid beam up
                        e.scale = e.defeatTimer; // Shrink to zero
                        if (e.defeatTimer <= 0) e.isAlive = false;
                        continue;
                    }        
        e.shootTimer -= dt;
        e.actionTimer = std::max(0.0f, e.actionTimer - dt);
        e.abilityCooldown = std::max(0.0f, e.abilityCooldown - dt);
        
        Vector3 toPlayer = Vector3Subtract(player.pos, e.pos);
        toPlayer.y = 0;
        float dist = Vector3Length(toPlayer);
        Vector3 dirToPlayer = Vector3Normalize(toPlayer);
        
        if(dist > 1.0f) {
            e.rotation = atan2f(toPlayer.x, toPlayer.z);
        }
        
        // --- AI LOGIC ---
        
        // 1. Dodge Logic (For agile enemies)
        if (e.aiState != 2 && e.abilityCooldown <= 0 && (e.type == WATCHER || e.type == WHISPERER || e.type == GLITCH_SPECTRE)) {
            // Check for incoming bullets
            for (const auto& b : world.bullets) {
                if (b.playerBullet && Vector3Distance(e.pos, b.pos) < 15.0f) {
                    // Dodge perpendicular to bullet
                    Vector3 side = Vector3CrossProduct(b.vel, {0, 1, 0});
                    side = Vector3Normalize(side);
                    if (GetRandomValue(0, 1) == 0) side = Vector3Negate(side);
                    
                    e.aiState = 2; // DODGING
                    e.actionTimer = 0.4f; // Dodge duration
                    e.vel = Vector3Scale(side, 40.0f); // DASH SPEED
                    e.abilityCooldown = 2.0f + (GetRandomValue(0, 20)/10.0f);
                    SpawnParticles(e.pos, e.color, 10, 5.0f);
                    break;
                }
            }
        }
        
        // 2. Sprint/Chase Logic (For aggressors)
        if (e.aiState == 0 && (e.type == ASHBOUND || e.type == BOSS_KEEPER) && dist > 40.0f && e.abilityCooldown <= 0) {
            e.aiState = 3; // CHARGING
            e.actionTimer = 1.5f;
            e.abilityCooldown = 4.0f;
            e.chargeDir = dirToPlayer;
            if(e.type == BOSS_KEEPER) {
                world.message = "DIVINE WRATH IMMINENT";
                world.messageTimer = 1.0f;
                SpawnParticles(e.pos, GOLD, 30, 10.0f);
            }
        }

        // 3. Boss Defensive Logic (Neural Barrier)
        if (e.isBoss && e.aiState == 0 && e.abilityCooldown <= 0 && GetRandomValue(0, 100) < 30) {
            e.aiState = 4; // SHIELDING (Re-using state 4)
            e.actionTimer = 2.0f;
            e.abilityCooldown = 6.0f;
            world.message = "HEAVENLY AEGIS MANIFESTED";
            world.messageTimer = 1.5f;
        }
        
        // --- MOVEMENT APPLICATION ---
        
        Vector3 moveForce = {0, 0, 0};
        
        // State-based movement override
        if (e.aiState == 2) { // Dodging
            // Velocity is already set, just decay it less for smooth dash
            // No steering input while dodging
        } 
        else if (e.aiState == 3) { // Charging
            // Accelerate heavily in charge direction
            moveForce = Vector3Scale(e.chargeDir, e.moveSpeed * 8.0f); 
            // Turning while charging is hard (limited steering)
            moveForce = Vector3Add(moveForce, Vector3Scale(dirToPlayer, e.moveSpeed * 2.0f));
            
            // End temporary states
            if (e.actionTimer <= 0) e.aiState = 0;
        }
        else if (e.aiState == 4) { // Shielding
            // No steering input, but decay velocity
            e.vel = Vector3Scale(e.vel, 0.95f);
            if (e.actionTimer <= 0) e.aiState = 0;
        }
        else if(e.isBoss) {
            // Boss steering: Seek player but maintain distance (Phase 1)
            // Phase 2+ (Aggressive) logic handled by Charge state above
            if(dist > 35.0f) {
                moveForce = Vector3Scale(dirToPlayer, e.moveSpeed * 2.0f);
            } else if(dist < 20.0f) {
                moveForce = Vector3Scale(Vector3Negate(dirToPlayer), e.moveSpeed * 2.0f);
            }
        } else if(e.type == GLITCH_SPECTRE) {
            // Glitch spectre still uses teleport logic primarily
            e.teleportTimer -= dt;
            if(e.teleportTimer <= 0) {
                float angle = GetRandomValue(0, 360) * DEG2RAD;
                float radius = 15.0f + GetRandomValue(0, 10);
                SpawnParticles(e.pos, e.color, 20, 10.0f); 
                e.pos = { player.pos.x + cosf(angle) * radius, 0, player.pos.z + sinf(angle) * radius };
                e.vel = {0, 0, 0}; 
                SpawnParticles(e.pos, e.color, 20, 10.0f);
                e.teleportTimer = 2.0f + (GetRandomValue(0, 100) / 100.0f);
            }
        } else {
            // Regular enemies seek player
            if(dist > 15.0f || e.type == ASHBOUND) { // Ashbound always close in
                float speed = e.moveSpeed;
                if(player.lampActive && dist < 15.0f) speed *= 0.6f; 
                moveForce = Vector3Scale(dirToPlayer, speed * 5.0f);
            }
        }
        
        // Apply Physics to Enemy
        if (e.type != GLITCH_SPECTRE || e.aiState == 2) { // Allow physics for spectre dodge
            e.vel = Vector3Add(e.vel, Vector3Scale(moveForce, dt));
            e.vel = Vector3Scale(e.vel, 0.92f); // Friction
            e.pos = Vector3Add(e.pos, Vector3Scale(e.vel, dt));
        }
        
        // Shooting
        if(e.shootTimer <= 0 && dist < 60.0f) {
            Vector3 dir = Vector3Normalize(toPlayer);
            
            switch(e.type) {
                case HOLLOWED:
                    FireBullet(Vector3Add(e.pos, {0, 2, 0}), Vector3Scale(dir, ENEMY_BULLET_SPEED), ORANGE, false, 10, 0.3f);
                    e.shootTimer = e.shootCooldown;
                    break;
                    
                case ASHBOUND:
                    for(int i = 0; i < 3; i++) {
                        Vector3 spread = {dir.x + (i-1)*0.2f, 0, dir.z + (i-1)*0.2f};
                        spread = Vector3Normalize(spread);
                        FireBullet(Vector3Add(e.pos, {0, 2, 0}), Vector3Scale(spread, ENEMY_BULLET_SPEED), ORANGE, false, 12, 0.3f);
                    }
                    e.shootTimer = e.shootCooldown;
                    break;
                    
                case WATCHER:
                    {
                        float sSpeed = ENEMY_BULLET_SPEED;
                        if (dist > 150.0f) sSpeed *= 4.0f; // Artillery Protocol
                        FireBullet(Vector3Add(e.pos, {0, 2, 0}), Vector3Scale(dir, sSpeed), GRAY, false, 15, 0.35f);
                        e.shootTimer = e.shootCooldown;
                    }
                    break;
                    
                case WHISPERER:
                    for(int i = -1; i <= 1; i++) {
                        float angle = atan2f(dir.z, dir.x) + i * 0.3f;
                        Vector3 bulletDir = {cosf(angle), 0, sinf(angle)};
                        FireBullet(Vector3Add(e.pos, {0, 2, 0}), Vector3Scale(bulletDir, ENEMY_BULLET_SPEED), PURPLE, false, 10, 0.25f);
                    }
                    e.shootTimer = e.shootCooldown;
                    break;
                    
                case SPIRAL:
                    for(int i = 0; i < 8; i++) {
                        float angle = e.patternAngle + i * (2 * PI / 8);
                        Vector3 spiralDir = {cosf(angle), 0, sinf(angle)};
                        FireBullet(Vector3Add(e.pos, {0, 2, 0}), Vector3Scale(spiralDir, ENEMY_BULLET_SPEED), VIOLET, false, 12, 0.3f);
                    }
                    e.patternAngle += 0.4f;
                    e.shootTimer = e.shootCooldown;
                    break;
                    
                case GLITCH_SPECTRE:
                    // Fires 2 quick shots
                    for(int i = 0; i < 2; i++) {
                        Vector3 bulletDir = Vector3Normalize(toPlayer);
                        FireBullet(Vector3Add(e.pos, {0, 1.5f, 0}), Vector3Scale(bulletDir, ENEMY_BULLET_SPEED * 1.3f), (Color){50, 255, 100, 255}, false, 8, 0.25f);
                    }
                    e.shootTimer = e.shootCooldown;
                    break;
                    
                case BOSS_KEEPER:
                    // Boss bullet hell patterns based on phase
                    if(e.health > 1000) e.bossPhase = 1;
                    else if(e.health > 500) e.bossPhase = 2;
                    else e.bossPhase = 3;
                    
                    float bSpeedMult = (dist > 200.0f) ? 3.0f : 1.0f; // Boss Artillery

                    if(e.bossPhase == 1) {
                        for(int i = 0; i < 16; i++) {
                            float angle = e.patternAngle + i * (2 * PI / 16);
                            Vector3 bossDir = {cosf(angle), 0, sinf(angle)};
                            FireBullet(Vector3Add(e.pos, {0, 4, 0}), Vector3Scale(bossDir, ENEMY_BULLET_SPEED * bSpeedMult), PURPLE, false, 15, 0.5f);
                        }
                        e.patternAngle += 0.3f;
                    } else if(e.bossPhase == 2) {
                        for(int i = 0; i < 24; i++) {
                            float angle = i * (2 * PI / 24);
                            Vector3 bossDir = {cosf(angle), 0, sinf(angle)};
                            FireBullet(Vector3Add(e.pos, {0, 4, 0}), Vector3Scale(bossDir, ENEMY_BULLET_SPEED * 1.1f * bSpeedMult), VIOLET, false, 18, 0.5f);
                        }
                    } else {
                        for(int i = 0; i < 32; i++) {
                            float angle = (i * 2 * PI / 32) + GetTime() * 2.0f;
                            Vector3 bossDir = {cosf(angle), 0, sinf(angle)};
                            FireBullet(Vector3Add(e.pos, {0, 4, 0}), Vector3Scale(bossDir, ENEMY_BULLET_SPEED * 1.2f * bSpeedMult), DARKPURPLE, false, 20, 0.6f);
                        }
                    }
                    e.shootTimer = e.shootCooldown;
                    break;
            }
        }
    }
}

void FireBullet(Vector3 pos, Vector3 vel, Color col, bool playerBullet, float damage, float size) {
    Bullet b;
    b.pos = pos;
    b.vel = vel;
    b.color = col;
    b.lifetime = 6.0f;
    b.playerBullet = playerBullet;
    b.reflected = false;
    b.damage = damage;
    b.size = size;
    world.bullets.push_back(b);
    
    // Muzzle Flash
    SpawnParticles(pos, Fade(col, 0.8f), 5, 2.0f);
    SpawnParticles(pos, WHITE, 2, 1.5f); // Core flash
}

void UpdateBullets(float dt) {
    const float GRID_SIZE = 50.0f;
    const int GRID_COLS = 20; // 1000 / 50
    const int GRID_ROWS = 20;
    std::vector<Enemy*> grid[GRID_COLS][GRID_ROWS];

    // Populate grid
    for (auto& e : world.enemies) {
        if (!e.isAlive || e.aiState == 5) continue;
        int gx = (int)((e.pos.x + 500.0f) / GRID_SIZE);
        int gz = (int)((e.pos.z + 500.0f) / GRID_SIZE);
        if (gx >= 0 && gx < GRID_COLS && gz >= 0 && gz < GRID_ROWS) {
            grid[gx][gz].push_back(&e);
        }
    }

    for (auto it = world.bullets.begin(); it != world.bullets.end();) {
        it->lifetime -= dt;
        
        // Homing behavior
        WeaponData* weapon = GetCurrentWeaponData();
        bool augmentHoming = (player.equippedTalisman && player.equippedTalisman->hasHomingShots) || 
                             (player.equippedRing && player.equippedRing->hasHomingShots);
                             
        if (it->playerBullet && weapon && (weapon->homing || augmentHoming)) {
            Enemy* nearest = nullptr;
            float minDist = 999999.0f;
            int gx = (int)((it->pos.x + 500.0f) / GRID_SIZE);
            int gz = (int)((it->pos.z + 500.0f) / GRID_SIZE);
            
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dz = -1; dz <= 1; ++dz) {
                    int nx = gx + dx;
                    int nz = gz + dz;
                    if (nx >= 0 && nx < GRID_COLS && nz >= 0 && nz < GRID_ROWS) {
                        for (Enemy* e : grid[nx][nz]) {
                            float dist = Vector3Distance(it->pos, e->pos);
                            if (dist < minDist && dist < 100.0f) { // Increased homing search range
                                minDist = dist;
                                nearest = e;
                            }
                        }
                    }
                }
            }
            if (nearest) {
                Vector3 toEnemy = Vector3Normalize(Vector3Subtract(nearest->pos, it->pos));
                float speed = Vector3Length(it->vel);
                it->vel = Vector3Lerp(it->vel, Vector3Scale(toEnemy, speed), 0.08f);
            }
        }
        
        it->pos = Vector3Add(it->pos, Vector3Scale(it->vel, dt));
        bool bulletDestroyed = false;

        // Ruin Collision for Bullets
        for (auto& r : world.ruins) {
            if (it->pos.x > r.pos.x - r.size.x/2.0f && it->pos.x < r.pos.x + r.size.x/2.0f &&
                it->pos.z > r.pos.z - r.size.z/2.0f && it->pos.z < r.pos.z + r.size.z/2.0f &&
                it->pos.y < r.pos.y + r.size.y/2.0f) {
                bulletDestroyed = true;
                SpawnParticles(it->pos, VIOLET, 5, 3.0f);
                break;
            }
        }
        
        if (bulletDestroyed) {
            it = world.bullets.erase(it);
            continue;
        }
        
        if (it->lifetime <= 0 || Vector3Length(it->pos) > 800) {
            it = world.bullets.erase(it);
            continue;
        }
        
        // Parry check
        if (!it->playerBullet && player.isParrying && Vector3Distance(it->pos, player.pos) < PRAYER_RANGE) {
            it->vel = Vector3Scale(Vector3Negate(it->vel), 2.0f);
            it->playerBullet = true;
            it->reflected = true;
            it->color = GOLD;
            it->damage *= 2;
            player.combo++;
            player.score += 50;
            player.syncMeter = std::min(player.maxSyncMeter, player.syncMeter + 20.0f);
            ShakeScreen(0.4f);
            SpawnParticles(it->pos, GOLD, 20, 15.0f);
        }
        
        if (it->playerBullet) {
            bool hit = false;
            bool augmentPiercing = (player.equippedArmor && player.equippedArmor->hasPiercing) ||
                                   (player.equippedTalisman && player.equippedTalisman->hasPiercing) ||
                                   (player.equippedRing && player.equippedRing->hasPiercing);
            bool piercing = weapon && (weapon->piercing || augmentPiercing);
            bool explosive = weapon && weapon->explosive;
            
            int gx = (int)((it->pos.x + 500.0f) / GRID_SIZE);
            int gz = (int)((it->pos.z + 500.0f) / GRID_SIZE);
            
            for (int dx = -1; dx <= 1 && !hit; ++dx) {
                for (int dz = -1; dz <= 1 && !hit; ++dz) {
                    int nx = gx + dx;
                    int nz = gz + dz;
                    if (nx >= 0 && nx < GRID_COLS && nz >= 0 && nz < GRID_ROWS) {
                        auto& cell = grid[nx][nz];
                        for (size_t i = 0; i < cell.size(); ++i) {
                            Enemy& e = *cell[i];
                            if (e.isAlive && e.aiState != 5 && Vector3Distance(it->pos, e.pos) < e.scale * 2.0f) {                                                            // Neural Barrier Check
                                                            if (e.aiState == 4) {
                                                                SpawnParticles(it->pos, GOLD, 5, 2.0f);
                                                                bulletDestroyed = true;
                                                                hit = true;
                                                                break;
                                                            }
                                                            e.health -= (int)it->damage;
                                                            ShakeScreen(0.2f);
                                                            SpawnParticles(it->pos, it->reflected ? GOLD : it->color, 10, 10.0f);
                                                            player.score += it->reflected ? 100 : 30;
                                                            
                                                            bool augmentExplosive = (player.equippedArmor && player.equippedArmor->hasExplosiveShots) ||
                                                                                    (player.equippedTalisman && player.equippedTalisman->hasExplosiveShots) ||
                                                                                    (player.equippedRing && player.equippedRing->hasExplosiveShots);
                                                                                    
                                                            if (explosive || augmentExplosive) {
                                                                ShakeScreen(0.8f);
                                                                SpawnParticles(it->pos, ORANGE, 40, 22.0f);                                    SpawnParticles(it->pos, VIOLET, 30, 18.0f);
                                    SpawnParticles(it->pos, YELLOW, 25, 15.0f);
                                    float radius = explosive ? 8.0f : 5.0f;
                                    for (auto& e2 : world.enemies) {
                                        if (e2.isAlive && &e2 != &e && Vector3Distance(it->pos, e2.pos) < radius) {
                                            e2.health -= (int)(it->damage * (explosive ? 0.7f : 0.5f));
                                            if (e2.health <= 0) {
                                                e2.aiState = 5;
                                                e2.defeatTimer = 1.0f;
                                                player.kills++;
                                                DropGrace(e2.pos, e2.graceReward);
                                                DropTokens(e2.pos, e2.graceReward / 2);
                                            }
                                        }
                                    }
                                }
                                
                                if (player.equippedTalisman && player.equippedTalisman->hasLifesteal) {
                                    player.health = std::min(player.health + (int)(it->damage * 0.1f), player.maxHealth);
                                }
                                
                                if (e.health <= 0) {
                                    e.aiState = 5;
                                    e.defeatTimer = 1.0f;
                                    player.kills++;
                                    player.combo += 5;
                                    player.score += 500;
                                    player.syncMeter = std::min(player.maxSyncMeter, player.syncMeter + 5.0f);
                                    DropGrace(e.pos, e.graceReward);
                                    DropTokens(e.pos, e.graceReward / 2);
                                    DropEquipment(e.pos, world.wave);
                                    if (e.hasWeaponDrop) {
                                        player.lightTokens += 150;
                                        std::string dropName = "Blueprint";
                                        // Find the actual weapon name and unlock blueprint
                                        for(auto& w : world.weaponArsenal) {
                                            if(w.type == e.weaponDrop) {
                                                dropName = w.name;
                                                w.blueprintFound = true; // UNLOCK IN SHOP
                                                break;
                                            }
                                        }
                                        world.message = "Blueprint: " + dropName;
                                        world.messageTimer = 3.0f;
                                    }
                                    SpawnParticles(e.pos, VIOLET, 40, 18.0f);
                                }
                                hit = true;
                                if (!piercing) {
                                    bulletDestroyed = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            if (Vector3Distance(it->pos, player.pos) < 1.5f && player.invulnTimer <= 0) {
                DamagePlayer((int)it->damage);
                bulletDestroyed = true;
            }
        }
        
        if (bulletDestroyed) {
            it = world.bullets.erase(it);
        } else {
            ++it;
        }
    }
}

void DamagePlayer(int damage) {
    if (debugMode) return;
    player.health -= damage;
    player.invulnTimer = 0.5f;
    player.combo = 0;
    ShakeScreen(1.2f);
    SpawnParticles(player.pos, GOLD, 25, 12.0f);
    
    if(player.health <= 0) {
        PlayerNeedsReboot();
    }
}

void PlayerNeedsReboot() {
    player.needsReboot = true;
    player.renewalPos = player.pos;
    player.graceAtDeath = player.grace;
    player.grace = 0;
    world.hasGraceToRecover = player.graceAtDeath > 0;
    state = RENEWAL;
}

void RebootSystem() {
    player.pos = world.currentSanctuary->pos;
    player.vel = {0, 0, 0};
    player.health = player.maxHealth;
    player.stamina = player.maxStamina;
    player.lampFaith = player.maxLampFaith;
    player.flasks = player.maxFlasks;
    player.needsReboot = false;
    player.syncMeter = 0;
    
    // Clear all lingering objects
    world.bullets.clear();
    world.particles.clear();
    world.tokens.clear();
    world.ghosts.clear();
    world.equipmentDrops.clear();
    
    // Completely Reset the Current Trial
    SpawnWave(world.wave);
    
    world.message = "SIMULATION RE-INITIALIZED";
    world.messageTimer = 3.0f;
}

void DropGrace(Vector3 pos, int amount) {
    int orbs = amount / 50;
    for(int i = 0; i < orbs; i++) {
        GraceOrb orb;
        orb.pos = Vector3Add(pos, {GetRandomValue(-30, 30)/10.0f, 2, GetRandomValue(-30, 30)/10.0f});
        orb.timer = 10.0f;
        orb.value = 50;
        world.graceOrbs.push_back(orb);
    }
    player.grace += amount % 50;
}

void CollectGrace(float dt) {
    for(auto it = world.graceOrbs.begin(); it != world.graceOrbs.end();) {
        it->timer -= dt;
        
        Vector3 toPlayer = Vector3Subtract(player.pos, it->pos);
        float dist = Vector3Length(toPlayer);
        
        if(dist < 5.0f || it->timer <= 0) {
            player.grace += it->value;
            it = world.graceOrbs.erase(it);
        } else {
            it->pos = Vector3Add(it->pos, Vector3Scale(Vector3Normalize(toPlayer), 15.0f * dt));
            ++it;
        }
    }
}

void SpawnParticles(Vector3 pos, Color col, int count, float speed) {
    for(int i = 0; i < count; i++) {
        Particle p;
        p.pos = pos;
        Vector3 dir = {GetRandomValue(-100, 100)/100.0f, GetRandomValue(30, 100)/100.0f, GetRandomValue(-100, 100)/100.0f};
        p.vel = Vector3Scale(Vector3Normalize(dir), speed);
        p.color = col;
        p.maxLifetime = 0.5f + GetRandomValue(0, 50)/100.0f;
        p.lifetime = p.maxLifetime;
        p.size = 0.1f + GetRandomValue(0, 20)/100.0f;
        world.particles.push_back(p);
    }
}

void UpdateParticles(float dt) {
    if (world.particles.empty()) return;

    // Sequential Update (Parallel STL policies like std::execution::par are not supported by Emscripten yet)
    std::for_each(world.particles.begin(), world.particles.end(), [dt](Particle& p) {
        p.lifetime -= dt;
        p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
        p.vel.y -= 20.0f * dt;
    });

    // Sequential sweep for deletion
    for (auto it = world.particles.begin(); it != world.particles.end();) {
        if (it->lifetime <= 0) {
            it = world.particles.erase(it);
        } else {
            ++it;
        }
    }
}

void ShakeScreen(float intensity) {
    if (intensity > screenShake) screenShake = intensity;
}

void UpdateCamera() {
    Vector3 desiredPos = Vector3Add(player.pos, {0, CAMERA_HEIGHT, -CAMERA_DISTANCE});
    camera.position = Vector3Lerp(camera.position, desiredPos, 10.0f * GetFrameTime());
    
    // Dynamic FOV based on speed
    float speed = Vector3Length(player.vel);
    float targetFOV = 60.0f + (speed / 28.0f) * 15.0f; // Pull back at high speed
    camera.fovy = Lerp(camera.fovy, targetFOV, 5.0f * GetFrameTime());

    // Apply screen shake
    if (screenShake > 0) {
        camera.position.x += GetRandomValue(-100, 100)/100.0f * screenShake;
        camera.position.y += GetRandomValue(-100, 100)/100.0f * screenShake;
        camera.position.z += GetRandomValue(-100, 100)/100.0f * screenShake;
    }
    
    camera.target = Vector3Add(player.pos, {0, 2, 0});
}

void UpdateGame(float dt) {
    UpdateCamera();
    if (screenShake > 0) screenShake -= 5.0f * dt;
    if (screenShake < 0) screenShake = 0;
    
    UpdatePlayer(dt);
    
    // Update ghosts
    for (auto it = world.ghosts.begin(); it != world.ghosts.end();) {
        it->lifetime -= dt;
        if (it->lifetime <= 0) it = world.ghosts.erase(it);
        else ++it;
    }
    
    UpdateEnemies(dt);
    UpdateBullets(dt);
    CollectGrace(dt);
    UpdateParticles(dt);
    
    // Check wave completion
    bool allDead = true;
    for(auto& e : world.enemies) {
        if(e.isAlive) allDead = false;
    }
    
    if(allDead) {
        // Check if it's a boss wave (every 5 waves)
        bool isBossWave = (world.wave % 5 == 0);
        
        if(isBossWave) {
            // After Wave 15 boss, victory
            if (world.wave >= 15) {
                state = VICTORY;
                return;
            }
            
            // Check if any weapons can be unlocked after this wave
            world.availableUnlocks.clear();
            world.selectedWeapon = 0; // Reset selection
            
            for(auto& w : world.weaponArsenal) {
                if(!w.unlocked && world.wave >= w.unlockWave) {
                    world.availableUnlocks.push_back(w.type);
                }
            }
            
            // If weapons available, show selection screen
            if(!world.availableUnlocks.empty()) {
                world.waveJustCompleted = true;
                state = WEAPON_SELECT;
            } else {
                // No weapons to unlock, continue to next wave
                world.wave++;
                SpawnWave(world.wave);
                player.health = std::min(player.health + 50, player.maxHealth); // Boss wave heal
                world.message = "SACRED ENTITY ASCENDED!";
                world.messageTimer = 4.0f;
            }
        } else {
            // Regular wave completion
            world.wave++;
            SpawnWave(world.wave);
            player.health = std::min(player.health + 30, player.maxHealth);
            world.message = "Spiritual Layer " + std::to_string(world.wave - 1) + " Cleansed!";
            world.messageTimer = 3.0f;
        }
    }
}

// ============================================================================
// DRAWING
// ============================================================================
void DrawGame3D() {
    float time = (float)GetTime();
    Color themeCol = GetWaveColor();

    // Ground: Sacred Obsidian Floor
    rlSetTexture(floorTexture.id);
    rlBegin(RL_QUADS);
        rlNormal3f(0.0f, 1.0f, 0.0f);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(-500, 0, -500);
        rlTexCoord2f(0.0f, 50.0f); rlVertex3f(-500, 0, 500);
        rlTexCoord2f(50.0f, 50.0f); rlVertex3f(500, 0, 500);
        rlTexCoord2f(50.0f, 0.0f); rlVertex3f(500, 0, -500);
    rlEnd();
    rlSetTexture(0);
    
    // Celestial Glow
    DrawCircle3D({0, 0.05f, 0}, 500.0f, {1, 0, 0}, 90.0f, Fade(themeCol, 0.15f));
    
    // Draw Radiant Manna (Ambient Light)
    for (int i = 0; i < 60; i++) {
        float rX = sinf(i * 123.456f) * 500.0f;
        rX = fmodf(rX, 1000.0f) - 500.0f;
        float rZ = cosf(i * 456.789f) * 100.0f;
        rZ = fmodf(rZ, 1000.0f) - 500.0f;
        float rY = fmodf(100.0f - time * 15.0f + (i * 10.0f), 100.0f);
        DrawLine3D({rX, rY, rZ}, {rX, rY + 1.5f, rZ}, Fade(i % 2 == 0 ? GOLD : WHITE, 0.3f));
    }

    // Distant Pillars of Truth
    for (int i = 0; i < 16; i++) {
        float angle = i * (2.0f * PI / 16.0f);
        float dist = 550.0f + sinf(time * 0.2f + i) * 20.0f;
        Vector3 mPos = { cosf(angle) * dist, 40.0f, sinf(angle) * dist };
        DrawCube(mPos, 15.0f, 200.0f, 15.0f, (Color){15, 10, 5, 255});
        DrawCubeWires(mPos, 15.1f, 200.1f, 15.1f, Fade(GOLD, 0.2f));
    }
    
    // Aim indicator - Celestial Seal
    Vector3 aimPoint = GetAimPoint();
    DrawCircle3D(aimPoint, 1.5f, {1, 0, 0}, 90, Fade(themeCol, 0.4f));
    DrawCircle3D(aimPoint, 0.5f, {1, 0, 0}, 90, WHITE);
    
    // Sanctuaries (Altars of Grace)
    for(auto& s : world.sanctuaries) {
        float sPulse = sinf(time * 2.0f) * 0.2f + 0.8f;
        Color sCol = s.discovered ? GOLD : DARKGRAY;
        
        // Sacred Shards
        for (int i = 0; i < 5; i++) {
            float angle = time * 0.5f + (i * PI * 2.0f / 5.0f);
            Vector3 shardPos = { s.pos.x + cosf(angle) * 1.5f, 0.5f, s.pos.z + sinf(angle) * 1.5f };
            DrawCube(shardPos, 0.4f, 1.0f, 0.4f, (Color){20, 15, 10, 255});
            DrawCubeWires(shardPos, 0.42f, 1.02f, 0.42f, sCol);
        }

        if (s.discovered) {
            // Sacred Circle
            DrawCircle3D(s.pos, s.radius, {1, 0, 0}, 90.0f, Fade(GOLD, 0.3f * sPulse));
            DrawCircle3D(s.pos, s.radius * 0.8f, {1, 0, 0}, 90.0f, Fade(WHITE, 0.2f));
            
            // Sacred Light (Pulsing Cubes)
            for (int i = 0; i < 3; i++) {
                float fOffset = fmodf(time * 0.5f + (i * 0.33f), 1.0f);
                float fSize = (1.0f - fOffset) * 0.6f;
                Vector3 fPos = { s.pos.x, 1.0f + fOffset * 3.5f, s.pos.z };
                DrawCube(fPos, fSize, fSize, fSize, Fade(GOLD, 1.0f - fOffset));
                DrawCubeWires(fPos, fSize + 0.05f, fSize + 0.05f, fSize + 0.05f, WHITE);
            }
        }
        
        // Interaction Prompt Indicator
        float dist = Vector3Distance(player.pos, s.pos);
        if (dist < s.radius) {
            Vector3 promptPos = Vector3Add(s.pos, {0, 4.0f + sinf(time * 4.0f) * 0.2f, 0});
            DrawSphere(promptPos, 0.15f, s.discovered ? WHITE : GRAY);
        }
    }
    
    // Items - Divine Offerings
    for(auto& item : world.items) {
        if(!item.collected) {
            Vector3 pos = item.pos;
            pos.y = 1 + sinf(GetTime() * 3) * 0.3f;
            Color col = item.type == 0 ? WHITE : item.type == 1 ? SKYBLUE : item.type == 2 ? GOLD : ORANGE;
            DrawCube(pos, 0.5f, 0.5f, 0.5f, col);
            DrawCubeWires(pos, 0.5f, 0.5f, 0.5f, WHITE);
        }
    }
    
    // Grace orbs - Sparks of Divinity
    for(auto& orb : world.graceOrbs) {
        float pulse = sinf(GetTime() * 8) * 0.3f + 0.7f;
        DrawSphere(orb.pos, 0.8f, (Color){255, 215, 100, (unsigned char)(pulse * 255)});
    }
    
    // Faith Embers
    for(auto& token : world.tokens) {
        float hover = sinf(GetTime() * 4 + token.pos.x) * 0.2f;
        Vector3 pos = Vector3Add(token.pos, {0, hover, 0});
        float pulse = sinf(GetTime() * 6) * 0.3f + 0.7f;
        DrawSphere(pos, 0.4f, (Color){255, 255, 200, (unsigned char)(pulse * 255)});
        DrawSphereWires(pos, 0.5f, 6, 6, GOLD);
    }
    
    // Equipment drops - Sacred Artifacts
    for(auto& drop : world.equipmentDrops) {
        if(!drop.collected) {
            float hover = sinf(GetTime() * 3 + drop.pos.x) * 0.3f;
            Vector3 pos = Vector3Add(drop.pos, {0, hover, 0});
            Color col = GetRarityColor(drop.equipment.rarity);
            DrawCube(pos, 0.6f, 0.6f, 0.6f, col);
            DrawCubeWires(pos, 0.6f, 0.6f, 0.6f, WHITE);
            float pulse = sinf(GetTime() * 4) * 0.4f + 0.6f;
            DrawSphere(pos, 0.8f, Fade(col, pulse * 0.4f));
        }
    }
    
    // Grace recovery marker - Lost Intent
    if(world.hasGraceToRecover) {
        float pulse = sinf(GetTime() * 4) * 0.3f + 0.7f;
        DrawSphere(player.renewalPos, 1.0f, (Color){255, 255, 255, (unsigned char)(pulse * 255)});
    }
    
    // Bullets - Radiance (Optimized with Culling)
    for(auto& b : world.bullets) {
        if (Vector3Distance(b.pos, camera.position) < 250.0f) {
            DrawBullet(b);
        }
    }

    // Sacred Ruins - Distance Culling
    for(auto& r : world.ruins) {
        if (Vector3Distance(r.pos, camera.position) < 400.0f) {
            DrawCubeV(r.pos, r.size, r.color);
            DrawCubeWiresV(r.pos, r.size, Fade(GOLD, 0.3f));
        }
    }
    
    // Particles (Optimized with Culling)
    for(auto& p : world.particles) {
        if (Vector3Distance(p.pos, camera.position) < 250.0f) {
            float alpha = p.lifetime / p.maxLifetime;
            Color c = p.color;
            c.a = (unsigned char)(alpha * 255);
            
            
            DrawCube(p.pos, p.size, p.size, p.size, c);
            
        }
    }
    
    // Enemies - Distance Culling
    for(auto& e : world.enemies) {
        if(e.isAlive && Vector3Distance(e.pos, camera.position) < 350.0f) {
            DrawEnemy(e);
        }
    }
    
    // Player
    DrawPlayer();
}

void DrawPlayer() {
    Color themeCol = GetWaveColor();
    // Draw Ghost Trail (Now represents kinetic after-images)
    for (auto& g : world.ghosts) {
        float alpha = g.lifetime / 0.4f;
        Color ghostCol = Fade(themeCol, alpha * 0.3f);
        DrawCube(g.pos, 1.2f, 0.8f, 1.2f, ghostCol);
    }

    Color mainCol = player.invulnTimer > 0 ? WHITE : themeCol;
    if(player.isParrying) mainCol = GOLD;
    
    // --- HEAVY COMBAT CHASSIS ---
    float time = GetTime();
    float pulse = sinf(time * 10.0f) * 0.05f + 0.95f;
    Vector3 basePos = Vector3Add(player.pos, {0, 0.4f, 0});
    
    // Main Chassis (The "Hull")
    DrawCube(basePos, 1.8f, 0.8f, 2.2f, (Color){20, 20, 30, 255});
    DrawCubeWires(basePos, 1.85f, 0.82f, 2.25f, mainCol);
    
    // Core Engine (Pulsing Center)
    Vector3 corePos = Vector3Add(basePos, {0, 0.5f, 0});
    DrawSphere(corePos, 0.5f * pulse, mainCol);
    DrawSphereWires(corePos, 0.55f, 8, 8, WHITE);
    
    // Corner "Tread" Shards (Rotating kinetic stabilizers)
    for (int i = 0; i < 4; i++) {
        float angle = time * 4.0f + (i * PI / 2.0f);
        float offset = 1.2f;
        Vector3 treadPos = {
            player.pos.x + (i < 2 ? offset : -offset),
            0.4f + sinf(time * 5.0f + i) * 0.15f,
            player.pos.z + (i % 2 == 0 ? offset : -offset)
        };
        DrawCube(treadPos, 0.6f, 0.6f, 0.6f, (Color){10, 10, 15, 255});
        DrawCubeWires(treadPos, 0.62f, 0.62f, 0.62f, mainCol);
    }
    
    // Heavy Turret (Head)
    Vector3 turretPos = Vector3Add(corePos, {0, 0.4f, 0});
    DrawCube(turretPos, 0.9f, 0.5f, 0.9f, (Color){25, 25, 35, 255});
    DrawCubeWires(turretPos, 0.92f, 0.52f, 0.92f, mainCol);
    
    // Dual Beam Emitters (Gun) with Recoil
    float recoil = 0.0f;
    if (player.shootCooldown > player.shootRate * 0.5f) {
        recoil = (player.shootCooldown - player.shootRate * 0.5f) * 2.0f;
    }
    
    for (int i = -1; i <= 1; i += 2) {
        Vector3 gunStart = Vector3Add(player.pos, {
            sinf(player.rotation) * (0.5f - recoil * 0.3f) + cosf(player.rotation) * (i * 0.25f),
            1.0f,
            cosf(player.rotation) * (0.5f - recoil * 0.3f) - sinf(player.rotation) * (i * 0.25f)
        });
        Vector3 gunEnd = Vector3Add(gunStart, {
            sinf(player.rotation) * 0.9f, 0, cosf(player.rotation) * 0.9f
        });
        DrawCylinderEx(gunStart, gunEnd, 0.06f, 0.06f, 6, DARKGRAY);
        DrawSphere(gunEnd, 0.08f, mainCol);
    }
    
    // Parry field
    if(player.isParrying) {
        DrawSphere(player.pos, PRAYER_RANGE, Fade(GOLD, 0.2f));
        DrawSphereWires(player.pos, PRAYER_RANGE, 10, 10, Fade(GOLD, 0.4f));
    }
    
    // Void Sensor: Integrated Array
    if(player.lampActive) {
        Vector3 lampPos = Vector3Add(turretPos, {0, 0.4f, 0});
        DrawSphere(lampPos, 0.15f, GOLD);
        DrawSphereWires(lampPos, 0.2f, 6, 6, WHITE);
        DrawSphereWires(player.pos, 3.5f, 12, 12, Fade(GOLD, 0.15f));
    }
}

void DrawEnemy(const Enemy& e) {
    float time = GetTime();
    Vector3 center = Vector3Add(e.pos, {0, 0.75f * e.scale, 0});
    
    // Pulse effect
    float pulse = sinf(time * 5.0f + e.pos.x) * 0.15f + 0.85f;
    Color coreCol = e.color;
    
    // Defeat Animation Overrides
    float animScale = 1.0f;
    if (e.aiState == 5) {
        animScale = e.defeatTimer; // Shrink
        coreCol = Fade(e.color, e.defeatTimer); // Fade
        if ((int)(e.defeatTimer * 20) % 2 == 0) coreCol = WHITE; // Flicker
        
        // Render Teleport Beam
        DrawCylinderEx(e.pos, Vector3Add(e.pos, {0, 50.0f, 0}), 1.5f * e.scale, 0.5f * e.scale, 8, Fade(coreCol, 0.3f));
        DrawCylinderWiresEx(e.pos, Vector3Add(e.pos, {0, 50.0f, 0}), 1.6f * e.scale, 0.6f * e.scale, 8, Fade(WHITE, 0.5f));
    }
    
    if (e.isBoss) {
        // Boss: Void Sentinel
        // Central massive cube
        DrawCubeV(center, {2.0f * e.scale * pulse * animScale, 2.0f * e.scale * pulse * animScale, 2.0f * e.scale * pulse * animScale}, coreCol);
        DrawCubeWiresV(center, {2.1f * e.scale * animScale, 2.1f * e.scale * animScale, 2.1f * e.scale * animScale}, WHITE);
        
        // Orbital rotating rings of spikes
        if (e.aiState != 5) {
            for (int i = 0; i < 8; i++) {
                float angle = time * 1.5f + (i * PI / 4.0f);
                Vector3 spikePos = {
                    center.x + cosf(angle) * 3.5f * e.scale,
                    center.y + sinf(time + i) * 1.0f * e.scale,
                    center.z + sinf(angle) * 3.5f * e.scale
                };
                DrawCube(spikePos, 0.5f * e.scale, 0.5f * e.scale, 0.5f * e.scale, coreCol);
                DrawCubeWires(spikePos, 0.55f * e.scale, 0.55f * e.scale, 0.55f * e.scale, WHITE);
            }
        }
        
        // Health Bar (Only if not defeated)
        if (e.aiState != 5) {
            float hp = (float)e.health / e.maxHealth;
            Vector3 barPos = Vector3Add(e.pos, {0, 6.0f * e.scale, 0});
            DrawCube(barPos, 8.0f, 0.4f, 0.1f, Fade(BLACK, 0.6f));
            DrawCube(Vector3Add(barPos, {-4.0f + 4.0f * hp, 0, 0}), 8.0f * hp, 0.35f, 0.08f, coreCol);
        }

        // Neural Barrier Visualization
        if (e.aiState == 4) {
            DrawSphereWires(center, 4.5f * e.scale, 8, 8, Fade(GOLD, 0.4f));
            DrawSphere(center, 4.2f * e.scale, Fade(GOLD, 0.1f));
        }
    } else if(e.type == GLITCH_SPECTRE) {
        // Glitch Spectre: Unstable Diamond
        float glitchX = (GetRandomValue(0, 10) > 8) ? (GetRandomValue(-5, 5)/10.0f) : 0;
        Vector3 glitchPos = Vector3Add(center, {glitchX, 0, 0});
        
        DrawCylinderEx(Vector3Add(glitchPos, {0, 0.8f * e.scale * animScale, 0}), glitchPos, 0.0f, 0.5f * e.scale * animScale, 4, coreCol);
        DrawCylinderEx(Vector3Add(glitchPos, {0, -0.8f * e.scale * animScale, 0}), glitchPos, 0.0f, 0.5f * e.scale * animScale, 4, coreCol);
        DrawCylinderWiresEx(Vector3Add(glitchPos, {0, 0.8f * e.scale * animScale, 0}), Vector3Add(glitchPos, {0, -0.8f * e.scale * animScale, 0}), 0.5f * e.scale * animScale, 0.5f * e.scale * animScale, 4, WHITE);
    } else {
        // Regular Enemy: Fragmented Soul
        // Pulsing core
        DrawSphere(center, 0.4f * e.scale * pulse * animScale, coreCol);
        DrawSphereWires(center, 0.45f * e.scale * animScale, 6, 6, WHITE);
        
        // 3 floating shards
        if (e.aiState != 5) {
            for (int i = 0; i < 3; i++) {
                float angle = time * 3.0f + (i * 2.0f * PI / 3.0f);
                Vector3 shardPos = {
                    center.x + cosf(angle) * 0.8f * e.scale,
                    center.y + sinf(time * 2.0f + i) * 0.3f * e.scale,
                    center.z + sinf(angle) * 0.8f * e.scale
                };
                DrawCube(shardPos, 0.25f * e.scale, 0.25f * e.scale, 0.25f * e.scale, coreCol);
                DrawCubeWires(shardPos, 0.28f * e.scale, 0.28f * e.scale, 0.28f * e.scale, WHITE);
            }
        }
    }
}

void DrawBullet(const Bullet& b) {
    
    // Set material color for the mesh
    
    DrawSphere(b.pos, b.size, b.color);
    
    if(b.reflected) {
        
        DrawSphereWires(b.pos, b.size * 1.5f, 6, 6, Fade(GOLD, 0.4f));
    }
    
    // Optimized Glow Trails (One extra pass instead of 2-3)
    if(b.playerBullet) {
        Color glowCol = Fade(WHITE, 0.3f);
        float glowScale = 1.4f;

        if(b.color.r == 255 && b.color.g == 215) { glowCol = Fade(GOLD, 0.4f); glowScale = 1.5f; } // Gold
        else if(b.color.r == 128 && b.color.b == 128) { glowCol = Fade(VIOLET, 0.3f); } // Purple
        else if(b.size > 0.6f) { glowCol = Fade(ORANGE, 0.4f); glowScale = 1.3f; } // Heavy

        
        DrawSphere(b.pos, b.size * glowScale, glowCol);
    }
}

void DrawCrosshair() {
    Vector2 mouse = GetMousePosition();
    DrawLineEx({mouse.x - 15, mouse.y}, {mouse.x + 15, mouse.y}, 2, WHITE);
    DrawLineEx({mouse.x, mouse.y - 15}, {mouse.x, mouse.y + 15}, 2, WHITE);
    DrawCircleLines((int)mouse.x, (int)mouse.y, 20, WHITE);
    DrawCircleLines((int)mouse.x, (int)mouse.y, 12, WHITE);
}

void DrawHUD() {
    Color themeCol = GetWaveColor();
    // Soul Purity - Gold/White
    float hp = (float)player.health / player.maxHealth;
    DrawRectangle(30, 30, 400, 35, Fade(BLACK, 0.7f));
    DrawRectangle(35, 35, (int)(390 * hp), 25, WHITE);
    DrawText("SOUL PURITY", 40, 35, 22, BLACK);
    
    // Spirit Buffer - Celestial Blue
    float sp = player.stamina / player.maxStamina;
    DrawRectangle(30, 75, 400, 20, Fade(BLACK, 0.7f));
    DrawRectangle(35, 78, (int)(390 * sp), 14, SKYBLUE);
    
    // Divine Sensor - Gold
    float lp = player.lampFaith / player.maxLampFaith;
    Color lampCol = player.lampActive ? GOLD : DARKGRAY;
    DrawRectangle(30, 105, 400, 20, Fade(BLACK, 0.7f));
    DrawRectangle(35, 108, (int)(390 * lp), 14, lampCol);
    DrawText("DIVINE SENSOR", 40, 107, 15, WHITE);
    
    // Prayer Meter (Override) - Gold
    float sy = player.syncMeter / player.maxSyncMeter;
    Color syncCol = (player.syncMeter >= player.maxSyncMeter) ? ( (int)(GetTime()*10)%2 == 0 ? WHITE : GOLD ) : GOLD;
    DrawRectangle(30, 135, 400, 25, Fade(BLACK, 0.7f));
    DrawRectangle(35, 138, (int)(390 * sy), 19, syncCol);
    DrawText(player.syncMeter >= player.maxSyncMeter ? "READY: [R] DIVINE WILL" : "PRAYER METER", 40, 139, 18, BLACK);
    
    // Stats Line 1
    char text[128];
    snprintf(text, 128, "DIVINE GRACE: %d | FAITH EMBERS: %d", player.grace, player.lightTokens);
    DrawText(text, 30, 175, 22, GOLD);
    
    // Stats Line 2
    snprintf(text, 128, "PILGRIM LVL: %d | TRIAL: %d", player.level, world.wave);
    DrawText(text, 30, 205, 22, WHITE);
    
    // Wave type indicator
    const char* waveType = "";
    if(world.wave % 5 == 0) waveType = "DIVINE JUDGMENT (BOSS)";
    else if(world.wave % 3 == 0) waveType = "TEST OF FAITH (SPIRAL)";
    else waveType = "THE PILGRIMAGE";
    
    int waveWidth = MeasureText(waveType, 32);
    DrawText(waveType, SCREEN_WIDTH/2 - waveWidth/2, 30, 32, 
             world.wave % 5 == 0 ? GOLD : world.wave % 3 == 0 ? SKYBLUE : WHITE);
    
    // Resources & Combat Stats
    snprintf(text, 128, "HOLY FLASKS: %d | ASCENSIONS: %d | SPIRIT: %d", player.flasks, player.kills, player.score);
    DrawText(text, 30, 235, 20, LIGHTGRAY);
    
    if(player.combo > 1) {
        char comboText[128];
        snprintf(comboText, 128, "DIVINE FOCUS x%d", player.combo);
        int comboWidth = MeasureText(comboText, 40);
        DrawText(comboText, SCREEN_WIDTH/2 - comboWidth/2, 70, 40, GOLD);
    }
    
    // Proximity Prompt for Sanctuaries
    for (auto& s : world.sanctuaries) {
        if (Vector3Distance(player.pos, s.pos) < s.radius) {
            Vector2 screenPos = GetWorldToScreen(Vector3Add(s.pos, {0, 4.0f, 0}), camera);
            float pulse = sinf(GetTime() * 5.0f) * 0.5f + 0.5f;
            const char* prompt = s.discovered ? "ALTAR OF GRACE READY: [F]" : "OFFERING PRAYER...";
            DrawText(prompt, (int)screenPos.x - 180, (int)screenPos.y, 25, Fade(GOLD, 0.5f + 0.5f * pulse));
            break;
        }
    }

    // Current weapon display
    WeaponData* currWeapon = GetCurrentWeaponData();
    if(currWeapon) {
        DrawRectangle(SCREEN_WIDTH - 420, SCREEN_HEIGHT - 130, 400, 110, Fade(BLACK, 0.8f));
        DrawRectangleLines(SCREEN_WIDTH - 420, SCREEN_HEIGHT - 130, 400, 110, GOLD);
        DrawText("ACTIVE SCEPTER:", SCREEN_WIDTH - 410, SCREEN_HEIGHT - 120, 22, WHITE);
        DrawText(currWeapon->name.c_str(), SCREEN_WIDTH - 410, SCREEN_HEIGHT - 85, 30, GOLD);
        
        char weaponStats[128];
        snprintf(weaponStats, 128, "FAITH: %.0f | FREQ: %.2fs | VEL: %.0f", 
                 currWeapon->damage, currWeapon->fireRate, currWeapon->bulletSpeed);
        DrawText(weaponStats, SCREEN_WIDTH - 410, SCREEN_HEIGHT - 50, 18, LIGHTGRAY);
    }
    
    // Message
    if(world.messageTimer > 0) {
        DrawRectangle(SCREEN_WIDTH/2 - 250, 200, 500, 70, Fade(BLACK, 0.9f));
        DrawRectangleLines(SCREEN_WIDTH/2 - 250, 200, 500, 70, GOLD);
        DrawText(world.message.c_str(), SCREEN_WIDTH/2 - 240, 220, 28, GOLD);
    }
    
    // Controls
    DrawText("LMB: RADIANCE | Q: PARRY | SPACE: DASH | L: SENSOR | E: FLASK | F: PRAY", 
             30, SCREEN_HEIGHT - 35, 18, Fade(WHITE, 0.6f));
    
    DrawMinimap();
}

void DrawMinimap() {
    const float mapRadius = 80.0f;
    const Vector2 mapCenter = { SCREEN_WIDTH - mapRadius - 30, mapRadius + 30 };
    Color themeCol = GetWaveColor();

    // Background
    DrawCircleSector(mapCenter, mapRadius, 0, 360, 16, Fade(BLACK, 0.6f));
    DrawCircleLinesV(mapCenter, mapRadius, Fade(themeCol, 0.4f));
    
    // Grid Lines
    DrawLineV({mapCenter.x - mapRadius, mapCenter.y}, {mapCenter.x + mapRadius, mapCenter.y}, Fade(themeCol, 0.1f));
    DrawLineV({mapCenter.x, mapCenter.y - mapRadius}, {mapCenter.x, mapCenter.y + mapRadius}, Fade(themeCol, 0.1f));

    auto worldToMap = [&](Vector3 pos) -> Vector2 {
        // Map +/- 500 to +/- mapRadius
        // Fix: X axis was inverted relative to movement
        return (Vector2){
            mapCenter.x - (pos.x / 500.0f) * mapRadius,
            mapCenter.y - (pos.z / 500.0f) * mapRadius 
        };
    };

    // Draw Ruins (Faint dots)
    for (auto& r : world.ruins) {
        Vector2 mPos = worldToMap(r.pos);
        DrawCircleV(mPos, 1.5f, Fade(DARKGRAY, 0.3f));
    }

    // Draw Sanctuaries (Gold)
    for (auto& s : world.sanctuaries) {
        Vector2 mPos = worldToMap(s.pos);
        DrawCircleV(mPos, 3.0f, s.discovered ? GOLD : DARKGRAY);
    }

    // Draw Corrupted Bots (Violet dots)
    for (auto& e : world.enemies) {
        if (!e.isAlive) continue;
        Vector2 mPos = worldToMap(e.pos);
        DrawCircleV(mPos, 2.5f, VIOLET);
    }

    // Draw Player (Cyan dot)
    Vector2 pPos = worldToMap(player.pos);
    DrawCircleV(pPos, 3.5f, (Color){ 0, 255, 255, 255 });
    
    DrawText("THREAT RADAR", (int)mapCenter.x - 50, (int)mapCenter.y + mapRadius + 5, 15, themeCol);
}

void DrawSanctuaryMenu() {
    Color themeCol = GetWaveColor();
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.9f));
    DrawRectangleLines(50, 50, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 100, GOLD);
    DrawText("ALTAR OF DIVINE GRACE", SCREEN_WIDTH/2 - 350, 70, 55, GOLD);
    DrawText(TextFormat("DIVINE GRACE: %d | FAITH EMBERS: %d", player.grace, player.lightTokens), 
             SCREEN_WIDTH/2 - 380, 150, 35, WHITE);
    
    int y = 240;
    const char* stats[] = {
        "PURIFY SOUL (Integrity)",
        "STRENGTHEN SPIRIT (Buffer)",
        "EXPAND VISION (Sensor)",
        "DIVINE RADIANCE (Signal)",
        "CONTINUE PILGRIMAGE"
    };
    const char* descriptions[] = {
        "+10 Soul Purity Capacity",
        "+12 Spiritual Buffer Recharge",
        "+15 Divine Sensor Range",
        "+3 Vel / +2 Pwr Projection",
        "Resume the Sacred Path"
    };
    int levels[] = {player.vigor, player.will, player.faith, player.strength, 0};
    
    for(int i = 0; i < 5; i++) {
        bool selected = (world.selectedSanctuaryOption == i);
        int cost = (i < 4) ? GetUpgradeCost(levels[i]) : 0;
        Color col = (i < 4 && player.grace < cost) ? DARKGRAY : (selected ? GOLD : WHITE);
        
        if (selected) {
            DrawRectangle(150, y - 5, 1140, 70, Fade(GOLD, 0.2f));
            DrawRectangleLines(150, y - 5, 1140, 70, GOLD);
        }

        char text[128];
        if (i < 4) {
            snprintf(text, 128, "%s [LV %d] - COST: %d", stats[i], levels[i], cost);
        } else {
            snprintf(text, 128, ">>> %s <<<", stats[i]);
        }
        
        DrawText(text, 180, y, 30, col);
        DrawText(descriptions[i], 920, y + 5, 20, Fade(WHITE, 0.6f));
        y += 85;
    }
    
    DrawText("UP/DOWN: Navigate | ENTER: Affirm | T: Temple of Wills", SCREEN_WIDTH/2 - 350, SCREEN_HEIGHT - 110, 25, GOLD);
    DrawText("Soul Refinement uses DIVINE GRACE to elevate your spiritual form.", SCREEN_WIDTH/2 - 400, SCREEN_HEIGHT - 70, 20, Fade(WHITE, 0.4f));
}

void DrawShopMenu() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.9f));
    DrawRectangleLines(30, 30, SCREEN_WIDTH - 60, SCREEN_HEIGHT - 60, GOLD);
    DrawText("TEMPLE OF SACRED WILLS", SCREEN_WIDTH/2 - 320, 40, 55, GOLD);
    DrawText(TextFormat("Faith Embers: %d", player.lightTokens), SCREEN_WIDTH/2 - 120, 110, 35, WHITE);
    
    // Weapons section (top left) - Available to purchase
    DrawText("SACRED SCEPTERS (Press W)", 80, 170, 30, GOLD);
    DrawRectangle(50, 200, 600, 200, Fade(GOLD, 0.1f));
    
    int wy = 220;
    int weaponsShown = 0;
    for(auto& w : world.weaponArsenal) {
        if(!w.unlocked && (world.wave >= w.unlockWave || w.blueprintFound) && weaponsShown < 3) {
            Color col = player.lightTokens >= w.unlockCost ? WHITE : DARKGRAY;
            char weaponInfo[256];
            snprintf(weaponInfo, 256, "%s - %d faith [PWR:%.0f] %s", 
                     w.name.c_str(), w.unlockCost, w.damage,
                     w.blueprintFound ? "[REVEALED]" : "");
            DrawText(weaponInfo, 70, wy, 22, col);
            DrawText(w.description.c_str(), 70, wy + 25, 18, LIGHTGRAY);
            wy += 55;
            weaponsShown++;
        }
    }
    if(weaponsShown == 0) {
        DrawText("All sacred scepters revealed!", 70, 250, 22, GOLD);
        DrawText("Complete more trials to find more!", 70, 280, 20, GRAY);
    }
    
    // Unlocked weapons section (top right) - Switch between them
    DrawText("ACTIVE SCEPTERS (1-9)", 720, 170, 28, WHITE);
    DrawRectangle(690, 200, 700, 200, Fade(WHITE, 0.1f));
    
    wy = 220;
    int unlockedCount = 0;
    for(size_t i = 0; i < world.weaponArsenal.size() && unlockedCount < 4; i++) {
        if(world.weaponArsenal[i].unlocked) {
            WeaponData& w = world.weaponArsenal[i];
            bool isEquipped = (player.currentWeapon == w.type);
            Color textCol = isEquipped ? GOLD : WHITE;
            
            char weaponLine[256];
            snprintf(weaponLine, 256, "%d) %s %s", 
                     unlockedCount + 1, 
                     w.name.c_str(),
                     isEquipped ? "[HELD]" : "");
            DrawText(weaponLine, 710, wy, 22, textCol);
            
            char stats[128];
            snprintf(stats, 128, "   PWR:%.0f FREQ:%.2fs", w.damage, w.fireRate);
            DrawText(stats, 710, wy + 24, 18, LIGHTGRAY);
            
            wy += 48;
            unlockedCount++;
        }
    }
    
    // Shop Items (equipment)
    DrawText("CELESTIAL AUGMENTS", 80, 420, 30, GOLD);
    DrawRectangle(50, 450, 600, 250, Fade(GOLD, 0.1f));
    
    int y = 470;
    int shopStart = 0;
    if (world.selectedShopItem >= 3) shopStart = world.selectedShopItem - 2;
    
    for(size_t i = shopStart; i < world.shopInventory.size() && i < shopStart + 3; i++) {
        Equipment& item = world.shopInventory[i];
        Color bgColor = (i == world.selectedShopItem) ? Fade(GOLD, 0.2f) : Fade(BLACK, 0.2f);
        DrawRectangle(60, y - 5, 580, 70, bgColor);
        
        Color rarityCol = GetRarityColor(item.rarity);
        DrawText(item.name.c_str(), 70, y, 24, rarityCol);
        
        char stats[256];
        snprintf(stats, 256, "STB+%d SPR+%d PWR+%d", 
                 item.bonusHealth, item.bonusStamina, item.bonusDamage);
        DrawText(stats, 70, y + 28, 18, LIGHTGRAY);
        
        Color priceCol = player.lightTokens >= item.buyValue ? WHITE : RED;
        DrawText(TextFormat("OFFER: %d", item.buyValue), 500, y + 15, 22, priceCol);
        
        y += 75;
    }
    
    DrawText("UP/DOWN: Select Augment | ENTER: Accept Offering", 60, 715, 20, WHITE);
    
    // Player Inventory (right side)
    DrawText("SACRED GEAR", 720, 420, 30, WHITE);
    DrawRectangle(690, 450, 700, 250, Fade(WHITE, 0.1f));
    
    y = 470;
    int invStart = 0;
    if (world.selectedInventoryItem >= 3) invStart = world.selectedInventoryItem - 2;
    
    for(size_t i = invStart; i < player.inventory.size() && i < invStart + 3; i++) {
        Equipment& item = player.inventory[i];
        Color bgColor = (i == world.selectedInventoryItem) ? Fade(WHITE, 0.2f) : Fade(BLACK, 0.2f);
        DrawRectangle(700, y - 5, 680, 70, bgColor);
        
        Color rarityCol = GetRarityColor(item.rarity);
        std::string displayName = item.name;
        if(item.equipped) displayName += " [SANCTIFIED]";
        DrawText(displayName.c_str(), 710, y, 22, rarityCol);
        
        char stats[256];
        snprintf(stats, 256, "STB+%d SPR+%d PWR+%d | TITHE: %d", 
                 item.bonusHealth, item.bonusStamina, item.bonusDamage, item.sellValue);
        DrawText(stats, 710, y + 28, 18, LIGHTGRAY);
        
        y += 75;
    }
    
    if(player.inventory.empty()) {
        DrawText("No sacred gear held.", 750, 550, 22, GRAY);
    }
    
    DrawText("LEFT/RIGHT: Select Gear | E: Sanctify | S: Sacrifice", 700, 715, 20, WHITE);
    
    // Currently Equipped display
    DrawText("SPIRITUAL ATTIRE", SCREEN_WIDTH/2 - 120, 745, 25, GOLD);
    
    WeaponData* currWeapon = GetCurrentWeaponData();
    const char* weaponName = currWeapon ? currWeapon->name.c_str() : "None";
    
    char equipped[256];
    snprintf(equipped, 256, "Scepter: %s | Vestment: %s | Relic: %s | Band: %s",
             weaponName,
             player.equippedArmor ? player.equippedArmor->name.c_str() : "None",
             player.equippedTalisman ? player.equippedTalisman->name.c_str() : "None",
             player.equippedRing ? player.equippedRing->name.c_str() : "None");
    DrawText(equipped, 50, 775, 18, LIGHTGRAY);
    
    DrawText("ESC/TAB: RETURN TO ALTAR", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT - 25, 22, WHITE);
}

void DrawWeaponSelectMenu() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.92f));
    
    DrawText("WAVE COMPLETED!", SCREEN_WIDTH/2 - 280, 80, 70, GOLD);
    DrawText("Choose a NEW WEAPON to unlock", SCREEN_WIDTH/2 - 280, 160, 35, YELLOW);
    
    int y = 250;
    for(size_t i = 0; i < world.availableUnlocks.size(); i++) {
        WeaponType wType = world.availableUnlocks[i];
        
        // Find weapon data
        WeaponData* weapon = nullptr;
        for(auto& w : world.weaponArsenal) {
            if(w.type == wType) {
                weapon = &w;
                break;
            }
        }
        
        if(!weapon) continue;
        
        // Highlight selected weapon
        Color bgColor = (i == world.selectedWeapon) ? Fade(GOLD, 0.4f) : Fade(BLACK, 0.3f);
        DrawRectangle(100, y - 10, SCREEN_WIDTH - 200, 100, bgColor);
        
        // Draw weapon info
        DrawText(weapon->name.c_str(), 120, y, 40, weapon->bulletColor);
        DrawText(weapon->description.c_str(), 120, y + 45, 25, LIGHTGRAY);
        
        // Draw weapon stats
        char stats[256];
        snprintf(stats, 256, "DMG: %.0f | RATE: %.2fs | SPEED: %.0f | PELLETS: %d", 
                 weapon->damage, weapon->fireRate, weapon->bulletSpeed, weapon->projectileCount);
        DrawText(stats, 120, y + 75, 20, WHITE);
        
        // Draw special properties
        if(weapon->piercing) DrawText("[PIERCING]", SCREEN_WIDTH - 350, y + 20, 25, PURPLE);
        if(weapon->explosive) DrawText("[EXPLOSIVE]", SCREEN_WIDTH - 350, y + 20, 25, ORANGE);
        if(weapon->homing) DrawText("[HOMING]", SCREEN_WIDTH - 350, y + 20, 25, GOLD);
        
        y += 120;
    }
    
    DrawText("UP/DOWN: Select  |  ENTER: Unlock & Equip  |  SPACE: Skip", 
             SCREEN_WIDTH/2 - 350, SCREEN_HEIGHT - 100, 30, WHITE);
    DrawText("(Unlocking is FREE after completing waves!)", 
             SCREEN_WIDTH/2 - 280, SCREEN_HEIGHT - 60, 25, GREEN);
}

void DrawDeathScreen() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.95f));
    
    // Radiant Background Lines
    for(int i=0; i<10; i++) {
        float y = GetRandomValue(0, SCREEN_HEIGHT);
        DrawLine(0, y, SCREEN_WIDTH, y, Fade(GOLD, 0.2f));
    }
    
    DrawText("MORTAL FORM EXPIRED", SCREEN_WIDTH/2 - 400, SCREEN_HEIGHT/2 - 120, 80, GOLD);
    DrawText("SEEKING REDEMPTION", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 20, 40, LIGHTGRAY);
    
    char text[128];
    snprintf(text, 128, "GRACE LOST: %d | TRIALS PASSED: %d", player.graceAtDeath, world.wave);
    DrawText(text, SCREEN_WIDTH/2 - 300, SCREEN_HEIGHT/2 + 50, 30, WHITE);
    
    DrawText("PRESS [R] TO RESURRECT", SCREEN_WIDTH/2 - 220, SCREEN_HEIGHT/2 + 150, 35, SKYBLUE);
}

void DrawVictoryScreen() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.95f));
    DrawRectangleLines(50, 50, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 100, GOLD);
    
    DrawText("ASCENSION ACHIEVED", SCREEN_WIDTH/2 - 380, SCREEN_HEIGHT/2 - 120, 80, GOLD);
    DrawText("THE VOID IS PURIFIED", SCREEN_WIDTH/2 - 240, SCREEN_HEIGHT/2 - 20, 40, SKYBLUE);
    
    char text[128];
    snprintf(text, 128, "TOTAL GRACE: %d | ENTITIES ASCENDED: %d", player.score, player.kills);
    DrawText(text, SCREEN_WIDTH/2 - 300, SCREEN_HEIGHT/2 + 60, 30, WHITE);
    
    DrawText("PRESS [ENTER] TO START A NEW PILGRIMAGE", SCREEN_WIDTH/2 - 320, SCREEN_HEIGHT/2 + 150, 30, LIGHTGRAY);
}