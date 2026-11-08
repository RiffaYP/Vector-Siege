#pragma once

#include "raylib.h"
#include <vector>

class Enemy;
class Bullet;
struct GameAudio;

enum class TowerType {
    BASIC,
    RAPID,
    CANNON
};

enum class TargetingMode {
    FIRST,      // First enemy in range (closest to goal)
    LAST,       // Last enemy in range (furthest from goal)
    STRONGEST,  // Enemy with highest HP
    CLOSEST     // Enemy closest to tower
};

class Tower {
private:
    TowerType type;
    Vector2 position;
    float range;
    int damage;
    float attackCooldown;
    float attackTimer;
    int cost;
    Vector2 barrelDirection;

    int level;
    int totalInvested;
    float aoeRadius;
    TargetingMode targetingMode;

    // Muzzle flash animation state
    float muzzleFlashTimer;

    // Smooth rotation
    float currentYaw;
    float targetYaw;
    float rotationSpeed;

public:
    Tower(Vector2 pos, TowerType towerType);

    void Update(float deltaTime, std::vector<Enemy>& enemies, std::vector<Bullet>& bullets, const GameAudio& audio);
    void Draw(bool isSelected = false) const;

    void TriggerMuzzleFlash();
    float GetMuzzleFlashTimer() const { return muzzleFlashTimer; }

    bool CanUpgrade() const { return level < 3; }
    int GetUpgradeCost() const;
    bool Upgrade();

    int GetSellValue() const { return static_cast<int>(totalInvested * 0.7f); }

    Vector2 GetPosition() const { return position; }
    Vector2 GetBarrelDirection() const { return barrelDirection; }
    float GetRange() const { return range; }
    TowerType GetType() const { return type; }
    int GetCost() const { return cost; }
    int GetLevel() const { return level; }
    int GetDamage() const { return damage; }
    float GetCooldown() const { return attackCooldown; }
    float GetAoERadius() const { return aoeRadius; }
    int GetTotalInvested() const { return totalInvested; }
    TargetingMode GetTargetingMode() const { return targetingMode; }
    void SetTargetingMode(TargetingMode mode) { targetingMode = mode; }
    // Used by save/load to restore the exact invested total (affects sell value).
    void SetTotalInvested(int invested) { totalInvested = invested; }
    void CycleTargetingMode();
    const char* GetTargetingModeName() const;

    static int GetTowerCost(TowerType type);
    static float GetTowerRange(TowerType type);
    static const char* GetTowerName(TowerType type);
};
