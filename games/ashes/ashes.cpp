// ======================================================================
// Echoes of the Feed – Ashes of the Scroll
// A Dark Souls / Elden Ring inspired raylib game
// Final Episode: Game Complete – Two Levels, Boss Victory, Polish & Fixes
// ======================================================================
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <random>

// ======================================================================
// Constants & Configuration
// ======================================================================
const int SCREEN_WIDTH = 1440;
const int SCREEN_HEIGHT = 810;
const float BASE_PLAYER_SPEED = 7.4f;
const float SPRINT_MULTIPLIER = 1.85f;
const float EXHAUSTED_MULTIPLIER = 0.45f;
const float ATTACK_RANGE = 6.2f;
const float ROLL_DURATION = 0.22f;
const float ROLL_DISTANCE = 13.0f;
const float ROLL_COST = 18.0f;
const float PERFECT_ROLL_WINDOW = 0.10f;
const int MAX_PLAYER_HEALTH = 420;
const int MAX_STAMINA = 145;
const float STAMINA_REGEN_RATE = 38.0f;
const float STAMINA_SPRINT_COST = 14.0f;
const float STAMINA_ATTACK_COST = 22.0f;
const float STAMINA_POWER_COST = 52.0f;
const float STAMINA_PARRY_COST = 28.0f;
const float NORMAL_ATTACK_DURATION = 0.42f;
const float POWER_ATTACK_CHARGE = 0.85f;
const float POWER_ATTACK_DURATION = 1.05f;
const float COMBO_RESET_TIME = 1.1f;
const float REGEN_DELAY_AFTER_ACTION = 0.8f;
const float CAMERA_DISTANCE = 19.0f;
const float CAMERA_HEIGHT = 15.2f;
const float CAMERA_SMOOTH = 13.5f;
const float MOUSE_SENSITIVITY = 0.28f;
const int MAX_FLASKS = 4;
const float FLASK_HEAL_AMOUNT = 135.0f;
const float FLASK_USE_TIME = 1.35f;
const float ENEMY_BASE_SPEED = 7.9f;
const float ENEMY_ATTACK_DURATION = 0.45f;
const float GRAVITY = -32.0f;
const float JUMP_VELOCITY = 14.0f;

// ======================================================================
// Enums
// ======================================================================
enum GameState { TITLE_SCREEN, PLAYING, PAUSED, RENEWAL, VICTORY };
enum EnemyType { GRUNT, TANK, AGILE, BOSS };
enum EnemyState { PATROL, ALERT, CHASE, SEARCH, STAGGERED };
enum AttackType { LIGHT_1, LIGHT_2, LIGHT_3, HEAVY, DASH_ATTACK };

// ======================================================================
// Structs
// ======================================================================
struct Particle {
    Vector3 position;
    Vector3 velocity;
    float lifetime;
    float maxLife;
    Color color;
    float size;
};

struct TrailPoint {
    Vector3 pos;
    float time;
};

struct Weapon {
    std::string name;
    float damageMultiplier;
    float poiseDamageMultiplier;
    float length;
    Color bladeColor;
    bool hasGlow;
};

struct Player {
    Vector3 position {0,0,0};
    Vector3 velocity {0,0,0};
    float yVelocity = 0.0f;
    float rotation = 0.0f;
    Weapon weapon {"Ashen Greatblade", 1.0f, 1.0f, 6.8f, {180,200,255,255}, true};
    int comboStep = 0;
    float comboTimer = 0.0f;
    bool isAttacking = false;
    float attackTimer = 0.0f;
    AttackType currentAttack = LIGHT_1;
    bool isCharging = false;
    float chargeTimer = 0.0f;
    bool powerReady = false;
    bool isRolling = false;
    float rollTimer = 0.0f;
    Vector3 rollDirection {0,0,0};
    Vector3 rollStartPos {0,0,0};
    bool isParrying = false;
    float parryTimer = 0.0f;
    int health = MAX_PLAYER_HEALTH;
    int maxHealth = MAX_PLAYER_HEALTH;
    float stamina = MAX_STAMINA;
    float staminaRegenDelay = 0.0f;
    int flasks = MAX_FLASKS;
    float poise = 120.0f;
    float maxPoise = 120.0f;
    float staggerTimer = 0.0f;
    float hitInvuln = 0.0f;
    int lockedTarget = -1;
    float targetSwitchCooldown = 0.0f;
    Vector3 bladeStart, bladeEnd;
    float swingYaw = 30.0f;
    float swingPitch = -30.0f;
    float shakeTimer = 0.0f;
    bool isRenewing = false;
    float renewalTimer = 0.0f;
    float renewalAngle = 0.0f;
    bool isHealing = false;
    float healTimer = 0.0f;
    float perfectRollTimer = 0.0f;
    float riposteTimer = 0.0f;
};

struct Enemy {
    EnemyType type;
    Vector3 position {0,0,0};
    Vector3 velocity {0,0,0};
    float rotation = 0.0f;
    int health = 220;
    int maxHealth = 220;
    float stamina = MAX_STAMINA;
    float staminaRegenDelay = 0.0f;
    float poise = 80.0f;
    float maxPoise = 80.0f;
    bool alive = true;
    bool isAttacking = false;
    float attackTimer = 0.0f;
    AttackType currentAttack = LIGHT_1;
    bool isHeavyAttack = false;
    bool isDodging = false;
    float dodgeTimer = 0.0f;
    Vector3 dodgeDirection {0,0,0};
    Vector3 dodgeStartPos {0,0,0};
    bool isBlocking = false;
    float blockTimer = 0.0f;
    float hitInvuln = 0.0f;
    float stunTimer = 0.0f;
    EnemyState state = PATROL;
    Vector3 homePosition;
    float patrolRadius = 22.0f;
    Vector3 patrolTarget;
    float patrolTimer = 0.0f;
    Vector3 lastKnownPlayerPos;
    float alertTimer = 0.0f;
    Color bodyColor;
    float scale = 1.0f;
    float speed = ENEMY_BASE_SPEED;
    float strafeSide = 1.0f;
    float strafeTimer = 4.0f;
    float attackCooldown = 0.0f;
    Vector3 bladeStart, bladeEnd;
    float swingYaw = 30.0f;
    float swingPitch = -30.0f;
    float attackDamage = 32.0f;
    float poiseDamage = 38.0f;
    float attackDur = ENEMY_ATTACK_DURATION;
    float dodgeChance = 0.55f;
    int comboStep = 0;
    float comboDelayTimer = 0.0f;
};

// ======================================================================
// Global Variables
// ======================================================================
GameState gameState = TITLE_SCREEN;
int currentLevel = 1;
Player player;
std::vector<Enemy> enemies;
std::vector<Vector3> obstacles;
Vector3 exitPosition;
bool exitActive = false;
std::vector<Particle> particles;
std::vector<TrailPoint> weaponTrail;
Camera3D camera = { 0 };
Vector3 camPos = {0, CAMERA_HEIGHT, CAMERA_DISTANCE};
float hitStopTimer = 0.0f;
std::vector<std::string> renewalMessages = {
    "Skill Issue", "Git Gud", "Just Roll", "You Got Parried", "Touch Grass",
    "Ratio + L", "Downvoted to Oblivion", "Engagement Farm Failed",
    "Praise the Algorithm", "Try Tongue But Hole", "404 – Skill Not Found"
};
const char* currentRenewalMessage = "Git Gud";

// ======================================================================
// Function Prototypes
// ======================================================================
void InitGame();
void ResetLevel();
void UpdateGame(float dt);
void UpdatePlayer(float dt);
void UpdateEnemies(float dt);
void UpdateParticles(float dt);
void UpdateCamera(float dt);
void Draw3DScene();
void DrawPlayer();
void DrawEnemy(const Enemy& e, int index);
void DrawHUD();
void DrawTitleScreen();
void DrawRenewalScreen();
void DrawVictoryScreen();
void SpawnBloodParticles(Vector3 pos, int count = 12);
void SpawnHitSparks(Vector3 pos, int count = 8);
void AddWeaponTrailPoint();
bool CanSeePlayer(const Enemy& e);
bool IsEnemyAttackSwingHittingPlayer(const Enemy& e);
void ApplyEnemyHitToPlayer(const Enemy& e);
bool CheckPlayerAttackHitEnemy(Enemy& e);

// ======================================================================
// Main
// ======================================================================
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Echoes of the Feed – Ashes of the Scroll");
    SetTargetFPS(60);
    HideCursor();
    DisableCursor();
    InitAudioDevice();
    InitGame();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (gameState == TITLE_SCREEN) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER)) {
                currentLevel = 1;
                gameState = PLAYING;
                ResetLevel();
            }
        }
        else if (gameState == PLAYING || gameState == PAUSED) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                gameState = (gameState == PLAYING) ? PAUSED : PLAYING;
            }
            if (gameState == PLAYING) {
                UpdateGame(dt);

                // Level transition (only for level 1 → level 2)
                if (currentLevel == 1 && exitActive && Vector3Distance(player.position, exitPosition) < 9.0f) {
                    currentLevel = 2;
                    ResetLevel();
                }

                // Death check
                if (player.health <= 0 && !player.isRenewing) {
                    player.isRenewing = true;
                    player.renewalTimer = 3.2f;
                    player.renewalAngle = 0.0f;
                    gameState = RENEWAL;
                    currentRenewalMessage = renewalMessages[GetRandomValue(0, (int)renewalMessages.size()-1)].c_str();
                }
            }
        }
        else if (gameState == RENEWAL) {
            if (IsKeyPressed(KEY_R)) {
                ResetLevel();
                gameState = PLAYING;
            }
        }
        else if (gameState == VICTORY) {
            if (IsKeyPressed(KEY_ESCAPE)) break;
        }

        BeginDrawing();
        ClearBackground({12, 12, 22, 255});

        BeginMode3D(camera);
        Draw3DScene();
        EndMode3D();

        DrawHUD();

        if (gameState == TITLE_SCREEN) DrawTitleScreen();
        if (gameState == RENEWAL) DrawRenewalScreen();
        if (gameState == VICTORY) DrawVictoryScreen();
        if (gameState == PAUSED) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.65f));
            DrawText("PAUSED", SCREEN_WIDTH/2 - MeasureText("PAUSED", 80)/2,
                     SCREEN_HEIGHT/2 - 60, 80, GOLD);
            DrawText("ESC to Resume", SCREEN_WIDTH/2 - MeasureText("ESC to Resume", 40)/2,
                     SCREEN_HEIGHT/2 + 40, 40, LIGHTGRAY);
        }

        EndDrawing();
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}

// ======================================================================
// Initialization & Level Generation
// ======================================================================
void InitGame() {
    camera.fovy = 62.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camera.up = {0,1,0};
    ResetLevel();
}

void ResetLevel() {
    player = {};
    player.position = {0,0,0};
    player.health = MAX_PLAYER_HEALTH;
    player.maxHealth = MAX_PLAYER_HEALTH;
    player.stamina = MAX_STAMINA;
    player.flasks = MAX_FLASKS;
    player.poise = 120.0f;
    player.maxPoise = 120.0f;
    player.weapon = {"Ashen Greatblade", 1.0f, 1.0f, 6.8f, {180,200,255,255}, true};
    player.swingYaw = 30.0f;
    player.swingPitch = -30.0f;

    enemies.clear();
    obstacles.clear();
    particles.clear();
    weaponTrail.clear();
    hitStopTimer = 0.0f;
    exitActive = false;

    // Border walls
    int border = 80;
    int step = 12;
    for (int i = -border; i <= border; i += step) {
        obstacles.push_back({(float)i, 0, (float)-border});
        obstacles.push_back({(float)i, 0, (float)border});
        obstacles.push_back({(float)-border, 0, (float)i});
        obstacles.push_back({(float)border, 0, (float)i});
    }

    if (currentLevel == 1) {
        // Random pillars in open field
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-border+15, border-15);
        for (int i = 0; i < 90; i++) {
            float x = dis(gen);
            float z = dis(gen);
            if (Vector3Distance({x,0,z}, {0,0,0}) > 18.0f) {
                obstacles.push_back({x, 0, z});
            }
        }

        // Enemies
        const int MAX_ENEMIES = 14;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            Vector3 pos;
            bool valid = false;
            int attempts = 0;
            while (!valid && attempts < 60) {
                attempts++;
                float angle = GetRandomValue(0, 359) * DEG2RAD;
                float dist = GetRandomValue(18, 75);
                pos = { cosf(angle)*dist, 0, sinf(angle)*dist };
                valid = Vector3Distance(pos, {0,0,0}) > 16.0f;
                for (const auto& obs : obstacles) {
                    if (Vector3Distance(pos, obs) < 9.0f) {
                        valid = false;
                        break;
                    }
                }
            }
            if (!valid) continue;

            Enemy e{};
            e.position = pos;
            e.homePosition = pos;
            e.patrolTarget = pos;
            e.patrolRadius = GetRandomValue(16, 32);
            e.alive = true;
            e.swingYaw = 30.0f;
            e.swingPitch = -30.0f;
            e.attackCooldown = (float)GetRandomValue(0, 100) / 100.0f;
            e.strafeTimer = (float)GetRandomValue(30, 80) / 10.0f;
            e.strafeSide = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;

            int typeRoll = GetRandomValue(0, 100);
            if (typeRoll < 45) {
                e.type = GRUNT;
                e.scale = 0.95f;
                e.health = 180; e.maxHealth = 180;
                e.poise = 65; e.maxPoise = 65;
                e.speed = ENEMY_BASE_SPEED * 1.05f;
                e.bodyColor = {140, 40, 60, 255};
                e.attackDamage = 31.0f;
                e.poiseDamage = 36.0f;
                e.attackDur = 0.43f;
                e.dodgeChance = 0.52f;
            }
            else if (typeRoll < 80) {
                e.type = TANK;
                e.scale = 1.28f;
                e.health = 340; e.maxHealth = 340;
                e.poise = 160; e.maxPoise = 160;
                e.speed = ENEMY_BASE_SPEED * 0.82f;
                e.bodyColor = {60, 80, 160, 255};
                e.patrolRadius *= 0.7f;
                e.attackDamage = 46.0f;
                e.poiseDamage = 60.0f;
                e.attackDur = 0.60f;
                e.dodgeChance = 0.25f;
            }
            else {
                e.type = AGILE;
                e.scale = 1.05f;
                e.health = 160; e.maxHealth = 160;
                e.poise = 55; e.maxPoise = 55;
                e.speed = ENEMY_BASE_SPEED * 1.25f;
                e.bodyColor = {100, 180, 80, 255};
                e.attackDamage = 27.0f;
                e.poiseDamage = 32.0f;
                e.attackDur = 0.36f;
                e.dodgeChance = 0.82f;
            }
            enemies.push_back(e);
        }

        // Exit portal position
        do {
            exitPosition.x = GetRandomValue(-border+25, border-25);
            exitPosition.z = GetRandomValue(-border+25, border-25);
        } while (Vector3Distance(exitPosition, {0,0,0}) < 55.0f);
        exitPosition.y = 0;
    }
    else if (currentLevel == 2) {
        // Player starts farther back for dramatic entrance
        player.position = {0, 0, -35.0f};

        // Boss arena pillars
        int numPillars = 16;
        float radius = 45.0f;
        for (int i = 0; i < numPillars; i++) {
            float ang = (float)i / numPillars * 2 * PI;
            Vector3 pos = {cosf(ang) * radius, 0, sinf(ang) * radius};
            obstacles.push_back(pos);
        }
        numPillars = 8;
        radius = 20.0f;
        for (int i = 0; i < numPillars; i++) {
            float ang = (float)i / numPillars * 2 * PI + PI / 16.0f;
            Vector3 pos = {cosf(ang) * radius, 0, sinf(ang) * radius};
            obstacles.push_back(pos);
        }

        // Boss
        Enemy boss{};
        boss.type = BOSS;
        boss.position = {0, 0, 40.0f};
        boss.homePosition = boss.position;
        boss.scale = 2.3f;
        boss.health = 1600;
        boss.maxHealth = 1600;
        boss.poise = 320.0f;
        boss.maxPoise = 320.0f;
        boss.speed = ENEMY_BASE_SPEED * 0.88f;
        boss.bodyColor = {180, 30, 50, 255};
        boss.attackDamage = 48.0f;
        boss.poiseDamage = 72.0f;
        boss.attackDur = 0.55f;
        boss.dodgeChance = 0.35f;
        enemies.push_back(boss);
    }

    gameState = PLAYING;
}

// ======================================================================
// Core Update Loop
// ======================================================================
void UpdateGame(float dt) {
    UpdateCamera(dt);

    float effectiveDt = dt;
    if (hitStopTimer > 0.0f) {
        hitStopTimer -= dt;
        if (hitStopTimer <= 0.0f) hitStopTimer = 0.0f;
        effectiveDt = 0.0f;
    }

    UpdatePlayer(effectiveDt);
    UpdateEnemies(effectiveDt);
    UpdateParticles(effectiveDt);

    // Floating ash particles
    if (GetRandomValue(0, 30) == 0) {
        float x = player.position.x + GetRandomValue(-80, 80);
        float z = player.position.z + GetRandomValue(-80, 80);
        Vector3 pos = {x, 35.0f + GetRandomValue(0, 20), z};
        Particle p{};
        p.position = pos;
        p.velocity = {GetRandomValue(-8, 8)/10.0f, -2.2f, GetRandomValue(-8, 8)/10.0f};
        p.lifetime = p.maxLife = 20.0f;
        p.color = Fade(GRAY, 0.35f);
        p.size = GetRandomValue(3, 8)/10.0f;
        particles.push_back(p);
    }

    // Victory conditions
    int aliveCount = 0;
    for (const auto& e : enemies) if (e.alive) aliveCount++;

    if (currentLevel == 1) {
        exitActive = (aliveCount == 0);
    } else if (currentLevel == 2 && aliveCount == 0 && gameState == PLAYING) {
        gameState = VICTORY;
    }
}

// ======================================================================
// Player Update
// ======================================================================
void UpdatePlayer(float dt) {
    player.perfectRollTimer = std::max(0.0f, player.perfectRollTimer - dt);
    player.riposteTimer = std::max(0.0f, player.riposteTimer - dt);

    if (player.isRenewing) {
        player.renewalTimer -= dt;
        player.renewalAngle = Lerp(player.renewalAngle, 90.0f, 5.0f * dt);
        return;
    }

    player.hitInvuln -= dt;
    player.staggerTimer -= dt;
    player.targetSwitchCooldown -= dt;
    player.comboTimer -= dt;

    // Mouse look
    Vector2 mouseDelta = GetMouseDelta();
    float sens = MOUSE_SENSITIVITY;
    if (player.isAttacking || player.isParrying || player.staggerTimer > 0) sens *= 0.4f;
    player.rotation -= mouseDelta.x * sens;

    // Target lock
    if (IsKeyPressed(KEY_F)) {
        if (player.lockedTarget != -1) {
            player.lockedTarget = -1;
        } else {
            float bestDist = 1e9f;
            int best = -1;
            for (size_t i = 0; i < enemies.size(); i++) {
                if (!enemies[i].alive) continue;
                float dist = Vector3Distance(player.position, enemies[i].position);
                if (dist > 45.0f) continue;
                Vector3 to = Vector3Subtract(enemies[i].position, player.position);
                to.y = 0;
                float angDiff = fabsf(atan2f(to.x, to.z) - player.rotation*DEG2RAD);
                if (angDiff > PI) angDiff = 2*PI - angDiff;
                float score = dist * 0.6f + angDiff * 30.0f;
                if (score < bestDist) {
                    bestDist = score;
                    best = (int)i;
                }
            }
            player.lockedTarget = best;
        }
    }

    // Target switch with mouse flick
    if (player.lockedTarget != -1 && player.targetSwitchCooldown <= 0.0f) {
        float flick = 110.0f;
        int dir = 0;
        if (mouseDelta.x > flick) dir = 1;
        else if (mouseDelta.x < -flick) dir = -1;
        if (dir != 0) {
            struct Candidate { float angle; int idx; };
            std::vector<Candidate> cands;
            for (size_t i = 0; i < enemies.size(); i++) {
                if (!enemies[i].alive) continue;
                Vector3 to = Vector3Subtract(enemies[i].position, player.position);
                float d = Vector3Length(to);
                if (d > 55.0f) continue;
                float ang = atan2f(to.x, to.z) - player.rotation*DEG2RAD;
                ang = fmodf(ang + 5*PI, 2*PI) - PI;
                cands.push_back({ang, (int)i});
            }
            if (cands.size() > 1) {
                std::sort(cands.begin(), cands.end(), [](const Candidate& a, const Candidate& b){
                    return a.angle < b.angle;
                });
                int curr = -1;
                for (size_t j = 0; j < cands.size(); j++) {
                    if (cands[j].idx == player.lockedTarget) { curr = (int)j; break; }
                }
                if (curr != -1) {
                    int next = (curr + dir + (int)cands.size()) % (int)cands.size();
                    player.lockedTarget = cands[next].idx;
                    player.targetSwitchCooldown = 0.35f;
                }
            }
        }
    }

    if (player.lockedTarget != -1) {
        if (!enemies[player.lockedTarget].alive ||
            Vector3Distance(player.position, enemies[player.lockedTarget].position) > 60.0f) {
            player.lockedTarget = -1;
        }
    }

    // Movement input
    Vector3 moveInput{0,0,0};
    if (IsKeyDown(KEY_W)) moveInput.z += 1;
    if (IsKeyDown(KEY_S)) moveInput.z -= 1;
    if (IsKeyDown(KEY_D)) moveInput.x -= 1;
    if (IsKeyDown(KEY_A)) moveInput.x += 1;
    bool hasMoveInput = Vector3Length(moveInput) > 0.01f;
    if (hasMoveInput) moveInput = Vector3Normalize(moveInput);

    Vector3 moveDir{0,0,0};
    float rad = player.rotation * DEG2RAD;
    Vector3 camForward{ sinf(rad), 0, cosf(rad) };
    Vector3 camRight { cosf(rad), 0,-sinf(rad) };

    if (player.lockedTarget == -1) {
        moveDir = Vector3Add(Vector3Scale(camForward, moveInput.z),
                             Vector3Scale(camRight, moveInput.x));
    } else {
        Vector3 toTarget = Vector3Subtract(enemies[player.lockedTarget].position, player.position);
        toTarget.y = 0;
        float len = Vector3Length(toTarget);
        Vector3 targetForward = (len > 0.4f) ? Vector3Normalize(toTarget) : camForward;
        Vector3 targetRight { targetForward.z, 0, -targetForward.x };
        moveDir = Vector3Add(Vector3Scale(targetForward, moveInput.z),
                             Vector3Scale(targetRight, moveInput.x));
    }

    // Speed & sprint
    float speed = BASE_PLAYER_SPEED;
    bool sprinting = IsKeyDown(KEY_LEFT_SHIFT) && hasMoveInput && player.stamina > 8.0f && !player.isRolling;
    if (sprinting) {
        speed *= SPRINT_MULTIPLIER;
        player.stamina -= STAMINA_SPRINT_COST * dt;
        player.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
    }
    if (player.stamina <= 0) speed *= EXHAUSTED_MULTIPLIER;
    if (player.isAttacking || player.isRolling || player.isParrying || player.isHealing || player.staggerTimer > 0) {
        speed *= 0.38f;
    }

    // Roll (Shift tap)
    if (IsKeyPressed(KEY_LEFT_SHIFT) && hasMoveInput && player.stamina >= ROLL_COST &&
        !player.isAttacking && !player.isRolling && !player.isParrying && !player.isHealing && player.staggerTimer <= 0) {
        player.isRolling = true;
        player.rollTimer = ROLL_DURATION;
        player.rollStartPos = player.position;
        player.rollDirection = moveDir;
        player.stamina -= ROLL_COST;
        player.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION * 0.6f;
        player.hitInvuln = ROLL_DURATION + 0.15f;
    }

    if (player.isRolling) {
        player.rollTimer -= dt;
        float progress = 1.0f - (player.rollTimer / ROLL_DURATION);
        float eased = progress * progress;
        player.position = Vector3Add(player.rollStartPos,
                                    Vector3Scale(player.rollDirection, ROLL_DISTANCE * eased));

        // Perfect roll detection
        bool perfectWindowActive = (player.rollTimer < PERFECT_ROLL_WINDOW);
        if (perfectWindowActive && player.perfectRollTimer <= 0.0f) {
            for (auto& e : enemies) {
                if (!e.alive || !e.isAttacking) continue;
                float dur = e.attackDur * (e.isHeavyAttack ? 1.75f : 1.0f);
                float prog = 1.0f - (e.attackTimer / dur);
                float hitStart = e.isHeavyAttack ? 0.22f : 0.20f;
                float hitEnd = e.isHeavyAttack ? 0.85f : 0.80f;
                if (prog > hitStart && prog < hitEnd && IsEnemyAttackSwingHittingPlayer(e)) {
                    player.perfectRollTimer = 1.5f;
                    player.stamina = std::min(player.stamina + 24.0f, (float)MAX_STAMINA);
                    SpawnHitSparks(player.position, 18);
                    hitStopTimer = std::max(hitStopTimer, 0.04f);
                    break;
                }
            }
        }

        if (player.rollTimer <= 0.0f) {
            player.isRolling = false;
        }
    }

    // Normal movement
    if (!player.isRolling) {
        Vector3 desiredVel = Vector3Scale(moveDir, speed);
        player.velocity = Vector3Lerp(player.velocity, desiredVel, 22.0f * dt);

        Vector3 newPos = Vector3Add(player.position, Vector3Scale(player.velocity, dt));
        bool collision = false;
        for (const auto& obs : obstacles) {
            if (Vector3Distance({newPos.x, player.position.y, newPos.z}, obs) < 6.8f) {
                collision = true;
                break;
            }
        }
        if (!collision) {
            player.position.x = newPos.x;
            player.position.z = newPos.z;
        } else {
            player.velocity = Vector3Scale(player.velocity, 0.15f);
        }
    }

    // Gravity & jump
    player.yVelocity += GRAVITY * dt;
    player.position.y += player.yVelocity * dt;
    if (player.position.y < 0.0f) {
        player.position.y = 0.0f;
        player.yVelocity = 0.0f;
    }

    bool grounded = (player.position.y <= 0.05f);
    if (IsKeyPressed(KEY_SPACE) && grounded && player.stamina >= 5.0f &&
        !player.isAttacking && !player.isRolling && !player.isParrying && !player.isHealing &&
        player.staggerTimer <= 0) {
        player.yVelocity = JUMP_VELOCITY;
        player.stamina -= 5.0f;
    }

    // Flask
    if (IsKeyPressed(KEY_E) && player.flasks > 0 && !player.isHealing &&
        !player.isAttacking && !player.isRolling && !player.isParrying && player.staggerTimer <= 0) {
        player.isHealing = true;
        player.healTimer = FLASK_USE_TIME;
        player.flasks--;
    }
    if (player.isHealing) {
        player.healTimer -= dt;
        if (player.healTimer <= 0.0f) {
            player.health = std::min(player.health + (int)FLASK_HEAL_AMOUNT, player.maxHealth);
            player.isHealing = false;
        }
    }

    // Parry
    if (IsKeyPressed(KEY_LEFT_CONTROL) && player.stamina >= STAMINA_PARRY_COST &&
        !player.isAttacking && !player.isRolling && !player.isHealing && player.staggerTimer <= 0) {
        player.isParrying = true;
        player.parryTimer = 0.38f;
        player.stamina -= STAMINA_PARRY_COST;
        player.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
    }
    if (player.isParrying) {
        player.parryTimer -= dt;
        if (player.parryTimer <= 0.0f) player.isParrying = false;
    }

    // Attack input
    bool attackInput = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool attackRelease = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    if (attackInput && !player.isCharging && !player.isAttacking && !player.isRolling &&
        !player.isParrying && !player.isHealing && player.stamina >= STAMINA_POWER_COST &&
        player.staggerTimer <= 0) {
        player.isCharging = true;
        player.chargeTimer = 0.0f;
        player.powerReady = false;
    }

    if (player.isCharging) {
        player.chargeTimer += dt;
        if (player.chargeTimer >= POWER_ATTACK_CHARGE) player.powerReady = true;
    }

    if (attackRelease && player.isCharging) {
        player.isCharging = false;
        if (player.powerReady) {
            player.isAttacking = true;
            player.attackTimer = POWER_ATTACK_DURATION;
            player.currentAttack = HEAVY;
            player.stamina -= STAMINA_POWER_COST;
            player.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION * 1.4f;
            player.comboStep = 0;
            player.comboTimer = COMBO_RESET_TIME;
        }
        else if (player.stamina >= STAMINA_ATTACK_COST) {
            player.isAttacking = true;
            player.stamina -= STAMINA_ATTACK_COST;
            player.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
            if (player.comboTimer <= 0 || player.comboStep == 0) {
                player.comboStep = 1;
            } else {
                player.comboStep = (player.comboStep % 3) + 1;
            }
            player.comboTimer = COMBO_RESET_TIME;
            player.attackTimer = NORMAL_ATTACK_DURATION;
            player.currentAttack = static_cast<AttackType>(player.comboStep - 1);
        }
        player.powerReady = false;
    }

    // Attack animation & hit detection
    if (player.isAttacking) {
        player.comboTimer = COMBO_RESET_TIME;
        float duration = (player.currentAttack == HEAVY) ? POWER_ATTACK_DURATION : NORMAL_ATTACK_DURATION;
        float progress = 1.0f - (player.attackTimer / duration);

        // Swing animation
        if (player.currentAttack == HEAVY) {
            float pp = progress * 3.0f;
            if (pp < 1.0f) {
                player.swingPitch = Lerp(160.0f, -110.0f, pp);
                player.swingYaw = Lerp(100.0f, -100.0f, pp);
            } else if (pp < 2.0f) {
                player.swingPitch = -110.0f;
                player.swingYaw = Lerp(-100.0f, 200.0f, pp-1.0f);
            } else {
                player.swingPitch = Lerp(-110.0f, 140.0f, pp-2.0f);
                player.swingYaw = Lerp(200.0f, 0.0f, pp-2.0f);
            }
        }
        else {
            if (player.comboStep == 1) {
                player.swingPitch = Lerp(110.0f, -95.0f, progress);
                player.swingYaw = Lerp(80.0f, -80.0f, progress);
            } else if (player.comboStep == 2) {
                player.swingPitch = Lerp(30.0f, -30.0f, progress);
                player.swingYaw = Lerp(-170.0f, 170.0f, progress);
            } else {
                player.swingPitch = Lerp(-90.0f, 125.0f, progress);
                player.swingYaw = Lerp(-70.0f, 90.0f, progress);
            }
        }

        // Hit window
        if (progress > 0.18f && progress < 0.82f) {
            for (auto& e : enemies) {
                CheckPlayerAttackHitEnemy(e);
            }
        }

        player.attackTimer -= dt;
        if (player.attackTimer <= 0.0f) {
            player.isAttacking = false;
        }
    }
    else if (player.staggerTimer <= 0.0f && !player.isRolling && !player.isParrying) {
        player.swingPitch = Lerp(player.swingPitch, -30.0f, 14.0f * dt);
        player.swingYaw = Lerp(player.swingYaw, 30.0f, 14.0f * dt);
        if (player.comboTimer <= 0.0f) player.comboStep = 0;
    }

    AddWeaponTrailPoint();

    // Blade position
    float yawRad = player.swingYaw * DEG2RAD;
    float pitchRad = player.swingPitch * DEG2RAD;
    Vector3 pivotLocal = {0.65f, 1.65f, 0.4f};
    Vector3 pivotWorld = Vector3Add(player.position,
                                    Vector3RotateByAxisAngle(pivotLocal, {0,1,0}, player.rotation * DEG2RAD));
    Vector3 baseLocal = {0, -0.7f, 0.6f};
    Vector3 tipLocal = {0, -0.7f, player.weapon.length};
    Vector3 base = Vector3RotateByAxisAngle(baseLocal, {1,0,0}, pitchRad);
    base = Vector3RotateByAxisAngle(base, {0,1,0}, yawRad);
    Vector3 tip = Vector3RotateByAxisAngle(tipLocal, {1,0,0}, pitchRad);
    tip = Vector3RotateByAxisAngle(tip, {0,1,0}, yawRad);
    player.bladeStart = Vector3Add(pivotWorld, base);
    player.bladeEnd = Vector3Add(pivotWorld, tip);

    // Stamina regen
    player.staminaRegenDelay -= dt;
    if (player.staminaRegenDelay <= 0.0f) {
        player.stamina = std::min(player.stamina + STAMINA_REGEN_RATE * dt, (float)MAX_STAMINA);
    }
}

// ======================================================================
// Enemy Update
// ======================================================================
void UpdateEnemies(float dt) {
    for (auto& e : enemies) {
        if (!e.alive) continue;

        e.hitInvuln -= dt;
        e.stunTimer -= dt;
        e.staminaRegenDelay -= dt;
        if (e.staminaRegenDelay <= 0) {
            e.stamina = std::min(e.stamina + 32.0f * dt, (float)MAX_STAMINA);
        }

        if (e.stunTimer > 0) {
            e.velocity = Vector3Lerp(e.velocity, {0,0,0}, 12.0f * dt);
            continue;
        }

        bool seesPlayer = CanSeePlayer(e);
        Vector3 toPlayer = Vector3Subtract(player.position, e.position);
        toPlayer.y = 0.0f;
        float distToPlayer = Vector3Length(toPlayer);

        Vector3 moveDir{0,0,0};
        float moveSpeed = e.speed * (e.stamina <= 0.0f ? EXHAUSTED_MULTIPLIER : 1.0f);

        if (e.type == BOSS) {
            e.state = CHASE;
            e.alertTimer = 10.0f;
            if (distToPlayer > 0.5f) {
                e.rotation = atan2f(toPlayer.x, toPlayer.z) * RAD2DEG;
            }
            Vector3 forward = Vector3Normalize(toPlayer);
            Vector3 tangent = {forward.z, 0.0f, -forward.x};
            tangent = Vector3Scale(tangent, e.strafeSide * 0.3f);
            moveDir = Vector3Add(forward, tangent);
            moveDir = Vector3Normalize(moveDir);
            moveSpeed *= 1.1f;

            e.comboDelayTimer -= dt;
            Vector3 eFacing = {sinf(e.rotation*DEG2RAD), 0, cosf(e.rotation*DEG2RAD)};
            float dot = Vector3DotProduct(eFacing, Vector3Normalize(toPlayer));
            if (distToPlayer <= ATTACK_RANGE + 5.0f && dot > 0.5f && !e.isAttacking && e.comboDelayTimer <= 0.0f && e.stamina >= 30.0f) {
                e.comboStep = (e.comboStep % 5) + 1;
                if (e.comboStep == 1) e.comboDelayTimer = 2.2f;
                e.isAttacking = true;
                float dur = (e.comboStep == 3 || e.comboStep == 5) ? 0.85f : 0.55f;
                e.attackTimer = dur;
                e.stamina -= 30.0f;
                e.staminaRegenDelay = 1.2f;
            }
        } else {
            // Awareness
            if (seesPlayer) {
                e.lastKnownPlayerPos = player.position;
                e.alertTimer = 12.0f;
                e.state = CHASE;
            } else if (e.alertTimer > 0.0f) {
                e.alertTimer -= dt;
                if (Vector3Distance(e.position, e.lastKnownPlayerPos) < 8.0f) {
                    e.state = SEARCH;
                }
            } else {
                e.state = PATROL;
            }

            e.attackCooldown -= dt;

            bool inCombatRange = (e.state != PATROL) && distToPlayer < 45.0f;
            if (inCombatRange) {
                e.strafeTimer -= dt;
                if (e.strafeTimer <= 0.0f) {
                    e.strafeSide *= -1.0f;
                    e.strafeTimer = (float)GetRandomValue(30, 70) / 10.0f;
                }
            }

            // Patrol behavior
            if (e.state == PATROL) {
                e.patrolTimer -= dt;
                if (e.patrolTimer <= 0.0f || Vector3Distance(e.position, e.patrolTarget) < 6.0f) {
                    float ang = (float)GetRandomValue(0, 359) * DEG2RAD;
                    float r = (float)GetRandomValue(0, (int)e.patrolRadius);
                    e.patrolTarget = Vector3Add(e.homePosition, {cosf(ang)*r, 0.0f, sinf(ang)*r});
                    e.patrolTimer = (float)GetRandomValue(6, 14);
                }
                Vector3 toPatrol = Vector3Subtract(e.patrolTarget, e.position);
                toPatrol.y = 0.0f;
                if (Vector3Length(toPatrol) > 1.0f) {
                    moveDir = Vector3Normalize(toPatrol);
                    moveSpeed *= 0.55f;
                }
                e.rotation = atan2f(toPatrol.x, toPatrol.z) * RAD2DEG;
            } else {
                if (seesPlayer) {
                    e.rotation = atan2f(toPlayer.x, toPlayer.z) * RAD2DEG;
                }

                if (distToPlayer > 45.0f) {
                    moveDir = Vector3Normalize(toPlayer);
                } else {
                    Vector3 forward = Vector3Normalize(toPlayer);
                    Vector3 tangent = {forward.z, 0.0f, -forward.x};
                    tangent = Vector3Scale(tangent, e.strafeSide);
                    float forwardAmt = (distToPlayer > ATTACK_RANGE + 3.0f) ? 0.6f : 0.3f;
                    float strafeAmt = 0.7f;
                    if (e.type == TANK) {
                        forwardAmt = (distToPlayer > ATTACK_RANGE + 3.0f) ? 0.8f : 0.6f;
                        strafeAmt = 0.3f;
                    } else if (e.type == AGILE) {
                        forwardAmt = (distToPlayer > ATTACK_RANGE + 3.0f) ? 0.4f : 0.1f;
                        strafeAmt = 0.9f;
                        moveSpeed *= 1.15f;
                    }
                    moveDir = Vector3Add(Vector3Scale(forward, forwardAmt), Vector3Scale(tangent, strafeAmt));
                    if (Vector3Length(moveDir) > 0.01f) moveDir = Vector3Normalize(moveDir);
                    moveSpeed *= 0.85f;
                }
            }

            // Attack decision
            Vector3 eFacing = {sinf(e.rotation * DEG2RAD), 0.0f, cosf(e.rotation * DEG2RAD)};
            float dot = Vector3DotProduct(eFacing, Vector3Normalize(toPlayer));
            if (distToPlayer <= ATTACK_RANGE + 1.8f && dot > 0.55f && e.attackCooldown <= 0.0f &&
                e.stamina >= 26.0f && !e.isAttacking && !e.isDodging && !e.isBlocking && e.stunTimer <= 0.0f) {
                bool wantHeavy = (e.type == TANK && GetRandomValue(0, 100) < 40);
                bool canHeavy = (e.stamina >= 48.0f);
                e.isHeavyAttack = wantHeavy && canHeavy;
                float staminaCost = e.isHeavyAttack ? 48.0f : 26.0f;
                float durMult = e.isHeavyAttack ? 1.75f : 1.0f;
                e.attackTimer = e.attackDur * durMult;
                e.currentAttack = e.isHeavyAttack ? LIGHT_1 : static_cast<AttackType>(GetRandomValue(0, 2));
                e.isAttacking = true;
                e.stamina -= staminaCost;
                e.staminaRegenDelay = e.isHeavyAttack ? 1.4f : 0.8f;
                float baseCd = (e.type == AGILE) ? 0.9f : ((e.type == TANK) ? 2.5f : 1.6f);
                baseCd += e.isHeavyAttack ? 1.3f : 0.0f;
                e.attackCooldown = baseCd + (float)GetRandomValue(0, 15) / 10.0f;
            }
        }

        // Commit to attack (no movement)
        if (e.isAttacking) {
            moveDir = {0,0,0};
        }

        e.velocity = Vector3Lerp(e.velocity, Vector3Scale(moveDir, moveSpeed), 12.0f * dt);
        e.position = Vector3Add(e.position, Vector3Scale(e.velocity, dt));

        // Dodge player attack
        if (player.isAttacking && distToPlayer < 9.0f && e.stamina >= 32.0f &&
            !e.isDodging && !e.isAttacking && !e.isBlocking &&
            GetRandomValue(0, 100) < (int)(e.dodgeChance * 100.0f)) {
            e.isDodging = true;
            e.dodgeTimer = ROLL_DURATION;
            e.dodgeStartPos = e.position;
            Vector3 dodgeDir = Vector3Normalize(Vector3Subtract(e.position, player.position));
            if (e.type == AGILE && GetRandomValue(0, 100) < 60) {
                Vector3 side = {dodgeDir.z, 0.0f, -dodgeDir.x};
                side = Vector3Scale(side, GetRandomValue(0, 1) ? 1.0f : -1.0f);
                dodgeDir = Vector3Normalize(Vector3Add(dodgeDir, side));
            }
            e.dodgeDirection = dodgeDir;
            e.stamina -= 32.0f;
            e.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
        }

        if (e.isDodging) {
            e.dodgeTimer -= dt;
            float progress = 1.0f - (e.dodgeTimer / ROLL_DURATION);
            float eased = progress * progress;
            e.position = Vector3Add(e.dodgeStartPos,
                                   Vector3Scale(e.dodgeDirection, 12.5f * eased));
            if (e.dodgeTimer <= 0.0f) e.isDodging = false;
        }

        // Tank block
        if (e.type == TANK && !e.isBlocking && !e.isAttacking && !e.isDodging &&
            player.isAttacking && distToPlayer < ATTACK_RANGE + 3.0f && e.stamina >= 22.0f &&
            GetRandomValue(0, 100) < 75) {
            e.isBlocking = true;
            e.blockTimer = 0.7f;
            e.stamina -= 22.0f;
            e.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
        }
        if (e.isBlocking) {
            e.blockTimer -= dt;
            if (e.blockTimer <= 0.0f) e.isBlocking = false;
        }

        // Attack execution
        if (e.isAttacking) {
            float dur = (e.type == BOSS) ? ((e.comboStep == 3 || e.comboStep == 5) ? 0.85f : 0.55f)
                                        : e.attackDur * (e.isHeavyAttack ? 1.75f : 1.0f);
            float progress = 1.0f - (e.attackTimer / dur);

            // Boss combo animations
            if (e.type == BOSS) {
                switch(e.comboStep) {
                    case 1: e.swingYaw = Lerp(80.0f, -80.0f, progress);
                            e.swingPitch = Lerp(90.0f, -70.0f, progress); break;
                    case 2: e.swingYaw = Lerp(-120.0f, 120.0f, progress);
                            e.swingPitch = Lerp(40.0f, -40.0f, progress); break;
                    case 3: e.swingYaw = Lerp(-180.0f, 180.0f, progress);
                            e.swingPitch = Lerp(0.0f, 0.0f, progress); break;
                    case 4: e.swingYaw = Lerp(60.0f, -60.0f, progress);
                            e.swingPitch = Lerp(-100.0f, 100.0f, progress); break;
                    case 5: {
                        float pp = progress * 3.0f;
                        if (pp < 1.0f) {
                            e.swingYaw = Lerp(100.0f, -100.0f, pp);
                            e.swingPitch = Lerp(160.0f, -110.0f, pp);
                        } else if (pp < 2.0f) {
                            e.swingYaw = Lerp(-100.0f, 200.0f, pp - 1.0f);
                            e.swingPitch = -110.0f;
                        } else {
                            e.swingYaw = Lerp(200.0f, 0.0f, pp - 2.0f);
                            e.swingPitch = Lerp(-110.0f, 140.0f, pp - 2.0f);
                        }
                    } break;
                }
            } else {
                if (e.currentAttack == LIGHT_1) {
                    e.swingPitch = Lerp(110.0f, -95.0f, progress);
                    e.swingYaw = Lerp(80.0f, -80.0f, progress);
                } else if (e.currentAttack == LIGHT_2) {
                    e.swingPitch = Lerp(30.0f, -30.0f, progress);
                    e.swingYaw = Lerp(-170.0f, 170.0f, progress);
                } else {
                    e.swingPitch = Lerp(-90.0f, 125.0f, progress);
                    e.swingYaw = Lerp(-70.0f, 90.0f, progress);
                }
            }

            // Hit window
            float hitStart = (e.type == BOSS && (e.comboStep == 3 || e.comboStep == 5)) ? 0.25f : 0.20f;
            float hitEnd = (e.type == BOSS && e.comboStep == 3) ? 0.85f : 0.80f;
            if (progress > hitStart && progress < hitEnd) {
                if (IsEnemyAttackSwingHittingPlayer(e)) {
                    if (player.isParrying && player.parryTimer > 0.12f) {
                        player.riposteTimer = 1.8f;
                        e.stunTimer = 2.8f;
                        Vector3 knockDir = Vector3Normalize(Vector3Subtract(e.position, player.position));
                        e.velocity = Vector3Add(e.velocity, Vector3Scale(knockDir, 28.0f));
                        SpawnHitSparks(e.position, 24);
                        hitStopTimer = std::max(hitStopTimer, 0.06f);
                        player.shakeTimer = std::max(player.shakeTimer, 0.32f);
                    } else if (!player.isRolling && player.hitInvuln <= 0.0f) {
                        ApplyEnemyHitToPlayer(e);
                    }
                }
            }

            e.attackTimer -= dt;
            if (e.attackTimer <= 0.0f) {
                e.isAttacking = false;
                e.isHeavyAttack = false;
            }
        } else if (!e.isBlocking && e.stunTimer <= 0.0f) {
            e.swingPitch = Lerp(e.swingPitch, -30.0f, 14.0f * dt);
            e.swingYaw = Lerp(e.swingYaw, 30.0f, 14.0f * dt);
        }

        // Blade position
        float bladeLen = (e.type == BOSS) ? 9.5f : 5.8f;
        float er = e.rotation * DEG2RAD;
        Vector3 epivot = Vector3Add(e.position, Vector3RotateByAxisAngle({0.65f,1.65f,0.4f}, {0,1,0}, er));
        Vector3 ebaseLocal = {0,-0.7f,0.6f};
        Vector3 etipLocal = {0,-0.7f, bladeLen};
        Vector3 ebase = Vector3RotateByAxisAngle(ebaseLocal, {1,0,0}, e.swingPitch*DEG2RAD);
        ebase = Vector3RotateByAxisAngle(ebase, {0,1,0}, e.swingYaw*DEG2RAD);
        Vector3 etip = Vector3RotateByAxisAngle(etipLocal, {1,0,0}, e.swingPitch*DEG2RAD);
        etip = Vector3RotateByAxisAngle(etip, {0,1,0}, e.swingYaw*DEG2RAD);
        e.bladeStart = Vector3Add(epivot, ebase);
        e.bladeEnd = Vector3Add(epivot, etip);
    }
}

// ======================================================================
// Hit Detection
// ======================================================================
bool IsEnemyAttackSwingHittingPlayer(const Enemy& e) {
    Vector3 toPlayer = Vector3Subtract(player.position, e.position);
    toPlayer.y = 0.0f;
    float dist = Vector3Length(toPlayer);
    float maxDist = (e.type == BOSS && e.comboStep == 3) ? ATTACK_RANGE + 3.0f : ATTACK_RANGE + 1.2f;
    if (dist > maxDist) return false;

    Vector3 norm = Vector3Normalize(toPlayer);
    Vector3 eFacing = {sinf(e.rotation*DEG2RAD),0,cosf(e.rotation*DEG2RAD)};
    float minDot = (e.type == BOSS && e.comboStep == 3) ? 0.3f : 0.6f;
    if (Vector3DotProduct(eFacing, norm) > minDot) return true;
    return false;
}

void ApplyEnemyHitToPlayer(const Enemy& e) {
    Vector3 toPlayer = Vector3Subtract(player.position, e.position);
    toPlayer.y = 0.0f;
    Vector3 norm = Vector3Normalize(toPlayer);

    float dmgMult = (e.type == BOSS) ? ((e.comboStep == 5) ? 2.1f : ((e.comboStep == 3) ? 1.6f : 1.3f))
                                    : (e.isHeavyAttack ? 1.75f : 1.0f);
    float poiseMult = (e.type == BOSS) ? ((e.comboStep == 5) ? 2.2f : ((e.comboStep == 3) ? 1.8f : 1.4f))
                                      : (e.isHeavyAttack ? 1.85f : 1.0f);
    float knockMult = (e.type == BOSS) ? ((e.comboStep == 5) ? 1.8f : 1.3f)
                                      : (e.isHeavyAttack ? 1.5f : 1.0f);

    int damage = (int)(e.attackDamage * dmgMult);
    float poiseDmg = e.poiseDamage * poiseMult;

    player.health -= damage;
    player.hitInvuln = 0.5f;
    player.velocity = Vector3Add(player.velocity, Vector3Scale(norm, 12.0f * knockMult));

    SpawnBloodParticles(player.position, (e.type == BOSS || e.isHeavyAttack) ? 24 : 16);

    float thisStop = (e.type == BOSS) ? ((e.comboStep == 5) ? 0.07f : 0.05f)
                                     : (e.isHeavyAttack ? 0.05f : 0.03f);
    float thisShake = (e.type == BOSS) ? 0.35f : (e.isHeavyAttack ? 0.28f : 0.20f);

    if (player.staggerTimer <= 0.0f) {
        player.poise -= poiseDmg;
        if (player.poise <= 0.0f) {
            player.poise = player.maxPoise;
            player.staggerTimer = 1.5f;
            player.velocity = Vector3Add(player.velocity, Vector3Scale(norm, 24.0f * knockMult));
            thisStop = 0.07f;
            thisShake = 0.42f;
        }
    }

    hitStopTimer = std::max(hitStopTimer, thisStop);
    player.shakeTimer = std::max(player.shakeTimer, thisShake);
}

bool CheckPlayerAttackHitEnemy(Enemy& e) {
    if (!e.alive || e.hitInvuln > 0) return false;

    Vector3 toEnemy = Vector3Subtract(e.position, player.position);
    toEnemy.y = 0;
    float dist = Vector3Length(toEnemy);
    if (dist > ATTACK_RANGE + 1.4f) return false;

    Vector3 normToEnemy = Vector3Normalize(toEnemy);
    Vector3 pFacing = {sinf(player.rotation*DEG2RAD), 0, cosf(player.rotation*DEG2RAD)};
    float dot = Vector3DotProduct(pFacing, normToEnemy);
    float minDot = (player.comboStep == 2) ? -0.45f : 0.35f;
    if (dot < minDot) return false;

    bool isHeavy = (player.currentAttack == HEAVY);
    int baseDamage = isHeavy ? 92 : 62;
    float basePoiseDmg = isHeavy ? 68.0f : 28.0f;

    if (isHeavy) {
        float prog = 1.0f - player.attackTimer / POWER_ATTACK_DURATION;
        float pp = prog * 3.0f;
        if (pp < 1.0f) baseDamage = 35;
        else if (pp < 2.0f) baseDamage = 42;
        else baseDamage = 55;
    }

    Vector3 eFacing = {sinf(e.rotation*DEG2RAD), 0, cosf(e.rotation*DEG2RAD)};
    float backstabDot = Vector3DotProduct(eFacing, normToEnemy);
    bool backstab = backstabDot < -0.75f;
    bool riposte = e.stunTimer > 0;

    float dmgMult = (backstab || riposte) ? 2.6f : 1.0f;
    float poiseMult = (backstab || riposte) ? 2.3f : 1.0f;
    if (e.stamina <= 0) poiseMult *= 1.7f;
    float knockMult = (backstab || riposte) ? 2.1f : 1.0f;

    bool blocked = e.isBlocking && !isHeavy;
    if (blocked) {
        dmgMult *= 0.4f;
        poiseMult *= 0.55f;
        e.isBlocking = false;
        SpawnHitSparks(e.position, 12);
    }

    int damage = (int)(baseDamage * dmgMult * player.weapon.damageMultiplier);
    float poiseDamage = basePoiseDmg * poiseMult * player.weapon.poiseDamageMultiplier;

    e.health -= damage;
    e.hitInvuln = 0.4f;
    e.velocity = Vector3Add(e.velocity, Vector3Scale(normToEnemy, 14.0f * knockMult));

    float thisStop = blocked ? 0.02f : (isHeavy ? 0.05f : 0.03f);
    if (backstab || riposte) thisStop = 0.06f;

    bool poiseBreak = false;
    if (e.stunTimer <= 0) {
        e.poise -= poiseDamage;
        if (e.poise <= 0) {
            e.poise = e.maxPoise;
            e.stunTimer = 2.4f;
            e.velocity = Vector3Add(e.velocity, Vector3Scale(normToEnemy, 26.0f));
            poiseBreak = true;
            thisStop = 0.07f;
        }
    }

    float thisShake = blocked ? 0.10f : (isHeavy ? 0.25f : 0.18f);
    if (backstab || riposte || poiseBreak) thisShake = 0.35f;

    hitStopTimer = std::max(hitStopTimer, thisStop);
    player.shakeTimer = std::max(player.shakeTimer, thisShake);

    if (backstab || riposte) {
        SpawnBloodParticles(e.position, 24);
    } else {
        SpawnBloodParticles(e.position, 12);
    }

    if (e.health <= 0) {
        e.alive = false;
        SpawnBloodParticles(e.position, 30);
    }

    return true;
}

// ======================================================================
// Camera
// ======================================================================
void UpdateCamera(float dt) {
    Vector3 desiredTarget = Vector3Add(player.position, {0, 2.0f, 0});

    if (player.lockedTarget != -1 && enemies[player.lockedTarget].alive) {
        Enemy& tgt = enemies[player.lockedTarget];
        desiredTarget = Vector3Lerp(desiredTarget, Vector3Add(tgt.position, {0, 2.8f, 0}), 0.55f);
        Vector3 to = Vector3Subtract(tgt.position, player.position);
        to.y = 0;
        float len = Vector3Length(to);
        if (len > 0.6f) {
            player.rotation = atan2f(to.x, to.z) * RAD2DEG;
        }
    }

    float rad = player.rotation * DEG2RAD;
    Vector3 behind = Vector3Scale({sinf(rad), 0, cosf(rad)}, -CAMERA_DISTANCE);
    Vector3 desiredPos = Vector3Add(player.position, Vector3Add(behind, {0, CAMERA_HEIGHT, 0}));

    camPos = Vector3Lerp(camPos, desiredPos, CAMERA_SMOOTH * dt);
    Vector3 camTarget = Vector3Lerp(camera.target, desiredTarget, CAMERA_SMOOTH * dt);

    Vector3 shake{0};
    if (player.shakeTimer > 0) {
        player.shakeTimer -= dt;
        float str = player.shakeTimer * 60.0f;
        shake.x = GetRandomValue(-100,100)/1000.0f * str;
        shake.y = GetRandomValue(-100,100)/1000.0f * str;
        shake.z = GetRandomValue(-100,100)/1000.0f * str;
    }

    camera.position = Vector3Add(camPos, shake);
    camera.target = camTarget;
}

// ======================================================================
// Drawing
// ======================================================================
void Draw3DScene() {
    DrawPlane({0,-1.0f,0}, {600,600}, {45,40,55,255});

    for (const auto& obs : obstacles) {
        DrawCube(obs, 8.0f, 16.0f, 8.0f, DARKGRAY);
        DrawCube(Vector3Add(obs, {0,9.0f,0}), 6.0f, 2.0f, 6.0f, GRAY);
    }

    // Exit portal only in level 1
    if (currentLevel == 1) {
        Color exitCol = exitActive ? GOLD : DARKGRAY;
        DrawCube(Vector3Add(exitPosition, {0,6.0f,0}), 10.0f, 12.0f, 4.0f, Fade(exitCol, 0.6f));
        DrawSphere(Vector3Add(exitPosition, {0,10.0f,0}), 4.0f, exitCol);
    }

    DrawPlayer();
    for (size_t i = 0; i < enemies.size(); i++) {
        if (enemies[i].alive) DrawEnemy(enemies[i], (int)i);
    }

    for (const auto& p : particles) {
        DrawSphere(p.position, p.size, p.color);
    }

    // Weapon trail
    for (size_t i = 1; i < weaponTrail.size(); i++) {
        float alpha = 1.0f - (weaponTrail[i].time / 0.5f);
        if (alpha <= 0) continue;
        Color c = player.powerReady ? Fade(ORANGE, alpha) : Fade(player.weapon.bladeColor, alpha*0.8f);
        DrawLine3D(weaponTrail[i-1].pos, weaponTrail[i].pos, c);
        DrawLine3D(Vector3Add(weaponTrail[i-1].pos, {0,0.2f,0}), Vector3Add(weaponTrail[i].pos, {0,0.2f,0}), c);
    }
}

void AddWeaponTrailPoint() {
    static float trailTimer = 0;
    trailTimer += GetFrameTime();
    if (trailTimer < 0.016f) return;
    trailTimer = 0;

    if (player.isAttacking || player.isCharging) {
        weaponTrail.push_back({player.bladeEnd, 0.0f});
    }

    for (auto it = weaponTrail.begin(); it != weaponTrail.end(); ) {
        it->time += GetFrameTime();
        if (it->time > 0.5f) {
            it = weaponTrail.erase(it);
        } else {
            ++it;
        }
    }
}

void DrawPlayer() {
    rlPushMatrix();
    rlTranslatef(player.position.x, player.position.y, player.position.z);
    rlRotatef(player.rotation, 0,1,0);
    if (player.isRenewing) {
        rlRotatef(player.renewalAngle, 1,0,0);
    }

    Color bodyColor = {60, 80, 140, 255};
    if (player.isHealing) bodyColor = GOLD;
    if (player.staggerTimer > 0) bodyColor = Fade(YELLOW, 0.8f);

    // Legs
    DrawCylinderEx({-0.4f, -0.9f, 0}, {-0.4f, 1.0f, 0}, 0.5f, 0.4f, 12, DARKGRAY);
    DrawCylinderEx({ 0.4f, -0.9f, 0}, { 0.4f, 1.0f, 0}, 0.5f, 0.4f, 12, DARKGRAY);
    DrawSphere({-0.4f, -0.9f, 0}, 0.52f, DARKGRAY);
    DrawSphere({ 0.4f, -0.9f, 0}, 0.52f, DARKGRAY);

    // Torso
    DrawCube({0, 0.9f, 0}, 1.7f, 2.9f, 1.3f, bodyColor);
    DrawCube({0, 1.1f, 0.45f}, 1.9f, 2.2f, 0.5f, Fade(bodyColor, 0.7f));

    // Arms & Head
    DrawSphere({-1.0f, 1.9f, 0}, 0.55f, bodyColor);
    DrawSphere({ 1.0f, 1.9f, 0}, 0.55f, bodyColor);
    DrawSphere({0, 2.4f, 0}, 0.62f, Fade(bodyColor, 0.9f));
    DrawCylinderEx({0, 2.4f, 0}, {0, 3.1f, 0}, 0.75f, 0.55f, 16, DARKGRAY);

    // Left arm (parry pose)
    rlPushMatrix();
    rlTranslatef(-0.9f, 1.4f, 0);
    float leftAngle = player.isParrying ? 80.0f : -25.0f;
    rlRotatef(leftAngle, 1, 0, 0);
    DrawCylinderEx({0, 0, 0}, {0, -1.4f, 0}, 0.35f, 0.3f, 12, bodyColor);
    DrawSphere({0, -1.4f, 0}, 0.38f, DARKGRAY);
    rlPopMatrix();

    // Weapon arm
    rlPushMatrix();
    rlTranslatef(0.65f, 1.65f, 0.4f);
    rlRotatef(player.swingYaw, 0, 1, 0);
    rlRotatef(player.swingPitch, 1, 0, 0);

    // Handle
    DrawCylinderEx({0, -0.4f, 0}, {0, -1.4f, 0}, 0.22f, 0.22f, 16, {139, 69, 19, 255});
    DrawSphere({0, -1.6f, 0}, 0.35f, GRAY);
    DrawCube({0, -0.2f, 0}, 0.5f, 0.4f, 1.0f, GRAY);
    DrawCylinderEx({-1.4f, -0.2f, 0}, {1.4f, -0.2f, 0}, 0.28f, 0.28f, 12, GRAY);

    // Blade
    float length = player.weapon.length;
    float thickness = 0.18f;
    float baseWidth = 1.3f;
    float tipWidth = 0.7f;
    Color bladeCol = player.weapon.bladeColor;

    DrawCube({0, 0.0f, length * 0.4f}, thickness, baseWidth, length * 0.8f, bladeCol);
    DrawCube({0, 0.0f, length - tipWidth * 0.4f}, thickness, tipWidth, length * 0.4f, bladeCol);
    DrawCube({0, 0.04f, length * 0.5f}, thickness * 0.6f, baseWidth * 0.55f, length * 0.9f, Fade(bladeCol, 0.7f));
    DrawLine3D({ thickness/2 + 0.04f, 0, 0}, { thickness/2 + 0.04f, 0, length}, WHITE);
    DrawLine3D({-thickness/2 - 0.04f, 0, 0}, {-thickness/2 - 0.04f, 0, length}, WHITE);

    // Glow
    if (player.weapon.hasGlow || player.isCharging || player.powerReady) {
        float pulse = 0.4f + 0.4f * sinf(GetTime() * 12.0f);
        float alpha = player.powerReady ? 0.9f : pulse;
        Color glow = player.powerReady ? ORANGE : player.weapon.bladeColor;
        DrawCube({0, 0.0f, length * 0.5f}, thickness * 2.2f, baseWidth * 1.4f, length * 1.15f, Fade(glow, alpha));
    }

    rlPopMatrix();
    rlPopMatrix();
}

void DrawEnemy(const Enemy& e, int index) {
    rlPushMatrix();
    rlTranslatef(e.position.x, e.position.y, e.position.z);
    rlRotatef(e.rotation, 0,1,0);
    rlScalef(e.scale, e.scale, e.scale);

    Color body = e.bodyColor;
    if (e.stunTimer > 0) body = YELLOW;
    if (e.isBlocking) body = Fade(SKYBLUE, 1.2f);
    if (e.isDodging) body = LIME;

    if (e.type == BOSS) {
        body = {200, 40, 60, 255};
        DrawCube({0, 1.2f, 0}, 2.4f, 3.8f, 1.8f, body);
        DrawSphere({0, 3.8f, 0}, 0.9f, body);
        DrawCylinderEx({0, 3.8f, 0}, {0, 5.2f, 0}, 1.1f, 0.7f, 16, DARKGRAY);
        DrawCylinderEx({-0.8f, 4.2f, 0}, {-1.4f, 5.8f, 0}, 0.3f, 0.1f, 8, GRAY);
        DrawCylinderEx({0.8f, 4.2f, 0}, {1.4f, 5.8f, 0}, 0.3f, 0.1f, 8, GRAY);
        DrawCube({0, 1.8f, -0.8f}, 2.6f, 3.8f, 0.3f, Fade(RED, 0.8f));
    } else {
        DrawCylinderEx({-0.4f, -0.9f, 0}, {-0.4f, 1.0f, 0}, 0.5f, 0.4f, 12, DARKGRAY);
        DrawCylinderEx({ 0.4f, -0.9f, 0}, { 0.4f, 1.0f, 0}, 0.5f, 0.4f, 12, DARKGRAY);
        DrawCube({0, 0.9f, 0}, 1.7f, 2.9f, 1.3f, body);
        DrawSphere({0, 2.4f, 0}, 0.62f, Fade(body, 0.9f));

        if (e.type == GRUNT) {
            DrawCylinderEx({0, 2.4f, 0}, {0, 3.1f, 0}, 0.75f, 0.55f, 16, MAROON);
            DrawCylinderEx({0, 3.3f, 0}, {0, 4.2f, 0}, 0.3f, 0.0f, 8, RED);
        } else if (e.type == TANK) {
            DrawCube({0, 2.7f, 0}, 1.5f, 1.8f, 1.5f, DARKGRAY);
        } else if (e.type == AGILE) {
            DrawCylinderEx({0, 2.4f, 0}, {0, 3.6f, 0}, 0.85f, 0.55f, 16, DARKGREEN);
        }
    }

    // Weapon
    rlPushMatrix();
    rlTranslatef(0.65f, 1.65f, 0.4f);
    rlRotatef(e.swingYaw, 0, 1, 0);
    rlRotatef(e.swingPitch, 1, 0, 0);
    float bladeLen = (e.type == BOSS) ? 9.5f : 5.8f;
    DrawCylinderEx({0, -0.3f, 0}, {0, -1.0f, 0}, 0.18f, 0.18f, 12, BROWN);
    DrawCylinderEx({-0.9f, -0.1f, 0}, {0.9f, -0.1f, 0}, 0.22f, 0.22f, 10, GRAY);
    DrawCube({0, 0.0f, 2.9f}, 0.14f, 0.7f, bladeLen, LIGHTGRAY);
    rlPopMatrix();

    // Tank shield
    if (e.type == TANK) {
        rlPushMatrix();
        rlTranslatef(-0.9f, 1.6f, 0.4f);
        rlRotatef(90, 0, 1, 0);
        float blockAngle = e.isBlocking ? 30.0f : -30.0f;
        rlRotatef(blockAngle, 1, 0, 0);
        float height = 3.8f;
        float width = 2.0f;
        float thick = 0.4f;
        DrawCube({0, 0, 0}, width, height, thick, Fade(body, 0.8f));
        DrawCube({0, 0, thick/2 + 0.08f}, width + 0.3f, height + 0.3f, 0.15f, DARKGRAY);
        DrawCylinder({0, 0, thick/2 + 0.1f}, 0.55f, 0.25f, 2, 20, GRAY);
        DrawCube({0, 0.9f, thick/2 + 0.15f}, 0.25f, 1.8f, 0.1f, GOLD);
        DrawCube({0, 0, thick/2 + 0.15f}, 1.4f, 0.25f, 0.1f, GOLD);
        rlPopMatrix();
    }

    // Lock-on indicator
    if (index == player.lockedTarget) {
        float pulse = 0.6f + 0.4f * sinf(GetTime() * 10.0f);
        Color lockCol = Fade(GOLD, pulse);
        DrawCircle3D({0, 1.5f, 0}, 3.5f, {1,0,0}, 90, lockCol);
        DrawCircle3D({0, 4.0f, 0}, 2.5f, {1,0,0}, 90, lockCol);
    }

    rlPopMatrix();
}

// ======================================================================
// HUD & UI
// ======================================================================
void DrawHUD() {
    // Health
    float hpRatio = (float)player.health / player.maxHealth;
    Color hpColor = (hpRatio > 0.5f) ? GREEN : (hpRatio > 0.25f) ? YELLOW : RED;
    DrawRectangle(40, 40, 480, 44, Fade(BLACK, 0.7f));
    DrawRectangle(44, 44, 472 * hpRatio, 36, hpColor);

    // Stamina
    float stamRatio = player.stamina / MAX_STAMINA;
    DrawRectangle(40, 94, 480, 44, Fade(BLACK, 0.7f));
    DrawRectangle(44, 98, 472 * stamRatio, 36, LIME);

    // Poise
    float poiseRatio = player.poise / player.maxPoise;
    DrawRectangle(40, 148, 480, 28, Fade(BLACK, 0.7f));
    DrawRectangle(44, 152, 472 * poiseRatio, 20, PURPLE);

    // Flasks
    DrawText(TextFormat("Flasks: %d", player.flasks), 40, 190, 36, ORANGE);

    // Lock indicator
    if (player.lockedTarget != -1) {
        DrawText("LOCKED", SCREEN_WIDTH - 220, 30, 44, GOLD);
    }

    // Heavy charge
    if (player.isCharging || player.powerReady) {
        float charge = player.chargeTimer / POWER_ATTACK_CHARGE;
        DrawRectangle(40, SCREEN_HEIGHT - 120, 480, 40, Fade(BLACK, 0.7f));
        DrawRectangle(44, SCREEN_HEIGHT - 116, 472 * charge, 32, player.powerReady ? RED : ORANGE);
        DrawText("HEAVY CHARGED", 540, SCREEN_HEIGHT - 110, 36, player.powerReady ? RED : ORANGE);
    }

    // Feedback text
    if (player.riposteTimer > 0.0f) DrawText("RIPOSTE!", SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/2 - 120, 64, GOLD);
    if (player.perfectRollTimer > 0.0f) DrawText("PERFECT ROLL!", SCREEN_WIDTH/2 - 220, SCREEN_HEIGHT/2 - 80, 64, LIME);

    // Boss health bar
    if (player.lockedTarget != -1 && enemies[player.lockedTarget].type == BOSS && enemies[player.lockedTarget].alive) {
        Enemy& boss = enemies[player.lockedTarget];
        float bossRatio = (float)boss.health / boss.maxHealth;
        DrawRectangle(SCREEN_WIDTH/2 - 310, 50, 620, 40, Fade(BLACK, 0.8f));
        DrawRectangle(SCREEN_WIDTH/2 - 300, 60, 600 * bossRatio, 20, RED);
        DrawText("THE SCROLLKEEPER", SCREEN_WIDTH/2 - MeasureText("THE SCROLLKEEPER", 50)/2, 20, 50, GOLD);
    }
}

void DrawTitleScreen() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, Fade(BLACK, 0.8f));

    DrawText("ECHOES OF THE FEED", SCREEN_WIDTH/2 - MeasureText("ECHOES OF THE FEED", 100)/2,
             SCREEN_HEIGHT/2 - 220, 100, GOLD);
    DrawText("Ashes of the Scroll", SCREEN_WIDTH/2 - MeasureText("Ashes of the Scroll", 50)/2,
             SCREEN_HEIGHT/2 - 120, 50, YELLOW);

    // Controls
    int y = SCREEN_HEIGHT/2 + 20;
    DrawText("WASD - Move          Shift (Tap) - Roll / (Hold) - Sprint", 180, y, 32, LIGHTGRAY); y += 40;
    DrawText("Mouse Left - Light Attack / Hold - Charge Heavy", 180, y, 32, LIGHTGRAY); y += 40;
    DrawText("Space - Jump          E - Estus Flask          Ctrl - Parry", 180, y, 32, LIGHTGRAY); y += 40;
    DrawText("F - Lock-on Target (flick mouse to switch)", 180, y, 32, LIGHTGRAY); y += 60;

    DrawText("Click or ENTER to Begin", SCREEN_WIDTH/2 - MeasureText("Click or ENTER to Begin", 50)/2,
             SCREEN_HEIGHT - 140, 50, WHITE);
}

void DrawRenewalScreen() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, Fade(BLACK, 0.9f));
    DrawText("SOUL ASCENDED", SCREEN_WIDTH/2 - MeasureText("SOUL ASCENDED", 140)/2,
             SCREEN_HEIGHT/2 - 140, 140, RED);
    DrawText(currentRenewalMessage, SCREEN_WIDTH/2 - MeasureText(currentRenewalMessage, 60)/2,
             SCREEN_HEIGHT/2 + 20, 60, ORANGE);
    DrawText("Press R to Try Again", SCREEN_WIDTH/2 - MeasureText("Press R to Try Again", 50)/2,
             SCREEN_HEIGHT/2 + 140, 50, WHITE);
}

void DrawVictoryScreen() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT, Fade(BLACK, 0.8f));
    if (currentLevel == 2) {
        DrawText("THE SCROLLKEEPER HARMONIZED!", SCREEN_WIDTH/2 - MeasureText("THE SCROLLKEEPER HARMONIZED!", 80)/2,
                 SCREEN_HEIGHT/2 - 140, 80, GOLD);
        DrawText("THE SCROLL IS ASHES", SCREEN_WIDTH/2 - MeasureText("THE SCROLL IS ASHES", 60)/2,
                 SCREEN_HEIGHT/2 - 20, 60, LIME);
        DrawText("Touch Grass – You Earned It", SCREEN_WIDTH/2 - MeasureText("Touch Grass – You Earned It", 50)/2,
                 SCREEN_HEIGHT/2 + 80, 50, WHITE);
    } else {
        DrawText("LEVEL 1 CLEARED", SCREEN_WIDTH/2 - MeasureText("LEVEL 1 CLEARED", 80)/2,
                 SCREEN_HEIGHT/2 - 100, 80, GOLD);
        DrawText("Entering the Boss Arena...", SCREEN_WIDTH/2 - MeasureText("Entering the Boss Arena...", 50)/2,
                 SCREEN_HEIGHT/2 + 20, 50, YELLOW);
    }
    DrawText("ESC to Quit", SCREEN_WIDTH/2 - MeasureText("ESC to Quit", 50)/2,
             SCREEN_HEIGHT/2 + 180, 50, WHITE);
}

// ======================================================================
// Particles
// ======================================================================
void SpawnBloodParticles(Vector3 pos, int count) {
    for (int i = 0; i < count; i++) {
        Particle p{};
        p.position = pos;
        p.velocity = {GetRandomValue(-100,100)/20.0f,
                      GetRandomValue(40,140)/20.0f,
                      GetRandomValue(-100,100)/20.0f};
        p.lifetime = p.maxLife = GetRandomValue(40,90)/100.0f;
        p.color = Fade(RED, 0.9f);
        p.size = GetRandomValue(4,12)/10.0f;
        particles.push_back(p);
    }
}

void SpawnHitSparks(Vector3 pos, int count) {
    for (int i = 0; i < count; i++) {
        Particle p{};
        p.position = pos;
        p.velocity = {GetRandomValue(-120,120)/15.0f,
                      GetRandomValue(60,180)/15.0f,
                      GetRandomValue(-120,120)/15.0f};
        p.lifetime = p.maxLife = GetRandomValue(30,70)/100.0f;
        p.color = Fade(YELLOW, 0.95f);
        p.size = GetRandomValue(3,9)/10.0f;
        particles.push_back(p);
    }
}

void UpdateParticles(float dt) {
    for (auto it = particles.begin(); it != particles.end(); ) {
        it->lifetime -= dt;
        if (it->lifetime <= 0) {
            it = particles.erase(it);
            continue;
        }
        it->position = Vector3Add(it->position, Vector3Scale(it->velocity, dt));
        it->velocity.y -= 3.5f * dt;
        ++it;
    }
}

// ======================================================================
// Utility
// ======================================================================
bool CanSeePlayer(const Enemy& e) {
    Vector3 eye = Vector3Add(e.position, {0,2.4f,0});
    Vector3 target = Vector3Add(player.position, {0,1.6f,0});
    Vector3 dir = Vector3Subtract(target, eye);
    float dist = Vector3Length(dir);
    if (dist > 40.0f) return false;

    Vector3 forward = {sinf(e.rotation*DEG2RAD), 0, cosf(e.rotation*DEG2RAD)};
    float dot = Vector3DotProduct(Vector3Normalize(dir), forward);
    if (dot < cosf(65.0f * DEG2RAD)) return false;

    Ray ray{eye, Vector3Normalize(dir)};
    for (const auto& obs : obstacles) {
        BoundingBox box = {Vector3Subtract(obs, {5,7,5}), Vector3Add(obs, {5,7,5})};
        RayCollision col = GetRayCollisionBox(ray, box);
        if (col.hit && col.distance < dist - 0.8f) return false;
    }
    return true;
}