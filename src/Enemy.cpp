#include "Enemy.h"
#include "raymath.h"
#include <algorithm>

Enemy::Enemy(Vector2 startPos, EnemyType enemyType)
    : type(enemyType), position(startPos), targetPoint(1), pathProgress(0.0f), alive(true),
      dying(false), deathTimer(0.0f), deathDuration(0.5f),
      walkTimer(0.0f), facingAngle(0.0f),
      leanAngle(0.0f), targetLeanAngle(0.0f), leanSpeed(8.0f),
      bobOffset(0.0f), bobSpeed(12.0f), animTimer(0.0f),
      hitFlashTimer(0.0f), hitFlashColor(WHITE) {
    switch (type) {
        case EnemyType::NORMAL:
            hp = 100;
            maxHp = 100;
            speed = 80.0f;
            reward = 25;
            radius = 20.0f;
            color = RED;
            deathDuration = 0.4f;
            break;
        case EnemyType::FAST:
            hp = 60;
            maxHp = 60;
            speed = 140.0f;
            reward = 30;
            radius = 15.0f;
            color = GOLD;
            deathDuration = 0.3f;
            break;
        case EnemyType::TANK:
            hp = 250;
            maxHp = 250;
            speed = 45.0f;
            reward = 60;
            radius = 26.0f;
            color = PURPLE;
            deathDuration = 0.6f;
            break;
        case EnemyType::BOSS:
            hp = 1000;
            maxHp = 1000;
            speed = 35.0f;
            reward = 200;
            radius = 30.0f;
            color = MAGENTA;
            deathDuration = 1.0f;
            break;
    }
}

void Enemy::Update(float deltaTime, float animDeltaTime, const std::vector<Vector2>& path) {
    if (dying) {
        deathTimer += deltaTime;
        if (deathTimer >= deathDuration) {
            alive = false;
        }
        return;
    }
    
    if (!alive) return;

    walkTimer += animDeltaTime;
    animTimer += animDeltaTime;

    if (targetPoint < path.size()) {
        Vector2 target = path[targetPoint];
        Vector2 direction = Vector2Subtract(target, position);
        float distance = Vector2Length(direction);

        if (distance > 1.0f) {
            float newFacingAngle = atan2f(direction.y, direction.x);
            
            // Calculate lean angle (difference in facing angle)
            float angleDiff = newFacingAngle - facingAngle;
            while (angleDiff > PI) angleDiff -= 2.0f * PI;
            while (angleDiff < -PI) angleDiff += 2.0f * PI;
            
            // Set target lean based on turn rate
            targetLeanAngle = angleDiff * 0.8f; // Lean into turn
            
            facingAngle = newFacingAngle;
            direction = Vector2Scale(Vector2Normalize(direction), std::min(speed * deltaTime, distance));
            position = Vector2Add(position, direction);
        }
        if (distance <= speed * deltaTime + 1.0f) {
            position = target;
            targetPoint++;
        }
    }

    // Lean animation DISABLED (causes stuttering)
    leanAngle = 0.0f;
    targetLeanAngle = 0.0f;
    // was: smooth lean interpolation with leanSpeed * animDeltaTime
    
    // Bob animation DISABLED (causes stuttering)
    bobOffset = 0.0f; // was: sinf(animTimer * bobSpeed) * 0.5f;

    // Hit flash timer
    if (hitFlashTimer > 0.0f) {
        hitFlashTimer -= deltaTime;
        if (hitFlashTimer < 0.0f) hitFlashTimer = 0.0f;
    }

    // Calculate path progress (0.0 to 1.0)
    float totalPathLen = 0.0f;
    for (size_t i = 1; i < path.size(); i++) {
        totalPathLen += Vector2Distance(path[i-1], path[i]);
    }
    float traveled = 0.0f;
    for (size_t i = 1; i < targetPoint && i < path.size(); i++) {
        traveled += Vector2Distance(path[i-1], path[i]);
    }
    if (targetPoint < path.size()) {
        traveled += Vector2Distance(path[targetPoint-1], position);
    }
    pathProgress = (totalPathLen > 0.0f) ? (traveled / totalPathLen) : 0.0f;
}

float Enemy::GetPathProgress() const {
    return pathProgress;
}

void Enemy::TakeDamage(int damage) {
    // No-op guard: zero/negative damage must not trigger hit flash, death,
    // or any other state change (e.g. when loading a save at full HP).
    if (damage <= 0) return;
    if (dying || !alive) return;

    hp -= damage;
    hitFlashTimer = 0.1f;
    hitFlashColor = WHITE;
    if (hp <= 0) {
        hp = 0;
        if (!dying) {
            dying = true;
            deathTimer = 0.0f;
        }
    }
}

void Enemy::RestoreHp(int newHp) {
    if (newHp < 0) newHp = 0;
    if (newHp > maxHp) newHp = maxHp;
    hp = newHp;
    hitFlashTimer = 0.0f;
    hitFlashColor = WHITE;

    // A restored enemy is always a living, non-dying unit.
    dying = false;
    deathTimer = 0.0f;
    alive = true;
}
