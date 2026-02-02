#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

// --- Thread-Safe Atomic Wrapper for Vectors ---
template <typename T> struct AtomicWrapper {
  std::atomic<T> val;
  AtomicWrapper() : val(T{}) {}
  AtomicWrapper(T v) : val(v) {}
  AtomicWrapper(const AtomicWrapper &o) : val(o.val.load()) {}
  AtomicWrapper &operator=(const AtomicWrapper &o) {
    val.store(o.val.load());
    return *this;
  }
  AtomicWrapper &operator=(T v) {
    val.store(v);
    return *this;
  }
  operator T() const { return val.load(); }
  T operator++(int) { return val.fetch_add(1); }
  T operator--(int) { return val.fetch_sub(1); }
  T operator+=(T v) {
    T expected = val.load();
    while (!val.compare_exchange_weak(expected, expected + v))
      ;
    return expected + v;
  }
  T operator-=(T v) {
    T expected = val.load();
    while (!val.compare_exchange_weak(expected, expected - v))
      ;
    return expected - v;
  }
};

// Forward Declarations
void QueueExplosion(Vector3 pos, Color color);
void QueueSound(Sound sfx);
void QueueText(Vector3 pos, const char *text, Color color);
void ProcessEffectBuffer();
void SpawnExplosion(Vector3 pos, Color color);
void SpawnBullet(Vector3 pos, Vector3 vel);
void SpawnEnemyBullet(Vector3 pos, Vector3 vel);

// --- Atomic Helper for Floats ---
void AtomicMax(std::atomic<float> &atom, float val) {
  float cur = atom.load();
  while (cur < val && !atom.compare_exchange_weak(cur, val))
    ;
}

// --- Command Buffer for Thread-Safe Effects ---
struct EffectCommand {
  enum Type { EXPLOSION, SOUND, TEXT };
  Type type;
  Vector3 pos;
  Color color;
  char text[32];
  Sound sfx;
};

static std::deque<EffectCommand> effectBuffer;
static std::mutex bufferMutex;

void QueueExplosion(Vector3 pos, Color color) {
  std::lock_guard<std::mutex> lock(bufferMutex);
  effectBuffer.push_back({EffectCommand::EXPLOSION, pos, color, "", {0}});
}

void QueueSound(Sound sfx) {
  std::lock_guard<std::mutex> lock(bufferMutex);
  effectBuffer.push_back({EffectCommand::SOUND, {0}, WHITE, "", sfx});
}

void QueueText(Vector3 pos, const char *text, Color color) {
  std::lock_guard<std::mutex> lock(bufferMutex);
  EffectCommand cmd = {EffectCommand::TEXT, pos, color, "", {0}};
  strncpy(cmd.text, text, 31);
  effectBuffer.push_back(cmd);
}

// --- Constants ---
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int MAX_BULLETS = 1000;
constexpr int MAX_ENEMIES = 100;
constexpr int MAX_PARTICLES = 2000;
constexpr int MAX_FLOATING_TEXTS = 50;

// --- Enums ---
enum GameScreen {
  SCREEN_MENU,
  SCREEN_BOOT,
  SCREEN_PLAYING,
  SCREEN_UPGRADE,
  SCREEN_GAMEOVER,
  SCREEN_VICTORY
};

// --- Data Structures ---
struct Player {
  Vector3 position;
  Vector3 velocity;
  float speed;
  AtomicWrapper<int> health;
  int maxHealth;

  // RPG Stats
  AtomicWrapper<int> xp;
  int level;
  int xpToNextLevel;
  float critChance;  // 0.0 to 1.0
  float healthRegen; // HP per second

  // Abilities
  float dashCooldown;
  float dashTimer;    // Time remaining in dash
  float dashRecharge; // Time until dash ready
  bool focusMode;

  // Stats modifiers
  float fireRateMult;
  float speedMult;
  float damageMult;

  // Virtues could be a map or array
  float virtues[4]; // 0:Efficiency, 1:Robustness, 2:Concurrency, 3:Memory
};

struct Bullet {
  Vector3 position;
  Vector3 velocity;
  float radius;
  AtomicWrapper<bool> active;
  Color color;
  bool isEnemyBullet;
};

struct Enemy {
  Vector3 position;
  Vector3 lastPosition; // For stuck detection
  float speed;
  AtomicWrapper<int> health;
  int maxHealth;
  AtomicWrapper<bool> active;
  int type; // 0=Basic, 1=Shooter, 2=Boss, 3=Tank, 4=Phantom, 5=Splitter,
            // 6=Support
  float shootCooldown;
  float hitTimer;     // For visual feedback
  float stuckTimer;   // Track how long we've been stuck
  float dashTimer;    // Active dash duration
  float dashCooldown; // Cooldown between dashes
};

struct Obstacle {
  Vector3 position;
  Vector3 size;
  Color color;
  bool active;
};

struct Particle {
  Vector3 position;
  Vector3 velocity;
  Color color;
  float size;
  float life; // 0.0 to 1.0 or seconds
  float decay;
  bool active;
};

struct FloatingText {
  Vector3 position;
  char text[32];
  Color color;
  float life;
  float speed;
  bool active;
};

struct GameData {
  Player player;
  std::vector<Enemy> enemies;
  std::vector<Bullet> playerBullets;
  std::vector<Bullet> enemyBullets;
  std::vector<Obstacle> obstacles;
  std::vector<Particle> particles;
  std::vector<FloatingText> floatingTexts;

  Camera3D camera;
  int wave;
  AtomicWrapper<int> score;
  bool gameOver;
  AtomicWrapper<int> currentScreen;  // Changed from Screen to GameScreen
  AtomicWrapper<float> hitStopTimer; // Cinematic freeze duration
  AtomicWrapper<float> hitShake;     // Impact screenshake intensity

  float spawnTimer;
  int enemiesToSpawn;
  int enemiesSpawned;
  bool debugMode;

  // Audio (moved from globals)
  Sound sfxShoot;
  Sound sfxDash;
  Sound sfxHit;
  Sound sfxPowerup;
  Sound sfxExplosion;
  Sound sfxEnemyDeath;
  Music musicGameplay;
  Music musicMenu;
  Sound sfxBonus;
  Sound sfxLevelUp;
  Sound sfxVictory;
  Sound sfxLowHealth;
  Sound sfxMenuClick;
  Sound sfxEnemyShoot;
  Sound sfxEnemySpawn;
  Sound sfxBlinker;
  int particleRollingIdx;
  std::atomic<int> bulletRollingIdx;
  std::atomic<int> enemyBulletRollingIdx;
};

// --- Globals (for simple monolithic access) ---
static GameData game; // Changed from GameState to GameData
static Shader postProcessShader;
static std::recursive_mutex
    gameMutex; // Protects shared game state (score, xp, spawning)

// --- Threading Infrastructure ---
class ThreadPool {
public:
  ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i)
      workers.emplace_back([this] {
        for (;;) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            this->condition.wait(
                lock, [this] { return this->stop || !this->tasks.empty(); });
            if (this->stop && this->tasks.empty())
              return;
            task = std::move(this->tasks.front());
            this->tasks.pop();
          }
          task();
        }
      });
  }

  template <class F, class... Args>
  auto enqueue(F &&f, Args &&...args)
      -> std::future<typename std::invoke_result<F, Args...>::type> {
    using return_type = typename std::invoke_result<F, Args...>::type;
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<return_type> res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      if (stop)
        throw std::runtime_error("enqueue on stopped ThreadPool");
      tasks.emplace([task]() { (*task)(); });
    }
    condition.notify_one();
    return res;
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers)
      worker.join();
  }

private:
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;
  std::mutex queue_mutex;
  std::condition_variable condition;
  bool stop;
};

static std::unique_ptr<ThreadPool> threadPool;

// --- Audio Engine ---
enum Waveform { SINE, SQUARE, TRIANGLE, SAW, NOISE };

Sound GenerateSynthSound(Waveform type, float freqStart, float freqEnd,
                         float duration, float volume) {
  Wave wave = {0};
  wave.frameCount = (unsigned int)(44100 * duration);
  wave.sampleRate = 44100;
  wave.sampleSize = 16;
  wave.channels = 1;
  wave.data = malloc(wave.frameCount * sizeof(short));
  short *samples = (short *)wave.data;

  float currentPhase = 0.0f;
  for (unsigned int i = 0; i < wave.frameCount; i++) {
    float progress = (float)i / wave.frameCount;
    float freq = freqStart + (freqEnd - freqStart) * progress;
    float sample = 0.0f;

    switch (type) {
    case SINE:
      sample = sinf(currentPhase * 2.0f * PI);
      break;
    case SQUARE:
      sample = (sinf(currentPhase * 2.0f * PI) > 0) ? 0.6f : -0.6f;
      break;
    case SAW:
      sample = 2.0f * (currentPhase - floorf(currentPhase + 0.5f));
      break;
    case TRIANGLE:
      sample =
          2.0f * fabsf(2.0f * (currentPhase - floorf(currentPhase + 0.5f))) -
          1.0f;
      break;
    case NOISE:
      sample = (float)GetRandomValue(-100, 100) / 100.0f;
      break;
    }

    // Linear Decay Envelope
    float envelope = 1.0f - progress;
    samples[i] = (short)(sample * volume * envelope * 32000.0f);

    currentPhase += freq / wave.sampleRate;
    if (currentPhase > 1.0f)
      currentPhase -= 1.0f;
  }

  Sound sound = LoadSoundFromWave(wave);
  UnloadWave(wave);
  return sound;
}

Sound GeneratePulseBGM(float freq) {
  float duration = 1.0f; // 1 second loop
  unsigned int frameCount = (unsigned int)(44100 * duration);
  Wave wave = {0};
  wave.frameCount = frameCount;
  wave.sampleRate = 44100;
  wave.sampleSize = 16;
  wave.channels = 1;
  wave.data = malloc(frameCount * sizeof(short));
  short *samples = (short *)wave.data;

  for (unsigned int i = 0; i < frameCount; i++) {
    float t = (float)i / wave.sampleRate;
    // Simple 4/4 Pulse (Kick + Bass)
    float pump = powf(sinf(2.0f * PI * 2.0f * t), 4.0f); // 2Hz pulse (120BPM)
    float sample = sinf(2.0f * PI * freq * t) * pump;
    samples[i] = (short)(sample * 0.3f * 32000.0f);
  }

  Sound sound = LoadSoundFromWave(wave);
  UnloadWave(wave);
  return sound;
}

static RenderTexture2D target;

// --- Forward Declarations ---
void InitGame();
void UpdateGame();
void DrawGame();
void UpdateDrawFrame();
void SpawnExplosion(Vector3 pos, Color color);
void SpawnFloatingText(Vector3 pos, const char *text, Color color);
void CheckLevelUp();
Sound GenerateBeep(float frequency, float duration);

int main() {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Cursor - Ascend the Code");
  SetTargetFPS(60);
  DisableCursor(); // Hide system cursor for 3D crosshair

  // Load Shader
  postProcessShader = LoadShader("resources/shaders/base.vs", "resources/shaders/crt.fs");

  // Create Render Texture
  target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

  // Initialize Thread Pool
  threadPool =
      std::make_unique<ThreadPool>(std::thread::hardware_concurrency());

  InitAudioDevice();
  InitGame();

  // Generate Procedural SFX (The "Programmer Sound" Overhaul)
  game.sfxShoot = GenerateSynthSound(SQUARE, 880.0f, 220.0f, 0.1f, 0.4f);
  game.sfxDash = GenerateSynthSound(SINE, 440.0f, 880.0f, 0.15f, 0.5f);
  game.sfxHit = GenerateSynthSound(NOISE, 1000.0f, 100.0f, 0.05f, 0.3f);
  game.sfxExplosion = GenerateSynthSound(NOISE, 200.0f, 50.0f, 0.4f, 0.6f);
  game.sfxPowerup = GenerateSynthSound(TRIANGLE, 440.0f, 1760.0f, 0.3f, 0.5f);
  game.sfxEnemyDeath = GenerateSynthSound(SAW, 330.0f, 110.0f, 0.2f, 0.4f);
  game.sfxLevelUp = GenerateSynthSound(SQUARE, 220.0f, 880.0f, 0.5f, 0.6f);
  game.sfxVictory = GenerateSynthSound(SINE, 110.0f, 1760.0f, 1.0f, 0.7f);
  game.sfxLowHealth = GenerateSynthSound(SINE, 100.0f, 100.0f, 0.1f, 0.5f);
  game.sfxMenuClick = GenerateSynthSound(NOISE, 800.0f, 800.0f, 0.02f, 0.3f);
  game.sfxEnemyShoot =
      GenerateSynthSound(SQUARE, 1200.0f, 440.0f, 0.08f, 0.35f);
  game.sfxEnemySpawn = GenerateSynthSound(NOISE, 440.0f, 880.0f, 0.15f, 0.4f);
  game.sfxBlinker = GenerateSynthSound(SINE, 880.0f, 1760.0f, 0.05f, 0.5f);

  // Procedural BGM (Loops)
  game.sfxBonus = GeneratePulseBGM(60.0f);

  while (!WindowShouldClose()) {
    // Update Shader Uniforms
    float time = (float)GetTime();
    Vector2 res = {(float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
    SetShaderValue(postProcessShader,
                   GetShaderLocation(postProcessShader, "time"), &time,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(postProcessShader,
                   GetShaderLocation(postProcessShader, "resolution"), &res,
                   SHADER_UNIFORM_VEC2);

    float aberration = 0.001f;
    if (game.hitStopTimer > 0)
      aberration = 0.005f;
    SetShaderValue(postProcessShader,
                   GetShaderLocation(postProcessShader, "aberration"),
                   &aberration, SHADER_UNIFORM_FLOAT);

    UpdateDrawFrame();
  }

  // Cleanup
  UnloadShader(postProcessShader);
  UnloadRenderTexture(target);
  CloseAudioDevice();
  CloseWindow();
  return 0;
}

// SFX Generation Helper replaced by GenerateSynthSound

// Helper: Spawn Floating Text
void SpawnFloatingText(Vector3 pos, const char *text, Color color) {
  QueueText(pos, text, color);
}

// Helper: Centralized Level Up Logic
void CheckLevelUp() {
  std::lock_guard<std::recursive_mutex> lock(gameMutex);
  while (game.player.xp >= game.player.xpToNextLevel) {
    game.player.xp -= game.player.xpToNextLevel;
    game.player.level++;

    // Supernova Effect
    for (int i = 0; i < 5; i++) {
      SpawnExplosion(game.player.position, SKYBLUE);
      SpawnExplosion(game.player.position, GOLD);
    }
    game.hitShake = 1.0f;
    PlaySound(game.sfxLevelUp);

    float lvl = (float)game.player.level;
    game.player.xpToNextLevel = (int)(100.0f * powf(lvl, 1.8f) + 50.0f * lvl);

    game.currentScreen = SCREEN_UPGRADE;
  }
}

void InitGame() {
  // Initialize Player
  game.player.position = {0.0f, 1.0f, 0.0f};
  game.player.speed = 10.0f;
  game.player.health = 100;
  game.player.maxHealth = 100;
  game.gameOver = false;
  game.score = 0;
  game.wave = 1;

  // --- Stats & State ---
  game.player.health = 100;
  game.player.maxHealth = 100;
  game.player.xp = 0;
  game.player.level = 1;
  game.player.xpToNextLevel = 100;
  game.player.critChance = 0.05f; // 5% base crit
  game.player.healthRegen = 2.0f; // 2 HP/sec base
  game.player.speed = 12.0f;
  game.player.dashCooldown = 0.0f;
  game.player.dashTimer = 0.0f;
  game.player.dashRecharge = 2.0f;
  game.player.focusMode = false;
  game.player.fireRateMult = 1.0f;
  game.player.speedMult = 1.0f;
  game.player.damageMult = 1.0f;
  for (int i = 0; i < 4; i++)
    game.player.virtues[i] = 0.0f;

  // --- Wave System ---
  game.currentScreen = SCREEN_MENU;
  game.enemiesToSpawn = 10;
  game.enemiesSpawned = 0;
  game.spawnTimer = 2.0f;

  // Initialize Vectors with pre-allocation
  game.playerBullets.assign(MAX_BULLETS,
                            {{0, 0, 0}, {0, 0, 0}, 0.1f, false, WHITE, false});
  game.enemyBullets.assign(MAX_BULLETS,
                           {{0, 0, 0}, {0, 0, 0}, 0.1f, false, RED, true});
  game.enemies.assign(MAX_ENEMIES, {{0, 0, 0},
                                    {0, 0, 0},
                                    2.0f,
                                    0,
                                    0,
                                    false,
                                    0,
                                    0.0f,
                                    0.0f,
                                    0.0f,
                                    0.0f,
                                    0.0f});
  game.particles.assign(MAX_PARTICLES,
                        {{0, 0, 0}, {0, 0, 0}, WHITE, 0.1f, 0.0f, 1.0f, false});
  game.floatingTexts.assign(MAX_FLOATING_TEXTS,
                            {{0, 0, 0}, "", WHITE, 0.0f, 0.0f, false});
  game.particleRollingIdx = 0;

  // Ensure all entities are deactivated at start
  for (auto &b : game.playerBullets)
    b.active = false;
  for (auto &b : game.enemyBullets)
    b.active = false;
  for (auto &e : game.enemies)
    e.active = false;
  for (auto &p : game.particles)
    p.active = false;
  for (auto &ft : game.floatingTexts)
    ft.active = false;

  // Initialize Obstacles
  game.obstacles.clear();
  for (int i = 0; i < 8; i++) {
    Obstacle obs;
    float angle = (float)i / 8.0f * 2.0f * PI;
    float radius = 25.0f;
    obs.position = {cosf(angle) * radius, 0.5f, sinf(angle) * radius};

    // Mix of box and round obstacles
    if (i % 2 == 0) {
      obs.size = {3.0f, 1.0f, 3.0f}; // Box
    } else {
      obs.size = {2.5f, 1.0f, 2.5f}; // Round (rendered as cylinder)
    }
    obs.color = (i % 2 == 0) ? PURPLE : MAROON;
    obs.active = true;
    game.obstacles.push_back(obs);
  }

  // Initialize Camera
  game.camera.position = {0.0f, 20.0f, 10.0f}; // Top-down angled
  game.camera.target = {0.0f, 0.0f, 0.0f};
  game.camera.up = {0.0f, 1.0f, 0.0f};
  game.camera.fovy = 45.0f;
  game.camera.projection = CAMERA_PERSPECTIVE;
}

// Helper: Check Bullet collision with obstacles
bool CheckBulletObstacles(Vector3 pos, float radius) {
  for (const auto &obs : game.obstacles) {
    // Simple sphere-box check
    Vector3 clamp = Vector3Max(
        Vector3Subtract(obs.position, Vector3Scale(obs.size, 0.5f)),
        Vector3Min(Vector3Add(obs.position, Vector3Scale(obs.size, 0.5f)),
                   pos));
    float dist = Vector3Distance(pos, clamp);
    if (dist < radius)
      return true;
  }
  return false;
}

void SpawnBullet(Vector3 pos, Vector3 vel) {
  int idx = game.bulletRollingIdx.fetch_add(1) % MAX_BULLETS;
  auto &b = game.playerBullets[idx];
  b.active = true;
  b.position = pos;
  b.velocity = vel;
  b.radius = 0.2f;
  b.color = SKYBLUE;
  b.isEnemyBullet = false;
}

void SpawnEnemyBullet(Vector3 pos, Vector3 vel) {
  int idx = game.enemyBulletRollingIdx.fetch_add(1) % MAX_BULLETS;
  auto &b = game.enemyBullets[idx];
  b.active = true;
  b.position = pos;
  b.velocity = vel;
  b.radius = 0.25f;
  b.color = RED;
  b.isEnemyBullet = true;
}

// Helper: Check entity collision with obstacles
bool CheckEntityObstacleCollision(Vector3 pos, float radius) {
  for (const auto &obs : game.obstacles) {
    if (!obs.active)
      continue;
    Vector3 halfSize = Vector3Scale(obs.size, 0.5f);
    Vector3 clamp = {fmaxf(obs.position.x - halfSize.x,
                           fminf(pos.x, obs.position.x + halfSize.x)),
                     fmaxf(obs.position.y - halfSize.y,
                           fminf(pos.y, obs.position.y + halfSize.y)),
                     fmaxf(obs.position.z - halfSize.z,
                           fminf(pos.z, obs.position.z + halfSize.z))};
    if (Vector3Distance(pos, clamp) < radius)
      return true;
  }
  return false;
}

// Helper: Check if bullet threatens entity
bool BulletThreatsEntity(const Bullet &b, Vector3 entityPos,
                         float threatRange) {
  if (!b.active)
    return false;
  float distToBullet = Vector3Distance(b.position, entityPos);
  if (distToBullet > threatRange)
    return false;
  Vector3 toBullet = Vector3Subtract(b.position, entityPos);
  float dotProduct = Vector3DotProduct(Vector3Normalize(b.velocity),
                                       Vector3Normalize(toBullet));
  return dotProduct < -0.3f;
}

// Helper: Get obstacle avoidance direction (improved)
Vector3 GetAvoidanceDirection(Vector3 pos, Vector3 forward, float lookAhead) {
  // First check if we're already stuck in an obstacle
  if (CheckEntityObstacleCollision(pos, 0.5f)) {
    // Already stuck! Try to escape in any clear direction
    Vector3 escapeDirections[] = {{1, 0, 0},
                                  {-1, 0, 0},
                                  {0, 0, 1},
                                  {0, 0, -1},
                                  {0.707f, 0, 0.707f},
                                  {-0.707f, 0, 0.707f},
                                  {0.707f, 0, -0.707f},
                                  {-0.707f, 0, -0.707f}};

    for (int i = 0; i < 8; i++) {
      Vector3 testPos =
          Vector3Add(pos, Vector3Scale(escapeDirections[i], 1.0f));
      if (!CheckEntityObstacleCollision(testPos, 0.5f)) {
        return escapeDirections[i]; // Found escape route
      }
    }
    return Vector3Negate(forward); // Last resort: back up
  }

  // Check multiple distances ahead
  Vector3 normForward = Vector3Normalize(forward);
  for (float dist = lookAhead; dist >= 1.0f; dist -= 0.5f) {
    Vector3 testPos = Vector3Add(pos, Vector3Scale(normForward, dist));
    if (CheckEntityObstacleCollision(testPos, 0.5f)) {
      // Obstacle detected! Find best tangent
      Vector3 right = {normForward.z, 0, -normForward.x};
      Vector3 left = Vector3Negate(right);

      // Test both tangents
      Vector3 testRight = Vector3Add(pos, Vector3Scale(right, lookAhead));
      Vector3 testLeft = Vector3Add(pos, Vector3Scale(left, lookAhead));

      bool rightClear = !CheckEntityObstacleCollision(testRight, 0.5f);
      bool leftClear = !CheckEntityObstacleCollision(testLeft, 0.5f);

      if (rightClear && !leftClear)
        return right;
      if (leftClear && !rightClear)
        return left;
      if (rightClear && leftClear) {
        // Both clear, pick randomly
        return GetRandomValue(0, 1) ? right : left;
      }

      // Both blocked, try diagonal
      Vector3 diagRight = Vector3Normalize(Vector3Add(normForward, right));
      Vector3 diagLeft = Vector3Normalize(Vector3Add(normForward, left));
      Vector3 testDiagRight =
          Vector3Add(pos, Vector3Scale(diagRight, lookAhead));
      Vector3 testDiagLeft = Vector3Add(pos, Vector3Scale(diagLeft, lookAhead));

      if (!CheckEntityObstacleCollision(testDiagRight, 0.5f))
        return diagRight;
      if (!CheckEntityObstacleCollision(testDiagLeft, 0.5f))
        return diagLeft;

      return Vector3Negate(normForward); // Blocked ahead, reverse
    }
  }

  return normForward; // Path clear
}

void UpdateGame() {
  if (game.currentScreen == SCREEN_MENU) {
    if (IsKeyPressed(KEY_SPACE)) {
      game.currentScreen = SCREEN_PLAYING;
      // Reset game state
      game.wave = 1;
      game.score = 0;
      game.player.health = game.player.maxHealth;
      game.enemiesToSpawn = 10;
      game.enemiesSpawned = 0;
      // Clear enemies and bullets
      for (auto &e : game.enemies)
        e.active = false;
      for (auto &b : game.playerBullets)
        b.active = false;
      for (auto &b : game.enemyBullets)
        b.active = false;
    }
    return;
  } else if (game.currentScreen == SCREEN_GAMEOVER) {
    if (IsKeyPressed(KEY_R))
      game.currentScreen = SCREEN_MENU;
    return;
  } else if (game.currentScreen == SCREEN_UPGRADE) {
    bool selected = false;
    if (IsKeyPressed(KEY_E)) { // OVERCLOCK: Speed
      game.player.speedMult += 0.2f;
      selected = true;
    } else if (IsKeyPressed(KEY_R)) { // FIREWALL: Health Regen
      game.player.healthRegen += 1.0f;
      game.player.health = game.player.maxHealth;
      selected = true;
    } else if (IsKeyPressed(KEY_F)) { // MULTITHREAD: Fire Rate
      game.player.fireRateMult += 0.2f;
      selected = true;
    }

    if (selected) {
      game.currentScreen = SCREEN_PLAYING;
      QueueSound(game.sfxPowerup);
    }
    return;
  }

  // --- Wave Logic ---
  bool allDead = true;
  for (const auto &e : game.enemies) {
    if (e.active) {
      allDead = false;
      break;
    }
  }

  if (allDead && game.enemiesSpawned >= game.enemiesToSpawn) {
    game.wave++;
    game.enemiesToSpawn += 5;
    game.enemiesSpawned = 0;
    game.spawnTimer = 2.0f;
  }

  // --- Focus Mode ---
  bool focusInput =
      IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsKeyDown(KEY_LEFT_SHIFT);
  game.player.focusMode = focusInput;

  // --- Debug Tools ---
  if (IsKeyPressed(KEY_ZERO)) {
    game.debugMode = !game.debugMode;
    game.enemiesSpawned = 0; // Reset spawn count?
  }

  if (game.debugMode) {
    int spawnType = -1;
    if (IsKeyPressed(KEY_ONE))
      spawnType = 0;
    else if (IsKeyPressed(KEY_TWO))
      spawnType = 1;
    else if (IsKeyPressed(KEY_THREE))
      spawnType = 2;
    else if (IsKeyPressed(KEY_FOUR))
      spawnType = 3;

    if (spawnType != -1) {
      for (auto &e : game.enemies) {
        if (!e.active) {
          e.active = true;
          e.type = spawnType;
          e.hitTimer = 0.0f;

          // Stats
          if (e.type == 2) { // Boss
            e.maxHealth = 200;
            e.shootCooldown = 0.5f;
            e.position = {0, 1, -20};
          } else if (e.type == 3) { // Tank
            e.maxHealth = 20;
            e.shootCooldown = 0;
            e.position = Vector3Add(game.player.position, {10, 0, 10});
          } else if (e.type == 1) { // Shooter
            e.maxHealth = 5;
            e.shootCooldown = 2.0f;
            e.position = Vector3Add(game.player.position, {-10, 0, -10});
          } else { // Chaser
            e.maxHealth = 2;
            e.shootCooldown = 0;
            e.position = Vector3Add(game.player.position, {10, 0, -10});
          }
          e.health = e.maxHealth;
          // Velocity init
          // velocity removed from struct

          break;
        }
      }
    }
  }

  // --- Time Scale ---
  float rawDt = GetFrameTime();
  game.hitStopTimer -= rawDt;
  game.hitShake -= rawDt * 2.5f; // Slightly faster shake decay
  if (game.hitShake < 0)
    game.hitShake = 0;

  float dt = rawDt;
  if (game.hitStopTimer > 0)
    dt *= 0.1f; // Cinematic slow-mo "dip" (impact noticeable but fluid)

  if (game.player.focusMode)
    dt *= 0.5f; // Slow motion

  // --- Player Movement & Dash ---
  game.player.dashCooldown -= dt;
  game.player.dashTimer -= dt;

  // Health Regeneration
  if (game.player.health < game.player.maxHealth) {
    static float regenAccumulator = 0.0f;
    regenAccumulator += game.player.healthRegen * dt;
    if (regenAccumulator >= 1.0f) {
      game.player.health += (int)regenAccumulator;
      regenAccumulator -= (int)regenAccumulator;
      if (game.player.health > game.player.maxHealth)
        game.player.health = game.player.maxHealth;
    }
  }

  float currentSpeed = game.player.speed * game.player.speedMult;
  if (game.player.focusMode)
    currentSpeed *= 0.5f;

  // Dash Input
  if (IsKeyPressed(KEY_SPACE) && game.player.dashCooldown <= 0.0f) {
    game.player.dashTimer = 0.15f;   // 150ms dash
    game.player.dashCooldown = 1.0f; // 1s cooldown
    QueueSound(game.sfxDash);
  }

  Vector3 move = {0};
  if (IsKeyDown(KEY_W))
    move.z -= 1.0f;
  if (IsKeyDown(KEY_S))
    move.z += 1.0f;
  if (IsKeyDown(KEY_A))
    move.x -= 1.0f;
  if (IsKeyDown(KEY_D))
    move.x += 1.0f;

  if (Vector3Length(move) > 0) {
    move = Vector3Normalize(move);

    // Apply Dash
    if (game.player.dashTimer > 0.0f) {
      currentSpeed *= 4.0f; // 4x speed during dash
      // Trail Particles
      Vector3 trailPos = Vector3Add(game.player.position,
                                    {(float)GetRandomValue(-2, 2) / 10.0f, 0,
                                     (float)GetRandomValue(-2, 2) / 10.0f});
      QueueExplosion(
          trailPos,
          GOLD); // Reuse spawn explosion for now, creates 5 small particles
      // Ideally we'd have a simpler SpawnParticle for trail, but this works for
      // "chaos"
    }

    game.player.position =
        Vector3Add(game.player.position, Vector3Scale(move, currentSpeed * dt));
  }

  // --- Aiming (Raycast to ground plane) ---
  Ray ray = GetMouseRay(GetMousePosition(), game.camera);
  // Intersection with plane Y=0. Ray: P = O + t*D. Plane: P.y = 0.
  // O.y + t*D.y = 0 => t = -O.y / D.y
  if (ray.direction.y != 0) {
    float t = -ray.position.y / ray.direction.y;
    if (t >= 0) {
      Vector3 target = Vector3Add(ray.position, Vector3Scale(ray.direction, t));

      // --- Shooting ---
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        static float shootTimer = 0.0f;
        shootTimer -= dt;

        float fireRate = 0.1f / game.player.fireRateMult; // Base 0.1s

        if (shootTimer <= 0.0f) {
          shootTimer = fireRate;

          Vector3 dir = Vector3Subtract(target, game.player.position);
          dir.y = 0; // Keep horizontal
          dir = Vector3Normalize(dir);

          SpawnBullet(game.player.position, Vector3Scale(dir, 20.0f));
          QueueSound(game.sfxShoot);

          // Recoil
          Vector3 recoilDir = Vector3Scale(dir, -0.2f);
          game.player.position = Vector3Add(game.player.position, recoilDir);
        }
      }
    }
  }

  // --- Parallel Update Tasks ---
  std::vector<std::future<void>> futures;

  // 1. Bullet Update (Player) - Partitioned
  const int bulletBatchSize = MAX_BULLETS / 4;
  for (int i = 0; i < 4; ++i) {
    int start = i * bulletBatchSize;
    int end = (i == 3) ? MAX_BULLETS : (i + 1) * bulletBatchSize;
    futures.emplace_back(threadPool->enqueue([dt, start, end] {
      for (int j = start; j < end; ++j) {
        auto &b = game.playerBullets[j];
        if (b.active) {
          b.position = Vector3Add(b.position, Vector3Scale(b.velocity, dt));
          if (Vector3Length(b.position) > 100.0f ||
              CheckBulletObstacles(b.position, b.radius))
            b.active = false;
        }
      }
    }));
  }

  // 2. Bullet Update (Enemy) - Partitioned
  for (int i = 0; i < 4; ++i) {
    int start = i * bulletBatchSize;
    int end = (i == 3) ? MAX_BULLETS : (i + 1) * bulletBatchSize;
    futures.emplace_back(threadPool->enqueue([dt, start, end] {
      for (int j = start; j < end; ++j) {
        auto &b = game.enemyBullets[j];
        if (b.active) {
          b.position = Vector3Add(b.position, Vector3Scale(b.velocity, dt));
          if (Vector3Length(b.position) > 100.0f ||
              CheckBulletObstacles(b.position, b.radius))
            b.active = false;

          // Hit Player
          float distSq = Vector3DistanceSqr(b.position, game.player.position);
          float combinedRadius = b.radius + 0.5f;
          if (distSq < combinedRadius * combinedRadius) {
            if (game.player.dashTimer <= 0.0f) {
              game.player.health -= 5;
              b.active = false;
              QueueExplosion(game.player.position, RED);
              if (game.player.health <= 0)
                game.currentScreen = (int)SCREEN_GAMEOVER;
            }
          }
        }
      }
    }));
  }

  // Wait for bullets to finish (since spawning depends on wave clear)
  for (auto &f : futures)
    f.get();
  futures.clear();

  // --- Spawning (Sequential) ---
  if (game.enemiesSpawned < game.enemiesToSpawn) {
    game.spawnTimer -= dt;
    if (game.spawnTimer <= 0.0f) {
      std::lock_guard<std::recursive_mutex> lock(gameMutex);
      // Check Boss (Every 5th wave)
      if (game.wave % 5 == 0 && game.enemiesSpawned == 0) {
        for (auto &e : game.enemies) {
          if (!e.active) {
            e.active = true;
            e.type = 2;
            e.maxHealth = 8000 + (game.wave * 1000);
            e.health = e.maxHealth;
            e.position = {0, 1, -20};
            e.shootCooldown = 0.5f;
            e.hitTimer = 0.0f;
            e.speed = 2.0f;
            e.lastPosition = e.position;
            e.stuckTimer = 0.0f;
            game.enemiesToSpawn = 1;
            game.enemiesSpawned++;
            QueueSound(game.sfxEnemySpawn);
            break;
          }
        }
        game.spawnTimer = 999.0f;
      } else if (game.wave % 5 != 0) {
        float spawnRate = 1.5f - (game.wave * 0.05f);
        if (spawnRate < 0.2f)
          spawnRate = 0.2f;
        game.spawnTimer = spawnRate;
        for (auto &e : game.enemies) {
          if (!e.active) {
            e.active = true;
            int roll = GetRandomValue(0, 100);
            if (game.wave >= 8 && roll > 95)
              e.type = 6;
            else if (game.wave >= 6 && roll > 85)
              e.type = 5;
            else if (game.wave >= 4 && roll > 75)
              e.type = 4;
            else if (game.wave >= 3 && roll > 60)
              e.type = 3;
            else if (game.wave >= 2 && roll > 40)
              e.type = 1;
            else
              e.type = 0;

            int difficulty = game.wave;
            if (e.type == 6) {
              e.maxHealth = 400 + (difficulty * 50);
              e.shootCooldown = 3.0f;
            } else if (e.type == 5) {
              e.maxHealth = 500 + (difficulty * 60);
              e.shootCooldown = 0;
            } else if (e.type == 4) {
              e.maxHealth = 250 + (difficulty * 40);
              e.shootCooldown = 1.5f;
            } else if (e.type == 3) {
              e.maxHealth = 600 + (difficulty * 80);
              e.shootCooldown = 0;
            } else if (e.type == 1) {
              e.maxHealth = 200 + (difficulty * 30);
              e.shootCooldown = 2.0f;
            } else {
              e.maxHealth = 250 + (difficulty * 25);
              e.shootCooldown = 0;
            }
            e.health = e.maxHealth;
            e.hitTimer = 0.0f;
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            e.position = {cosf(angle) * 35.0f, 1.0f, sinf(angle) * 35.0f};
            game.enemiesSpawned++;
            QueueSound(game.sfxEnemySpawn);
            break;
          }
        }
      }
    }
  } else {
    bool allDead = true;
    for (const auto &e : game.enemies)
      if (e.active)
        allDead = false;
    if (allDead) {
      std::lock_guard<std::recursive_mutex> lock(gameMutex);
      game.currentScreen = SCREEN_UPGRADE;
      game.spawnTimer = 2.0f;
    }
  }

  // 3. Enemy Update (Parallelized AI and Collision) - Partitioned
  const int enemyBatchSize = MAX_ENEMIES / 4;
  for (int i = 0; i < 4; ++i) {
    int start = i * enemyBatchSize;
    int end = (i == 3) ? MAX_ENEMIES : (i + 1) * enemyBatchSize;
    futures.emplace_back(threadPool->enqueue([dt, start, end] {
      for (int k = start; k < end; ++k) {
        auto &e = game.enemies[k];
        if (e.active) {
          // Update timers
          e.dashTimer -= dt;
          e.dashCooldown -= dt;

          Vector3 dir = Vector3Subtract(game.player.position, e.position);
          float dist = Vector3Length(dir);
          dir = Vector3Normalize(dir);

          // AI Logic
          float speedMult = (game.player.focusMode ? 0.5f : 1.0f);
          Vector3 moveDir = dir;
          bool isStuck = false;
          bool shouldDash = false;

          float movementThisFrame = Vector3Distance(e.position, e.lastPosition);
          if (movementThisFrame < 0.05f * dt) {
            e.stuckTimer += dt;
            if (e.stuckTimer > 0.2f) {
              isStuck = true;
              e.stuckTimer = 0.0f;
            }
          } else {
            e.stuckTimer = 0.0f;
          }
          e.lastPosition = e.position;

          if (isStuck && e.dashCooldown <= 0.0f) {
            shouldDash = true;
            float randomAngle = (float)GetRandomValue(0, 360) * DEG2RAD;
            moveDir = {cosf(randomAngle), 0, sinf(randomAngle)};
          } else if (e.type != 2 && e.type != 3) {
            float closestThreat = 999.0f;
            Vector3 threatDir = {0, 0, 0};
            for (const auto &b : game.playerBullets) {
              if (!b.active)
                continue;
              float distToBullet = Vector3Distance(b.position, e.position);
              Vector3 bulletDir = Vector3Normalize(b.velocity);
              Vector3 toBullet = Vector3Subtract(b.position, e.position);
              float dotProduct =
                  Vector3DotProduct(bulletDir, Vector3Normalize(toBullet));
              if (dotProduct < -0.5f && distToBullet < closestThreat) {
                closestThreat = distToBullet;
                threatDir = bulletDir;
              }
            }
            if (closestThreat < 4.0f && e.dashCooldown <= 0.0f) {
              shouldDash = true;
              Vector3 dodgeDir = {threatDir.z, 0, -threatDir.x};
              if (GetRandomValue(0, 1))
                dodgeDir = Vector3Negate(dodgeDir);
              moveDir = dodgeDir;
            }
          }

          if (!shouldDash && e.dashTimer <= 0.0f) {
            float lookAhead = (e.type == 3) ? 2.0f : 3.5f;
            Vector3 avoidDir =
                GetAvoidanceDirection(e.position, moveDir, lookAhead);
            if (Vector3Length(Vector3Subtract(avoidDir, moveDir)) > 0.1f)
              moveDir = Vector3Normalize(Vector3Lerp(moveDir, avoidDir, 0.85f));
          }

          if (shouldDash) {
            e.dashTimer = 0.2f;
            e.dashCooldown = 2.0f;
          }

          float dashBoost = (e.dashTimer > 0.0f) ? 15.0f : 1.0f;

          // Type Specific Movement
          if (e.type == 0 || e.type == 4) {
            Vector3 newPos = Vector3Add(
                e.position,
                Vector3Scale(moveDir, 5.0f * speedMult * dashBoost * dt));
            if (e.type == 4) {
              e.shootCooldown -= dt;
              if (e.shootCooldown <= 0.0f) {
                e.shootCooldown = 1.5f;
                Vector3 blinkTarget =
                    Vector3Add(e.position, Vector3Scale(moveDir, 6.0f));
                if (!CheckEntityObstacleCollision(blinkTarget, 0.5f)) {
                  e.position = blinkTarget;
                  QueueExplosion(e.position, MAGENTA);
                  QueueSound(game.sfxBlinker);
                }
              }
            }
            if (e.dashTimer > 0.0f ||
                !CheckEntityObstacleCollision(newPos, 0.5f))
              e.position = newPos;
          } else if (e.type == 6) {
            Vector3 newPos = Vector3Add(
                e.position, Vector3Scale(moveDir, 2.0f * speedMult * dt));
            if (!CheckEntityObstacleCollision(newPos, 0.5f))
              e.position = newPos;
            e.shootCooldown -= dt;
            if (e.shootCooldown <= 0.0f) {
              e.shootCooldown = 3.0f;
              QueueExplosion(e.position, SKYBLUE);
              for (auto &other : game.enemies) {
                if (other.active &&
                    Vector3DistanceSqr(e.position, other.position) < 64.0f) {
                  other.health += 10;
                  if (other.health > other.maxHealth)
                    other.health = other.maxHealth;
                }
              }
            }
          } else if (e.type == 1) {
            if (e.dashTimer > 0.0f) {
              e.position = Vector3Add(
                  e.position,
                  Vector3Scale(moveDir, 4.0f * speedMult * dashBoost * dt));
            } else {
              float idealDist = 12.0f;
              if (dist > idealDist + 3.0f) {
                Vector3 newPos = Vector3Add(
                    e.position, Vector3Scale(moveDir, 4.0f * speedMult * dt));
                if (!CheckEntityObstacleCollision(newPos, 0.5f))
                  e.position = newPos;
              } else if (dist < idealDist - 3.0f) {
                Vector3 newPos = Vector3Subtract(
                    e.position, Vector3Scale(moveDir, 3.0f * dt));
                if (!CheckEntityObstacleCollision(newPos, 0.5f))
                  e.position = newPos;
              } else {
                Vector3 strafeDir = {moveDir.z, 0, -moveDir.x};
                Vector3 newPos =
                    Vector3Add(e.position, Vector3Scale(strafeDir, 2.0f * dt));
                if (!CheckEntityObstacleCollision(newPos, 0.5f))
                  e.position = newPos;
              }
            }
            e.shootCooldown -= dt;
            if (e.shootCooldown <= 0.0f) {
              e.shootCooldown = 2.5f;
              SpawnEnemyBullet(e.position, Vector3Scale(dir, 15.0f));
              QueueSound(game.sfxEnemyShoot);
            }
          } else if (e.type == 2) {
            e.position.x += sinf(GetTime()) * dt * 5.0f;
            e.position.z += cosf(GetTime() * 0.5f) * dt * 2.0f;
            e.shootCooldown -= dt;
            if (e.shootCooldown <= 0.0f) {
              e.shootCooldown = 0.15f;
              static float spiralAngle = 0.0f;
              spiralAngle += 20.0f;
              if (spiralAngle > 360)
                spiralAngle -= 360;
              Vector3 shotDir = {cosf(spiralAngle * DEG2RAD), 0,
                                 sinf(spiralAngle * DEG2RAD)};
              SpawnEnemyBullet(e.position, Vector3Scale(shotDir, 15.0f));
              SpawnEnemyBullet(e.position,
                               Vector3Scale(Vector3Negate(shotDir), 15.0f));
              QueueSound(game.sfxEnemyShoot);
            }
          } else if (e.type == 3) {
            Vector3 newPos = Vector3Add(
                e.position, Vector3Scale(moveDir, 2.5f * speedMult * dt));
            if (!CheckEntityObstacleCollision(newPos, 0.8f))
              e.position = newPos;
            else
              e.position =
                  Vector3Subtract(e.position, Vector3Scale(moveDir, 0.5f * dt));
          }

          e.hitTimer -= dt;

          // Collision Logic
          float radius = (e.type == 2) ? 3.0f : (e.type == 3 ? 0.8f : 0.5f);
          float playerDistSq =
              Vector3DistanceSqr(game.player.position, e.position);
          float combinedPlayerR = 0.5f + radius;
          if (playerDistSq < combinedPlayerR * combinedPlayerR) {
            if (game.player.dashTimer <= 0.0f && e.hitTimer <= 0.0f) {
              game.player.health -= 10;
              e.health -= 50; // Damage the enemy too
              e.hitTimer = 0.2f;
              AtomicMax(game.hitShake.val, 0.5f);

              QueueSound(game.sfxHit);
              QueueExplosion(e.position, ORANGE);

              // Knockback
              Vector3 kb = Vector3Normalize(
                  Vector3Subtract(game.player.position, e.position));
              game.player.position =
                  Vector3Add(game.player.position, Vector3Scale(kb, 1.0f));
              e.position = Vector3Subtract(e.position, Vector3Scale(kb, 1.0f));

              if (e.health <= 0 && e.type != 2) {
                e.active = false;
                game.player.xp += (e.type == 3 ? 100 : 25);
                game.score += (e.type == 3 ? 150 : 50);
                QueueSound(game.sfxExplosion);
              }

              if (game.player.health <= 0)
                game.currentScreen = (int)SCREEN_GAMEOVER;
            } else if (game.player.dashTimer > 0.0f && e.type != 2) {
              // Dashing through kills weak enemies
              game.player.xp += (e.type == 3 ? 100 : 25);
              game.score += (e.type == 3 ? 150 : 50);
              e.active = false;
              QueueSound(game.sfxExplosion);
              QueueExplosion(e.position, ORANGE);
            }
          }

          for (auto &b : game.playerBullets) {
            if (b.active) {
              radius = (e.type == 2) ? 3.0f : (e.type == 3 ? 0.8f : 0.5f);
              float bulletDistSq = Vector3DistanceSqr(b.position, e.position);
              float combinedBulletR = b.radius + radius;
              if (bulletDistSq < combinedBulletR * combinedBulletR) {
                float damage = 20.0f * game.player.damageMult;
                bool isCrit = false;
                if ((float)GetRandomValue(0, 1000) / 1000.0f <
                    game.player.critChance) {
                  damage *= 2.0f;
                  isCrit = true;
                }
                e.health -= (int)damage;
                e.hitTimer = 0.1f;
                b.active = false;
                QueueExplosion(b.position, WHITE);

                float knockbackValue =
                    (e.type == 2) ? 0.1f : (e.type == 3 ? 0.2f : 0.5f);
                Vector3 knockDir =
                    Vector3Scale(Vector3Normalize(b.velocity), knockbackValue);
                e.position = Vector3Add(e.position, knockDir);

                if (isCrit) {
                  AtomicMax(game.hitStopTimer.val, 0.08f);
                  AtomicMax(game.hitShake.val, 0.3f);
                  QueueText(e.position, TextFormat("%d CRIT!", (int)damage),
                            GOLD);
                } else {
                  AtomicMax(game.hitStopTimer.val, 0.05f);
                  AtomicMax(game.hitShake.val, 0.15f);
                  QueueText(e.position, TextFormat("%d", (int)damage), WHITE);
                }

                if (e.health <= 0) {
                  AtomicMax(game.hitStopTimer.val, 0.12f);
                  AtomicMax(game.hitShake.val, 0.5f);
                  e.active = false;
                  QueueSound(game.sfxExplosion);
                  Color ec = (e.type == 2)   ? PURPLE
                             : (e.type == 3) ? DARKGREEN
                             : (e.type == 1) ? MAROON
                             : (e.type == 4) ? MAGENTA
                             : (e.type == 5) ? LIME
                             : (e.type == 6) ? SKYBLUE
                                             : RED;
                  QueueExplosion(e.position, ec);

                  if (e.type == 5) {
                    int spawned = 0;
                    for (auto &bit : game.enemies) {
                      if (!bit.active && spawned < 3) {
                        bit.active = true;
                        bit.type = 0;
                        bit.maxHealth = 40;
                        bit.health = 40;
                        bit.speed = 8.0f;
                        bit.position = Vector3Add(
                            e.position, {(float)GetRandomValue(-1, 1), 0,
                                         (float)GetRandomValue(-1, 1)});
                        bit.hitTimer = 0.0f;
                        bit.lastPosition = bit.position;
                        bit.stuckTimer = 0.0f;
                        spawned++;
                      }
                    }
                  }
                  game.player.xp +=
                      (e.type == 2 ? 500 : (e.type == 3 ? 100 : 25));
                  game.score += (e.type == 2 ? 1000 : (e.type == 3 ? 150 : 50));
                  CheckLevelUp();
                }
                break;
              }
            }
          }
        }
      }
    }));
  }

  // 4. Particle Update (Fine-Grained Partitioning: 8 Batches)
  const int particleBatchSize = MAX_PARTICLES / 8;
  for (int i = 0; i < 8; ++i) {
    int start = i * particleBatchSize;
    int end = (i == 7) ? MAX_PARTICLES : (i + 1) * particleBatchSize;
    futures.emplace_back(threadPool->enqueue([dt, start, end] {
      for (int j = start; j < end; ++j) {
        auto &p = game.particles[j];
        if (p.active) {
          p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
          p.life -= p.decay * dt;
          if (p.life <= 0)
            p.active = false;
        }
      }
    }));
  }

  // 5. Floating Text Update
  futures.emplace_back(threadPool->enqueue([dt] {
    for (auto &ft : game.floatingTexts) {
      if (ft.active) {
        ft.position.y += ft.speed * dt;
        ft.life -= dt;
        if (ft.life <= 0)
          ft.active = false;
      }
    }
  }));

  // Final Join
  for (auto &f : futures)
    f.get();

  // --- Camera Follow & Breathing ---
  float time = (float)GetTime();
  float breathing = sinf(time * 2.0f) * 0.2f; // Smooth floating effect

  game.camera.target = game.player.position;
  game.camera.position.x = game.player.position.x;
  game.camera.position.z = game.player.position.z + 20.0f;
  game.camera.position.y = 20.0f + breathing;

  // Heartbeat SFX (Low Health)
  if (game.player.health < 30 && (int)(time * 2.0f) % 2 == 0) {
    if (!IsSoundPlaying(game.sfxLowHealth))
      QueueSound(game.sfxLowHealth);
  }

  // Victory Condition
  if (game.wave > 25) {
    game.currentScreen = (int)SCREEN_VICTORY;
  }

  CheckLevelUp();
  ProcessEffectBuffer();
}

void DrawGame() {
  // 1. Draw 3D Scene to Texture
  BeginTextureMode(target);
  ClearBackground(BLACK);

  if (game.currentScreen == SCREEN_PLAYING) {
    // Screen Shake (Hit + Glitch)
    Vector3 shake = {0};
    if (game.player.health < 30) {
      shake.x += (float)GetRandomValue(-2, 2) / 10.0f;
      shake.y += (float)GetRandomValue(-2, 2) / 10.0f;
    }
    if (game.hitShake > 0) {
      shake.x += (float)GetRandomValue(-100, 100) / 100.0f * game.hitShake;
      shake.z += (float)GetRandomValue(-100, 100) / 100.0f * game.hitShake;
    }

    BeginMode3D(game.camera);
    rlPushMatrix();
    rlTranslatef(shake.x, shake.y, shake.z);

    // Draw Grid (Animated)
    float time = (float)GetTime();
    rlPushMatrix();
    rlTranslatef(0, 0, fmod(time * 5.0f, 2.0f));
    DrawGrid(40, 2.0f);
    rlPopMatrix();

    // Draw Crosshair (Mouse position on plane)
    Ray ray = GetMouseRay(GetMousePosition(), game.camera);
    float t = -ray.position.y / ray.direction.y;
    Vector3 groundPos =
        Vector3Add(ray.position, Vector3Scale(ray.direction, t));
    DrawCircle3D(groundPos, 0.5f, {0, 1, 0}, 90.0f, ColorAlpha(SKYBLUE, 0.5f));
    DrawCircle3D(groundPos, 0.2f, {0, 1, 0}, 90.0f, WHITE);

    // Draw Obstacles
    for (const auto &obs : game.obstacles) {
      if (obs.active) {
        DrawCube(obs.position, obs.size.x, obs.size.y, obs.size.z, obs.color);
        DrawCubeWires(obs.position, obs.size.x, obs.size.y, obs.size.z,
                      LIGHTGRAY);
      }
    }

    // Draw Player (Cursor)
    Color playerColor = game.player.dashTimer > 0.0f
                            ? GOLD
                            : (game.player.focusMode ? ORANGE : SKYBLUE);
    DrawCube(game.player.position, 1.0f, 1.0f, 1.0f, playerColor);
    DrawCubeWires(game.player.position, 1.0f, 1.0f, 1.0f, BLUE);

    // Focus Ring (Pulse in Slow-Mo)
    if (game.player.focusMode) {
      float ringWave = sinf(GetTime() * 10.0f) * 0.2f + 1.5f;
      DrawCircle3D(game.player.position, ringWave, {0, 1, 0}, 90.0f,
                   ColorAlpha(ORANGE, 0.4f));
    }

    // Draw Bullets (Player)
    for (const auto &b : game.playerBullets) {
      if (b.active) {
        // Trail
        DrawLine3D(
            b.position,
            Vector3Subtract(b.position,
                            Vector3Scale(Vector3Normalize(b.velocity), 1.0f)),
            b.color);
        // Glow
        DrawSphere(b.position, b.radius * 2.5f, ColorAlpha(b.color, 0.4f));
        // Core
        DrawSphere(b.position, b.radius, WHITE);
      }
    }
    // Draw Bullets (Enemy)
    for (const auto &b : game.enemyBullets) {
      if (b.active) {
        // Trail
        DrawLine3D(
            b.position,
            Vector3Subtract(b.position,
                            Vector3Scale(Vector3Normalize(b.velocity), 1.0f)),
            b.color);
        // Glow (Increased for better readability)
        DrawSphere(b.position, b.radius * 4.0f, ColorAlpha(b.color, 0.5f));
        // Core
        DrawSphere(b.position, b.radius, WHITE);
      }
    }

    // Draw Enemies
    for (const auto &enemy : game.enemies) {
      if (enemy.active) {
        Color ec;
        if (enemy.hitTimer > 0.0f) {
          ec = WHITE;
        } else {
          if (enemy.type == 2)
            ec = PURPLE;
          else if (enemy.type == 3)
            ec = DARKGREEN;
          else if (enemy.type == 1)
            ec = MAROON;
          else if (enemy.type == 4)
            ec = MAGENTA; // PHANTOM
          else if (enemy.type == 5)
            ec = LIME; // SPLITTER
          else if (enemy.type == 6)
            ec = SKYBLUE; // SUPPORT
          else
            ec = RED;
        }

        if (enemy.type == 2) { // boss
          DrawCube(enemy.position, 3.0f, 3.0f, 3.0f, ec);
          DrawCubeWires(enemy.position, 3.0f, 3.0f, 3.0f, DARKPURPLE);
        } else if (enemy.type == 3) { // Tank
          DrawCube(enemy.position, 1.5f, 1.5f, 1.5f, ec);
          DrawCubeWires(enemy.position, 1.5f, 1.5f, 1.5f, GREEN);
        } else {
          DrawCube(enemy.position, 1.0f, 1.0f, 1.0f, ec);
          DrawCubeWires(enemy.position, 1.0f, 1.0f, 1.0f, DARKGRAY);
        }

        // Debug Visuals
        if (game.debugMode) {
          float radius =
              (enemy.type == 2) ? 3.0f : (enemy.type == 3 ? 0.8f : 0.5f);
          DrawSphereWires(enemy.position, radius, 8, 8, GREEN);
        }
      }
    }

    // Draw Particles
    for (const auto &p : game.particles) {
      if (p.active) {
        Color c = p.color;
        c.a = (unsigned char)(255.0f * (p.life > 1.0f ? 1.0f : p.life));
        DrawCube(p.position, p.size, p.size, p.size, c);
      }
    }
    // Draw Floating Text (3D)
    for (const auto &ft : game.floatingTexts) {
      if (ft.active) {
        Vector2 screenPos = GetWorldToScreen(ft.position, game.camera);
        DrawText(ft.text, (int)screenPos.x - MeasureText(ft.text, 12) / 2,
                 (int)screenPos.y, 12, ColorAlpha(ft.color, ft.life));
      }
    }

    rlPopMatrix();
    EndMode3D();

  } else if (game.currentScreen == SCREEN_UPGRADE) {
    ClearBackground(BLACK); // Ensure clean background
    DrawText("SYSTEM OPTIMIZATION REQUIRED", 100, 100, 30, GREEN);
    DrawText("CHOOSE UPGRADE MODULE", 100, 150, 20, LIME);
    DrawText("> E - OVERCLOCK (SPEED++)", 150, 250, 20, WHITE);
    DrawText("> R - FIREWALL (HEALTH++)", 150, 300, 20, WHITE);
    DrawText("> F - MULTITHREAD (FIRE RATE++)", 150, 350, 20, WHITE);
    DrawText("PRESS ENTER TO CONTINUE (PLACEHOLDER)", 150, 500, 20, GRAY);

  } else if (game.currentScreen == SCREEN_GAMEOVER) {
    ClearBackground(BLACK);
    DrawText("FATAL SYSTEM ERROR", SCREEN_WIDTH / 2 - 150,
             SCREEN_HEIGHT / 2 - 50, 30, RED);
    DrawText(TextFormat("FINAL SCORE: %i", (int)game.score),
             SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 50, 25, GREEN);
    DrawText("PRESS R TO REBOOT", SCREEN_WIDTH / 2 - 100,
             SCREEN_HEIGHT / 2 + 40, 20, LIGHTGRAY);
  } else if (game.currentScreen == SCREEN_VICTORY) {
    if (IsKeyPressed(KEY_R)) {
      game.currentScreen = SCREEN_MENU;
      InitGame();
    }
    ClearBackground(BLACK);
    const char *vText = "SYSTEM PURIFIED";
    const char *vSub = "Wave 25 Cleared - Efficiency: 100%";

    // Pulsing effect for victory text
    float wave = sinf(GetTime() * 3.0f) * 0.1f + 1.0f;
    Color vCol = ColorAlpha(SKYBLUE, 0.8f + wave * 0.2f);

    DrawText(vText, SCREEN_WIDTH / 2 - MeasureText(vText, 40) / 2,
             SCREEN_HEIGHT / 2 - 40, 40, vCol);
    DrawText(vSub, SCREEN_WIDTH / 2 - MeasureText(vSub, 20) / 2,
             SCREEN_HEIGHT / 2 + 20, 20, GOLD);
    DrawText("PRESS R TO RESTART", SCREEN_WIDTH / 2 - 100,
             SCREEN_HEIGHT / 2 + 80, 20, GRAY);
  }
  EndTextureMode();

  // 2. Draw Texture to Screen with Shader
  BeginDrawing();
  ClearBackground(BLACK);
  BeginShaderMode(postProcessShader);
  // Note: RenderTextures are y-flipped in OpenGL
  DrawTextureRec(
      target.texture,
      {0, 0, (float)target.texture.width, (float)-target.texture.height},
      {0, 0}, WHITE);
  EndShaderMode();

  // 3. Draw UI on TOP of Shader (crisp text)
  if (game.currentScreen == SCREEN_PLAYING) {
    DrawText(TextFormat("WAVE: %i", game.wave), 20, 20, 20, PURPLE);
    DrawText(TextFormat("SCORE: %06i", (int)game.score), 20, 50, 20, GREEN);

    // Health Bar (with vibration)
    int hv = (game.hitShake > 0.1f) ? GetRandomValue(-4, 4) : 0;
    DrawRectangle(20 + hv, 80 + hv, 200, 20, DARKGRAY);
    DrawRectangle(
        20 + hv, 80 + hv,
        (int)(200.0f * ((float)game.player.health / game.player.maxHealth)), 20,
        RED);
    DrawRectangleLines(20, 80, 200, 20, WHITE);
    DrawText("CORE_INTEGRITY", 25, 82, 16, WHITE);

    // XP Bar (Bottom of screen)
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    // Holographic Glow for XP Bar
    DrawRectangle(0, sh - 22, sw, 2, ColorAlpha(SKYBLUE, 0.3f));
    DrawRectangle(0, sh - 20, sw, 20, ColorAlpha(DARKGRAY, 0.5f));
    DrawRectangle(0, sh - 20,
                  (int)(sw * (float)game.player.xp / game.player.xpToNextLevel),
                  20, SKYBLUE);
    // XP Bar Glow
    if (game.player.xp > 0) {
      int fill = (int)(sw * (float)game.player.xp / game.player.xpToNextLevel);
      DrawRectangle(0, sh - 20, fill, 2, ColorAlpha(WHITE, 0.4f));
    }
    DrawRectangleLines(0, sh - 20, sw, 20, WHITE);
    DrawText(TextFormat("LEVEL: %i", game.player.level), sw / 2 - 40, sh - 18,
             16, WHITE);

    // Dash Cooldown
    if (game.player.dashCooldown > 0.0f) {
      DrawRectangle(20, 110, (int)(100.0f * (game.player.dashCooldown / 1.0f)),
                    10, BLUE);
    } else {
      DrawText("DASH READY", 20, 110, 10, SKYBLUE);
    }

    // Boss Health Bar
    if (game.wave % 5 == 0) {
      float bossHealthPct = 0.0f;
      bool bossActive = false;
      for (const auto &e : game.enemies) {
        if (e.active && e.type == 2) {
          bossHealthPct = (float)e.health / e.maxHealth;
          bossActive = true;
          break;
        }
      }
      if (bossActive) {
        DrawText("WARNING: COMPILER DETECTED", GetScreenWidth() / 2 - 150, 50,
                 20, RED);
        DrawRectangle(GetScreenWidth() / 2 - 200, 80, 400, 30, DARKGRAY);
        DrawRectangle(GetScreenWidth() / 2 - 200, 80,
                      (int)(400.0f * bossHealthPct), 30, PURPLE);
        DrawRectangleLines(GetScreenWidth() / 2 - 200, 80, 400, 30, WHITE);
      }
    }

    if (game.debugMode) {
      DrawText("DEBUG MODE ACTIVE", 20, 140, 20, GREEN);
      DrawText("1:Bug 2:Sht 3:Boss 4:Tnk", 20, 160, 10, LIME);
    }
  }
  EndDrawing();
}

void DrawMenu() {
  BeginDrawing();
  ClearBackground(BLACK);

  // Title
  const char *title = "CURSOR";
  const char *subtitle = "Ascend the Code";
  int titleW = MeasureText(title, 80);
  int subtitleW = MeasureText(subtitle, 30);

  DrawText(title, GetScreenWidth() / 2 - titleW / 2, 180, 80, SKYBLUE);
  DrawText(subtitle, GetScreenWidth() / 2 - subtitleW / 2, 270, 30, ORANGE);

  // Instructions
  DrawText("Press SPACE to Start", GetScreenWidth() / 2 - 140, 380, 20, WHITE);
  DrawText("WASD: Move | Mouse: Aim & Shoot", GetScreenWidth() / 2 - 165, 440,
           16, LIGHTGRAY);
  DrawText("SPACE: Dash | SHIFT: Focus (Slow-Mo)", GetScreenWidth() / 2 - 175,
           465, 16, LIGHTGRAY);

  // Credits
  DrawText("Phase 1-7 by Gemini | Phase 8 by Claude",
           GetScreenWidth() / 2 - 160, 620, 12, DARKGRAY);
  DrawText("Made with Raylib 5.5", GetScreenWidth() / 2 - 85, 640, 12,
           DARKGRAY);

  EndDrawing();
}

void UpdateDrawFrame() {
  UpdateGame();

  // Handle Looping BGM
  if (!IsSoundPlaying(game.sfxBonus)) {
    QueueSound(game.sfxBonus);
  }

  if (game.currentScreen == SCREEN_MENU) {
    DrawMenu();
  } else {
    DrawGame();
  }
}

void SpawnExplosion(Vector3 pos, Color color) { QueueExplosion(pos, color); }

void ProcessEffectBuffer() {
  std::lock_guard<std::mutex> lock(bufferMutex);
  while (!effectBuffer.empty()) {
    EffectCommand cmd = effectBuffer.front();
    effectBuffer.pop_front();

    if (cmd.type == EffectCommand::EXPLOSION) {
      for (int i = 0; i < 20; ++i) {
        int idx = game.particleRollingIdx % MAX_PARTICLES;
        auto &p = game.particles[idx];
        p.active = true;
        p.position = cmd.pos;
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(5, 15) / 10.0f;
        p.velocity = {cosf(angle) * speed, (float)GetRandomValue(-5, 5) / 10.0f,
                      sinf(angle) * speed};
        p.color = cmd.color;
        p.size = (float)GetRandomValue(1, 4) / 10.0f;
        p.life = 1.0f;
        p.decay = (float)GetRandomValue(50, 100) / 10.0f;
        game.particleRollingIdx++;
      }
    } else if (cmd.type == EffectCommand::SOUND) {
      PlaySound(cmd.sfx);
    } else if (cmd.type == EffectCommand::TEXT) {
      for (auto &ft : game.floatingTexts) {
        if (!ft.active) {
          ft.active = true;
          ft.position = cmd.pos;
          strncpy(ft.text, cmd.text, 31);
          ft.text[31] = '\0';
          ft.color = cmd.color;
          ft.life = 1.0f;
          ft.speed = 2.0f;
          break;
        }
      }
    }
  }
}
