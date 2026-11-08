#pragma once

#include "raylib.h"
#include <vector>

enum class EnemyType {
    NORMAL,
    FAST,
    TANK,
    BOSS
};

class Enemy {
private:
    EnemyType type;
    Vector2 position;
    float speed;
    int hp;
    int maxHp;
    int reward;
    float radius;
    Color color;

    size_t targetPoint;
    float pathProgress;
    bool alive;
    bool dying;
    float deathTimer;
    float deathDuration;

    float walkTimer;
    float facingAngle;

    // Lean/bob animation
    float leanAngle;
    float targetLeanAngle;
    float leanSpeed;
    float bobOffset;
    float bobSpeed;
    float animTimer;  // Real-time animation timer (not affected by timeScale)
    
    // Hit feedback
    float hitFlashTimer;
    Color hitFlashColor;

public:
    Enemy(Vector2 startPos, EnemyType enemyType);

    void Update(float deltaTime, float animDeltaTime, const std::vector<Vector2>& path);

    void TakeDamage(int damage);

    // Restores HP directly (used by save/load). Bypasses damage/crit logic and
    // never kills the enemy; clamps to [0, maxHp] and revives if needed.
    void RestoreHp(int newHp);

    bool IsAlive() const { return alive; }
    void SetAlive(bool state) { alive = state; }
    bool IsDying() const { return dying; }
    float GetDeathTimer() const { return deathTimer; }
    float GetDeathDuration() const { return deathDuration; }

    bool HasReachedEnd(size_t pathSize) const { return alive && targetPoint >= pathSize; }

    size_t GetTargetPoint() const { return targetPoint; }
    void SetTargetPoint(size_t p) { targetPoint = p < 1 ? 1 : p; }

    Vector2 GetPosition() const { return position; }
    int GetHp() const { return hp; }
    int GetMaxHp() const { return maxHp; }
    int GetReward() const { return reward; }
    float GetRadius() const { return radius; }
    EnemyType GetType() const { return type; }

    float GetWalkTimer() const { return walkTimer; }
    float GetFacingAngle() const { return facingAngle; }
    float GetLeanAngle() const { return leanAngle; }
    float GetBobOffset() const { return bobOffset; }
    float GetHitFlashTimer() const { return hitFlashTimer; }
    float GetPathProgress() const;
};
