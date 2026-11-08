#pragma once

#include "Bullet.h"
#include "Enemy.h"
#include "Tower.h"
#include "Renderer3D.h"
#include "raylib.h"
#include <vector>
#include <string>
#include <algorithm>

// ============================================================================
// DESIGN TOKENS - Centralized visual design system
// ============================================================================
namespace Design {
  // Colors
  constexpr Color BG_DARK       = {8, 16, 26, 255};
  constexpr Color BG_PANEL      = {10, 20, 32, 255};
  constexpr Color BG_PANEL_ALT  = {15, 23, 42, 255};
  constexpr Color BORDER_PRIMARY = {0, 200, 240, 255};
  constexpr Color BORDER_ACCENT  = {34, 197, 94, 255};
  constexpr Color BORDER_WARN    = {239, 68, 68, 255};
  constexpr Color BORDER_GOLD    = {250, 204, 21, 255};
  constexpr Color TEXT_PRIMARY   = {248, 250, 252, 255};
  constexpr Color TEXT_SECONDARY = {148, 163, 184, 255};
  constexpr Color TEXT_MUTED     = {100, 116, 139, 255};
  constexpr Color TEXT_GOLD      = {250, 204, 21, 255};
  constexpr Color HEALTH_GREEN   = {34, 197, 94, 255};
  constexpr Color HEALTH_YELLOW  = {234, 179, 8, 255};
  constexpr Color HEALTH_RED     = {239, 68, 68, 255};
  constexpr Color MANA_CYAN      = {6, 182, 212, 255};
  constexpr Color MANA_BLUE      = {14, 165, 233, 255};
  constexpr Color PURPLE_MAGIC   = {168, 85, 247, 255};
  constexpr Color ORANGE_FIRE    = {249, 115, 22, 255};

  // Tower type colors
  constexpr Color TOWER_BASIC   = {34, 197, 94, 255};
  constexpr Color TOWER_RAPID   = {14, 165, 233, 255};
  constexpr Color TOWER_CANNON  = {239, 68, 68, 255};

  // Enemy type colors
  constexpr Color ENEMY_NORMAL  = {239, 68, 68, 255};
  constexpr Color ENEMY_FAST    = {249, 115, 22, 255};
  constexpr Color ENEMY_TANK    = {168, 85, 247, 255};
  constexpr Color ENEMY_BOSS    = {236, 72, 153, 255};

  // Spacing & Sizing
  constexpr float SPACING_XS    = 4.0f;
  constexpr float SPACING_SM    = 8.0f;
  constexpr float SPACING_MD    = 16.0f;
  constexpr float SPACING_LG    = 24.0f;
  constexpr float SPACING_XL    = 32.0f;
  constexpr float RADIUS_SM     = 0.08f;
  constexpr float RADIUS_MD     = 0.12f;
  constexpr float RADIUS_LG     = 0.2f;
  constexpr float BORDER_THIN   = 1.0f;
  constexpr float BORDER_THICK  = 2.0f;

  // Typography
  constexpr int FONT_XS  = 10;
  constexpr int FONT_SM  = 12;
  constexpr int FONT_MD  = 14;
  constexpr int FONT_LG  = 16;
  constexpr int FONT_XL  = 20;
  constexpr int FONT_2XL = 26;
  constexpr int FONT_3XL = 36;

  // Animation
  constexpr float FADE_SPEED     = 8.0f;
  constexpr float SLIDE_SPEED    = 10.0f;
  constexpr float PULSE_SPEED    = 3.0f;
  constexpr float BOB_SPEED      = 8.0f;
  constexpr float LEAN_SPEED     = 8.0f;

  // Z-depth layers (for 3D)
  constexpr float Z_GROUND      = 0.0f;
  constexpr float Z_AMBIENT     = 0.2f;
  constexpr float Z_EFFECTS     = 0.5f;
  constexpr float Z_PROJECTILES = 1.4f;
  constexpr float Z_FLOATING    = 2.8f;

  // ---- Tower geometry (2D gameplay units) ---------------------------------
  // The 3D renderer draws tower bases with a world radius of up to 3.0 units
  // (cannon), and To3D() scales 2D->3D by 0.045. A 3.0-unit base therefore
  // spans ~33 px per side in gameplay space.
  constexpr float TO3D_SCALE          = 0.045f;
  constexpr float TOWER_MAX_BASE_UNITS = 3.0f;                 // widest footprint (cannon)
  // Visual half-extent of the widest tower, in gameplay pixels.
  constexpr float TOWER_VISUAL_RADIUS =
      (TOWER_MAX_BASE_UNITS * 0.5f) / TO3D_SCALE;              // ~33.3 px
  // Click / hover radius must comfortably cover the drawn footprint.
  constexpr float TOWER_CLICK_RADIUS  = TOWER_VISUAL_RADIUS + 10.0f;  // ~43 px
  // Minimum distance between two tower centers. Must exceed the sum of two
  // visual radii so placed towers never overlap on screen.
  constexpr float TOWER_MIN_SPACING   = TOWER_VISUAL_RADIUS * 2.0f + 8.0f; // ~75 px

  // ---- Virtual canvas (fixed-resolution rendering) ------------------------
  // The whole game is rendered into a fixed 1280x720 framebuffer and then
  // letterboxed onto whatever window size / display mode is active. This
  // keeps every hard-coded UI coordinate valid in windowed, borderless and
  // fullscreen modes on any monitor.
  constexpr int VIRTUAL_W = 1280;
  constexpr int VIRTUAL_H = 720;
}

class Game {
private:
  // Window settings
  const int screenWidth;
  const int screenHeight;

  // Player & Base
  int money;
  int baseHP;
  const int maxBaseHP;

  // Tower Selection & Build Mode
  TowerType selectedTowerType;
  bool isBuildMode;

  // Bottom Tower Selection Cards
  Rectangle btnBasic;
  Rectangle btnRapid;
  Rectangle btnCannon;

  // Placed Tower Selection & Info Panel
  int selectedTowerIndex;
  Rectangle infoPanelRec;
  Rectangle btnUpgrade;
  Rectangle btnSell;
  float notEnoughMoneyTimer;

  // Wave system & Boss Wave
  int wave;
  int enemiesToSpawn;
  int enemiesSpawned;
  float spawnTimer;
  const float spawnInterval;
  bool waveInProgress;
  float nextWaveTimer;
  float bossWaveTimer;
  bool waveReadyToStart;
  Rectangle btnStartWave;

  // Undo placement
  struct LastPlacement {
    Vector2 position;
    TowerType type;
    int cost;
    bool valid;
  };
  LastPlacement lastPlacement;
  bool canUndo;

  // Game state & controls
  bool gameOver;
  bool isPaused;
  bool victory;
  bool endlessMode;   // true once the player chose "Continue" after wave 50
  float gameSpeed;

  // High Scores
  int highScoreWave;
  int highScoreMoney;

  // Screen Shake
  float screenShakeIntensity;
  float screenShakeTimer;
  Vector2 screenShakeOffset;

  // Screen Juice / Hit Pause / Time Scale
  float hitPauseTimer;
  float hitPauseDuration;
  float timeScale;
  float timeScaleTarget;
  float timeScaleLerpSpeed;
  float timeScaleRestoreTimer;

  // Camera micro-follow
  // REMOVED: disabled for player comfort; trigger/callbacks were dead code.

  // Settings
  float masterVolume;
  float sfxVolume;
  float musicVolume;
  bool fullscreen;
  int windowWidth;
  int windowHeight;
  bool showSettings;

  // Control Buttons
  Rectangle btnPause;
  Rectangle btnSpeed;
  Rectangle btnRestartGameOver;
  Rectangle btnResumeModal;
  Rectangle btnRestartModal;
  Rectangle btnSaveModal;
  Rectangle btnLoadModal;
  Rectangle btnSettingsModal;
  Rectangle btnBackSettings;
  Rectangle btnFullscreenToggle;
  Rectangle btnResolutionDropdown;
  Rectangle masterVolSlider;
  Rectangle sfxVolSlider;
  Rectangle musicVolSlider;
  Rectangle btnQuitModal;
  Rectangle btnVictoryRestart;
  Rectangle btnVictoryContinue;
  Rectangle btnSellConfirmYes;
  Rectangle btnSellConfirmNo;
  Rectangle btnRestartConfirmYes;
  Rectangle btnRestartConfirmNo;
  bool draggingMasterVol;
  bool draggingSfxVol;
  bool draggingMusicVol;
  bool showSellConfirm;
  int pendingSellTowerIndex;
  
  // Restart confirmation
  bool showRestartConfirm;
  bool pendingRestartFromPause;

  // Tutorial
  bool showTutorial;
  float tutorialTimer;

  // Tooltip system
  std::string tooltipText;
  Vector2 tooltipPos;
  float tooltipTimer;
  bool showTooltip;

  // Path
  std::vector<Vector2> path;

  // Entitas game & VFX
  std::vector<Enemy> enemies;
  std::vector<Tower> towers;
  std::vector<Bullet> bullets;
  std::vector<Explosion> explosions;
  std::vector<Particle> particles;
  std::vector<FloatingText> floatingTexts;
  std::vector<Particle> ambientParticles;

  // UI Animation State
  float uiWavePreviewTimer;
  bool uiWavePreviewActive;
  float uiTooltipFade;
  float uiPanelSlide;
  bool uiPanelVisible;

  // Audio SFX
  GameAudio audio;
  Sound sndBuild;
  Sound sndSell;
  Sound sndBaseDamage;
  Sound sndBossWave;
  Sound sndGameOver;

  // 3D Camera & Time
  Camera3D camera;
  float globalTime;

  // Custom TrueType font (Rajdhani, SIL OFL). Used for every piece of text so
  // the UI no longer relies on raylib's tiny built-in pixel font.
  Font uiFont;
  Font uiFontBold;

  // Virtual-resolution framebuffer + letterbox transform
  RenderTexture2D canvas;
  Rectangle canvasDest;     // where the virtual canvas lands on screen
  float canvasScale;        // dest / virtual ratio (uniform, letterboxed)

  // Screen resolution options offered in the settings menu
  static constexpr int RES_COUNT = 5;
  int resIndex;             // currently applied windowed resolution
  bool showResDropdown;     // resolution picker open?
  Rectangle resOptionRec[RES_COUNT];

  // Helper functions
  bool IsOnRoad(Vector2 position) const;
  bool IsNearTower(Vector2 position) const;
  void ToggleBuildMode(TowerType type);
  bool IsBossWave(int w) const { return w > 0 && (w % 5 == 0); }
  void Restart();
  void AddFloatingText(Vector2 pos, const char* text, Color color, float duration = 0.9f);
  void AddShockwaveParticles(Vector2 pos, Color color, int count = 16);
  void CheckAndUpdateHighScores();
  void TriggerScreenShake(float intensity, float duration);
  void TriggerHitPause(float duration);
  void TriggerTimeScale(float targetScale);

  // Virtual-canvas helpers
  void UpdateCanvasLayout();          // recompute canvasDest + canvasScale
  Vector2 GetMousePositionVirtual() const;  // mouse in 1280x720 space
  Vector2 ScreenToVirtual(Vector2 screen) const;
  Camera3D GetVirtualCamera() const;  // camera honouring screen->virtual

  // 3D mouse picking
  Vector2 GetMouseWorld2D() const;

  // Resolution helpers
  static const char* GetResolutionLabel(int index);
  static void GetResolution(int index, int& w, int& h);

public:
  Game();
  ~Game();

  void Run();
  void Update(float deltaTime);
  void Draw();

  // Save/Load system
  void SaveGame(const std::string& slotName = "save1");
  void LoadGame(const std::string& slotName = "save1");
  bool HasSaveFile(const std::string& slotName = "save1") const;

  // Settings system
  void LoadSettings();
  void SaveSettings();
  void ApplySettings();
  void ToggleFullscreen();
};
