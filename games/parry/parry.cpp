// ======================================================================
// Parry the Storm – Ashes of the Bullet (Dark Souls Edition)
// Episode 11: The Ultimate Trial – Bullet Hell with True Soulslike Punishment
// Collect souls, level up at bonfire, lose everything on death. Git Gud.
// ======================================================================
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <random>
#include <set>

const int SCREEN_WIDTH = 1440;
const int SCREEN_HEIGHT = 810;

// Tuned for punishing difficulty
const float PLAYER_BASE_SPEED = 8.2f;
const float SPRINT_MULTIPLIER = 1.65f;
const float ROLL_SPEED = 22.0f;
const float ROLL_DURATION = 0.30f;
const float ROLL_RECOVERY = 0.35f;
const float ROLL_COST = 32.0f;
const float SHOOT_RATE_BASE = 0.14f;
const float PLAYER_BULLET_SPEED_BASE = 35.0f;
const float ENEMY_BULLET_SPEED = 20.0f;
const float PARRY_WINDOW_BASE = 0.22f;
const float PARRY_RANGE = 7.0f;
const float PARRY_COST = 35.0f;
const int BASE_MAX_HEALTH = 80;
const int BASE_MAX_STAMINA = 140;
const float STAMINA_REGEN_BASE = 28.0f;
const int MAX_FLASKS = 5;
const float FLASK_HEAL_BASE = 35.0f;
const float FLASK_TIME = 1.3f;
const float CAMERA_HEIGHT = 38.0f;
const float CAMERA_DISTANCE = 28.0f;
const float CAMERA_SMOOTH = 12.0f;

const float BULLET_LIFETIME = 5.5f;
const float BULLET_SIZE = 0.65f;
const float PERFECT_PARRY_BONUS = 2.8f;

const int UPGRADE_COST_BASE = 300;
const int UPGRADE_COST_MULTIPLIER = 180;

// ======================================================================
// Enums & Structs
// ======================================================================
enum GameState { TITLE, PLAYING, BONFIRE, PAUSED, DEAD, VICTORY };
enum EnemyType { GRUNT, SPIRAL, WALL, RAPID, SHIELDED, BOSS };

struct Bullet {
    Vector3 pos;
    Vector3 vel;
    Color color;
    float life;
    bool playerBullet = false;
    bool reflected = false;
};

struct Particle {
    Vector3 pos;
    Vector3 vel;
    float life;
    float maxLife;
    Color color;
    float size;
};

struct SoulOrb {
    Vector3 pos;
    float timer;
};

struct Player {
    Vector3 pos {0,0,0};
    float rotation = 0.0f;
    int health = BASE_MAX_HEALTH;
    int maxHealth = BASE_MAX_HEALTH;
    float stamina = BASE_MAX_STAMINA;
    int maxStamina = BASE_MAX_STAMINA;
    int flasks = 0;
    float shootCD = 0.0f;
    float shootRate = SHOOT_RATE_BASE;
    float bulletSpeed = PLAYER_BULLET_SPEED_BASE;
    bool isRolling = false;
    float rollTimer = 0.0f;
    float recoveryTimer = 0.0f;
    Vector3 rollDir {0,0,0};
    bool isParrying = false;
    float parryTimer = 0.0f;
    float parryWindow = PARRY_WINDOW_BASE;
    float hitInvuln = 0.0f;
    float healTimer = 0.0f;
    bool isHealing = false;
    int score = 0;
    int combo = 0;
    int souls = 0;
    int vitality = 0;
    int endurance = 0;
    int strength = 0;
    int dexterity = 0;
    float shake = 0.0f;
};

struct Enemy {
    EnemyType type;
    Vector3 pos;
    float rotation;
    int health;
    int maxHealth;
    float shootTimer;
    float patternAngle = 0.0f;
    float speed = 3.2f;
    float scale = 1.0f;
    bool alive = true;
    Color color;
    int soulValue = 100;
};

GameState state = TITLE;
int wave = 1;
Player player;
std::vector<Enemy> enemies;
std::vector<Bullet> bullets;
std::vector<Particle> particles;
std::vector<SoulOrb> soulOrbs;
Camera3D camera = {0};
float hitStop = 0.0f;
int totalEnemyBullets = 0;
int neutralized = 0;
float accuracy = 0.0f;

Vector3 bonfirePos = {0, 0, 0};

std::vector<std::string> deathQuotes = {
    "Bullet Issue", "Git Gud @ Dodging", "Parry Failed", "Souls Lost Forever",
    "Accuracy = 0%", "Try Shooting Them", "Flask Harder", "Roll Punished",
    "Combo Lost", "Bonfire Denied", "Humanity Drained", "You Died... Again"
};

// ======================================================================
// Functions
// ======================================================================
void InitGame();
void ResetWave(bool fullReset = false);
void UpdateGame(float dt);
void UpdatePlayer(float dt);
void UpdateEnemies(float dt);
void UpdateBullets(float dt);
void UpdateParticles(float dt);
void CollectSouls(float dt);
void UpdateCamera();
void SpawnBullet(Vector3 pos, Vector3 vel, Color col, bool playerOwned, bool reflected = false);
void SpawnParticles(Vector3 pos, Color col, int count, float speed);
void DropSouls(Vector3 pos, int amount);
void RestAtBonfire();
int GetUpgradeCost(int level);
void Draw3D();
void DrawPlayer();
void DrawEnemy(const Enemy& e);
void DrawCrosshairAndAimMarker();
void DrawHUD();
void DrawBonfireMenu();
void DrawTitle();
void DrawDeath();
void DrawVictory();
Vector3 GetAimPoint();

// ======================================================================
// Main
// ======================================================================
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Parry the Storm – Ashes of the Bullet (Dark Souls Edition)");

    SetExitKey(KEY_NULL);

    SetTargetFPS(60);
    HideCursor();
    InitAudioDevice();
    InitGame();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (hitStop > 0.0f) {
            hitStop -= dt;
            dt = 0.0f;
        }

        if (state == TITLE) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_ENTER)) {
                wave = 1;
                state = PLAYING;
                ResetWave();
            }
        } else if (state == PLAYING || state == PAUSED || state == BONFIRE) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                state = (state == PLAYING || state == BONFIRE) ? PAUSED : PLAYING;
            }
            if (state == PLAYING) {
                UpdateGame(dt);
            } else if (state == BONFIRE) {
                if (IsKeyPressed(KEY_ONE) && player.souls >= GetUpgradeCost(player.vitality)) {
                    player.souls -= GetUpgradeCost(player.vitality++);
                    player.maxHealth += 12;
                    player.health = player.maxHealth;
                }
                if (IsKeyPressed(KEY_TWO) && player.souls >= GetUpgradeCost(player.endurance)) {
                    player.souls -= GetUpgradeCost(player.endurance++);
                    player.maxStamina += 15;
                    player.stamina = player.maxStamina;
                }
                if (IsKeyPressed(KEY_THREE) && player.souls >= GetUpgradeCost(player.strength)) {
                    player.souls -= GetUpgradeCost(player.strength++);
                    player.bulletSpeed += 5.0f;
                }
                if (IsKeyPressed(KEY_FOUR) && player.souls >= GetUpgradeCost(player.dexterity)) {
                    player.souls -= GetUpgradeCost(player.dexterity++);
                    player.shootRate *= 0.92f;
                    player.parryWindow += 0.02f;
                }
                if (IsKeyPressed(KEY_SPACE)) {
                    ResetWave();
                    state = PLAYING;
                }
            }
        } else if (state == DEAD) {
            if (IsKeyPressed(KEY_R)) {
                wave = 1;
                ResetWave(true);
                state = PLAYING;
            }
        }

        BeginDrawing();
        ClearBackground({8, 8, 18, 255});

        BeginMode3D(camera);
        Draw3D();
        EndMode3D();

        DrawCrosshairAndAimMarker();
        DrawHUD();
        if (state == TITLE) DrawTitle();
        if (state == DEAD) DrawDeath();
        if (state == VICTORY) DrawVictory();
        if (state == BONFIRE) DrawBonfireMenu();
        if (state == PAUSED) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
            DrawText("PAUSED - GIT GUD", SCREEN_WIDTH/2 - MeasureText("PAUSED - GIT GUD", 80)/2, SCREEN_HEIGHT/2 - 40, 80, GOLD);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

void InitGame() {
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camera.up = {0,1,0};
    ResetWave(true);
}

void ResetWave(bool fullReset) {
    if (fullReset) {
        player = {};
        player.maxHealth = BASE_MAX_HEALTH;
        player.health = player.maxHealth;
        player.maxStamina = BASE_MAX_STAMINA;
        player.stamina = player.maxStamina;
        player.shootRate = SHOOT_RATE_BASE;
        player.bulletSpeed = PLAYER_BULLET_SPEED_BASE;
        player.parryWindow = PARRY_WINDOW_BASE;
        player.souls = 0;
        player.vitality = player.endurance = player.strength = player.dexterity = 0;
    } else {
        player.health = player.maxHealth;
        player.stamina = player.maxStamina;
    }
    player.flasks = 0;
    player.pos = {0, 0, 25.0f};

    enemies.clear();
    bullets.clear();
    particles.clear();
    soulOrbs.clear();
    totalEnemyBullets = 0;
    neutralized = 0;
    player.score = 0;
    player.combo = 0;

    auto spawnEnemy = [&](EnemyType t, int count, int hp, int souls) {
        for (int i = 0; i < count; i++) {
            Enemy e{};
            e.type = t;
            e.health = e.maxHealth = hp;
            e.soulValue = souls;
            e.shootTimer = (float)i * 0.25f;
            float angle = (float)i / count * 2 * PI + GetRandomValue(-30,30) * DEG2RAD;
            float radius = 55.0f;
            e.pos = {cosf(angle) * radius, 0, sinf(angle) * radius};
            e.color = (t == BOSS) ? MAROON : (t == SHIELDED) ? DARKGRAY : (t == RAPID) ? ORANGE : (t == SPIRAL) ? PURPLE : RED;
            e.scale = (t == BOSS) ? 3.5f : (t == SHIELDED) ? 1.4f : 1.0f;
            enemies.push_back(e);
        }
    };

    if (wave == 1) {
        spawnEnemy(GRUNT, 10, 70, 80);
    } else if (wave == 2) {
        spawnEnemy(GRUNT, 4, 90, 120);
        spawnEnemy(SPIRAL, 3, 60, 140);
        spawnEnemy(RAPID, 4, 55, 110);
    } else if (wave == 3) {
        spawnEnemy(WALL, 4, 100, 180);
        spawnEnemy(SHIELDED, 4, 140, 250);
        spawnEnemy(BOSS, 1, 3200, 5000);
    }
}

void RestAtBonfire() {
    player.flasks = MAX_FLASKS;
    player.health = player.maxHealth;
    player.stamina = player.maxStamina;
    player.pos = bonfirePos;
}

int GetUpgradeCost(int level) {
    return UPGRADE_COST_BASE + level * UPGRADE_COST_MULTIPLIER;
}

void DropSouls(Vector3 pos, int amount) {
    int orbs = amount / 80;
    for (int i = 0; i < orbs; i++) {
        SoulOrb s;
        s.pos = Vector3Add(pos, {GetRandomValue(-60,60)/10.0f, 3.0f, GetRandomValue(-60,60)/10.0f});
        s.timer = 10.0f;
        soulOrbs.push_back(s);
    }
    player.souls += amount % 80;
}

void CollectSouls(float dt) {
    for (auto it = soulOrbs.begin(); it != soulOrbs.end(); ) {
        Vector3 toPlayer = Vector3Subtract(player.pos, it->pos);
        float dist = Vector3Length(toPlayer);
        if (dist < 6.0f || it->timer <= 0.0f) {
            player.souls += 80;
            it = soulOrbs.erase(it);
        } else {
            it->pos = Vector3Add(it->pos, Vector3Scale(Vector3Normalize(toPlayer), 20.0f * dt));
            it->timer -= dt;
            ++it;
        }
    }
}

Vector3 GetAimPoint() {
    Ray ray = GetMouseRay(GetMousePosition(), camera);
    if (ray.direction.y != 0.0f) {
        float t = -ray.position.y / ray.direction.y;
        if (t > 0) return Vector3Add(ray.position, Vector3Scale(ray.direction, t));
    }
    return player.pos;
}

void UpdatePlayer(float dt) {
    player.hitInvuln = std::max(0.0f, player.hitInvuln - dt);
    player.shake = std::max(0.0f, player.shake - dt);
    player.shootCD = std::max(0.0f, player.shootCD - dt);

    if (player.isHealing) {
        player.healTimer -= dt;
        if (player.healTimer <= 0.0f) player.isHealing = false;
    }

    player.stamina = std::min(player.stamina + STAMINA_REGEN_BASE * dt, (float)player.maxStamina);

    Vector3 aimPoint = GetAimPoint();
    Vector3 toAim = Vector3Subtract(aimPoint, player.pos);
    toAim.y = 0;
    if (Vector3Length(toAim) > 0.1f) {
        player.rotation = atan2f(toAim.x, toAim.z);
    }

    Vector3 input {0,0,0};
    if (IsKeyDown(KEY_W)) input.z += 1;
    if (IsKeyDown(KEY_S)) input.z -= 1;
    if (IsKeyDown(KEY_D)) input.x += 1;
    if (IsKeyDown(KEY_A)) input.x -= 1;
    bool moving = Vector3Length(input) > 0.1f;

    Vector3 camDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    camDir.y = 0;
    camDir = Vector3Normalize(camDir);
    Vector3 camRight = Vector3CrossProduct(camDir, {0,1,0});

    Vector3 moveDir = Vector3Add(Vector3Scale(camDir, input.z), Vector3Scale(camRight, input.x));
    if (moving) moveDir = Vector3Normalize(moveDir);

    float speed = PLAYER_BASE_SPEED;
    if (IsKeyDown(KEY_LEFT_SHIFT) && moving && player.stamina > 10.0f) speed *= SPRINT_MULTIPLIER;

    if (player.recoveryTimer > 0.0f) {
        player.recoveryTimer -= dt;
        speed *= 0.4f;
    }

    static float shiftTimer = 0.0f;
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        shiftTimer += dt;
    } else {
        if (shiftTimer > 0.0f && shiftTimer < 0.22f && moving && player.stamina >= ROLL_COST && !player.isRolling && player.recoveryTimer <= 0.0f) {
            player.isRolling = true;
            player.rollTimer = ROLL_DURATION;
            player.rollDir = moveDir;
            player.stamina -= ROLL_COST;
            player.hitInvuln = ROLL_DURATION + 0.15f;
        }
        shiftTimer = 0.0f;
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && player.shootCD <= 0.0f) {
        Vector3 shootDir = Vector3Normalize(toAim);
        Vector3 muzzle = Vector3Add(player.pos, Vector3Scale(shootDir, 2.0f));
        muzzle.y = 1.5f;
        SpawnBullet(muzzle, Vector3Scale(shootDir, player.bulletSpeed), SKYBLUE, true);
        player.shootCD = player.shootRate;
        SpawnParticles(muzzle, YELLOW, 6, 8.0f);
    }

    if (IsKeyPressed(KEY_SPACE) && player.stamina >= PARRY_COST && !player.isParrying) {
        player.isParrying = true;
        player.parryTimer = player.parryWindow;
        player.stamina -= PARRY_COST;
    }
    if (player.isParrying) {
        player.parryTimer -= dt;
        if (player.parryTimer <= 0.0f) player.isParrying = false;
    }

    if (IsKeyPressed(KEY_E) && player.flasks > 0 && !player.isHealing) {
        player.isHealing = true;
        player.healTimer = FLASK_TIME;
        player.flasks--;
    }
    if (player.isHealing && player.healTimer <= 0.5f) {
        player.health = std::min(player.health + (int)FLASK_HEAL_BASE, player.maxHealth);
    }

    if (player.isRolling) {
        player.rollTimer -= dt;
        player.pos = Vector3Add(player.pos, Vector3Scale(player.rollDir, ROLL_SPEED * dt));
        if (player.rollTimer <= 0.0f) {
            player.isRolling = false;
            player.recoveryTimer = ROLL_RECOVERY;
        }
    } else {
        player.pos = Vector3Add(player.pos, Vector3Scale(moveDir, speed * dt));
    }

    float limit = 80.0f;
    player.pos.x = Clamp(player.pos.x, -limit, limit);
    player.pos.z = Clamp(player.pos.z, -limit, limit);
}

void UpdateEnemies(float dt) {
    for (auto& e : enemies) {
        if (!e.alive) continue;

        Vector3 toPlayer = Vector3Subtract(player.pos, e.pos);
        toPlayer.y = 0;
        float dist = Vector3Length(toPlayer);
        if (dist > 1.0f) {
            e.rotation = atan2f(toPlayer.x, toPlayer.z);
        }

        if (e.type != BOSS) {
            Vector3 dir = Vector3Normalize(toPlayer);
            e.pos = Vector3Add(e.pos, Vector3Scale(dir, e.speed * dt));
        }

        e.shootTimer -= dt;
        if (e.shootTimer <= 0.0f && dist < 70.0f) {
            Vector3 dir = Vector3Normalize(toPlayer);
            if (Vector3Length(dir) < 0.1f) dir = {0,0,1};

            switch (e.type) {
                case GRUNT:
                    SpawnBullet(Vector3Add(e.pos, {0,2,0}), Vector3Scale(dir, ENEMY_BULLET_SPEED), RED, false);
                    totalEnemyBullets++;
                    e.shootTimer = 1.8f;
                    break;
                case SPIRAL:
                    for (int i = 0; i < 8; i++) {
                        float ang = e.patternAngle + i * PI/4;
                        Vector3 spd = {sinf(ang), 0, cosf(ang)};
                        SpawnBullet(Vector3Add(e.pos, {0,2,0}), Vector3Scale(spd, ENEMY_BULLET_SPEED), PURPLE, false);
                        totalEnemyBullets++;
                    }
                    e.patternAngle += 0.4f;
                    e.shootTimer = 0.9f;
                    break;
                case RAPID:
                    SpawnBullet(Vector3Add(e.pos, {0,2,0}), Vector3Scale(dir, ENEMY_BULLET_SPEED * 1.3f), ORANGE, false);
                    totalEnemyBullets++;
                    e.shootTimer = 0.25f;
                    break;
                case WALL:
                    for (int i = -4; i <= 4; i++) {
                        Vector3 side = Vector3CrossProduct(dir, {0,1,0});
                        Vector3 offset = Vector3Scale(side, i * 3.0f);
                        SpawnBullet(Vector3Add(e.pos, offset), Vector3Scale(dir, ENEMY_BULLET_SPEED), MAROON, false);
                        totalEnemyBullets++;
                    }
                    e.shootTimer = 2.2f;
                    break;
                case SHIELDED:
                    SpawnBullet(Vector3Add(e.pos, {0,2,0}), Vector3Scale(dir, ENEMY_BULLET_SPEED * 0.9f), DARKGRAY, false);
                    totalEnemyBullets++;
                    e.shootTimer = 2.0f;
                    break;
                case BOSS: {
                    int phase = (e.health > 1600) ? 1 : (e.health > 800) ? 2 : 3;
                    if (phase == 1) {
                        for (int i = 0; i < 12; i++) {
                            float ang = e.patternAngle + i * PI/6;
                            Vector3 spd = {sinf(ang), 0, cosf(ang)};
                            SpawnBullet(Vector3Add(e.pos, {0,4,0}), Vector3Scale(spd, ENEMY_BULLET_SPEED), RED, false);
                            totalEnemyBullets++;
                        }
                        e.patternAngle += 0.3f;
                        e.shootTimer = 0.6f;
                    } else if (phase == 2) {
                        for (int i = 0; i < 5; i++) {
                            SpawnBullet(Vector3Add(e.pos, {0,4,0}), Vector3Scale(dir, ENEMY_BULLET_SPEED * (1 + i*0.2f)), MAROON, false);
                            totalEnemyBullets++;
                        }
                        e.shootTimer = 1.4f;
                    } else {
                        for (int i = 0; i < 20; i++) {
                            float ang = (float)i / 20 * 2 * PI;
                            Vector3 spd = {sinf(ang), 0, cosf(ang)};
                            SpawnBullet(Vector3Add(e.pos, {0,4,0}), Vector3Scale(spd, ENEMY_BULLET_SPEED * 1.2f), VIOLET, false);
                            totalEnemyBullets++;
                        }
                        e.shootTimer = 0.8f;
                    }
                    break;
                }
            }
        }
    }
}

void SpawnBullet(Vector3 pos, Vector3 vel, Color col, bool playerOwned, bool reflected) {
    Bullet b;
    b.pos = pos;
    b.pos.y = 2.0f;
    b.vel = vel;
    b.color = col;
    b.life = BULLET_LIFETIME;
    b.playerBullet = playerOwned;
    b.reflected = reflected;
    bullets.push_back(b);
}

void SpawnParticles(Vector3 pos, Color col, int count, float speed) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.pos = pos;
        Vector3 dir = {GetRandomValue(-100,100)/100.0f, GetRandomValue(30,100)/100.0f, GetRandomValue(-100,100)/100.0f};
        p.vel = Vector3Scale(Vector3Normalize(dir), speed);
        p.life = p.maxLife = GetRandomValue(30,80)/100.0f;
        p.color = col;
        p.size = GetRandomValue(4,12)/10.0f;
        particles.push_back(p);
    }
}

void UpdateParticles(float dt) {
    for (auto it = particles.begin(); it != particles.end(); ) {
        it->pos = Vector3Add(it->pos, Vector3Scale(it->vel, dt));
        it->vel.y -= 20.0f * dt;
        it->life -= dt;
        if (it->life <= 0.0f) it = particles.erase(it);
        else ++it;
    }
}

void UpdateCamera() {
    Vector3 desiredPos = Vector3Add(player.pos, {0, CAMERA_HEIGHT, CAMERA_DISTANCE});
    camera.position = Vector3Lerp(camera.position, desiredPos, CAMERA_SMOOTH * GetFrameTime());
    camera.target = Vector3Add(player.pos, {0, 3.0f, 0});

    if (player.shake > 0.0f) {
        Vector3 shakeOffset = {GetRandomValue(-100,100)/100.0f * player.shake * 10,
                               GetRandomValue(-100,100)/100.0f * player.shake * 10,
                               GetRandomValue(-100,100)/100.0f * player.shake * 10};
        camera.position = Vector3Add(camera.position, shakeOffset);
    }
}

void UpdateBullets(float dt) {
    for (auto& b : bullets) {
        b.pos = Vector3Add(b.pos, Vector3Scale(b.vel, dt));
        b.life -= dt;
    }

    std::set<size_t> toRemove;

    for (size_t i = 0; i < bullets.size(); ++i) {
        const Bullet& b = bullets[i];

        if (b.life <= 0.0f || Vector3Length(b.pos) > 120.0f) {
            toRemove.insert(i);
            continue;
        }

        if (!b.playerBullet && Vector3Distance(b.pos, player.pos) < 3.0f && player.hitInvuln <= 0.0f) {
            player.health -= 12;
            player.hitInvuln = 0.6f;
            player.combo = 0;
            player.shake = 0.4f;
            hitStop = 0.06f;
            SpawnParticles(b.pos, RED, 25, 14.0f);
            toRemove.insert(i);
        }
    }

    for (size_t i = 0; i < bullets.size(); ++i) {
        Bullet& b = bullets[i];
        if (!b.playerBullet && player.isParrying && Vector3Distance(b.pos, player.pos) < PARRY_RANGE) {
            b.vel = Vector3Scale(Vector3Normalize(Vector3Negate(b.vel)), Vector3Length(b.vel) * PERFECT_PARRY_BONUS);
            b.playerBullet = true;
            b.reflected = true;
            b.color = GOLD;
            neutralized++;
            player.combo++;
            player.score += 30 * player.combo;
            SpawnParticles(b.pos, YELLOW, 35, 18.0f);
            hitStop = 0.09f;
            player.shake = 0.5f;
        }
    }

    for (size_t i = 0; i < bullets.size(); ++i) {
        if (!bullets[i].playerBullet) continue;
        const Vector3& ppos = bullets[i].pos;

        for (size_t j = 0; j < bullets.size(); ++j) {
            if (bullets[j].playerBullet) continue;
            if (Vector3Distance(ppos, bullets[j].pos) < BULLET_SIZE * 2) {
                neutralized++;
                player.combo++;
                player.score += 15 * player.combo;
                SpawnParticles(ppos, WHITE, 15, 12.0f);
                toRemove.insert(i);
                toRemove.insert(j);
            }
        }

        for (auto& e : enemies) {
            if (!e.alive) continue;
            Vector3 fromBulletToEnemy = Vector3Subtract(e.pos, ppos);
            float dot = Vector3DotProduct(Vector3Normalize(fromBulletToEnemy), 
                                         Vector3Normalize({sinf(e.rotation*DEG2RAD), 0, cosf(e.rotation*DEG2RAD)}));
            bool blocked = (e.type == SHIELDED && dot > 0.35f);
            if (Vector3Distance(ppos, e.pos) < e.scale * 4.0f) {
                if (blocked) {
                    SpawnParticles(ppos, GRAY, 20, 10.0f);
                } else {
                    int dmg = bullets[i].reflected ? 35 : 18;
                    e.health -= dmg;
                    SpawnParticles(ppos, bullets[i].reflected ? GOLD : SKYBLUE, 15, 10.0f);
                    player.score += bullets[i].reflected ? 80 : 30;
                    if (e.health <= 0) {
                        e.alive = false;
                        player.score += 1000;
                        player.combo += 10;
                        SpawnParticles(e.pos, RED, 60, 16.0f);
                        DropSouls(e.pos, e.soulValue);
                    }
                }
                toRemove.insert(i);
            }
        }
    }

    for (auto rit = toRemove.rbegin(); rit != toRemove.rend(); ++rit) {
        bullets.erase(bullets.begin() + *rit);
    }
}

void UpdateGame(float dt) {
    if (dt == 0.0f) return;

    UpdateCamera();
    UpdatePlayer(dt);
    UpdateEnemies(dt);
    UpdateBullets(dt);
    CollectSouls(dt);
    UpdateParticles(dt);

    bool allDead = true;
    for (const auto& e : enemies) if (e.alive) allDead = false;
    if (allDead) {
        if (wave < 3) {
            wave++;
            state = BONFIRE;
            RestAtBonfire();
        } else {
            state = VICTORY;
        }
    }

    if (player.health <= 0) {
        state = DEAD;
    }
}

void Draw3D() {
    DrawPlane({0,0,0}, {200,200}, {20,25,40,255});

    Vector3 aimPoint = GetAimPoint();
    DrawCircle3D(aimPoint, 3.0f, {1,0,0}, 90.0f, Fade(LIME, 0.5f));
    DrawCircle3D(aimPoint, 1.5f, {1,0,0}, 90.0f, Fade(LIME, 0.8f));

    for (const auto& b : bullets) {
        DrawSphere(b.pos, BULLET_SIZE, b.color);
        if (b.reflected) DrawSphere(b.pos, BULLET_SIZE * 1.6f, Fade(GOLD, 0.4f));
    }

    for (const auto& p : particles) {
        DrawSphere(p.pos, p.size * (p.life / p.maxLife), Fade(p.color, p.life / p.maxLife));
    }

    for (const auto& s : soulOrbs) {
        DrawSphere(s.pos, 1.0f, Fade(GOLD, 0.7f + 0.3f * sinf(GetTime() * 8)));
    }

    // Bonfire
    DrawCylinder(bonfirePos, 2.2f, 1.8f, 9.0f, 16, DARKBROWN);
    for (int i = 0; i < 25; i++) {
        float ang = i / 25.0f * PI * 2;
        float h = 3.0f + sinf(GetTime() * 10 + i) * 2.0f;
        Vector3 flame = {cosf(ang) * 2.2f, h, sinf(ang) * 2.2f};
        DrawSphere(Vector3Add(bonfirePos, flame), 1.0f, Fade(ORANGE, 0.8f));
    }

    DrawPlayer();
    for (const auto& e : enemies) if (e.alive) DrawEnemy(e);
}

void DrawPlayer() {
    rlPushMatrix();
    rlTranslatef(player.pos.x, player.pos.y, player.pos.z);
    rlRotatef(player.rotation * RAD2DEG, 0,1,0);

    Color body = player.isParrying ? GOLD : SKYBLUE;
    if (player.hitInvuln > 0.0f) body = Fade(body, 0.6f + 0.4f * sinf(GetTime() * 30));

    DrawCylinderEx({0,0,0}, {0,3,0}, 1.2f, 0.8f, 16, body);
    DrawSphere({0,3.5f,0}, 0.9f, body);
    DrawCylinderEx({-0.8f,1.5f,0}, {-1.6f,0.5f,0}, 0.4f, 0.3f, 12, DARKGRAY);
    DrawCylinderEx({0.8f,2.0f,0.6f}, {1.4f,0.8f,1.2f}, 0.35f, 0.25f, 12, GRAY);

    if (player.isParrying) {
        DrawSphere({0,1.5f,0}, 5.0f, Fade(GOLD, 0.4f + 0.4f * sinf(GetTime() * 20)));
    }

    rlPopMatrix();
}

void DrawEnemy(const Enemy& e) {
    rlPushMatrix();
    rlTranslatef(e.pos.x, e.pos.y, e.pos.z);
    rlRotatef(e.rotation * RAD2DEG, 0,1,0);
    rlScalef(e.scale, e.scale, e.scale);
    DrawSphere({0,2,0}, 1.8f, e.color);
    DrawCylinderEx({0,2,0}, {0,5,0}, 0.8f, 0.4f, 12, Fade(e.color, 0.7f));

    // Shield visual for SHIELDED
    if (e.type == SHIELDED) {
        rlPushMatrix();
        rlTranslatef(-1.2f, 2.0f, 0);
        rlRotatef(90.0f, 0,1,0);
        DrawCube({0,0,0}, 2.5f, 4.0f, 0.5f, DARKGRAY);
        rlPopMatrix();
    }

    rlPopMatrix();
}

void DrawCrosshairAndAimMarker() {
    Vector2 mousePos = GetMousePosition();

    DrawLineEx({mousePos.x - 12, mousePos.y}, {mousePos.x + 12, mousePos.y}, 2.0f, WHITE);
    DrawLineEx({mousePos.x, mousePos.y - 12}, {mousePos.x, mousePos.y + 12}, 2.0f, WHITE);
    DrawCircleLines((int)mousePos.x, (int)mousePos.y, 18.0f, WHITE);
    DrawCircleLines((int)mousePos.x, (int)mousePos.y, 10.0f, WHITE);
}

void DrawHUD() {
    int y = 30;

    // Health
    DrawRectangle(30, y, 400, 40, Fade(BLACK, 0.7f));
    DrawRectangle(35, y+5, 390 * (float)player.health / player.maxHealth, 30, RED);
    DrawText("HEALTH", 40, y+8, 28, WHITE);
    y += 60;

    // Stamina
    DrawRectangle(30, y, 400, 30, Fade(BLACK, 0.7f));
    DrawRectangle(35, y+5, 390 * player.stamina / player.maxStamina, 20, LIME);
    y += 50;

    // Score & Combo
    DrawText(TextFormat("SCORE: %d", player.score), 30, y, 40, GOLD);
    if (player.combo > 1) DrawText(TextFormat("COMBO x%d", player.combo), 30, y+50, 50, ORANGE);
    y += 100;

    // Accuracy
    if (totalEnemyBullets > 0) {
        accuracy = 100.0f * neutralized / totalEnemyBullets;
        Color accCol = accuracy > 80 ? LIME : accuracy > 50 ? YELLOW : RED;
        DrawText(TextFormat("ACCURACY: %.1f%%", accuracy), 30, y, 40, accCol);
    }
    y += 60;

    // Souls & Stats
    DrawText(TextFormat("Souls: %d", player.souls), SCREEN_WIDTH - 320, 30, 50, YELLOW);
    DrawText(TextFormat("VIT %d | END %d | STR %d | DEX %d", 
                        player.vitality, player.endurance, player.strength, player.dexterity),
             SCREEN_WIDTH - 520, 90, 40, DARKGRAY);

    // Wave & Flasks
    DrawText(TextFormat("WAVE %d", wave), SCREEN_WIDTH - 300, 150, 50, GOLD);
    DrawText(TextFormat("FLASKS: %d", player.flasks), SCREEN_WIDTH - 300, 210, 40, ORANGE);

    // Boss health
    for (const auto& e : enemies) {
        if (e.alive && e.type == BOSS) {
            float ratio = (float)e.health / e.maxHealth;
            DrawRectangle(SCREEN_WIDTH/2 - 400, 40, 800, 50, Fade(BLACK, 0.8f));
            DrawRectangle(SCREEN_WIDTH/2 - 390, 50, 780 * ratio, 30, RED);
            DrawText("BULLET LORD", SCREEN_WIDTH/2 - MeasureText("BULLET LORD", 60)/2, 20, 60, GOLD);
        }
    }
}

void DrawBonfireMenu() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,Fade(BLACK,0.85f));
    DrawText("SITE OF GRACE - LEVEL UP", SCREEN_WIDTH/2 - MeasureText("SITE OF GRACE - LEVEL UP", 70)/2, 120, 70, GOLD);
    DrawText(TextFormat("Souls: %d", player.souls), SCREEN_WIDTH/2 - 120, 220, 60, YELLOW);

    int y = 320;
    const char* stats[] = {"Vitality (+12 HP)", "Endurance (+15 Stamina)", "Strength (+5 Bullet Speed)", "Dexterity (Faster Fire/Parry)"};
    int levels[] = {player.vitality, player.endurance, player.strength, player.dexterity};
    for (int i = 0; i < 4; i++) {
        int cost = GetUpgradeCost(levels[i]);
        Color col = player.souls >= cost ? LIME : RED;
        DrawText(TextFormat("%d - %s (Lv %d) - Cost %d", i+1, stats[i], levels[i], cost), 300, y, 45, col);
        y += 70;
    }

    DrawText("SPACE to Continue Into the Storm", SCREEN_WIDTH/2 - MeasureText("SPACE to Continue Into the Storm", 40)/2, SCREEN_HEIGHT - 140, 40, LIGHTGRAY);
}

void DrawTitle() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,Fade(BLACK,0.85f));
    DrawText("PARRY THE STORM", SCREEN_WIDTH/2 - MeasureText("PARRY THE STORM", 100)/2, 150, 100, GOLD);
    DrawText("Ashes of the Bullet - Dark Souls Edition", SCREEN_WIDTH/2 - MeasureText("Ashes of the Bullet - Dark Souls Edition", 50)/2, 270, 50, YELLOW);
    DrawText("WASD Move • Mouse Aim/Shoot • SPACE Parry • SHIFT Roll • E Flask", 200, 420, 36, LIGHTGRAY);
    DrawText("Die and lose everything. Git Gud eternally.", 200, 480, 36, ORANGE);
    DrawText("Click or ENTER to begin the trial", SCREEN_WIDTH/2 - MeasureText("Click or ENTER to begin the trial", 40)/2, SCREEN_HEIGHT - 120, 40, WHITE);
}

void DrawDeath() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,Fade(BLACK,0.9f));
    DrawText("YOU DIED", SCREEN_WIDTH/2 - MeasureText("YOU DIED", 140)/2, SCREEN_HEIGHT/2 - 100, 140, RED);
    const char* quote = deathQuotes[GetRandomValue(0, deathQuotes.size()-1)].c_str();
    DrawText(quote, SCREEN_WIDTH/2 - MeasureText(quote, 60)/2, SCREEN_HEIGHT/2 + 40, 60, ORANGE);
    DrawText("All souls and upgrades lost...", SCREEN_WIDTH/2 - MeasureText("All souls and upgrades lost...", 50)/2, SCREEN_HEIGHT/2 + 120, 50, DARKGRAY);
    if (totalEnemyBullets > 0) {
        DrawText(TextFormat("Final Accuracy: %.1f%%", accuracy), SCREEN_WIDTH/2 - MeasureText("Final Accuracy: 100.0%", 50)/2, SCREEN_HEIGHT/2 + 180, 50, accuracy > 80 ? LIME : RED);
    }
    DrawText("R to Try Again From the Beginning", SCREEN_WIDTH/2 - MeasureText("R to Try Again From the Beginning", 40)/2, SCREEN_HEIGHT/2 + 260, 40, WHITE);
}

void DrawVictory() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,Fade(BLACK,0.8f));
    DrawText("VICTORY – THE STORM IS PARRIED", SCREEN_WIDTH/2 - MeasureText("VICTORY – THE STORM IS PARRIED", 80)/2, 150, 80, GOLD);
    DrawText(TextFormat("FINAL SCORE: %d", player.score), SCREEN_WIDTH/2 - MeasureText("FINAL SCORE: 999999", 60)/2, 280, 60, YELLOW);
    DrawText(TextFormat("FINAL ACCURACY: %.1f%%", accuracy), SCREEN_WIDTH/2 - MeasureText("FINAL ACCURACY: 100.0%", 60)/2, 360, 60, accuracy >= 99.0f ? LIME : WHITE);
    if (accuracy >= 99.0f) DrawText("TRUE GIT GUD ACHIEVED", SCREEN_WIDTH/2 - MeasureText("TRUE GIT GUD ACHIEVED", 60)/2, 460, 60, GOLD);
    DrawText("You have conquered the ultimate trial.", SCREEN_WIDTH/2 - MeasureText("You have conquered the ultimate trial.", 40)/2, SCREEN_HEIGHT - 120, 40, LIGHTGRAY);
}