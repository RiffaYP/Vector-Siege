#pragma once

#include "raylib.h"
#include <vector>
#include <string>
#include <functional>

class Enemy;

struct Explosion {
  Vector2 position;
  float radius;
  float timer;
  float maxLifetime;
};

struct Particle {
  Vector2 position;
  Vector2 velocity;
  Color color;
  float radius;
  float lifetime;
  float maxLifetime;
};

struct FloatingText {
  Vector2 position;
  char text[32];
  Color color;
  float lifetime;
  float maxLifetime;
  float floatSpeed;
};

struct GameAudio {
  Sound shootBasic;
  Sound shootRapid;
  Sound shootCannon;
  Sound explosion;
  Sound enemyDeath;
};

class Bullet {
private:
  Vector2 position;
  Vector2 direction;
  float speed;
  int damage;
  float radius;
  Color color;
  bool active;
  bool isAoE;
  float aoeRadius;
  float age;          // seconds alive, used to expire stray shots
  std::vector<Vector2> trailPositions;
  static const int MAX_TRAIL_LENGTH = 8;
  // A bullet that has not hit anything within this many seconds is culled.
  // Keeps the bullet list bounded when shots miss completely.
  static constexpr float MAX_LIFETIME = 3.0f;

public:
  Bullet(Vector2 startPos, Vector2 dir, float spd, int dmg, float rad = 5.0f,
         Color col = YELLOW, bool aoe = false, float aoeRad = 0.0f);

  void Update(float deltaTime);
  void CheckCollision(std::vector<Enemy> &enemies, int &playerMoney,
                      std::vector<Explosion> &explosions,
                      std::vector<Particle> &particles,
                      std::vector<FloatingText> &floatingTexts,
                      const GameAudio &audio,
                      std::function<void(Vector2, bool, bool, Color)> onHit = nullptr);

  bool IsActive() const { return active; }
  Vector2 GetPosition() const { return position; }
  Vector2 GetDirection() const { return direction; }
  Color GetColor() const { return color; }
  bool IsAoE() const { return isAoE; }
  float GetRadius() const { return radius; }
  const std::vector<Vector2>& GetTrailPositions() const { return trailPositions; }
};
