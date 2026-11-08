#include "Game.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>

// ==========================================
// DRAW HELPERS
// ==========================================

static void DrawTextCentered(const char *text, float centerX, float y,
                             int fontSize, Color color) {
  int w = MeasureText(text, fontSize);
  DrawText(text, (int)(centerX - w * 0.5f), (int)y, fontSize, color);
}

static void DrawTextCenteredInRect(const char *text, Rectangle rec,
                                   int fontSize, Color color) {
  int w = MeasureText(text, fontSize);
  int h = fontSize;
  DrawText(text, (int)(rec.x + (rec.width - w) * 0.5f),
           (int)(rec.y + (rec.height - h) * 0.5f), fontSize, color);
}

// ==========================================
// PROCEDURAL AUDIO SYNTHESIS HELPERS (PCM)
// ==========================================

static Sound GenerateSynthSound(float startFreq, float endFreq, float duration, float volume, int waveType) {
  int sampleRate = 44100;
  int frameCount = (int)(sampleRate * duration);
  short *buffer = new short[frameCount];
  float pi2 = 2.0f * PI;

  for (int i = 0; i < frameCount; i++) {
    float t = (float)i / (float)frameCount;
    // Swept-frequency phase (linear chirp). The instantaneous frequency is
    // startFreq + (endFreq - startFreq) * t, integrated over time.
    float phase = pi2 * (startFreq * t + 0.5f * (endFreq - startFreq) * t * t) * duration;

    float sample = 0.0f;
    if (waveType == 0) { // Sine wave
      sample = sinf(phase);
    } else if (waveType == 1) { // Triangle wave
      sample = (2.0f / PI) * asinf(sinf(phase));
    } else if (waveType == 2) { // Square wave
      sample = (sinf(phase) >= 0.0f) ? 0.65f : -0.65f;
    } else if (waveType == 3) { // White noise
      sample = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f);
    }

    float env = powf(1.0f - t, 1.8f);
    float finalVal = sample * env * volume * 32767.0f;
    if (finalVal > 32767.0f) finalVal = 32767.0f;
    if (finalVal < -32767.0f) finalVal = -32767.0f;
    buffer[i] = (short)finalVal;
  }

  Wave wave = {(unsigned int)frameCount, (unsigned int)sampleRate, 16, 1, buffer};
  Sound snd = LoadSoundFromWave(wave);
  delete[] buffer;
  return snd;
}

static Sound GenerateExplosionSound(float duration, float volume) {
  int sampleRate = 44100;
  int frameCount = (int)(sampleRate * duration);
  short *buffer = new short[frameCount];
  float pi2 = 2.0f * PI;

  float lastNoise = 0.0f;
  for (int i = 0; i < frameCount; i++) {
    float t = (float)i / (float)frameCount;
    float noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f);
    lastNoise = lastNoise * 0.75f + noise * 0.25f;

    float subPhase = pi2 * (120.0f * t + 0.5f * (40.0f - 120.0f) * t * t) * duration;
    float sub = sinf(subPhase) * 0.5f;

    float env = powf(1.0f - t, 1.5f);
    float sample = (lastNoise * 0.7f + sub * 0.5f) * env * volume * 32767.0f;
    if (sample > 32767.0f) sample = 32767.0f;
    if (sample < -32767.0f) sample = -32767.0f;
    buffer[i] = (short)sample;
  }

  Wave wave = {(unsigned int)frameCount, (unsigned int)sampleRate, 16, 1, buffer};
  Sound snd = LoadSoundFromWave(wave);
  delete[] buffer;
  return snd;
}

static Sound GenerateChimeSound(float f1, float f2, float duration, float volume) {
  int sampleRate = 44100;
  int frameCount = (int)(sampleRate * duration);
  short *buffer = new short[frameCount];
  float pi2 = 2.0f * PI;

  for (int i = 0; i < frameCount; i++) {
    float t = (float)i / (float)frameCount;
    float freq = (t < 0.35f) ? f1 : f2;
    float phase = pi2 * freq * (float)i / (float)sampleRate;
    float sample = sinf(phase) * 0.75f + sinf(phase * 2.0f) * 0.25f;

    float env = (t < 0.35f) ? (1.0f - (t / 0.35f) * 0.3f) : powf(1.0f - ((t - 0.35f) / 0.65f), 1.5f);
    float finalVal = sample * env * volume * 32767.0f;
    if (finalVal > 32767.0f) finalVal = 32767.0f;
    if (finalVal < -32767.0f) finalVal = -32767.0f;
    buffer[i] = (short)finalVal;
  }

  Wave wave = {(unsigned int)frameCount, (unsigned int)sampleRate, 16, 1, buffer};
  Sound snd = LoadSoundFromWave(wave);
  delete[] buffer;
  return snd;
}

static Sound GenerateSirenSound(float fMin, float fMax, float duration, float volume, float cycles) {
  int sampleRate = 44100;
  int frameCount = (int)(sampleRate * duration);
  short *buffer = new short[frameCount];
  float pi2 = 2.0f * PI;

  float phase = 0.0f;
  for (int i = 0; i < frameCount; i++) {
    float t = (float)i / (float)frameCount;
    float lfo = 0.5f + 0.5f * sinf(pi2 * cycles * t);
    float freq = fMin + (fMax - fMin) * lfo;
    phase += pi2 * freq / (float)sampleRate;

    float sample = (sinf(phase) >= 0.0f) ? 0.6f : -0.6f;
    float env = powf(1.0f - t, 0.8f);
    float finalVal = sample * env * volume * 32767.0f;
    if (finalVal > 32767.0f) finalVal = 32767.0f;
    if (finalVal < -32767.0f) finalVal = -32767.0f;
    buffer[i] = (short)finalVal;
  }

  Wave wave = {(unsigned int)frameCount, (unsigned int)sampleRate, 16, 1, buffer};
  Sound snd = LoadSoundFromWave(wave);
  delete[] buffer;
  return snd;
}

Game::Game()
    // NOTE: initializer order here must mirror the member declaration order in
    // Game.h, otherwise GCC warns (-Wreorder) and members can be read before
    // they are constructed.
    : screenWidth(1280), screenHeight(720), money(500), baseHP(10),
      maxBaseHP(10), selectedTowerType(TowerType::BASIC), isBuildMode(false),
      btnBasic{340.0f, 584.0f, 190.0f, 106.0f},
      btnRapid{545.0f, 584.0f, 190.0f, 106.0f},
      btnCannon{750.0f, 584.0f, 190.0f, 106.0f}, selectedTowerIndex(-1),
      infoPanelRec{15.0f, 385.0f, 245.0f, 320.0f},
      btnUpgrade{28.0f, 612.0f, 219.0f, 42.0f},
      btnSell{28.0f, 662.0f, 219.0f, 35.0f}, notEnoughMoneyTimer(0.0f), wave(1),
      enemiesToSpawn(5), enemiesSpawned(0), spawnTimer(2.0f),
      spawnInterval(1.0f), waveInProgress(false), nextWaveTimer(0.0f),
      bossWaveTimer(0.0f), waveReadyToStart(true),
      btnStartWave{535.0f, 355.0f, 210.0f, 42.0f},
      lastPlacement{{0,0}, TowerType::BASIC, 0, false}, canUndo(false),
      gameOver(false), isPaused(false), victory(false),
      endlessMode(false), gameSpeed(1.0f),
      highScoreWave(0), highScoreMoney(0),
      screenShakeIntensity(0.0f), screenShakeTimer(0.0f), screenShakeOffset({0, 0}),
      hitPauseTimer(0.0f), hitPauseDuration(0.0f),
      timeScale(1.0f), timeScaleTarget(1.0f), timeScaleLerpSpeed(8.0f), timeScaleRestoreTimer(0.0f),
      masterVolume(0.8f), sfxVolume(1.0f), musicVolume(0.5f), fullscreen(false),
      windowWidth(1280), windowHeight(720), showSettings(false),
      btnPause{772.0f, 16.0f, 42.0f, 38.0f},
      btnSpeed{820.0f, 16.0f, 44.0f, 38.0f},
      btnRestartGameOver{535.0f, 368.0f, 210.0f, 42.0f},
      btnResumeModal{460.0f, 270.0f, 170.0f, 42.0f},
      btnRestartModal{650.0f, 270.0f, 170.0f, 42.0f},
      btnSaveModal{460.0f, 330.0f, 170.0f, 42.0f},
      btnLoadModal{650.0f, 330.0f, 170.0f, 42.0f},
      btnSettingsModal{460.0f, 390.0f, 360.0f, 42.0f},
      btnBackSettings{430.0f, 482.0f, 420.0f, 42.0f},
      btnFullscreenToggle{430.0f, 335.0f, 420.0f, 42.0f},
      btnResolutionDropdown{430.0f, 412.0f, 420.0f, 42.0f},
      masterVolSlider{430.0f, 165.0f, 320.0f, 18.0f},
      sfxVolSlider{430.0f, 225.0f, 320.0f, 18.0f},
      musicVolSlider{430.0f, 285.0f, 320.0f, 18.0f},
      btnQuitModal{460.0f, 450.0f, 360.0f, 42.0f},
      btnVictoryRestart{485.0f, 410.0f, 145.0f, 42.0f},
      btnVictoryContinue{650.0f, 410.0f, 145.0f, 42.0f},
      btnSellConfirmYes{535.0f, 400.0f, 100.0f, 42.0f},
      btnSellConfirmNo{645.0f, 400.0f, 100.0f, 42.0f},
      btnRestartConfirmYes{535.0f, 400.0f, 100.0f, 42.0f},
      btnRestartConfirmNo{645.0f, 400.0f, 100.0f, 42.0f},
      draggingMasterVol(false), draggingSfxVol(false), draggingMusicVol(false),
      showSellConfirm(false), pendingSellTowerIndex(-1),
      showRestartConfirm(false), pendingRestartFromPause(false),
      showTutorial(true), tutorialTimer(0.0f),
      tooltipText(""), tooltipPos({0, 0}), tooltipTimer(0.0f), showTooltip(false),
      uiWavePreviewTimer(0.0f), uiWavePreviewActive(false), uiTooltipFade(0.0f),
      uiPanelSlide(0.0f), uiPanelVisible(true) {

  InitWindow(screenWidth, screenHeight, "Vector Siege");
  SetExitKey(KEY_NULL);
  SetTargetFPS(60);

  // Load settings before initializing audio
  LoadSettings();

  // Initialize native procedural audio system FIRST
  InitAudioDevice();
  audio.shootBasic = GenerateSynthSound(800.0f, 320.0f, 0.08f, 0.35f, 0);
  audio.shootRapid = GenerateSynthSound(1200.0f, 650.0f, 0.045f, 0.25f, 1);
  audio.shootCannon = GenerateSynthSound(190.0f, 50.0f, 0.18f, 0.55f, 0);
  audio.explosion = GenerateExplosionSound(0.28f, 0.55f);
  audio.enemyDeath = GenerateSynthSound(480.0f, 160.0f, 0.09f, 0.35f, 1);

  sndBuild = GenerateChimeSound(520.0f, 780.0f, 0.18f, 0.45f);
  sndSell = GenerateChimeSound(660.0f, 990.0f, 0.16f, 0.40f);
  sndBaseDamage = GenerateSynthSound(130.0f, 65.0f, 0.22f, 0.55f, 2);
  sndBossWave = GenerateSirenSound(420.0f, 720.0f, 0.75f, 0.55f, 3.0f);
  sndGameOver = GenerateSynthSound(340.0f, 110.0f, 0.65f, 0.50f, 0);
  SetSoundVolume(audio.shootBasic, sfxVolume);
  SetSoundVolume(audio.shootRapid, sfxVolume);
  SetSoundVolume(audio.shootCannon, sfxVolume);
  SetSoundVolume(audio.explosion, sfxVolume);
  SetSoundVolume(audio.enemyDeath, sfxVolume);
  SetSoundVolume(sndBuild, sfxVolume);
  SetSoundVolume(sndSell, sfxVolume);
  SetSoundVolume(sndBaseDamage, sfxVolume);
  SetSoundVolume(sndBossWave, sfxVolume);
  SetSoundVolume(sndGameOver, sfxVolume);

  // Apply window/fullscreen settings AFTER audio is ready
  ApplySettings();

  // Initialize 3D Top-Down Orthographic Camera (True Top-Down view)
  // Pitch = 80.0 degrees: Almost vertical from above, keeping map axes 100% rectilinear
  // (no diagonal isometric slant, roads stay strictly horizontal and vertical),
  // while revealing 3D tower bodies, pedestals, barrels, and heights.
  const float pitchDeg = 80.0f;
  const float pitchRad = pitchDeg * DEG2RAD;
  const float camDist = 45.0f;

  // Path center in screen coords: first(0,150) to last(1280,500) -> center(640, 325)
  // Convert to world: To3D(640, 325) = (0, 0, -1.575)
  const Vector3 pathCenterWorld = {0.0f, 0.0f, -1.575f};
  
  camera.target     = pathCenterWorld;
  camera.position   = Vector3{ pathCenterWorld.x, camDist * sinf(pitchRad), pathCenterWorld.z + camDist * cosf(pitchRad) };
  camera.up         = Vector3{ 0.0f, cosf(pitchRad), -sinf(pitchRad) };
  camera.fovy       = 32.4f;
  camera.projection = CAMERA_ORTHOGRAPHIC;
  globalTime = 0.0f;
  path = {{0, 150},   {250, 150}, {250, 400},  {550, 400}, {550, 200},
          {850, 200}, {850, 500}, {1100, 500}, {1280, 500}};
}

Game::~Game() {
  UnloadSound(audio.shootBasic);
  UnloadSound(audio.shootRapid);
  UnloadSound(audio.shootCannon);
  UnloadSound(audio.explosion);
  UnloadSound(audio.enemyDeath);

  UnloadSound(sndBuild);
  UnloadSound(sndSell);
  UnloadSound(sndBaseDamage);
  UnloadSound(sndBossWave);
  UnloadSound(sndGameOver);

  CloseAudioDevice();
  CloseWindow();
}

void Game::Restart() {
  money = 500;
  baseHP = maxBaseHP;
  wave = 1;
  enemiesToSpawn = 5;
  enemiesSpawned = 0;
  spawnTimer = 2.0f;
  waveInProgress = false;
  nextWaveTimer = 0.0f;
  bossWaveTimer = 0.0f;
  gameOver = false;
  victory = false;
  endlessMode = false;
  isPaused = false;
  gameSpeed = 1.0f;
  selectedTowerIndex = -1;
  isBuildMode = false;
  notEnoughMoneyTimer = 0.0f;
  globalTime = 0.0f;
  waveReadyToStart = true;
  canUndo = false;
  lastPlacement.valid = false;

  enemies.clear();
  towers.clear();
  bullets.clear();
  explosions.clear();
  particles.clear();
  floatingTexts.clear();

}

// 3D Mouse Ray Picking: project mouse cursor onto the Y=0 ground plane
Vector2 Game::GetMouseWorld2D() const {
  Vector2 mouse = GetMousePosition();
  Ray ray = GetScreenToWorldRay(mouse, camera);
  // Intersect with plane Y=0
  if (fabsf(ray.direction.y) < 0.0001f) return mouse; // Parallel guard
  float t = -ray.position.y / ray.direction.y;
  float wx = ray.position.x + ray.direction.x * t;
  float wz = ray.position.z + ray.direction.z * t;
  // Convert 3D world coords back to 2D game coords
  return Vector2{ wx / Design::TO3D_SCALE + 640.0f, wz / Design::TO3D_SCALE + 360.0f };
}

void Game::AddFloatingText(Vector2 pos, const char* text, Color color, float duration) {
  FloatingText ft;
  ft.position = pos;
  TextCopy(ft.text, text);
  ft.color = color;
  ft.lifetime = duration;
  ft.maxLifetime = duration;
  ft.floatSpeed = 35.0f;
  floatingTexts.push_back(ft);
}

void Game::AddShockwaveParticles(Vector2 pos, Color color, int count) {
  for (int i = 0; i < count; i++) {
    float angle = (float)i * (360.0f / (float)count) * DEG2RAD;
    float spd = (float)GetRandomValue(60, 110);
    float lifetime = 0.35f + (float)GetRandomValue(0, 15) / 100.0f;
    particles.push_back({
      pos,
      Vector2{cosf(angle) * spd, sinf(angle) * spd},
      color,
      (float)GetRandomValue(3, 5),
      lifetime,
      lifetime
    });
  }
}

void Game::ToggleBuildMode(TowerType type) {
  if (!isBuildMode) {
    selectedTowerType = type;
    isBuildMode = true;
    selectedTowerIndex = -1;
  } else {
    if (selectedTowerType == type) {
      isBuildMode = false;
    } else {
      selectedTowerType = type;
      selectedTowerIndex = -1;
    }
  }
}

void Game::Run() {
  while (!WindowShouldClose()) {
    float deltaTime = GetFrameTime();
    Update(deltaTime);
    Draw();
  }
}

void Game::Update(float deltaTime) {
  Vector2 mouse = GetMousePosition();

  // =============================
  // GAME OVER STATE: HANDLE RESTART
  // =============================
  if (gameOver) {
    if (IsKeyPressed(KEY_R)) {
      showRestartConfirm = true;
      pendingRestartFromPause = false;
      return;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(mouse, btnRestartGameOver)) {
      showRestartConfirm = true;
      pendingRestartFromPause = false;
      return;
    }
    return;
  }

  // =============================
  // VICTORY STATE: HANDLE RESTART/CONTINUE
  // =============================
  if (victory) {
    if (IsKeyPressed(KEY_R)) {
      Restart();
      return;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (CheckCollisionPointRec(mouse, btnVictoryRestart)) {
        Restart();
        return;
      }
      if (CheckCollisionPointRec(mouse, btnVictoryContinue)) {
        // Continue to endless mode: keep going from the wave that was just
        // cleared (progress is NOT reset to 50) and use the normal scaling.
        victory = false;
        endlessMode = true;              // never trigger victory again
        wave = 51;                       // first endless wave after beating 50
        enemiesToSpawn = 5 + (wave - 1) * 2;
        enemiesSpawned = 0;
        spawnTimer = 0.5f;
        waveInProgress = true;
        waveReadyToStart = false;
        nextWaveTimer = 0.0f;
        if (IsBossWave(wave)) {
          bossWaveTimer = 3.0f;
          PlaySound(sndBossWave);
        }
        return;
      }
    }
    return;
  }

  // =============================
  // PAUSE & SPEED KEYBOARD SHORTCUTS (only when not in menus)
  // =============================
  if (!showSettings && !showSellConfirm && !showRestartConfirm && !gameOver && !victory) {
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)) {
      isPaused = !isPaused;
    }
    if (!isPaused) {
      if (IsKeyPressed(KEY_F) || IsKeyPressed(KEY_TAB)) {
        if (gameSpeed == 1.0f) gameSpeed = 2.0f;
        else if (gameSpeed == 2.0f) gameSpeed = 3.0f;
        else gameSpeed = 1.0f;
      }

      // Start wave with Enter
      if (waveReadyToStart && !waveInProgress) {
        if (IsKeyPressed(KEY_ENTER)) {
          waveReadyToStart = false;
          waveInProgress = true;
          if (enemiesSpawned > 0) wave++;
          enemiesSpawned = 0;
          spawnTimer = 0.5f;
          
          if (wave > 50 && !endlessMode) {
            victory = true;
            wave = 50;
            CheckAndUpdateHighScores();
          } else {
            enemiesToSpawn = 5 + (wave - 1) * 2;
            if (IsBossWave(wave)) {
              bossWaveTimer = 3.0f;
              PlaySound(sndBossWave);
            }
          }
        }
      }
    }
  }
  if (IsKeyPressed(KEY_F5)) {
    SaveGame();
  }
  if (IsKeyPressed(KEY_F9)) {
    LoadGame();
  }
  // Dismiss tutorial
  if (showTutorial && (IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER))) {
    showTutorial = false;
    tutorialTimer = 0.0f;
  }
  // Update tutorial timer
  if (showTutorial && !gameOver && !victory) {
    tutorialTimer += deltaTime;
  }
  // Tower shortcuts (when tower selected, not in build mode, not in menus)
  if (!isBuildMode && !isPaused && !showSettings && !showSellConfirm && !gameOver && !victory && selectedTowerIndex >= 0 && selectedTowerIndex < (int)towers.size()) {
    if (IsKeyPressed(KEY_U)) {
      Tower &selTower = towers[selectedTowerIndex];
      if (selTower.CanUpgrade()) {
        int upCost = selTower.GetUpgradeCost();
        if (money >= upCost) {
          money -= upCost;
          Vector2 towerPos = selTower.GetPosition();
          int newLevel = selTower.GetLevel() + 1;
          selTower.Upgrade();
          PlaySound(sndBuild);
          
          Color upgradeColor = (selTower.GetType() == TowerType::BASIC) ? Color{34, 197, 94, 255} :
                               (selTower.GetType() == TowerType::RAPID) ? Color{14, 165, 233, 255} : Color{239, 68, 68, 255};
          AddFloatingText(towerPos, TextFormat("-$%d", upCost), Color{6, 182, 212, 255});
          AddShockwaveParticles(towerPos, Color{250, 204, 21, 255}, 24);
          
          for (int i = 0; i < 30; i++) {
            float angle = (float)i * (360.0f / 30.0f) * DEG2RAD;
            float spd = (float)GetRandomValue(80, 150);
            float lifetime = 0.5f + (float)GetRandomValue(0, 30) / 100.0f;
            particles.push_back({
              towerPos,
              Vector2{cosf(angle) * spd, sinf(angle) * spd},
              (i % 3 == 0) ? Color{250, 204, 21, 255} : upgradeColor,
              (float)GetRandomValue(4, 8),
              lifetime,
              lifetime
            });
          }
          
          for (int i = 0; i < 15; i++) {
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float spd = (float)GetRandomValue(50, 100);
            float lifetime = 0.8f + (float)GetRandomValue(0, 20) / 100.0f;
            particles.push_back({
              towerPos,
              Vector2{cosf(angle) * spd, sinf(angle) * spd - (float)GetRandomValue(100, 200)},
              Color{250, 204, 21, 255},
              (float)GetRandomValue(3, 6),
              lifetime,
              lifetime
            });
          }
          
          AddFloatingText(Vector2{towerPos.x, towerPos.y - 40}, TextFormat("LEVEL %d!", newLevel), Color{250, 204, 21, 255}, 1.5f);
        } else {
          notEnoughMoneyTimer = 1.5f;
        }
      }
    }
    if (IsKeyPressed(KEY_S)) {
      if (!showSellConfirm) {
        showSellConfirm = true;
        pendingSellTowerIndex = selectedTowerIndex;
      }
    }
    if (IsKeyPressed(KEY_T)) {
      towers[selectedTowerIndex].CycleTargetingMode();
      AddFloatingText(towers[selectedTowerIndex].GetPosition(), 
        TextFormat("TARGET: %s", towers[selectedTowerIndex].GetTargetingModeName()), 
        Color{0, 200, 240, 255}, 1.0f);
    }
  }

  // Undo placement (Z key)
  if (!isBuildMode && !isPaused && !showSettings && !showSellConfirm && !gameOver && !victory && canUndo) {
    if (IsKeyPressed(KEY_Z)) {
      // Remove last placed tower and refund
      if (!towers.empty()) {
        Tower& lastTower = towers.back();
        if (Vector2Distance(lastTower.GetPosition(), lastPlacement.position) < 1.0f &&
            lastTower.GetType() == lastPlacement.type) {
          towers.pop_back();
          money += lastPlacement.cost;
          PlaySound(sndSell);
          AddFloatingText(lastPlacement.position, TextFormat("+$%d (UNDO)", lastPlacement.cost), Color{34, 197, 94, 255});
          AddShockwaveParticles(lastPlacement.position, Color{239, 68, 68, 255}, 12);
          canUndo = false;
          lastPlacement.valid = false;
        }
      }
    }
  }

  // =============================
  // TOOLTIP SYSTEM
  // =============================
  // Update tooltip timer
  if (tooltipTimer > 0.0f) {
    tooltipTimer -= deltaTime;
    if (tooltipTimer <= 0.0f) {
      showTooltip = false;
    }
  }

  // Check for tooltip triggers
  showTooltip = false;
  Vector2 worldMouse = GetMouseWorld2D();
  
  // Tower tooltips
  for (size_t i = 0; i < towers.size(); i++) {
    if (Vector2Distance(worldMouse, towers[i].GetPosition()) <= Design::TOWER_CLICK_RADIUS) {
      Tower& t = towers[i];
      tooltipText = TextFormat("%s Tower (Lv.%d)\nDMG: %d | RNG: %.0f | CD: %.1fs\nTarget: %s\n[U] Upgrade ($%d) | [S] Sell ($%d) | [T] Target",
        Tower::GetTowerName(t.GetType()), t.GetLevel(),
        t.GetDamage(), t.GetRange(), t.GetCooldown(),
        t.GetTargetingModeName(),
        t.GetUpgradeCost(), t.GetSellValue());
      tooltipPos = Vector2{mouse.x + 15, mouse.y + 15};
      tooltipTimer = 0.3f;
      showTooltip = true;
      break;
    }
  }
  
  // Enemy tooltips
  if (!showTooltip) {
    for (const auto& enemy : enemies) {
      if (!enemy.IsAlive() || enemy.IsDying()) continue;
      if (Vector2Distance(worldMouse, enemy.GetPosition()) <= enemy.GetRadius() + 5) {
        tooltipText = TextFormat("%s\nHP: %d/%d | SPD: %.0f | Reward: $%d",
          (enemy.GetType() == EnemyType::NORMAL) ? "Normal" :
          (enemy.GetType() == EnemyType::FAST) ? "Fast" :
          (enemy.GetType() == EnemyType::TANK) ? "Tank" : "BOSS",
          enemy.GetHp(), enemy.GetMaxHp(), 
          (enemy.GetType() == EnemyType::FAST) ? 140.0f :
          (enemy.GetType() == EnemyType::TANK) ? 45.0f :
          (enemy.GetType() == EnemyType::BOSS) ? 35.0f : 80.0f,
          enemy.GetReward());
        tooltipPos = Vector2{mouse.x + 15, mouse.y + 15};
        tooltipTimer = 0.3f;
        showTooltip = true;
        break;
      }
    }
  }

  // =============================
  // TOP HUD CONTROLS CLICK (PAUSE & SPEED)
  // =============================
  if (!showSettings && !showSellConfirm && !showRestartConfirm && !gameOver && !victory) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (CheckCollisionPointRec(mouse, btnPause)) {
        isPaused = !isPaused;
        return;
      }
      if (CheckCollisionPointRec(mouse, btnSpeed)) {
        if (gameSpeed == 1.0f) gameSpeed = 2.0f;
        else if (gameSpeed == 2.0f) gameSpeed = 3.0f;
        else gameSpeed = 1.0f;
        return;
      }
    }
  }

  // =============================
  // PAUSED STATE: MODAL INTERACTION
  // =============================
  if (isPaused && !showSettings && !showSellConfirm && !showRestartConfirm) {
    if (IsKeyPressed(KEY_R)) {
      Restart();
      return;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (CheckCollisionPointRec(mouse, btnResumeModal)) {
        isPaused = false;
        return;
      }
      if (CheckCollisionPointRec(mouse, btnRestartModal)) {
        showRestartConfirm = true;
        pendingRestartFromPause = true;
        return;
      }
      if (CheckCollisionPointRec(mouse, btnSaveModal)) {
        SaveGame();
        return;
      }
      if (CheckCollisionPointRec(mouse, btnLoadModal)) {
        LoadGame();
        return;
      }
      if (CheckCollisionPointRec(mouse, btnSettingsModal)) {
        showSettings = true;
        return;
      }
      if (CheckCollisionPointRec(mouse, btnQuitModal)) {
        CloseWindow();
        exit(0);
      }
    }
    return; // Bekukan seluruh game logic saat paused
  }

  // =============================
  // SETTINGS MENU INTERACTION
  // =============================
  if (showSettings) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
      showSettings = false;
      SaveSettings();
      return;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (CheckCollisionPointRec(mouse, btnBackSettings)) {
        showSettings = false;
        SaveSettings();
        return;
      }
      if (CheckCollisionPointRec(mouse, btnFullscreenToggle)) {
        ToggleFullscreen();
        return;
      }
    }
    // Volume sliders drag handling
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      Vector2 mousePos = GetMousePosition();
      if (CheckCollisionPointRec(mousePos, masterVolSlider) || draggingMasterVol) {
        draggingMasterVol = true;
        float relX = (mousePos.x - masterVolSlider.x) / masterVolSlider.width;
        masterVolume = std::clamp(relX, 0.0f, 1.0f);
        SetMasterVolume(masterVolume);
      } else if (CheckCollisionPointRec(mousePos, sfxVolSlider) || draggingSfxVol) {
        draggingSfxVol = true;
        float relX = (mousePos.x - sfxVolSlider.x) / sfxVolSlider.width;
        sfxVolume = std::clamp(relX, 0.0f, 1.0f);
        SetSoundVolume(audio.shootBasic, sfxVolume);
        SetSoundVolume(audio.shootRapid, sfxVolume);
        SetSoundVolume(audio.shootCannon, sfxVolume);
        SetSoundVolume(audio.explosion, sfxVolume);
        SetSoundVolume(audio.enemyDeath, sfxVolume);
        SetSoundVolume(sndBuild, sfxVolume);
        SetSoundVolume(sndSell, sfxVolume);
        SetSoundVolume(sndBaseDamage, sfxVolume);
        SetSoundVolume(sndBossWave, sfxVolume);
        SetSoundVolume(sndGameOver, sfxVolume);
      } else if (CheckCollisionPointRec(mousePos, musicVolSlider) || draggingMusicVol) {
        draggingMusicVol = true;
        float relX = (mousePos.x - musicVolSlider.x) / musicVolSlider.width;
        musicVolume = std::clamp(relX, 0.0f, 1.0f);
      }
    } else {
      draggingMasterVol = false;
      draggingSfxVol = false;
      draggingMusicVol = false;
    }
    return;
  }

  // =============================
  // SELL CONFIRMATION DIALOG
  // =============================
  if (showSellConfirm) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_N)) {
      showSellConfirm = false;
      pendingSellTowerIndex = -1;
      return;
    }
    // [Y] shortcut: confirm sell (matches the "YES [Y]" button label).
    if (IsKeyPressed(KEY_Y)) {
      if (pendingSellTowerIndex >= 0 && pendingSellTowerIndex < (int)towers.size()) {
        Tower &selTower = towers[pendingSellTowerIndex];
        int sellVal = selTower.GetSellValue();
        money += sellVal;
        PlaySound(sndSell);
        Vector2 tPos = selTower.GetPosition();
        AddFloatingText(tPos, TextFormat("+$%d", sellVal), Color{34, 197, 94, 255});
        AddShockwaveParticles(tPos, Color{239, 68, 68, 255}, 14);
        towers.erase(towers.begin() + pendingSellTowerIndex);
      }
      showSellConfirm = false;
      pendingSellTowerIndex = -1;
      selectedTowerIndex = -1;
      return;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (CheckCollisionPointRec(mouse, btnSellConfirmYes)) {
        if (pendingSellTowerIndex >= 0 && pendingSellTowerIndex < (int)towers.size()) {
          Tower &selTower = towers[pendingSellTowerIndex];
          int sellVal = selTower.GetSellValue();
          money += sellVal;
          PlaySound(sndSell);
          Vector2 tPos = selTower.GetPosition();
          AddFloatingText(tPos, TextFormat("+$%d", sellVal), Color{34, 197, 94, 255});
          AddShockwaveParticles(tPos, Color{239, 68, 68, 255}, 14);

          towers.erase(towers.begin() + pendingSellTowerIndex);
        }
        showSellConfirm = false;
        pendingSellTowerIndex = -1;
        selectedTowerIndex = -1;
        return;
      }
      if (CheckCollisionPointRec(mouse, btnSellConfirmNo)) {
        showSellConfirm = false;
        pendingSellTowerIndex = -1;
        return;
      }
    }
    return;
  }

  // =============================
  // RESTART CONFIRMATION DIALOG
  // =============================
  if (showRestartConfirm) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_N)) {
      showRestartConfirm = false;
      pendingRestartFromPause = false;
      return;
    }
    // [Y] shortcut: confirm restart (matches the "YES [Y]" button label).
    if (IsKeyPressed(KEY_Y)) {
      Restart();
      showRestartConfirm = false;
      pendingRestartFromPause = false;
      return;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (CheckCollisionPointRec(mouse, btnRestartConfirmYes)) {
        Restart();
        showRestartConfirm = false;
        pendingRestartFromPause = false;
        return;
      }
      if (CheckCollisionPointRec(mouse, btnRestartConfirmNo)) {
        showRestartConfirm = false;
        pendingRestartFromPause = false;
        return;
      }
    }
    return;
  }

  // =============================
  // EFFECTIVE DELTA TIME (SPEED 1X / 2X) - base delta, screen juice modifies later
  // =============================
  float baseDelta = deltaTime * gameSpeed;
  globalTime += baseDelta;

  if (notEnoughMoneyTimer > 0.0f) {
    notEnoughMoneyTimer -= deltaTime;
  }

  if (bossWaveTimer > 0.0f) {
    bossWaveTimer -= baseDelta;
  }

  // =============================
  // SCREEN JUICE: Hit Pause, Time Scale, Camera Follow, Screen Shake
  // =============================

  // Hit pause (impact "juice") - previously froze the whole game for a few
  // frames, which made the game stutter constantly during rapid fire.
  // Now it only heavily scales down simulation time, so input, UI and
  // rendering keep running while the world briefly slows down.
  float hitPauseScale = 1.0f;
  if (hitPauseTimer > 0.0f) {
    hitPauseTimer -= deltaTime; // Use real deltaTime for hit pause
    if (hitPauseTimer <= 0.0f) {
      hitPauseTimer = 0.0f;
      hitPauseDuration = 0.0f;
    } else {
      // ~12% simulation speed during the impact window (no hard freeze).
      hitPauseScale = 0.12f;
    }
  }

  // Time scale lerp (smooth transitions for slowmo)
  if (timeScale != timeScaleTarget) {
    float diff = timeScaleTarget - timeScale;
    timeScale += diff * timeScaleLerpSpeed * deltaTime;
    if (fabsf(timeScale - timeScaleTarget) < 0.01f) timeScale = timeScaleTarget;
  }

  // Time scale auto-restore timer (uses real deltaTime)
  if (timeScaleRestoreTimer > 0.0f) {
    timeScaleRestoreTimer -= deltaTime;
    if (timeScaleRestoreTimer <= 0.0f) {
      timeScaleRestoreTimer = 0.0f;
      timeScaleTarget = 1.0f;
    }
  }

  float effectiveDelta = deltaTime * timeScale * gameSpeed * hitPauseScale;

  // Screen shake DISABLED for player comfort
  // if (screenShakeTimer > 0.0f) {
  //   screenShakeTimer -= effectiveDelta;
  //   float intensity = screenShakeIntensity * (screenShakeTimer / 0.5f);
  //   screenShakeOffset.x = (float)GetRandomValue(-100, 100) / 100.0f * intensity;
  //   screenShakeOffset.y = (float)GetRandomValue(-100, 100) / 100.0f * intensity;
  // } else {
  //   screenShakeOffset = {0, 0};
  // }
  screenShakeOffset = {0, 0}; // Always zero

  // =============================
  // CANCEL BUILD MODE (ESC / RIGHT-CLICK)
  // =============================
  if (isBuildMode) {
    if (IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      isBuildMode = false;
    }
  }

  // =============================
  // TOGGLE BUILD MODE VIA KEYBOARD (1, 2, 3)
  // =============================
  if (IsKeyPressed(KEY_ONE)) {
    ToggleBuildMode(TowerType::BASIC);
  } else if (IsKeyPressed(KEY_TWO)) {
    ToggleBuildMode(TowerType::RAPID);
  } else if (IsKeyPressed(KEY_THREE)) {
    ToggleBuildMode(TowerType::CANNON);
  }

  bool handledInput = false;

  // Pastikan index seleksi tower valid
  if (selectedTowerIndex >= (int)towers.size()) {
    selectedTowerIndex = -1;
  }

  // =============================
  // MOUSE CLICK HANDLING (LEFT-CLICK)
  // =============================
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    // 1. Cek klik pada Header Atas (y <= 70) -> abaikan untuk placement
    if (mouse.y <= 70.0f) {
      handledInput = true;
    }

    // 2. Cek klik pada Bottom Tower Cards & Console Container
    if (!handledInput) {
      if (CheckCollisionPointRec(mouse, btnBasic)) {
        ToggleBuildMode(TowerType::BASIC);
        handledInput = true;
      } else if (CheckCollisionPointRec(mouse, btnRapid)) {
        ToggleBuildMode(TowerType::RAPID);
        handledInput = true;
      } else if (CheckCollisionPointRec(mouse, btnCannon)) {
        ToggleBuildMode(TowerType::CANNON);
        handledInput = true;
      } else if (mouse.x >= 320.0f && mouse.x <= 960.0f && mouse.y >= 545.0f) {
        // Di dalam area container console bawah
        handledInput = true;
      }
    }

    // 3. Cek klik pada panel informasi tower (jika sedang terbuka)
    if (!handledInput && selectedTowerIndex >= 0 &&
        selectedTowerIndex < (int)towers.size()) {
      if (CheckCollisionPointRec(mouse, infoPanelRec)) {
        handledInput = true;

        // Tombol Upgrade
        if (CheckCollisionPointRec(mouse, btnUpgrade)) {
          Tower &selTower = towers[selectedTowerIndex];
          if (selTower.CanUpgrade()) {
            int upCost = selTower.GetUpgradeCost();
            if (money >= upCost) {
              money -= upCost;
              Vector2 towerPos = selTower.GetPosition();
              int newLevel = selTower.GetLevel() + 1;
              selTower.Upgrade();
              PlaySound(sndBuild);
              
              // Enhanced upgrade VFX
              Color upgradeColor = (selTower.GetType() == TowerType::BASIC) ? Color{34, 197, 94, 255} :
                                   (selTower.GetType() == TowerType::RAPID) ? Color{14, 165, 233, 255} : Color{239, 68, 68, 255};
              AddFloatingText(towerPos, TextFormat("-$%d", upCost), Color{6, 182, 212, 255});
              AddShockwaveParticles(towerPos, Color{250, 204, 21, 255}, 24);
              
              // Level-up particles
              for (int i = 0; i < 30; i++) {
                float angle = (float)i * (360.0f / 30.0f) * DEG2RAD;
                float spd = (float)GetRandomValue(80, 150);
                float lifetime = 0.5f + (float)GetRandomValue(0, 30) / 100.0f;
                particles.push_back({
                  towerPos,
                  Vector2{cosf(angle) * spd, sinf(angle) * spd},
                  (i % 3 == 0) ? Color{250, 204, 21, 255} : upgradeColor,
                  (float)GetRandomValue(4, 8),
                  lifetime,
                  lifetime
                });
              }
              
              // Rising particles
              for (int i = 0; i < 15; i++) {
                float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
                float spd = (float)GetRandomValue(50, 100);
                float lifetime = 0.8f + (float)GetRandomValue(0, 20) / 100.0f;
                particles.push_back({
                  towerPos,
                  Vector2{cosf(angle) * spd, sinf(angle) * spd - (float)GetRandomValue(100, 200)},
                  Color{250, 204, 21, 255},
                  (float)GetRandomValue(3, 6),
                  lifetime,
                  lifetime
                });
              }
              
              // Level up text
              AddFloatingText(Vector2{towerPos.x, towerPos.y - 40}, TextFormat("LEVEL %d!", newLevel), Color{250, 204, 21, 255}, 1.5f);
            } else {
              notEnoughMoneyTimer = 1.5f;
            }
          }
        }
        // Tombol Sell
        else if (CheckCollisionPointRec(mouse, btnSell)) {
          if (!showSellConfirm) {
            showSellConfirm = true;
            pendingSellTowerIndex = selectedTowerIndex;
          }
        }
      }
    }

    // 4. JIKA SEDANG DALAM BUILD MODE (isBuildMode == true)
    if (!handledInput && isBuildMode && mouse.y > 70.0f) {
      // Use 3D ray-picked position for actual placement
      Vector2 worldMouse = GetMouseWorld2D();
      bool onRoad = IsOnRoad(worldMouse);
      bool nearTower = IsNearTower(worldMouse);
      int currentCost = Tower::GetTowerCost(selectedTowerType);

      if (!onRoad && !nearTower) {
        if (money >= currentCost) {
          towers.emplace_back(worldMouse, selectedTowerType);
          money -= currentCost;
          PlaySound(sndBuild);
          Color tColor = (selectedTowerType == TowerType::BASIC) ? Color{34, 197, 94, 255} :
                         (selectedTowerType == TowerType::RAPID) ? Color{14, 165, 233, 255} : Color{239, 68, 68, 255};
          AddFloatingText(worldMouse, TextFormat("-$%d", currentCost), Color{239, 68, 68, 255});
          AddShockwaveParticles(worldMouse, tColor, 16);
          
          // Track for undo
          lastPlacement.position = worldMouse;
          lastPlacement.type = selectedTowerType;
          lastPlacement.cost = currentCost;
          lastPlacement.valid = true;
          canUndo = true;
          
          isBuildMode = false;
          handledInput = true;
        } else {
          notEnoughMoneyTimer = 1.5f;
          handledInput = true;
        }
      } else {
        handledInput = true;
      }
    }

    // 5. JIKA SEDANG DALAM NORMAL MODE (isBuildMode == false)
    if (!handledInput && !isBuildMode && mouse.y > 70.0f) {
      // Use 3D ray-picked position for tower hit-testing
      Vector2 worldMouse = GetMouseWorld2D();
      bool clickedExistingTower = false;
      for (size_t i = 0; i < towers.size(); i++) {
        if (Vector2Distance(worldMouse, towers[i].GetPosition()) <= Design::TOWER_CLICK_RADIUS) {
          selectedTowerIndex = (int)i;
          clickedExistingTower = true;
          handledInput = true;
          break;
        }
      }

      // Jika klik pada map kosong (bukan tower):
      if (!clickedExistingTower) {
        if (selectedTowerIndex >= 0) {
          selectedTowerIndex = -1;
          handledInput = true;
        }
      }
    }
  }

  // =============================
  // SPAWN ENEMY (DENGAN BOSS PADA BOSS WAVE)
  // =============================
  if (waveInProgress) {
    spawnTimer -= effectiveDelta;

    if (spawnTimer <= 0 && enemiesSpawned < enemiesToSpawn) {
      EnemyType typeToSpawn = EnemyType::NORMAL;

      if (IsBossWave(wave)) {
        // Pada Boss Wave: Boss muncul tepat SATU KALI di urutan terakhir wave
        if (enemiesSpawned == enemiesToSpawn - 1) {
          typeToSpawn = EnemyType::BOSS;
        } else if (enemiesSpawned % 3 == 0) {
          typeToSpawn = EnemyType::TANK;
        } else if (enemiesSpawned % 2 == 1) {
          typeToSpawn = EnemyType::FAST;
        } else {
          typeToSpawn = EnemyType::NORMAL;
        }
      } else if (wave == 1) {
        typeToSpawn = EnemyType::NORMAL;
      } else if (wave == 2) {
        if (enemiesSpawned % 2 == 1) {
          typeToSpawn = EnemyType::FAST;
        } else {
          typeToSpawn = EnemyType::NORMAL;
        }
      } else if (wave == 3) {
        if (enemiesSpawned == enemiesToSpawn - 1 ||
            enemiesSpawned == enemiesToSpawn / 2) {
          typeToSpawn = EnemyType::TANK;
        } else if (enemiesSpawned % 2 == 1) {
          typeToSpawn = EnemyType::FAST;
        } else {
          typeToSpawn = EnemyType::NORMAL;
        }
      } else {
        int mod = enemiesSpawned % 5;
        if (mod == 4) {
          typeToSpawn = EnemyType::TANK;
        } else if (mod == 1 || mod == 3) {
          typeToSpawn = EnemyType::FAST;
        } else {
          typeToSpawn = EnemyType::NORMAL;
        }
      }

      enemies.emplace_back(path[0], typeToSpawn);

      enemiesSpawned++;
      spawnTimer = spawnInterval;
    }
  }

  // =============================
  // CEK WAVE SELESAI
  // =============================
  if (waveInProgress && enemiesSpawned >= enemiesToSpawn && enemies.empty()) {
    waveInProgress = false;
    waveReadyToStart = true;
    nextWaveTimer = 0.0f; // Restart the "wave clear" stopwatch
    // Wave clear juice
    TriggerHitPause(0.05f);
    TriggerTimeScale(0.7f);
    TriggerScreenShake(3.0f, 0.3f);
  }

  // =============================
  // WAVE BERIKUTNYA
  // =============================
  // Between waves we wait for the player to press START. Track how long the
  // intermission has lasted so the HUD can show meaningful feedback instead
  // of a frozen "0.0s".
  if (!waveInProgress && waveReadyToStart && !gameOver && !victory) {
    nextWaveTimer += deltaTime;
  } else if (waveInProgress) {
    nextWaveTimer = 0.0f;
  }

  // Start wave when player clicks button
  if (waveReadyToStart && !waveInProgress && !gameOver && !victory) {
    Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, btnStartWave)) {
      waveReadyToStart = false;
      waveInProgress = true;
      if (enemiesSpawned > 0) wave++;
      enemiesSpawned = 0;
      spawnTimer = 0.5f;

      // Check for victory (wave 50). In endless mode the player keeps going.
      if (wave > 50 && !endlessMode) {
        victory = true;
        wave = 50;
        CheckAndUpdateHighScores();
        // Victory juice
        TriggerHitPause(0.2f);
        TriggerTimeScale(0.2f);
        TriggerScreenShake(20.0f, 1.5f);
      } else {
        enemiesToSpawn = 5 + (wave - 1) * 2;

        // Jika wave berikutnya adalah Boss Wave, aktifkan banner timer selama 3
        // detik
        if (IsBossWave(wave)) {
          bossWaveTimer = 3.0f;
          PlaySound(sndBossWave);
          TriggerHitPause(0.15f);
          TriggerTimeScale(0.3f);
          TriggerScreenShake(15.0f, 1.0f);
        }
      }
    }
  }

  // =============================
  // ENEMY MOVEMENT
  // =============================
  for (Enemy &enemy : enemies) {
    enemy.Update(effectiveDelta, baseDelta, path);
  }

  // =============================
  // ENEMY SAMPAI BASE
  // =============================
  for (Enemy &enemy : enemies) {
    if (enemy.HasReachedEnd(path.size())) {
      enemy.SetAlive(false);
      baseHP--;
      PlaySound(sndBaseDamage);
      TriggerHitPause(0.08f);
      TriggerTimeScale(0.5f);
      TriggerScreenShake(12.0f, 0.6f);
      AddFloatingText(Vector2{1190.0f, 470.0f}, "-1 HP", Color{239, 68, 68, 255}, 1.2f);
      AddShockwaveParticles(Vector2{1190.0f, 480.0f}, Color{239, 68, 68, 255}, 12);

      if (baseHP <= 0) {
        baseHP = 0;
        gameOver = true;
        PlaySound(sndGameOver);
        CheckAndUpdateHighScores();
      }
    }
  }

  // =============================
  // TOWER ATTACK
  // =============================
  for (Tower &tower : towers) {
    tower.Update(effectiveDelta, enemies, bullets, audio);
  }

  // =============================
  // BULLET MOVEMENT & COLLISION
  // =============================
  for (Bullet &bullet : bullets) {
    bullet.Update(effectiveDelta);
    bullet.CheckCollision(enemies, money, explosions, particles, floatingTexts, audio,
      [this](Vector2 /*pos*/, bool isKill, bool isCrit, Color /*bulletColor*/) {
        // Hit pause: brief impact slowdown (no longer a hard freeze)
        float pauseDur = isKill ? 0.06f : (isCrit ? 0.05f : 0.03f);
        TriggerHitPause(pauseDur);

        // Time scale: slight slowmo on crit/kill
        if (isCrit || isKill) {
          TriggerTimeScale(0.6f);
          // Auto-restore is handled by timeScaleRestoreTimer in Update().
        }

        // Screen shake on crit/kill
        if (isKill) {
          TriggerScreenShake(4.0f, 0.3f);
        } else if (isCrit) {
          TriggerScreenShake(2.5f, 0.2f);
        } else {
          TriggerScreenShake(1.5f, 0.15f);
        }
      });
  }

  // =============================
  // UPDATE EXPLOSIONS
  // =============================
  for (auto &exp : explosions) {
    exp.timer -= effectiveDelta;
  }
  explosions.erase(
      std::remove_if(explosions.begin(), explosions.end(),
                     [](const Explosion &exp) { return exp.timer <= 0.0f; }),
      explosions.end());

  // =============================
  // UPDATE PARTICLES & FLOATING TEXTS
  // =============================
  for (auto &p : particles) {
    p.position.x += p.velocity.x * effectiveDelta;
    p.position.y += p.velocity.y * effectiveDelta;
    p.velocity.x *= (1.0f - 2.5f * effectiveDelta);
    p.velocity.y *= (1.0f - 2.5f * effectiveDelta);
    p.lifetime -= effectiveDelta;
  }
  particles.erase(
      std::remove_if(particles.begin(), particles.end(),
                     [](const Particle &p) { return p.lifetime <= 0.0f; }),
      particles.end());

  // =============================
  // AMBIENT PARTICLES (wind, dust, pollen)
  // =============================
  // Spawn new ambient particles
  if (ambientParticles.size() < 200) {
    int spawnCount = 2;
    for (int i = 0; i < spawnCount; i++) {
      Vector2 spawnPos = { (float)GetRandomValue(0, screenWidth), (float)GetRandomValue(80, screenHeight) };
      float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
      float speed = (float)GetRandomValue(5, 25);
      ambientParticles.push_back({
        spawnPos,
        Vector2{cosf(angle) * speed, sinf(angle) * speed},
        Fade(Color{200, 220, 255, 255}, (float)GetRandomValue(30, 80) / 100.0f),
        (float)GetRandomValue(1, 3),
        (float)GetRandomValue(500, 1500) / 1000.0f,
        (float)GetRandomValue(500, 1500) / 1000.0f
      });
    }
  }

  // Update ambient particles
  for (auto &p : ambientParticles) {
    p.position.x += p.velocity.x * baseDelta; // Use baseDelta, not affected by time scale
    p.position.y += p.velocity.y * baseDelta;
    // Gentle wind drift
    p.velocity.x += sinf(globalTime * 0.5f + p.position.y * 0.01f) * 0.5f * baseDelta;
    p.velocity.y += cosf(globalTime * 0.7f + p.position.x * 0.01f) * 0.3f * baseDelta;
    p.lifetime -= baseDelta;
    // Wrap around screen
    if (p.position.x < -50) p.position.x = screenWidth + 50;
    if (p.position.x > screenWidth + 50) p.position.x = -50;
    if (p.position.y < 30) p.position.y = screenHeight + 50;
    if (p.position.y > screenHeight + 50) p.position.y = 30;
  }
  ambientParticles.erase(
      std::remove_if(ambientParticles.begin(), ambientParticles.end(),
                     [](const Particle &p) { return p.lifetime <= 0.0f; }),
      ambientParticles.end());

  // =============================
  // UI ANIMATIONS
  // =============================
  // Wave preview timer
  if (waveReadyToStart && !waveInProgress && !gameOver && !victory) {
    uiWavePreviewTimer += deltaTime;
    uiWavePreviewActive = true;
  } else {
    uiWavePreviewTimer = 0.0f;
    uiWavePreviewActive = false;
  }

  // Tooltip fade
  if (showTooltip) {
    uiTooltipFade = std::min(1.0f, uiTooltipFade + deltaTime * 8.0f);
  } else {
    uiTooltipFade = std::max(0.0f, uiTooltipFade - deltaTime * 8.0f);
  }

  // Panel slide
  if (selectedTowerIndex >= 0) {
    uiPanelSlide = std::min(1.0f, uiPanelSlide + deltaTime * 10.0f);
    uiPanelVisible = true;
  } else {
    uiPanelSlide = std::max(0.0f, uiPanelSlide - deltaTime * 10.0f);
    if (uiPanelSlide <= 0.0f) uiPanelVisible = false;
  }

  // Floating texts update
  for (auto &ft : floatingTexts) {
    ft.position.y -= ft.floatSpeed * effectiveDelta;
    ft.lifetime -= effectiveDelta;
  }
  floatingTexts.erase(
      std::remove_if(floatingTexts.begin(), floatingTexts.end(),
                     [](const FloatingText &ft) { return ft.lifetime <= 0.0f; }),
      floatingTexts.end());

  // =============================
  // HAPUS ENEMY MATI
  // =============================
  enemies.erase(
      std::remove_if(enemies.begin(), enemies.end(),
                     [](const Enemy &enemy) { return !enemy.IsAlive(); }),
      enemies.end());

  // =============================
  // HAPUS BULLET MATI
  // =============================
  bullets.erase(
      std::remove_if(bullets.begin(), bullets.end(),
                     [](const Bullet &bullet) { return !bullet.IsActive(); }),
      bullets.end());
}

// ============================================================================
// SCI-FI UI DRAWING HELPERS
// ============================================================================
static void DrawSciFiPanel(Rectangle rec, Color borderCol, Color bgCol,
                           float roundness = 0.08f, float glowAlpha = 0.25f) {
  if (glowAlpha > 0.0f) {
    DrawRectangleRounded(Rectangle{rec.x - 2.0f, rec.y - 2.0f, rec.width + 4.0f,
                                   rec.height + 4.0f},
                         roundness, 6, Fade(borderCol, glowAlpha * 0.4f));
  }
  DrawRectangleRounded(rec, roundness, 6, bgCol);
  DrawRectangleRoundedLinesEx(rec, roundness, 6, 1.5f, borderCol);
}

static void DrawIconSword(Vector2 pos, float size, Color col) {
  Vector2 tip{pos.x + size * 0.45f, pos.y - size * 0.45f};
  Vector2 pommel{pos.x - size * 0.45f, pos.y + size * 0.45f};
  DrawLineEx(pommel, tip, 2.0f, col);
  Vector2 guardMid{pos.x - size * 0.15f, pos.y + size * 0.15f};
  DrawLineEx(Vector2{guardMid.x - size * 0.25f, guardMid.y - size * 0.25f},
             Vector2{guardMid.x + size * 0.25f, guardMid.y + size * 0.25f},
             2.0f, col);
}

static void DrawIconTarget(Vector2 pos, float size, Color col) {
  DrawCircleLines(pos.x, pos.y, size * 0.48f, col);
  DrawCircle(pos.x, pos.y, 2.0f, col);
  DrawLineEx(Vector2{pos.x - size * 0.65f, pos.y},
             Vector2{pos.x + size * 0.65f, pos.y}, 1.2f, col);
  DrawLineEx(Vector2{pos.x, pos.y - size * 0.65f},
             Vector2{pos.x, pos.y + size * 0.65f}, 1.2f, col);
}

static void DrawIconClock(Vector2 pos, float size, Color col) {
  DrawCircleLines(pos.x, pos.y, size * 0.48f, col);
  DrawLineEx(pos, Vector2{pos.x, pos.y - size * 0.35f}, 1.5f, col);
  DrawLineEx(pos, Vector2{pos.x + size * 0.28f, pos.y}, 1.5f, col);
}

static void DrawIconAoE(Vector2 pos, float size, Color col) {
  DrawCircle(pos.x, pos.y, 2.5f, col);
  DrawCircleLines(pos.x, pos.y, size * 0.35f, Fade(col, 0.7f));
  DrawCircleLines(pos.x, pos.y, size * 0.6f, Fade(col, 0.4f));
  for (int i = 0; i < 4; i++) {
    float angle = i * 45.0f * DEG2RAD;
    Vector2 p1{pos.x + cosf(angle) * (size * 0.2f),
               pos.y + sinf(angle) * (size * 0.2f)};
    Vector2 p2{pos.x + cosf(angle) * (size * 0.65f),
               pos.y + sinf(angle) * (size * 0.65f)};
    DrawLineEx(p1, p2, 1.2f, col);
  }
}

static void DrawIconShield(Vector2 pos, float size, Color col) {
  Vector2 p1{pos.x, pos.y - size * 0.6f};
  Vector2 p2{pos.x + size * 0.55f, pos.y - size * 0.3f};
  Vector2 p3{pos.x + size * 0.45f, pos.y + size * 0.25f};
  Vector2 p4{pos.x, pos.y + size * 0.65f};
  Vector2 p5{pos.x - size * 0.45f, pos.y + size * 0.25f};
  Vector2 p6{pos.x - size * 0.55f, pos.y - size * 0.3f};
  DrawLineEx(p1, p2, 2.0f, col);
  DrawLineEx(p2, p3, 2.0f, col);
  DrawLineEx(p3, p4, 2.0f, col);
  DrawLineEx(p4, p5, 2.0f, col);
  DrawLineEx(p5, p6, 2.0f, col);
  DrawLineEx(p6, p1, 2.0f, col);
  DrawCircle(pos.x, pos.y, 2.5f, col);
}

static void DrawIconCoin(Vector2 pos, float size, Color col) {
  DrawCircle(pos.x, pos.y, size * 0.52f, Color{161, 98, 7, 255});
  DrawCircleLines(pos.x, pos.y, size * 0.52f, col);
  DrawCircleLines(pos.x, pos.y, size * 0.38f, col);
  DrawText("$", (int)pos.x - 4, (int)pos.y - 6, 13, WHITE);
}

static void DrawIconUpgradeArrow(Vector2 pos, float size, Color col) {
  Vector2 top{pos.x, pos.y - size * 0.5f};
  Vector2 left{pos.x - size * 0.42f, pos.y};
  Vector2 right{pos.x + size * 0.42f, pos.y};
  DrawTriangle(left, top, right, col);
  DrawRectangle((int)(pos.x - size * 0.16f), (int)pos.y, (int)(size * 0.32f),
                (int)(size * 0.45f), col);
}

static void DrawIconTrash(Vector2 pos, float size, Color col) {
  DrawRectangleRounded(Rectangle{pos.x - size * 0.32f, pos.y - size * 0.22f,
                                 size * 0.64f, size * 0.72f},
                       0.2f, 4, col);
  DrawRectangle((int)(pos.x - size * 0.42f), (int)(pos.y - size * 0.38f),
                (int)(size * 0.84f), (int)(size * 0.14f), col);
  DrawRectangle((int)(pos.x - size * 0.14f), (int)(pos.y - size * 0.52f),
                (int)(size * 0.28f), (int)(size * 0.14f), col);
  DrawLineEx(Vector2{pos.x - size * 0.12f, pos.y - size * 0.08f},
             Vector2{pos.x - size * 0.12f, pos.y + size * 0.32f}, 1.5f,
             Color{127, 29, 29, 255});
  DrawLineEx(Vector2{pos.x + size * 0.12f, pos.y - size * 0.08f},
             Vector2{pos.x + size * 0.12f, pos.y + size * 0.32f}, 1.5f,
             Color{127, 29, 29, 255});
}

static void DrawMiniTowerCardGraphic(Vector2 center, TowerType type,
                                     Color accent) {
  // Drop shadow
  DrawCircle(center.x, center.y + 4.0f, 15.0f, Fade(BLACK, 0.45f));
  // Glowing pedestal ring
  DrawRing(center, 11.0f, 14.0f, 0, 360, 20, Fade(accent, 0.65f));
  DrawCircle(center.x, center.y, 11.0f, Color{15, 23, 42, 255});

  if (type == TowerType::BASIC) {
    DrawCircle(center.x, center.y, 8.0f, Color{34, 197, 94, 255});
    DrawCircle(center.x, center.y, 4.0f, Color{187, 247, 208, 255});
    Vector2 barrelEnd{center.x, center.y - 13.0f};
    DrawLineEx(center, barrelEnd, 4.0f, Color{22, 101, 52, 255});
    DrawCircle(barrelEnd.x, barrelEnd.y, 2.5f, Color{74, 222, 128, 255});
  } else if (type == TowerType::RAPID) {
    DrawCircle(center.x, center.y, 8.0f, Color{14, 165, 233, 255});
    DrawCircle(center.x, center.y, 4.0f, Color{224, 242, 254, 255});
    DrawLineEx(Vector2{center.x - 3.5f, center.y},
               Vector2{center.x - 3.5f, center.y - 12.0f}, 2.5f,
               Color{3, 105, 161, 255});
    DrawLineEx(Vector2{center.x + 3.5f, center.y},
               Vector2{center.x + 3.5f, center.y - 12.0f}, 2.5f,
               Color{3, 105, 161, 255});
    DrawCircle(center.x - 3.5f, center.y - 12.0f, 2.0f,
               Color{56, 189, 248, 255});
    DrawCircle(center.x + 3.5f, center.y - 12.0f, 2.0f,
               Color{56, 189, 248, 255});
  } else {
    DrawCircle(center.x, center.y, 9.0f, Color{239, 68, 68, 255});
    DrawCircle(center.x, center.y, 4.5f, Color{254, 226, 226, 255});
    Vector2 barrelEnd{center.x - 9.0f, center.y - 10.0f};
    DrawLineEx(center, barrelEnd, 6.0f, Color{127, 29, 29, 255});
    DrawCircle(barrelEnd.x, barrelEnd.y, 3.5f, Color{248, 113, 113, 255});
  }
}

void Game::Draw() {
  BeginDrawing();

  // =================================
  // 3D WORLD SCENE - Dynamic Sky Mood
  // =================================
  // Sky color shifts based on wave progress (day -> dusk -> night)
  float waveProgress = (float)wave / 50.0f;
  if (waveProgress > 1.0f) waveProgress = 1.0f;
  // Base colors: morning -> noon -> dusk -> night
  Color skyColor;
  if (waveProgress < 0.25f) {        // Morning (waves 1-12)
    float t = waveProgress / 0.25f;
    skyColor = (Color){
      (unsigned char)(15 + t * 20),
      (unsigned char)(23 + t * 30),
      (unsigned char)(42 + t * 50),
      255
    };
  } else if (waveProgress < 0.5f) {  // Noon (waves 13-25)
    float t = (waveProgress - 0.25f) / 0.25f;
    skyColor = (Color){
      (unsigned char)(35 + t * 15),
      (unsigned char)(53 + t * 20),
      (unsigned char)(92 + t * 30),
      255
    };
  } else if (waveProgress < 0.75f) { // Dusk (waves 26-37)
    float t = (waveProgress - 0.5f) / 0.25f;
    skyColor = (Color){
      (unsigned char)(50 - t * 25),
      (unsigned char)(73 - t * 30),
      (unsigned char)(122 - t * 50),
      255
    };
  } else {                            // Night (waves 38-50+)
    float t = (waveProgress - 0.75f) / 0.25f;
    if (t > 1.0f) t = 1.0f;
    skyColor = (Color){
      (unsigned char)(25 - t * 15),
      (unsigned char)(43 - t * 20),
      (unsigned char)(72 - t * 30),
      255
    };
  }
  // Subtle time-of-day pulse
  float dayPulse = 0.5f + 0.5f * sinf(globalTime * 0.5f);
  skyColor.r = (unsigned char)(skyColor.r * (0.95f + 0.05f * dayPulse));
  skyColor.g = (unsigned char)(skyColor.g * (0.95f + 0.05f * dayPulse));
  skyColor.b = (unsigned char)(skyColor.b * (0.95f + 0.05f * dayPulse));
  
  ClearBackground(skyColor);

  // Apply screen shake to camera (position only)
  Camera3D shakenCamera = camera;
  shakenCamera.position.x += screenShakeOffset.x;
  shakenCamera.position.y += screenShakeOffset.y;
  shakenCamera.target.x += screenShakeOffset.x;
  shakenCamera.target.y += screenShakeOffset.y;

  BeginMode3D(shakenCamera);

  // Terrain (Meadow + Cobblestone Path + Trees + Boulders + Portal + Fortress)
  Renderer3D::DrawTerrain(path, globalTime);

  // Towers with animated crew characters
  Vector2 hoverWorld = GetMouseWorld2D();
  Vector2 hoverScreen = GetMousePosition();
  for (size_t i = 0; i < towers.size(); i++) {
    bool isSel = ((int)i == selectedTowerIndex);
    bool isHovered = !isBuildMode && !showSettings && !gameOver &&
                     hoverScreen.y > 70.0f &&
                     Vector2Distance(hoverWorld, towers[i].GetPosition()) <= Design::TOWER_CLICK_RADIUS;
    Renderer3D::DrawTower3D(towers[i], isSel, globalTime);
    if (isHovered && !isSel) {
      Renderer3D::DrawRangeIndicator3D(towers[i].GetPosition(), towers[i].GetRange(),
                                      Fade(Color{0, 200, 240, 255}, 0.45f));
    }
  }

  // Enemies with procedural walk animation
  for (const Enemy& enemy : enemies) {
    Renderer3D::DrawEnemy3D(enemy, globalTime);
  }

  // 3D Projectiles (Arrows, Magic Bolts, Flaming Boulders)
  for (const Bullet& bullet : bullets) {
    Renderer3D::DrawBullet3D(bullet);
  }

  // 3D AoE Explosions
  for (const Explosion& exp : explosions) {
    Renderer3D::DrawExplosion3D(exp);
  }

  // Tower placement preview
  Vector2 mouse = GetMousePosition();
  bool hoveringBottomCards =
      (mouse.x >= 320.0f && mouse.x <= 960.0f && mouse.y >= 545.0f);
  bool hoveringPanel =
      (selectedTowerIndex >= 0 && CheckCollisionPointRec(mouse, infoPanelRec));

  if (isBuildMode && mouse.y > 70.0f && !hoveringBottomCards &&
      !hoveringPanel && !gameOver) {
    Vector2 worldMouse = GetMouseWorld2D();
    bool onRoad = IsOnRoad(worldMouse);
    bool nearTower = IsNearTower(worldMouse);
    bool canPlace = !onRoad && !nearTower && money >= Tower::GetTowerCost(selectedTowerType);
    Renderer3D::DrawTowerPreview3D(worldMouse, (int)selectedTowerType,
                                   Tower::GetTowerRange(selectedTowerType), canPlace);

    // Placement feedback near cursor
    const char* placeMsg = canPlace ? "CLICK TO PLACE" : (money < Tower::GetTowerCost(selectedTowerType) ? "NOT ENOUGH MONEY" : "INVALID POSITION");
    Color placeColor = canPlace ? Color{34, 197, 94, 255} : Color{239, 68, 68, 255};
    int msgW = MeasureText(placeMsg, 14);
    Vector2 msgPos = {mouse.x - msgW / 2.0f, mouse.y + 35.0f};
    DrawRectangleRounded(Rectangle{msgPos.x - 6, msgPos.y - 2, (float)msgW + 12, 22.0f}, 0.2f, 4, Fade(Color{10, 15, 25, 255}, 0.9f));
    DrawRectangleRoundedLinesEx(Rectangle{msgPos.x - 6, msgPos.y - 2, (float)msgW + 12, 22.0f}, 0.2f, 4, 1.0f, placeColor);
    DrawText(placeMsg, (int)msgPos.x, (int)msgPos.y, 14, placeColor);
    
    // Show cost
    const char* costMsg = TextFormat("$%d", Tower::GetTowerCost(selectedTowerType));
    int costW = MeasureText(costMsg, 12);
    Vector2 costPos = {mouse.x - costW / 2.0f, mouse.y + 58.0f};
    DrawText(costMsg, (int)costPos.x, (int)costPos.y, 12, Color{250, 204, 21, 255});
  }

  // 3D Particles (Shockwave sparks in world space)
  for (const Particle& p : particles) {
    float alpha = p.lifetime / p.maxLifetime;
    if (alpha > 1.0f) alpha = 1.0f;
    if (alpha < 0.0f) alpha = 0.0f;
    Vector3 p3 = Renderer3D::To3D(p.position, 0.5f + (1.0f - alpha) * 1.5f);
    DrawSphere(p3, p.radius * 0.045f * (0.3f + 0.7f * alpha), Fade(p.color, alpha));
  }

  // Ambient particles (drawn in world space, subtle)
  for (const Particle& p : ambientParticles) {
    float alpha = p.lifetime / p.maxLifetime;
    if (alpha > 1.0f) alpha = 1.0f;
    if (alpha < 0.0f) alpha = 0.0f;
    Vector3 p3 = Renderer3D::To3D(p.position, 0.2f + alpha * 0.5f);
    DrawSphere(p3, p.radius * 0.045f * (0.2f + 0.8f * alpha), Fade(p.color, alpha * 0.4f));
  }

  EndMode3D();

  // =================================
  // WAVE READY DIMMING OVERLAY (darken like pause when waiting to start wave)
  // =================================
  if (waveReadyToStart && !waveInProgress && !gameOver && !victory) {
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
  }

  // =================================
  // BILLBOARD: FLOATING TEXTS (projected from 3D to screen)
  // =================================
  for (const FloatingText& ft : floatingTexts) {
    float alpha = ft.lifetime / ft.maxLifetime;
    if (alpha > 1.0f) alpha = 1.0f;
    if (alpha < 0.0f) alpha = 0.0f;
    // Project 3D world position to screen
    Vector3 ftPos3 = Renderer3D::To3D(ft.position, 2.8f);
    Vector2 screenPos = GetWorldToScreen(ftPos3, camera);
    int textW = MeasureText(ft.text, 15);
    DrawText(ft.text, (int)screenPos.x - textW / 2 + 1, (int)screenPos.y + 1, 15, Fade(BLACK, alpha * 0.7f));
    DrawText(ft.text, (int)screenPos.x - textW / 2, (int)screenPos.y, 15, Fade(ft.color, alpha));
  }


  DrawLineEx(Vector2{980.0f, 715.0f}, Vector2{1040.0f, 715.0f}, 2.0f,
             Fade(Color{0, 200, 240, 255}, 0.5f));
  DrawLineEx(Vector2{1040.0f, 715.0f}, Vector2{1060.0f, 695.0f}, 2.0f,
             Fade(Color{0, 200, 240, 255}, 0.7f));
  DrawLineEx(Vector2{1060.0f, 695.0f}, Vector2{1250.0f, 695.0f}, 2.0f,
             Fade(Color{0, 200, 240, 255}, 0.5f));
  DrawLineEx(Vector2{1250.0f, 695.0f}, Vector2{1265.0f, 710.0f}, 2.0f,
             Fade(Color{0, 200, 240, 255}, 0.3f));

  // =================================
  // TOP HUD (MODERN SCI-FI COMMAND BAR)
  // =================================
  DrawRectangle(0, 0, screenWidth, 70, Fade(Color{8, 16, 26, 255}, 0.94f));
  DrawLine(0, 69, screenWidth, 69, Fade(Color{0, 200, 240, 255}, 0.35f));

  // Kiri: Shield Icon & Title
  DrawIconShield(Vector2{34.0f, 35.0f}, 26.0f, Color{0, 210, 245, 255});
  DrawText("VECTOR SIEGE", 60, 16, 22, WHITE);
  DrawText("DEFEND THE BASE", 62, 42, 11, Color{0, 210, 245, 255});

  // Tengah: Wave Container & Status
  Rectangle waveBox{515.0f, 10.0f, 250.0f, 50.0f};
  bool bossW = IsBossWave(wave);
  Color waveBorderCol =
      bossW ? Color{239, 68, 68, 255} : Color{0, 200, 240, 255};
  DrawSciFiPanel(waveBox, waveBorderCol, Fade(Color{10, 20, 32, 255}, 0.9f),
                 0.15f, 0.3f);
  DrawText("/ /", 528, 22, 18, Fade(waveBorderCol, 0.8f));
  DrawText("\\ \\", 735, 22, 18, Fade(waveBorderCol, 0.8f));
  if (bossW) {
    DrawTextCentered(TextFormat("WAVE %d (BOSS)", wave),
                     waveBox.x + waveBox.width * 0.5f, waveBox.y + 4.0f, 21,
                     Color{239, 68, 68, 255});
  } else {
    DrawTextCentered(TextFormat("WAVE %d", wave),
                     waveBox.x + waveBox.width * 0.5f, waveBox.y + 4.0f, 21,
                     WHITE);
  }
  if (waveInProgress) {
    DrawTextCentered(TextFormat("ENEMIES: %d  /  %d", enemiesSpawned,
                                enemiesToSpawn),
                     waveBox.x + waveBox.width * 0.5f, waveBox.y + 30.0f, 12,
                     Color{180, 210, 230, 255});
  } else if (!gameOver) {
    if (waveReadyToStart) {
      // Show a live intermission stopwatch so the HUD never looks frozen.
      DrawTextCentered(TextFormat("READY - CLICK START (%.1fs)", nextWaveTimer),
                       waveBox.x + waveBox.width * 0.5f, waveBox.y + 30.0f, 12,
                       Color{34, 197, 94, 255});
    } else {
      DrawTextCentered("STANDBY", waveBox.x + waveBox.width * 0.5f,
                       waveBox.y + 30.0f, 12, Color{250, 204, 21, 255});
    }
  }

  // Wave composition preview (hover wave box)
  bool hovWaveBox = CheckCollisionPointRec(mouse, waveBox);
  if (hovWaveBox && !gameOver) {
    int nextWave = waveInProgress ? wave : wave + 1;
    bool nextIsBoss = IsBossWave(nextWave);
    int nextEnemiesToSpawn = waveInProgress ? enemiesToSpawn : (5 + (nextWave - 1) * 2);
    
    // Calculate enemy composition for next wave
    std::vector<EnemyType> nextWaveComposition;
    for (int i = 0; i < nextEnemiesToSpawn; i++) {
      EnemyType typeToSpawn = EnemyType::NORMAL;
      if (nextIsBoss) {
        if (i == nextEnemiesToSpawn - 1) typeToSpawn = EnemyType::BOSS;
        else if (i % 3 == 0) typeToSpawn = EnemyType::TANK;
        else if (i % 2 == 1) typeToSpawn = EnemyType::FAST;
        else typeToSpawn = EnemyType::NORMAL;
      } else if (nextWave == 1) {
        typeToSpawn = EnemyType::NORMAL;
      } else if (nextWave == 2) {
        typeToSpawn = (i % 2 == 1) ? EnemyType::FAST : EnemyType::NORMAL;
      } else if (nextWave == 3) {
        if (i == nextEnemiesToSpawn - 1 || i == nextEnemiesToSpawn / 2) typeToSpawn = EnemyType::TANK;
        else if (i % 2 == 1) typeToSpawn = EnemyType::FAST;
        else typeToSpawn = EnemyType::NORMAL;
      } else {
        int mod = i % 5;
        if (mod == 4) typeToSpawn = EnemyType::TANK;
        else if (mod == 1 || mod == 3) typeToSpawn = EnemyType::FAST;
        else typeToSpawn = EnemyType::NORMAL;
      }
      nextWaveComposition.push_back(typeToSpawn);
    }
    
    // Count each type
    int nNormal = 0, nFast = 0, nTank = 0, nBoss = 0;
    for (auto t : nextWaveComposition) {
      if (t == EnemyType::NORMAL) nNormal++;
      else if (t == EnemyType::FAST) nFast++;
      else if (t == EnemyType::TANK) nTank++;
      else if (t == EnemyType::BOSS) nBoss++;
    }
    
    // Draw preview panel (below the top bar so it never hides the HUD) - with slide animation
    float previewSlide = uiWavePreviewActive ? 1.0f : 0.0f;
    if (previewSlide > 0.0f) {
      Rectangle previewBox{waveBox.x, 74.0f, 280.0f, 56.0f};
      Color previewBorder = nextIsBoss ? Design::BORDER_WARN : Design::BORDER_PRIMARY;
      DrawSciFiPanel(previewBox, previewBorder, Fade(Design::BG_PANEL, 0.96f), Design::RADIUS_MD, 0.4f);
      
      DrawText("NEXT WAVE:", (int)previewBox.x + Design::SPACING_MD, (int)previewBox.y + Design::SPACING_XS, Design::FONT_SM, Design::TEXT_SECONDARY);
      DrawText(TextFormat("Wave %d%s", nextWave, nextIsBoss ? " (BOSS)" : ""), 
               (int)previewBox.x + Design::SPACING_MD, (int)previewBox.y + 19, Design::FONT_MD, 
               nextIsBoss ? Design::BORDER_WARN : Design::TEXT_PRIMARY);
      
      int x = (int)previewBox.x + Design::SPACING_MD;
      float iconY = previewBox.y + 46.0f;
      if (nNormal > 0) {
        DrawCircle(x + 8, iconY, 6, Design::ENEMY_NORMAL);
        DrawText(TextFormat("x%d", nNormal), x + 18, (int)previewBox.y + 40, Design::FONT_XS, Design::TEXT_SECONDARY);
        x += 55;
      }
      if (nFast > 0) {
        DrawCircle(x + 8, iconY, 6, Design::ENEMY_FAST);
        DrawText(TextFormat("x%d", nFast), x + 18, (int)previewBox.y + 40, Design::FONT_XS, Design::TEXT_SECONDARY);
        x += 55;
      }
      if (nTank > 0) {
        DrawCircle(x + 8, iconY, 6, Design::ENEMY_TANK);
        DrawText(TextFormat("x%d", nTank), x + 18, (int)previewBox.y + 40, Design::FONT_XS, Design::TEXT_SECONDARY);
        x += 55;
      }
      if (nBoss > 0) {
        DrawCircle(x + 8, iconY, 6, Design::ENEMY_BOSS);
        DrawText(TextFormat("x%d", nBoss), x + 18, (int)previewBox.y + 40, Design::FONT_XS, Design::TEXT_SECONDARY);
      }
    }
  }

  // =================================
  // TOP HUD CONTROLS: PAUSE & SPEED
  // =================================
  bool isHovPause = CheckCollisionPointRec(mouse, btnPause);
  Color pauseBorder = isPaused ? Color{250, 204, 21, 255}
                               : (isHovPause ? Color{0, 240, 255, 255}
                                             : Color{0, 180, 230, 255});
  Color pauseBg = isPaused ? Fade(Color{161, 98, 7, 255}, 0.65f)
                           : (isHovPause ? Fade(Color{14, 116, 144, 255}, 0.7f)
                                         : Fade(Color{10, 20, 32, 255}, 0.85f));
  DrawSciFiPanel(btnPause, pauseBorder, pauseBg, 0.25f,
                 isHovPause ? 0.35f : (isPaused ? 0.3f : 0.0f));

  if (isPaused) {
    // Play triangle â–¶ icon
    Vector2 p1{btnPause.x + 15.0f, btnPause.y + 11.0f};
    Vector2 p2{btnPause.x + 15.0f, btnPause.y + 27.0f};
    Vector2 p3{btnPause.x + 29.0f, btnPause.y + 19.0f};
    DrawTriangle(p1, p2, p3, Color{250, 204, 21, 255});
  } else {
    // Pause || bars icon
    DrawRectangle((int)btnPause.x + 13, (int)btnPause.y + 11, 5, 16, WHITE);
    DrawRectangle((int)btnPause.x + 24, (int)btnPause.y + 11, 5, 16, WHITE);
  }

// Speed Button (1X / 2X / 3X)
  bool isHovSpeed = CheckCollisionPointRec(mouse, btnSpeed);
  bool isFast = (gameSpeed > 1.0f);
  Color speedBorder = (gameSpeed >= 3.0f) ? Color{239, 68, 68, 255} : (isFast ? Color{250, 204, 21, 255}
                              : (isHovSpeed ? Color{0, 240, 255, 255}
                                            : Color{0, 180, 230, 255}));
  Color speedBg = (gameSpeed >= 3.0f) ? Fade(Color{185, 28, 28, 255}, 0.65f) : (isFast ? Fade(Color{161, 98, 7, 255}, 0.65f)
                          : (isHovSpeed ? Fade(Color{14, 116, 144, 255}, 0.7f)
                                        : Fade(Color{10, 20, 32, 255}, 0.85f)));
  DrawSciFiPanel(btnSpeed, speedBorder, speedBg, 0.25f,
                 (gameSpeed >= 3.0f || isFast) ? 0.35f : (isHovSpeed ? 0.25f : 0.0f));
  const char* speedText = (gameSpeed >= 3.0f) ? "3X" : (gameSpeed >= 2.0f ? "2X" : "1X");
  Color speedTextColor = (gameSpeed >= 3.0f) ? Color{239, 68, 68, 255} : (isFast ? Color{250, 204, 21, 255} : WHITE);
  DrawText(speedText, (int)btnSpeed.x + 11, (int)btnSpeed.y + 11, 16, speedTextColor);

  // Start Wave Button (center top when wave ready)
  if (waveReadyToStart && !waveInProgress && !gameOver && !victory) {
    bool isHovStart = CheckCollisionPointRec(mouse, btnStartWave);
    Color startBorder = isHovStart ? Color{34, 255, 100, 255} : Color{34, 197, 94, 255};
    Color startBg = isHovStart ? Fade(Color{22, 163, 74, 255}, 0.85f) : Fade(Color{21, 128, 61, 255}, 0.85f);
    DrawSciFiPanel(btnStartWave, startBorder, startBg, 0.2f, isHovStart ? 0.4f : 0.15f);
    DrawText("START WAVE", (int)btnStartWave.x + 38, (int)btnStartWave.y + 13, 16, WHITE);
    
    // Keyboard hint
    DrawText("[ENTER]", (int)btnStartWave.x + 65, (int)btnStartWave.y + 30, 10, Color{148, 163, 184, 255});
  }

  // Kanan: Money Card
  Rectangle moneyRec{870.0f, 10.0f, 160.0f, 50.0f};
  DrawSciFiPanel(moneyRec, Color{250, 204, 21, 255},
                 Fade(Color{10, 20, 32, 255}, 0.85f), 0.2f, 0.25f);
  DrawIconCoin(Vector2{895.0f, 35.0f}, 24.0f, Color{250, 204, 21, 255});
  DrawText("MONEY", 922, 15, 10, Fade(Color{250, 204, 21, 255}, 0.9f));
  DrawText(TextFormat("$%d", money), 922, 28, 22, WHITE);

  // Kanan: Base HP Card & Integrated Bar
  Rectangle baseRec{1045.0f, 10.0f, 215.0f, 50.0f};
  Color baseBorderCol =
      (baseHP <= 3)
          ? Color{239, 68, 68, 255}
          : ((baseHP <= 6) ? Color{234, 179, 8, 255} : Color{0, 210, 245, 255});
  DrawSciFiPanel(baseRec, baseBorderCol, Fade(Color{10, 20, 32, 255}, 0.85f),
                 0.2f, 0.25f);
  DrawIconShield(Vector2{1065.0f, 35.0f}, 22.0f, baseBorderCol);
  DrawText("BASE", 1085, 14, 11, Color{0, 210, 245, 255});
  DrawText(TextFormat("%d / %d", baseHP, maxBaseHP), 1195, 14, 12,
           baseBorderCol);

  // Integrated rounded Base HP Bar
  Rectangle hpBarRec{1085.0f, 32.0f, 160.0f, 14.0f};
  DrawRectangleRounded(hpBarRec, 0.5f, 4, Fade(Color{15, 23, 42, 255}, 0.9f));
  DrawRectangleRoundedLinesEx(hpBarRec, 0.5f, 4, 1.0f, Fade(WHITE, 0.25f));
  float hpRatio = (float)baseHP / (float)maxBaseHP;
  if (hpRatio > 0.0f) {
    float fillW = 160.0f * hpRatio;
    if (fillW < 6.0f)
      fillW = 6.0f;
    Color hpFillCol = (baseHP <= 3) ? Color{239, 68, 68, 255}
                                    : ((baseHP <= 6) ? Color{234, 179, 8, 255}
                                                     : Color{34, 197, 94, 255});
    DrawRectangleRounded(Rectangle{1085.0f, 32.0f, fillW, 14.0f}, 0.5f, 4,
                         hpFillCol);
  }

  // =================================
  // BOTTOM TOWER MENU CONSOLE
  // =================================
  Rectangle consoleRec{325.0f, 548.0f, 630.0f, 156.0f};
  DrawSciFiPanel(consoleRec, Color{0, 180, 230, 255},
                 Fade(Color{10, 22, 36, 255}, 0.94f), 0.08f, 0.35f);

  // Console Header Strip (Mode pill badge + Instructions)
  float modeWidth = isBuildMode ? 230.0f : 145.0f;
  Rectangle modeRec{340.0f, 555.0f, modeWidth, 22.0f};
  Color modeBorder = isBuildMode ? ((selectedTowerType == TowerType::BASIC)
                                        ? Color{34, 197, 94, 255}
                                    : (selectedTowerType == TowerType::RAPID)
                                        ? Color{14, 165, 233, 255}
                                        : Color{239, 68, 68, 255})
                                 : Color{0, 200, 240, 255};
  DrawRectangleRounded(modeRec, 0.45f, 4, Fade(Color{15, 28, 44, 255}, 0.85f));
  DrawRectangleRoundedLinesEx(modeRec, 0.45f, 4, 1.0f, modeBorder);
  DrawCircle(modeRec.x + 12.0f, modeRec.y + 11.0f, 4.0f, modeBorder);
  DrawCircleLines(modeRec.x + 12.0f, modeRec.y + 11.0f, 6.0f,
                  Fade(modeBorder, 0.7f));
  DrawText("MODE", (int)modeRec.x + 23, (int)modeRec.y + 5, 11,
           Fade(WHITE, 0.75f));
  if (isBuildMode) {
    DrawText(TextFormat("BUILDING %s", Tower::GetTowerName(selectedTowerType)),
             (int)modeRec.x + 59, (int)modeRec.y + 5, 11, modeBorder);
  } else {
    DrawText("NORMAL", (int)modeRec.x + 62, (int)modeRec.y + 5, 11, WHITE);
  }

  if (isBuildMode) {
    DrawText("ESC / Right-Click = Cancel",
             (int)modeRec.x + (int)modeRec.width + 16, 559, 12,
             Color{250, 204, 21, 255});
  } else {
    DrawText("Select tower card or press (1, 2, 3) to Build",
             (int)modeRec.x + (int)modeRec.width + 16, 559, 12,
             Color{148, 163, 184, 255});
    if (canUndo) {
      DrawText("[Z] Undo last placement", (int)modeRec.x + (int)modeRec.width + 300, 559, 12, Color{34, 197, 94, 255});
    }
  }

  // --- CARD 1: BASIC TOWER ---
  bool isSelBasic = (isBuildMode && selectedTowerType == TowerType::BASIC);
  bool isHovBasic = CheckCollisionPointRec(mouse, btnBasic);
  bool canAffordBasic = (money >= 100);
  Color colBasic = Color{34, 197, 94, 255};

  Color bgBasic = isSelBasic
                      ? Fade(Color{20, 83, 45, 255}, 0.65f)
                      : (isHovBasic ? Fade(Color{30, 41, 59, 255}, 0.85f)
                                    : Fade(Color{12, 24, 38, 255}, 0.85f));
  DrawSciFiPanel(
      btnBasic,
      isSelBasic ? colBasic
                 : (isHovBasic ? Color{74, 222, 128, 255}
                               : (canAffordBasic ? Fade(colBasic, 0.5f)
                                                 : Fade(GRAY, 0.3f))),
      bgBasic, 0.08f, isSelBasic ? 0.4f : (isHovBasic ? 0.25f : 0.0f));

  // Badge [1]
  DrawRectangleRounded(
      Rectangle{btnBasic.x + 8.0f, btnBasic.y + 8.0f, 22.0f, 18.0f}, 0.35f, 4,
      canAffordBasic ? colBasic : GRAY);
  DrawText("1", btnBasic.x + 16.0f, btnBasic.y + 11.0f, 12, WHITE);

  // Mini Tower Vector Graphic
  DrawMiniTowerCardGraphic(Vector2{btnBasic.x + 46.0f, btnBasic.y + 44.0f},
                           TowerType::BASIC, colBasic);

  // Text details
  DrawText("BASIC TOWER", btnBasic.x + 76.0f, btnBasic.y + 12.0f, 13,
           canAffordBasic ? WHITE : GRAY);
  DrawText("$100", btnBasic.x + 76.0f, btnBasic.y + 30.0f, 17,
           canAffordBasic ? Color{250, 204, 21, 255} : GRAY);

  // Stats footer
  DrawIconSword(Vector2{btnBasic.x + 16.0f, btnBasic.y + 87.0f}, 12.0f,
                canAffordBasic ? colBasic : GRAY);
  DrawText("DMG 20", btnBasic.x + 28.0f, btnBasic.y + 82.0f, 11,
           canAffordBasic ? Color{203, 213, 225, 255} : GRAY);
  DrawIconTarget(Vector2{btnBasic.x + 104.0f, btnBasic.y + 87.0f}, 12.0f,
                 canAffordBasic ? colBasic : GRAY);
  DrawText("RNG 160", btnBasic.x + 116.0f, btnBasic.y + 82.0f, 11,
           canAffordBasic ? Color{203, 213, 225, 255} : GRAY);

  // --- CARD 2: RAPID TOWER ---
  bool isSelRapid = (isBuildMode && selectedTowerType == TowerType::RAPID);
  bool isHovRapid = CheckCollisionPointRec(mouse, btnRapid);
  bool canAffordRapid = (money >= 150);
  Color colRapid = Color{14, 165, 233, 255};

  Color bgRapid = isSelRapid
                      ? Fade(Color{12, 74, 110, 255}, 0.65f)
                      : (isHovRapid ? Fade(Color{30, 41, 59, 255}, 0.85f)
                                    : Fade(Color{12, 24, 38, 255}, 0.85f));
  DrawSciFiPanel(
      btnRapid,
      isSelRapid ? colRapid
                 : (isHovRapid ? Color{56, 189, 248, 255}
                               : (canAffordRapid ? Fade(colRapid, 0.5f)
                                                 : Fade(GRAY, 0.3f))),
      bgRapid, 0.08f, isSelRapid ? 0.4f : (isHovRapid ? 0.25f : 0.0f));

  // Badge [2]
  DrawRectangleRounded(
      Rectangle{btnRapid.x + 8.0f, btnRapid.y + 8.0f, 22.0f, 18.0f}, 0.35f, 4,
      canAffordRapid ? colRapid : GRAY);
  DrawText("2", btnRapid.x + 16.0f, btnRapid.y + 11.0f, 12, WHITE);

  // Mini Tower Vector Graphic
  DrawMiniTowerCardGraphic(Vector2{btnRapid.x + 46.0f, btnRapid.y + 44.0f},
                           TowerType::RAPID, colRapid);

  // Text details
  DrawText("RAPID TOWER", btnRapid.x + 76.0f, btnRapid.y + 12.0f, 13,
           canAffordRapid ? WHITE : GRAY);
  DrawText("$150", btnRapid.x + 76.0f, btnRapid.y + 30.0f, 17,
           canAffordRapid ? Color{250, 204, 21, 255} : GRAY);

  // Stats footer
  DrawIconSword(Vector2{btnRapid.x + 16.0f, btnRapid.y + 87.0f}, 12.0f,
                canAffordRapid ? colRapid : GRAY);
  DrawText("DMG 8", btnRapid.x + 28.0f, btnRapid.y + 82.0f, 11,
           canAffordRapid ? Color{203, 213, 225, 255} : GRAY);
  DrawIconTarget(Vector2{btnRapid.x + 104.0f, btnRapid.y + 87.0f}, 12.0f,
                 canAffordRapid ? colRapid : GRAY);
  DrawText("RNG 130", btnRapid.x + 116.0f, btnRapid.y + 82.0f, 11,
           canAffordRapid ? Color{203, 213, 225, 255} : GRAY);

  // --- CARD 3: CANNON TOWER ---
  bool isSelCannon = (isBuildMode && selectedTowerType == TowerType::CANNON);
  bool isHovCannon = CheckCollisionPointRec(mouse, btnCannon);
  bool canAffordCannon = (money >= 250);
  Color colCannon = Color{239, 68, 68, 255};

  Color bgCannon = isSelCannon
                       ? Fade(Color{127, 29, 29, 255}, 0.65f)
                       : (isHovCannon ? Fade(Color{30, 41, 59, 255}, 0.85f)
                                      : Fade(Color{12, 24, 38, 255}, 0.85f));
  DrawSciFiPanel(
      btnCannon,
      isSelCannon ? colCannon
                  : (isHovCannon ? Color{248, 113, 113, 255}
                                 : (canAffordCannon ? Fade(colCannon, 0.5f)
                                                    : Fade(GRAY, 0.3f))),
      bgCannon, 0.08f, isSelCannon ? 0.4f : (isHovCannon ? 0.25f : 0.0f));

  // Badge [3]
  DrawRectangleRounded(
      Rectangle{btnCannon.x + 8.0f, btnCannon.y + 8.0f, 22.0f, 18.0f}, 0.35f, 4,
      canAffordCannon ? colCannon : GRAY);
  DrawText("3", btnCannon.x + 16.0f, btnCannon.y + 11.0f, 12, WHITE);

  // Mini Tower Vector Graphic
  DrawMiniTowerCardGraphic(Vector2{btnCannon.x + 46.0f, btnCannon.y + 44.0f},
                           TowerType::CANNON, colCannon);

  // Text details
  DrawText("CANNON TOWER", btnCannon.x + 76.0f, btnCannon.y + 12.0f, 13,
           canAffordCannon ? WHITE : GRAY);
  DrawText("$250", btnCannon.x + 76.0f, btnCannon.y + 30.0f, 17,
           canAffordCannon ? Color{250, 204, 21, 255} : GRAY);

  // Stats footer
  DrawIconSword(Vector2{btnCannon.x + 16.0f, btnCannon.y + 87.0f}, 12.0f,
                canAffordCannon ? colCannon : GRAY);
  DrawText("DMG 60", btnCannon.x + 28.0f, btnCannon.y + 82.0f, 11,
           canAffordCannon ? Color{203, 213, 225, 255} : GRAY);
  DrawIconTarget(Vector2{btnCannon.x + 104.0f, btnCannon.y + 87.0f}, 12.0f,
                 canAffordCannon ? colCannon : GRAY);
  DrawText("RNG 200", btnCannon.x + 116.0f, btnCannon.y + 82.0f, 11,
           canAffordCannon ? Color{203, 213, 225, 255} : GRAY);

  // =================================
  // SELECTED TOWER INFO PANEL (BOTTOM LEFT)
  // =================================
  if (selectedTowerIndex >= 0 && selectedTowerIndex < (int)towers.size()) {
    const Tower &selTower = towers[selectedTowerIndex];
    Color tColor =
        (selTower.GetType() == TowerType::BASIC)   ? Color{34, 197, 94, 255}
        : (selTower.GetType() == TowerType::RAPID) ? Color{14, 165, 233, 255}
                                                   : Color{239, 68, 68, 255};

    // Panel background with sci-fi glowing border
    DrawSciFiPanel(infoPanelRec, tColor, Fade(Color{10, 20, 32, 255}, 0.94f),
                   0.08f, 0.35f);

    // Top-right corner tech stripes
    DrawLineEx(Vector2{infoPanelRec.x + infoPanelRec.width - 24.0f,
                       infoPanelRec.y + 10.0f},
               Vector2{infoPanelRec.x + infoPanelRec.width - 10.0f,
                       infoPanelRec.y + 24.0f},
               1.5f, Fade(tColor, 0.7f));
    DrawLineEx(Vector2{infoPanelRec.x + infoPanelRec.width - 18.0f,
                       infoPanelRec.y + 10.0f},
               Vector2{infoPanelRec.x + infoPanelRec.width - 10.0f,
                       infoPanelRec.y + 18.0f},
               1.5f, Fade(tColor, 0.7f));

    // Header: Tower Avatar & Name
    Vector2 avatarCenter{infoPanelRec.x + 36.0f, infoPanelRec.y + 36.0f};
    DrawCircle(avatarCenter.x, avatarCenter.y, 20.0f,
               Fade(Color{15, 23, 42, 255}, 0.9f));
    DrawCircleLines(avatarCenter.x, avatarCenter.y, 20.0f, tColor);
    DrawMiniTowerCardGraphic(avatarCenter, selTower.GetType(), tColor);

    DrawText(TextFormat("%s TOWER", Tower::GetTowerName(selTower.GetType())),
             infoPanelRec.x + 68.0f, infoPanelRec.y + 20.0f, 15, WHITE);
    DrawText(TextFormat("Level %d / 3", selTower.GetLevel()),
             infoPanelRec.x + 68.0f, infoPanelRec.y + 40.0f, 13,
             Color{148, 163, 184, 255});

    // Stats rows with icons
    float statY = infoPanelRec.y + 76.0f;
    float rowSpacing = 26.0f;
    float labelX = infoPanelRec.x + 46.0f;
    float valueX = infoPanelRec.x + infoPanelRec.width - 20.0f; // Right-aligned with padding

    // Helper lambda for right-aligned stat text
    auto DrawStatValue = [&](const char* text, float y, Color col) {
        int w = MeasureText(text, 14);
        float x = valueX - w;
        if (x < labelX + 10) x = labelX + 10; // Prevent overlap with label
        DrawText(text, (int)x, (int)y, 14, col);
    };

    // Row 1: Damage
    DrawIconSword(Vector2{infoPanelRec.x + 28.0f, statY + 8.0f}, 13.0f, tColor);
    DrawText("Damage", labelX, statY, 13, Color{203, 213, 225, 255});
    DrawStatValue(TextFormat("%d", selTower.GetDamage()), statY, WHITE);
    statY += rowSpacing;

    // Row 2: Range
    DrawIconTarget(Vector2{infoPanelRec.x + 28.0f, statY + 8.0f}, 13.0f, tColor);
    DrawText("Range", labelX, statY, 13, Color{203, 213, 225, 255});
    DrawStatValue(TextFormat("%.0f", selTower.GetRange()), statY, WHITE);
    statY += rowSpacing;

    // Row 3: Cooldown
    DrawIconClock(Vector2{infoPanelRec.x + 28.0f, statY + 8.0f}, 13.0f, tColor);
    DrawText("Cooldown", labelX, statY, 13, Color{203, 213, 225, 255});
    DrawStatValue(TextFormat("%.1fs", selTower.GetCooldown()), statY, WHITE);
    statY += rowSpacing;

    // Row 4: AoE Radius (for Cannon)
    if (selTower.GetType() == TowerType::CANNON) {
      DrawIconAoE(Vector2{infoPanelRec.x + 28.0f, statY + 8.0f}, 13.0f, Color{249, 115, 22, 255});
      DrawText("AoE Radius", labelX, statY, 13, Color{249, 115, 22, 255});
      DrawStatValue(TextFormat("%.0f", selTower.GetAoERadius()), statY, Color{249, 115, 22, 255});
    }

    // Warning feedback if not enough money
    if (notEnoughMoneyTimer > 0.0f) {
      const char* msg = "Not enough money!";
      int msgW = MeasureText(msg, 13);
      float x = infoPanelRec.x + (infoPanelRec.width - msgW) * 0.5f;
      float y = btnUpgrade.y - 20.0f;
      DrawText(msg, (int)x, (int)y, 13, Color{239, 68, 68, 255});
    }

    // Action Button 1: UPGRADE
    if (selTower.CanUpgrade()) {
      int upCost = selTower.GetUpgradeCost();
      bool isHovUp = CheckCollisionPointRec(mouse, btnUpgrade);
      bool canAfford = (money >= upCost);

      Color upBg = canAfford ? (isHovUp ? Color{8, 145, 178, 255}
                                        : Color{14, 116, 144, 255})
                             : Fade(Color{30, 41, 59, 255}, 0.8f);
      Color upBorder = canAfford ? Color{6, 182, 212, 255} : Fade(GRAY, 0.4f);

      DrawSciFiPanel(btnUpgrade, upBorder, upBg, 0.25f,
                     canAfford && isHovUp ? 0.35f : 0.0f);

      // Circular badge with Up Arrow on left
      DrawCircle(btnUpgrade.x + 24.0f, btnUpgrade.y + btnUpgrade.height * 0.5f,
                 12.0f, Fade(Color{15, 23, 42, 255}, 0.7f));
      DrawIconUpgradeArrow(Vector2{btnUpgrade.x + 24.0f,
                                   btnUpgrade.y + btnUpgrade.height * 0.5f},
                           12.0f, canAfford ? Color{34, 211, 238, 255} : GRAY);

      DrawText("UPGRADE", btnUpgrade.x + 50.0f, btnUpgrade.y + 8.0f, 13,
               canAfford ? WHITE : LIGHTGRAY);
      DrawText(TextFormat("$%d", upCost), btnUpgrade.x + 50.0f,
               btnUpgrade.y + 23.0f, 13,
               canAfford ? Color{103, 232, 249, 255} : GRAY);
    } else {
      DrawSciFiPanel(btnUpgrade, Color{250, 204, 21, 255},
                     Fade(Color{15, 23, 42, 255}, 0.85f), 0.25f, 0.2f);
      DrawText("MAX LEVEL", btnUpgrade.x + 60.0f, btnUpgrade.y + 14.0f, 14,
               Color{250, 204, 21, 255});
    }

    // Action Button 2: SELL
    int sellVal = selTower.GetSellValue();
    bool isHovSell = CheckCollisionPointRec(mouse, btnSell);
    Color sellBg =
        isHovSell ? Color{185, 28, 28, 255} : Color{127, 29, 29, 255};
    DrawSciFiPanel(btnSell, Color{239, 68, 68, 255}, sellBg, 0.25f,
                   isHovSell ? 0.35f : 0.0f);

    // Circular badge with Trash icon on left
    DrawCircle(btnSell.x + 24.0f, btnSell.y + btnSell.height * 0.5f, 11.0f,
               Fade(Color{15, 23, 42, 255}, 0.7f));
    DrawIconTrash(Vector2{btnSell.x + 24.0f, btnSell.y + btnSell.height * 0.5f},
                  11.0f, Color{254, 202, 202, 255});

    DrawText("SELL", btnSell.x + 50.0f, btnSell.y + 11.0f, 13, WHITE);
    DrawText(TextFormat("$%d", sellVal), btnSell.x + 105.0f, btnSell.y + 11.0f,
             13, Color{254, 202, 202, 255});
  }

  // Feedback "Not enough money" saat di Build Mode
  if (isBuildMode && notEnoughMoneyTimer > 0.0f) {
    const char* msg = "Not enough money!";
    int msgW = MeasureText(msg, 18);
    int x = (int)mouse.x - msgW / 2;
    int y = (int)mouse.y - 50;
    // Keep on screen
    if (x < 10) x = 10;
    if (x + msgW > screenWidth - 10) x = screenWidth - msgW - 10;
    if (y < 10) y = 10;
    DrawText(msg, x, y, 18, Color{239, 68, 68, 255});
  }

  // =================================
  // TOWER HOVER INFO PANEL (shows DPS, range, target mode when hovering unselected tower)
  // =================================
  if (!isBuildMode && !showSettings && !gameOver && !victory && !isPaused) {
    Vector2 hoverWorld = GetMouseWorld2D();
    Vector2 hoverScreen = GetMousePosition();
    
    for (size_t i = 0; i < towers.size(); i++) {
      if ((int)i == selectedTowerIndex) continue; // Don't show for selected tower (has its own panel)
      
      bool isHovered = hoverScreen.y > 70.0f &&
                       Vector2Distance(hoverWorld, towers[i].GetPosition()) <= Design::TOWER_CLICK_RADIUS;
      
      if (isHovered) {
        const Tower &t = towers[i];
        Color tColor =
            (t.GetType() == TowerType::BASIC)   ? Color{34, 197, 94, 255}
            : (t.GetType() == TowerType::RAPID) ? Color{14, 165, 233, 255}
                                                : Color{239, 68, 68, 255};
        
        // Panel dimensions
        float panelW = 200.0f;
        float panelH = 140.0f;
        float panelX = hoverScreen.x + 20.0f;
        float panelY = hoverScreen.y - panelH - 10.0f;
        
        // Keep panel on screen
        if (panelX + panelW > screenWidth - 10) panelX = hoverScreen.x - panelW - 20.0f;
        if (panelY < 80.0f) panelY = 80.0f;
        
        Rectangle hoverPanel{panelX, panelY, panelW, panelH};
        DrawSciFiPanel(hoverPanel, tColor, Fade(Color{10, 20, 32, 255}, 0.95f), 0.08f, 0.35f);
        
        // Tower name
        DrawText(TextFormat("%s TOWER", Tower::GetTowerName(t.GetType())),
                 (int)panelX + 12, (int)panelY + 10, 14, WHITE);
        DrawText(TextFormat("Level %d", t.GetLevel()),
                 (int)panelX + 12, (int)panelY + 28, 12, Color{148, 163, 184, 255});
        
        // Stats
        float statY = panelY + 50.0f;
        float rowH = 22.0f;
        float labelX = panelX + 12.0f;
        float valX = panelX + panelW - 12.0f;
        
        auto DrawHoverStat = [&](const char* label, const char* value, Color vCol) {
          int vW = MeasureText(value, 13);
          DrawText(label, (int)labelX, (int)statY, 12, Color{203, 213, 225, 255});
          DrawText(value, (int)(valX - vW), (int)statY, 13, vCol);
          statY += rowH;
        };
        
        DrawHoverStat("Damage", TextFormat("%d", t.GetDamage()), WHITE);
        DrawHoverStat("Range", TextFormat("%.0f", t.GetRange()), WHITE);
        DrawHoverStat("Cooldown", TextFormat("%.1fs", t.GetCooldown()), WHITE);
        DrawHoverStat("DPS", TextFormat("%.1f", t.GetDamage() / t.GetCooldown()), Color{250, 204, 21, 255});
        
        // Target mode
        const char* targetModeStr = "";
        switch (t.GetTargetingMode()) {
          case TargetingMode::FIRST: targetModeStr = "First"; break;
          case TargetingMode::LAST: targetModeStr = "Last"; break;
          case TargetingMode::CLOSEST: targetModeStr = "Closest"; break;
          case TargetingMode::STRONGEST: targetModeStr = "Strongest"; break;
        }
        DrawHoverStat("Target", targetModeStr, Color{56, 189, 248, 255});
        
        break; // Only show one hover panel at a time
      }
    }
  }

  // =================================
  // BOSS WAVE INDICATOR BANNER
  // =================================
  if (bossWaveTimer > 0.0f) {
    float bannerAlpha = (bossWaveTimer > 0.5f) ? 1.0f : (bossWaveTimer / 0.5f);

    int bannerW = 460;
    int bannerH = 135;
    int bannerX = (screenWidth - bannerW) / 2;
    int bannerY = 160;

    Rectangle bannerRec{(float)bannerX, (float)bannerY, (float)bannerW,
                        (float)bannerH};
    DrawSciFiPanel(bannerRec, Fade(Color{239, 68, 68, 255}, bannerAlpha),
                   Fade(Color{15, 10, 20, 255}, 0.92f * bannerAlpha), 0.1f,
                   0.5f * bannerAlpha);

    DrawText(">>  BOSS WAVE  <<", bannerX + 90, bannerY + 24, 30,
             Fade(Color{239, 68, 68, 255}, bannerAlpha));
    DrawText(TextFormat("WAVE %d", wave), bannerX + 185, bannerY + 62, 20,
             Fade(Color{250, 204, 21, 255}, bannerAlpha));
    DrawText("WARNING: HEAVY SIGNATURE DETECTED!", bannerX + 50, bannerY + 94,
             16, Fade(WHITE, bannerAlpha));
  }

  // =================================
  // PAUSE OVERLAY & MODAL
  // =================================
  if (isPaused && !gameOver && !victory && !showSettings && !showSellConfirm &&
      !showRestartConfirm) {
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
    Rectangle pauseBox{(float)(screenWidth - 400) / 2.0f, 150.0f, 400.0f, 420.0f};
    DrawSciFiPanel(pauseBox, Color{0, 200, 240, 255},
                   Fade(Color{12, 22, 36, 255}, 0.95f), 0.12f, 0.45f);
    DrawTextCentered("GAME PAUSED", pauseBox.x + pauseBox.width * 0.5f,
                     pauseBox.y + 28.0f, 26, Color{0, 200, 240, 255});
    DrawTextCentered("[SPACE] Resume  |  [R] Restart",
                     pauseBox.x + pauseBox.width * 0.5f, pauseBox.y + 70.0f, 14,
                     Color{148, 163, 184, 255});

    // Resume button
    bool isHovResume = CheckCollisionPointRec(mouse, btnResumeModal);
    DrawSciFiPanel(btnResumeModal, Color{0, 200, 240, 255},
                   isHovResume ? Color{8, 145, 178, 255} : Color{14, 116, 144, 255},
                   0.25f, isHovResume ? 0.35f : 0.0f);
    DrawTextCenteredInRect("RESUME", btnResumeModal, 14, WHITE);

    // Restart button in Pause Modal
    bool isHovRestartM = CheckCollisionPointRec(mouse, btnRestartModal);
    DrawSciFiPanel(btnRestartModal, Color{239, 68, 68, 255},
                   isHovRestartM ? Color{185, 28, 28, 255} : Color{127, 29, 29, 255},
                   0.25f, isHovRestartM ? 0.35f : 0.0f);
    DrawTextCenteredInRect("RESTART", btnRestartModal, 14, WHITE);

    // Save button
    bool isHovSave = CheckCollisionPointRec(mouse, btnSaveModal);
    DrawSciFiPanel(btnSaveModal, Color{34, 197, 94, 255},
                   isHovSave ? Color{22, 163, 74, 255} : Color{21, 128, 61, 255},
                   0.25f, isHovSave ? 0.35f : 0.0f);
    DrawTextCenteredInRect("SAVE [F5]", btnSaveModal, 14, WHITE);

    // Load button
    bool isHovLoad = CheckCollisionPointRec(mouse, btnLoadModal);
    bool hasSave = HasSaveFile();
    DrawSciFiPanel(btnLoadModal, hasSave ? Color{14, 165, 233, 255} : Color{100, 116, 139, 255},
                   hasSave ? (isHovLoad ? Color{3, 105, 161, 255} : Color{14, 116, 144, 255})
                           : Color{51, 65, 85, 255},
                   0.25f, hasSave && isHovLoad ? 0.35f : 0.0f);
    DrawTextCenteredInRect(hasSave ? "LOAD [F9]" : "NO SAVE", btnLoadModal, 14,
                           hasSave ? WHITE : Color{148, 163, 184, 255});

    // Settings button
    bool isHovSettings = CheckCollisionPointRec(mouse, btnSettingsModal);
    DrawSciFiPanel(btnSettingsModal, Color{250, 204, 21, 255},
                   isHovSettings ? Color{234, 179, 8, 255} : Color{161, 98, 7, 255},
                   0.25f, isHovSettings ? 0.35f : 0.0f);
    DrawTextCenteredInRect("SETTINGS", btnSettingsModal, 14, WHITE);

    // Quit button
    bool isHovQuit = CheckCollisionPointRec(mouse, btnQuitModal);
    DrawSciFiPanel(btnQuitModal, Color{239, 68, 68, 255},
                   isHovQuit ? Color{185, 28, 28, 255} : Color{127, 29, 29, 255},
                   0.25f, isHovQuit ? 0.35f : 0.0f);
    DrawTextCenteredInRect("QUIT TO DESKTOP", btnQuitModal, 14, WHITE);

    DrawTextCentered("Progress is saved only via SAVE [F5]",
                     pauseBox.x + pauseBox.width * 0.5f, pauseBox.y + 372.0f, 12,
                     Color{100, 116, 139, 255});
  }

  // =================================
  // SETTINGS MENU
  // =================================
  if (showSettings) {
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.6f));
    Rectangle settingsBox{(float)(screenWidth - 500) / 2.0f, 70.0f, 500.0f, 540.0f};
    DrawSciFiPanel(settingsBox, Color{0, 200, 240, 255},
                   Fade(Color{12, 22, 36, 255}, 0.95f), 0.12f, 0.45f);
    DrawTextCentered("SETTINGS", settingsBox.x + settingsBox.width * 0.5f,
                     settingsBox.y + 26.0f, 26, Color{0, 200, 240, 255});

    auto DrawSlider = [](Rectangle slider, const char *label, float value,
                         Color color) {
      DrawText(label, (int)slider.x, (int)slider.y - 22, 14, WHITE);
      DrawText(TextFormat("%.0f%%", value * 100),
               (int)(slider.x + slider.width + 12), (int)slider.y - 2, 14,
               Color{250, 204, 21, 255});
      DrawRectangleRec(slider, Fade(Color{30, 41, 59, 255}, 0.8f));
      DrawRectangle((int)slider.x, (int)slider.y,
                    (int)(slider.width * value), (int)slider.height, color);
      DrawRectangleLinesEx(slider, 1.0f, color);
    };

    // Master Volume
    DrawSlider(masterVolSlider, "MASTER VOLUME", masterVolume,
               Color{0, 200, 240, 255});

    // SFX Volume
    DrawSlider(sfxVolSlider, "SFX VOLUME", sfxVolume, Color{34, 197, 94, 255});

    // Music Volume
    DrawSlider(musicVolSlider, "MUSIC VOLUME", musicVolume,
               Color{239, 68, 68, 255});

    // Fullscreen Toggle
    bool isHovFS = CheckCollisionPointRec(mouse, btnFullscreenToggle);
    DrawSciFiPanel(btnFullscreenToggle, fullscreen ? Color{34, 197, 94, 255} : Color{100, 116, 139, 255},
                   isHovFS ? (fullscreen ? Color{22, 163, 74, 255} : Color{82, 96, 114, 255})
                           : (fullscreen ? Color{21, 128, 61, 255} : Color{51, 65, 85, 255}),
                   0.25f, isHovFS ? 0.35f : 0.0f);
    DrawTextCenteredInRect(fullscreen ? "FULLSCREEN: ON" : "FULLSCREEN: OFF",
                           btnFullscreenToggle, 14, WHITE);

    // Resolution Display
    DrawText(TextFormat("RESOLUTION: %dx%d", windowWidth, windowHeight),
             (int)btnResolutionDropdown.x, (int)btnResolutionDropdown.y - 22, 14, WHITE);
    bool isHovRes = CheckCollisionPointRec(mouse, btnResolutionDropdown);
    DrawSciFiPanel(btnResolutionDropdown, Color{100, 116, 139, 255},
                   isHovRes ? Color{82, 96, 114, 255} : Color{51, 65, 85, 255},
                   0.25f, isHovRes ? 0.35f : 0.0f);
    DrawTextCenteredInRect("CHANGE (REQUIRES RESTART)", btnResolutionDropdown, 14,
                           Color{148, 163, 184, 255});

    // Back button
    bool isHovBack = CheckCollisionPointRec(mouse, btnBackSettings);
    DrawSciFiPanel(btnBackSettings, Color{239, 68, 68, 255},
                   isHovBack ? Color{185, 28, 28, 255} : Color{127, 29, 29, 255},
                   0.25f, isHovBack ? 0.35f : 0.0f);
    DrawTextCenteredInRect("BACK [ESC]", btnBackSettings, 14, WHITE);

    DrawTextCentered("[ESC/BACKSPACE] Close  |  Drag sliders to adjust",
                     settingsBox.x + settingsBox.width * 0.5f,
                     settingsBox.y + settingsBox.height - 40.0f, 12,
                     Color{148, 163, 184, 255});
  }

  // =================================
  // SELL CONFIRMATION DIALOG
  // =================================
  if (showSellConfirm) {
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
    Rectangle confirmBox{(float)(screenWidth - 380) / 2.0f, 250.0f, 380.0f, 220.0f};
    DrawSciFiPanel(confirmBox, Color{239, 68, 68, 255},
                   Fade(Color{15, 23, 42, 255}, 0.95f), 0.12f, 0.45f);
    float cx = confirmBox.x + confirmBox.width * 0.5f;
    DrawTextCentered("SELL TOWER?", cx, confirmBox.y + 20.0f, 26,
                     Color{239, 68, 68, 255});
    
    if (pendingSellTowerIndex >= 0 && pendingSellTowerIndex < (int)towers.size()) {
      Tower &selTower = towers[pendingSellTowerIndex];
      int sellVal = selTower.GetSellValue();
      DrawTextCentered(TextFormat("Tower: %s (Level %d)", Tower::GetTowerName(selTower.GetType()), selTower.GetLevel()),
                       cx, confirmBox.y + 58.0f, 16, WHITE);
      DrawTextCentered(TextFormat("Sell Value: $%d", sellVal), cx,
                       confirmBox.y + 82.0f, 18, Color{250, 204, 21, 255});
    }
    DrawTextCentered("This action cannot be undone!", cx,
                     confirmBox.y + 110.0f, 14, Color{239, 68, 68, 255});

    // Yes button
    bool isHovYes = CheckCollisionPointRec(mouse, btnSellConfirmYes);
    DrawSciFiPanel(btnSellConfirmYes, Color{34, 197, 94, 255},
                   isHovYes ? Color{22, 163, 74, 255} : Color{21, 128, 61, 255},
                   0.25f, isHovYes ? 0.35f : 0.0f);
    DrawTextCenteredInRect("YES [Y]", btnSellConfirmYes, 14, WHITE);

    // No button
    bool isHovNo = CheckCollisionPointRec(mouse, btnSellConfirmNo);
    DrawSciFiPanel(btnSellConfirmNo, Color{100, 116, 139, 255},
                   isHovNo ? Color{82, 96, 114, 255} : Color{51, 65, 85, 255},
                   0.25f, isHovNo ? 0.35f : 0.0f);
    DrawTextCenteredInRect("NO [N/ESC]", btnSellConfirmNo, 14, WHITE);
  }

  // =================================
  // RESTART CONFIRMATION DIALOG
  // =================================
  if (showRestartConfirm) {
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
    Rectangle confirmBox{(float)(screenWidth - 380) / 2.0f, 250.0f, 380.0f, 220.0f};
    DrawSciFiPanel(confirmBox, Color{239, 68, 68, 255},
                   Fade(Color{15, 23, 42, 255}, 0.95f), 0.12f, 0.45f);
    float cx = confirmBox.x + confirmBox.width * 0.5f;
    DrawTextCentered("RESTART GAME?", cx, confirmBox.y + 20.0f, 26,
                     Color{239, 68, 68, 255});
    DrawTextCentered("All progress will be lost!", cx,
                     confirmBox.y + 66.0f, 16, WHITE);
    DrawTextCentered("This action cannot be undone!", cx,
                     confirmBox.y + 92.0f, 14, Color{239, 68, 68, 255});

    // Yes button
    bool isHovYes = CheckCollisionPointRec(mouse, btnRestartConfirmYes);
    DrawSciFiPanel(btnRestartConfirmYes, Color{34, 197, 94, 255},
                   isHovYes ? Color{22, 163, 74, 255} : Color{21, 128, 61, 255},
                   0.25f, isHovYes ? 0.35f : 0.0f);
    DrawTextCenteredInRect("YES [Y]", btnRestartConfirmYes, 14, WHITE);

    // No button
    bool isHovNo = CheckCollisionPointRec(mouse, btnRestartConfirmNo);
    DrawSciFiPanel(btnRestartConfirmNo, Color{100, 116, 139, 255},
                   isHovNo ? Color{82, 96, 114, 255} : Color{51, 65, 85, 255},
                   0.25f, isHovNo ? 0.35f : 0.0f);
    DrawTextCenteredInRect("NO [N/ESC]", btnRestartConfirmNo, 14, WHITE);
  }

  // =================================
  // GAME OVER OVERLAY
  // =================================
  if (gameOver) {
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.75f));
    Rectangle overBox{(float)(screenWidth - 440) / 2.0f, 210.0f, 440.0f, 250.0f};
    DrawSciFiPanel(overBox, Color{239, 68, 68, 255},
                   Fade(Color{15, 23, 42, 255}, 0.95f), 0.1f, 0.5f);
    float cx = overBox.x + overBox.width * 0.5f;
    DrawTextCentered("GAME OVER", cx, overBox.y + 18.0f, 42,
                     Color{239, 68, 68, 255});
    DrawTextCentered(TextFormat("Kamu bertahan sampai WAVE %d", wave), cx,
                     overBox.y + 82.0f, 20, WHITE);
    
    // High Score display
    DrawTextCentered(TextFormat("HIGH SCORE: Wave %d  |  $%d", highScoreWave, highScoreMoney),
                     cx, overBox.y + 112.0f, 16, Color{250, 204, 21, 255});
    if (wave >= highScoreWave && money >= highScoreMoney) {
      DrawTextCentered("NEW RECORD!", cx, overBox.y + 134.0f, 18,
                       Color{250, 204, 21, 255});
    }

    // Interactive Restart Button
    bool isHovRestartGO = CheckCollisionPointRec(mouse, btnRestartGameOver);
    DrawSciFiPanel(btnRestartGameOver, Color{0, 200, 240, 255},
                   isHovRestartGO ? Color{8, 145, 178, 255} : Color{14, 116, 144, 255},
                   0.25f, isHovRestartGO ? 0.4f : 0.1f);
    DrawTextCenteredInRect("PLAY AGAIN [R]", btnRestartGameOver, 16, WHITE);

    DrawTextCentered("Atau tutup window untuk keluar", cx,
                     overBox.y + 214.0f, 14, Color{148, 163, 184, 255});
  }

  // =================================
  // VICTORY OVERLAY
  // =================================
  if (victory) {
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
    Rectangle vicBox{(float)(screenWidth - 500) / 2.0f, 180.0f, 500.0f, 320.0f};
    DrawSciFiPanel(vicBox, Color{250, 204, 21, 255},
                   Fade(Color{15, 23, 42, 255}, 0.95f), 0.1f, 0.5f);
    float cx = vicBox.x + vicBox.width * 0.5f;
    DrawTextCentered("VICTORY!", cx, vicBox.y + 24.0f, 48,
                     Color{250, 204, 21, 255});
    DrawTextCentered("Kamu telah menyelesaikan 50 wave!", cx,
                     vicBox.y + 78.0f, 20, WHITE);
    DrawTextCentered(TextFormat("Base HP tersisa: %d / %d", baseHP, maxBaseHP),
                     cx, vicBox.y + 108.0f, 17, Color{34, 197, 94, 255});
    DrawTextCentered(TextFormat("Uang tersisa: $%d", money), cx,
                     vicBox.y + 132.0f, 17, Color{250, 204, 21, 255});
    DrawTextCentered(TextFormat("BEST: Wave %d  |  $%d", highScoreWave, highScoreMoney),
                     cx, vicBox.y + 156.0f, 15, Color{250, 204, 21, 255});
    DrawTextCentered("Lanjutkan ke Endless Mode?", cx, vicBox.y + 182.0f, 17,
                     Color{148, 163, 184, 255});

    // Restart button
    bool isHovVicRestart = CheckCollisionPointRec(mouse, btnVictoryRestart);
    DrawSciFiPanel(btnVictoryRestart, Color{0, 200, 240, 255},
                   isHovVicRestart ? Color{8, 145, 178, 255} : Color{14, 116, 144, 255},
                   0.25f, isHovVicRestart ? 0.4f : 0.1f);
    DrawTextCenteredInRect("RESTART [R]", btnVictoryRestart, 16, WHITE);

    // Continue button (Endless Mode)
    bool isHovVicContinue = CheckCollisionPointRec(mouse, btnVictoryContinue);
    DrawSciFiPanel(btnVictoryContinue, Color{34, 197, 94, 255},
                   isHovVicContinue ? Color{22, 163, 74, 255} : Color{21, 128, 61, 255},
                   0.25f, isHovVicContinue ? 0.4f : 0.1f);
    DrawTextCenteredInRect("CONTINUE", btnVictoryContinue, 16, WHITE);
  }

  // =================================
  // TOOLTIP (with fade animation)
  // =================================
  if (showTooltip && tooltipTimer > 0.0f) {
    float baseAlpha = tooltipTimer / 0.3f;
    if (baseAlpha > 1.0f) baseAlpha = 1.0f;
    float alpha = baseAlpha * uiTooltipFade;
    
    // Calculate tooltip size
    int maxLineWidth = 0;
    std::string text = tooltipText;
    size_t start = 0;
    size_t end = text.find('\n');
    int lineCount = 0;
    while (end != std::string::npos) {
      std::string line = text.substr(start, end - start);
      int w = MeasureText(line.c_str(), 12);
      if (w > maxLineWidth) maxLineWidth = w;
      lineCount++;
      start = end + 1;
      end = text.find('\n', start);
    }
    std::string lastLine = text.substr(start);
    int w = MeasureText(lastLine.c_str(), 12);
    if (w > maxLineWidth) maxLineWidth = w;
    lineCount++;
    
    int tooltipWidth = maxLineWidth + 20;
    int tooltipHeight = lineCount * 16 + 10;
    
    // Keep tooltip on screen
    float tx = tooltipPos.x;
    float ty = tooltipPos.y;
    if (tx + tooltipWidth > screenWidth - 10) tx = screenWidth - tooltipWidth - 10;
    if (ty + tooltipHeight > screenHeight - 10) ty = screenHeight - tooltipHeight - 10;
    
    Rectangle tooltipRect{tx, ty, (float)tooltipWidth, (float)tooltipHeight};
    DrawRectangleRounded(tooltipRect, 0.15f, 4, Fade(Design::BG_PANEL, 0.95f * alpha));
    DrawRectangleRoundedLinesEx(tooltipRect, 0.15f, 4, 1.5f, Fade(Design::BORDER_PRIMARY, alpha));
    
    // Pointer arrow (pointing to cursor)
    Vector2 arrowTip{tooltipPos.x, tooltipPos.y};
    Vector2 arrowLeft{tx - 8, ty + tooltipHeight * 0.3f};
    Vector2 arrowRight{tx - 8, ty + tooltipHeight * 0.7f};
    DrawTriangle(arrowTip, arrowLeft, arrowRight, Fade(Design::BORDER_PRIMARY, alpha));
    DrawTriangle(arrowTip, arrowLeft, arrowRight, Fade(Design::BG_PANEL, 0.95f * alpha));
    
    // Draw text lines
    start = 0;
    end = text.find('\n');
    int lineY = (int)ty + 5;
    while (end != std::string::npos) {
      std::string line = text.substr(start, end - start);
      DrawText(line.c_str(), (int)tx + 10, lineY, 12, Fade(Design::TEXT_PRIMARY, alpha));
      lineY += 16;
      start = end + 1;
      end = text.find('\n', start);
    }
    DrawText(lastLine.c_str(), (int)tx + 10, lineY, 12, Fade(Design::TEXT_PRIMARY, alpha));
  }

  // =================================
  // TUTORIAL OVERLAY
  // =================================
  if (showTutorial && !gameOver && !victory) {
    float alpha = 1.0f;
    if (tutorialTimer > 30.0f) {
      alpha = 1.0f - (tutorialTimer - 30.0f) / 5.0f;
      if (alpha <= 0.0f) {
        showTutorial = false;
        alpha = 0.0f;
      }
    }
    
if (alpha > 0.0f) {
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.6f * alpha));
        Rectangle tutBox{(float)(screenWidth - 500) / 2.0f, 180.0f, 500.0f, 380.0f};
        DrawSciFiPanel(tutBox, Color{0, 200, 240, 255},
                       Fade(Color{12, 22, 36, 255}, 0.95f * alpha), 0.1f, 0.4f * alpha);
        
        int titleW = MeasureText("CONTROLS", 26);
        DrawText("CONTROLS", (int)tutBox.x + (500 - titleW) / 2, (int)tutBox.y + 20, 26, Fade(Color{0, 200, 240, 255}, alpha));
        
        const char* controls[] = {
          "[1,2,3] / Click cards  - Select tower to build",
          "[ESC] / Right-click    - Cancel build mode",
          "[Click tower]          - Select placed tower",
          "[U]                    - Upgrade selected tower",
          "[S]                    - Sell selected tower",
          "[T]                    - Cycle targeting mode",
          "[Z]                    - Undo last placement",
          "[SPACE]                - Pause / Resume",
          "[F] / [TAB]            - Cycle game speed (1X/2X/3X)",
          "[ENTER]                - Start wave (when ready)",
          "[F5]                   - Save game",
          "[F9]                   - Load game",
          "",
          "Click anywhere or press any key to dismiss"
        };
        
        int y = (int)tutBox.y + 65;
        for (const char* line : controls) {
          int w = MeasureText(line, 14);
          DrawText(line, (int)tutBox.x + (500 - w) / 2, y, 14, Fade(WHITE, alpha));
          y += 22;
        }
      }
  }

  EndDrawing();
}

bool Game::IsOnRoad(Vector2 position) const {
  const float roadWidth = 80.0f;

  for (size_t i = 0; i < path.size() - 1; i++) {
    Vector2 start = path[i];
    Vector2 end = path[i + 1];

    // Jalan horizontal
    if (start.y == end.y) {
      float minX = std::min(start.x, end.x);
      float maxX = std::max(start.x, end.x);

      if (position.x >= minX - roadWidth / 2 &&
          position.x <= maxX + roadWidth / 2 &&
          position.y >= start.y - roadWidth / 2 &&
          position.y <= start.y + roadWidth / 2) {
        return true;
      }
    }
    // Jalan vertical
    else if (start.x == end.x) {
      float minY = std::min(start.y, end.y);
      float maxY = std::max(start.y, end.y);

      if (position.x >= start.x - roadWidth / 2 &&
          position.x <= start.x + roadWidth / 2 &&
          position.y >= minY - roadWidth / 2 &&
          position.y <= maxY + roadWidth / 2) {
        return true;
      }
    }
  }

  return false;
}

bool Game::IsNearTower(Vector2 position) const {
  for (const Tower &tower : towers) {
    if (Vector2Distance(position, tower.GetPosition()) < Design::TOWER_MIN_SPACING) {
      return true;
    }
  }

  return false;
}

#include "json.hpp"
using json = nlohmann::json;

void Game::SaveGame(const std::string& slotName) {
  json j;
  j["saveVersion"] = 2;
  j["money"] = money;
  j["baseHP"] = baseHP;
  j["wave"] = wave;
  j["enemiesToSpawn"] = enemiesToSpawn;
  j["enemiesSpawned"] = enemiesSpawned;
  j["spawnTimer"] = spawnTimer;
  j["waveInProgress"] = waveInProgress;
  j["nextWaveTimer"] = nextWaveTimer;
  j["bossWaveTimer"] = bossWaveTimer;
  j["gameSpeed"] = gameSpeed;
  j["selectedTowerType"] = static_cast<int>(selectedTowerType);
  j["isBuildMode"] = isBuildMode;
  j["selectedTowerIndex"] = selectedTowerIndex;
  j["waveReadyToStart"] = waveReadyToStart;
  j["globalTime"] = globalTime;

  json towersJson = json::array();
  for (const auto& tower : towers) {
    json t;
    t["x"] = tower.GetPosition().x;
    t["y"] = tower.GetPosition().y;
    t["type"] = static_cast<int>(tower.GetType());
    t["level"] = tower.GetLevel();
    t["damage"] = tower.GetDamage();
    t["range"] = tower.GetRange();
    t["cooldown"] = tower.GetCooldown();
    t["aoeRadius"] = tower.GetAoERadius();
    t["totalInvested"] = tower.GetTotalInvested();
    t["targetingMode"] = static_cast<int>(tower.GetTargetingMode());
    towersJson.push_back(t);
  }
  j["towers"] = towersJson;

  json enemiesJson = json::array();
  for (const auto& enemy : enemies) {
    if (!enemy.IsAlive()) continue;
    json e;
    e["x"] = enemy.GetPosition().x;
    e["y"] = enemy.GetPosition().y;
    e["type"] = static_cast<int>(enemy.GetType());
    e["hp"] = enemy.GetHp();
    e["maxHp"] = enemy.GetMaxHp();
    e["targetPoint"] = (int)enemy.GetTargetPoint();
    enemiesJson.push_back(e);
  }
  j["enemies"] = enemiesJson;

  std::string filename = slotName + ".json";
  std::ofstream file(filename);
  if (file.is_open()) {
    file << j.dump(2);
    file.close();
    AddFloatingText(Vector2{640, 360}, "Game Saved!", Color{34, 197, 94, 255}, 1.5f);
  }
}

void Game::LoadGame(const std::string& slotName) {
  std::string filename = slotName + ".json";
  std::ifstream file(filename);
  if (!file.is_open()) {
    AddFloatingText(Vector2{640, 360}, "No save file found!", Color{239, 68, 68, 255}, 1.5f);
    return;
  }

  json j;
  try {
    file >> j;
  } catch (...) {
    file.close();
    AddFloatingText(Vector2{640, 360}, "Corrupted save file!", Color{239, 68, 68, 255}, 1.5f);
    return;
  }
  file.close();

  // Accept both v1 (legacy) and v2 saves. v2 adds targetingMode per tower and
  // drops the bogus per-enemy "speed" field.
  int saveVersion = j.value("saveVersion", 0);
  if (saveVersion != 1 && saveVersion != 2) {
    AddFloatingText(Vector2{640, 360}, "Incompatible save version!", Color{239, 68, 68, 255}, 1.5f);
    return;
  }

  money = j.value("money", 500);
  baseHP = j.value("baseHP", maxBaseHP);
  wave = j.value("wave", 1);
  enemiesToSpawn = j.value("enemiesToSpawn", 5);
  enemiesSpawned = j.value("enemiesSpawned", 0);
  spawnTimer = j.value("spawnTimer", 2.0f);
  waveInProgress = j.value("waveInProgress", true);
  nextWaveTimer = j.value("nextWaveTimer", 0.0f);
  bossWaveTimer = j.value("bossWaveTimer", 0.0f);
  gameSpeed = j.value("gameSpeed", 1.0f);
  selectedTowerType = static_cast<TowerType>(j.value("selectedTowerType", 0));
  isBuildMode = j.value("isBuildMode", false);
  selectedTowerIndex = j.value("selectedTowerIndex", -1);
  waveReadyToStart = j.value("waveReadyToStart", true);
  globalTime = j.value("globalTime", 0.0f);

  // Reset transient session state so a load never inherits a stale result.
  gameOver = false;
  victory = false;
  endlessMode = wave > 50;   // a save past wave 50 is already endless
  isPaused = false;
  showSellConfirm = false;
  pendingSellTowerIndex = -1;
  showRestartConfirm = false;
  pendingRestartFromPause = false;
  notEnoughMoneyTimer = 0.0f;
  canUndo = false;
  lastPlacement.valid = false;
  hitPauseTimer = 0.0f;
  hitPauseDuration = 0.0f;
  timeScale = 1.0f;
  timeScaleTarget = 1.0f;
  timeScaleRestoreTimer = 0.0f;

  towers.clear();
  if (j.contains("towers")) {
    for (const auto& t : j["towers"]) {
      Vector2 pos{t.value("x", 0.0f), t.value("y", 0.0f)};
      TowerType type = static_cast<TowerType>(t.value("type", 0));
      Tower tower(pos, type);
      int level = t.value("level", 1);
      for (int i = 1; i < level; i++) {
        tower.Upgrade();
      }
      // Restore the exact invested total so the sell value matches the save
      // (upgrade() only reflects base costs, not the actual amount paid).
      if (t.contains("totalInvested")) {
        tower.SetTotalInvested(t.value("totalInvested", tower.GetTotalInvested()));
      }
      if (saveVersion >= 2) {
        int mode = t.value("targetingMode", 0);
        if (mode < 0 || mode > 3) mode = 0;
        tower.SetTargetingMode(static_cast<TargetingMode>(mode));
      }
      towers.push_back(std::move(tower));
    }
  }

  // Selection index must still be valid after loading the tower list.
  if (selectedTowerIndex < -1 || selectedTowerIndex >= (int)towers.size()) {
    selectedTowerIndex = -1;
  }

  enemies.clear();
  if (j.contains("enemies")) {
    for (const auto& e : j["enemies"]) {
      Vector2 pos{e.value("x", 0.0f), e.value("y", 0.0f)};
      EnemyType type = static_cast<EnemyType>(e.value("type", 0));
      Enemy enemy(pos, type);
      int hp = e.value("hp", enemy.GetMaxHp());
      enemy.RestoreHp(hp);
      enemy.SetTargetPoint((size_t)e.value("targetPoint", 1));
      enemies.push_back(std::move(enemy));
    }
  }

  bullets.clear();
  explosions.clear();
  particles.clear();
  floatingTexts.clear();

  AddFloatingText(Vector2{640, 360}, "Game Loaded!", Color{34, 197, 94, 255}, 1.5f);
}

bool Game::HasSaveFile(const std::string& slotName) const {
  std::string filename = slotName + ".json";
  std::ifstream file(filename);
  return file.good();
}

void Game::LoadSettings() {
  std::string filename = "settings.json";
  std::ifstream file(filename);
  if (!file.is_open()) {
    // Default values
    masterVolume = 0.8f;
    sfxVolume = 1.0f;
    musicVolume = 0.5f;
    fullscreen = false;
    windowWidth = 1280;
    windowHeight = 720;
    highScoreWave = 0;
    highScoreMoney = 0;
    return;
  }

  json j;
  try {
    file >> j;
  } catch (...) {
    file.close();
    return;
  }
  file.close();

  masterVolume = j.value("masterVolume", 0.8f);
  sfxVolume = j.value("sfxVolume", 1.0f);
  musicVolume = j.value("musicVolume", 0.5f);
  fullscreen = j.value("fullscreen", false);
  windowWidth = j.value("windowWidth", 1280);
  windowHeight = j.value("windowHeight", 720);
  highScoreWave = j.value("highScoreWave", 0);
  highScoreMoney = j.value("highScoreMoney", 0);
}

void Game::SaveSettings() {
  json j;
  j["masterVolume"] = masterVolume;
  j["sfxVolume"] = sfxVolume;
  j["musicVolume"] = musicVolume;
  j["fullscreen"] = fullscreen;
  j["windowWidth"] = windowWidth;
  j["windowHeight"] = windowHeight;
  j["highScoreWave"] = highScoreWave;
  j["highScoreMoney"] = highScoreMoney;

  std::string filename = "settings.json";
  std::ofstream file(filename);
  if (file.is_open()) {
    file << j.dump(2);
    file.close();
  }
}

void Game::ApplySettings() {
  SetMasterVolume(masterVolume);
  SetSoundVolume(audio.shootBasic, sfxVolume);
  SetSoundVolume(audio.shootRapid, sfxVolume);
  SetSoundVolume(audio.shootCannon, sfxVolume);
  SetSoundVolume(audio.explosion, sfxVolume);
  SetSoundVolume(audio.enemyDeath, sfxVolume);
  SetSoundVolume(sndBuild, sfxVolume);
  SetSoundVolume(sndSell, sfxVolume);
  SetSoundVolume(sndBaseDamage, sfxVolume);
  SetSoundVolume(sndBossWave, sfxVolume);
  SetSoundVolume(sndGameOver, sfxVolume);
  if (fullscreen) {
    int monitor = GetCurrentMonitor();
    int w = GetMonitorWidth(monitor);
    int h = GetMonitorHeight(monitor);
    SetWindowSize(w, h);
    ToggleFullscreen();
  } else {
    SetWindowSize(windowWidth, windowHeight);
  }
}

void Game::ToggleFullscreen() {
  fullscreen = !fullscreen;
  ::ToggleFullscreen();
  if (!fullscreen) {
    SetWindowSize(windowWidth, windowHeight);
  }
}

void Game::CheckAndUpdateHighScores() {
  bool newHighScore = false;
  if (wave > highScoreWave) {
    highScoreWave = wave;
    newHighScore = true;
  }
  if (money > highScoreMoney) {
    highScoreMoney = money;
    newHighScore = true;
  }
  if (newHighScore) {
    SaveSettings();
    AddFloatingText(Vector2{640, 300}, "NEW HIGH SCORE!", Color{250, 204, 21, 255}, 2.0f);
  }
}

void Game::TriggerScreenShake(float intensity, float duration) {
  screenShakeIntensity = intensity;
  screenShakeTimer = duration;
}

void Game::TriggerHitPause(float duration) {
  hitPauseTimer = duration;
  hitPauseDuration = duration;
}

void Game::TriggerTimeScale(float targetScale) {
  timeScaleTarget = targetScale;
  timeScaleRestoreTimer = 0.15f; // Restore after 0.15s real time
}
