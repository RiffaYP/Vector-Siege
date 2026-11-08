#include "Tower.h"
#include "Enemy.h"
#include "Bullet.h"
#include "raymath.h"

int Tower::GetTowerCost(TowerType type) {
    switch (type) {
        case TowerType::BASIC: return 100;
        case TowerType::RAPID: return 150;
        case TowerType::CANNON: return 250;
    }
    return 100;
}

float Tower::GetTowerRange(TowerType type) {
    switch (type) {
        case TowerType::BASIC: return 160.0f;
        case TowerType::RAPID: return 130.0f;
        case TowerType::CANNON: return 200.0f;
    }
    return 160.0f;
}

const char* Tower::GetTowerName(TowerType type) {
    switch (type) {
        case TowerType::BASIC: return "ARCHER";
        case TowerType::RAPID: return "MAGE";
        case TowerType::CANNON: return "CATAPULT";
    }
    return "UNKNOWN";
}

Tower::Tower(Vector2 pos, TowerType towerType)
    : type(towerType), position(pos), attackTimer(0.0f), barrelDirection({0, -1}),
      level(1), totalInvested(0), aoeRadius(0.0f), targetingMode(TargetingMode::FIRST),
      muzzleFlashTimer(0.0f), currentYaw(0.0f), targetYaw(0.0f), rotationSpeed(720.0f) {
    cost = GetTowerCost(type);
    totalInvested = cost;
    range = GetTowerRange(type);

    switch (type) {
        case TowerType::BASIC:
            damage = 20;
            attackCooldown = 0.8f;
            break;
        case TowerType::RAPID:
            damage = 8;
            attackCooldown = 0.25f;
            break;
        case TowerType::CANNON:
            damage = 60;
            attackCooldown = 1.8f;
            aoeRadius = 70.0f;
            break;
    }
}

int Tower::GetUpgradeCost() const {
    if (level >= 3) return 0;

    switch (type) {
        case TowerType::BASIC:
            return (level == 1) ? 150 : 250;
        case TowerType::RAPID:
            return (level == 1) ? 200 : 300;
        case TowerType::CANNON:
            return (level == 1) ? 250 : 400;
    }
    return 0;
}

bool Tower::Upgrade() {
    if (level >= 3) return false;

    int upCost = GetUpgradeCost();
    totalInvested += upCost;
    level++;

    switch (type) {
        case TowerType::BASIC:
            if (level == 2) {
                damage = 30;
                range = 175.0f;
                attackCooldown = 0.75f;
            } else if (level == 3) {
                damage = 45;
                range = 190.0f;
                attackCooldown = 0.70f;
            }
            break;
        case TowerType::RAPID:
            if (level == 2) {
                damage = 12;
                range = 140.0f;
                attackCooldown = 0.20f;
            } else if (level == 3) {
                damage = 17;
                range = 150.0f;
                attackCooldown = 0.15f;
            }
            break;
        case TowerType::CANNON:
            if (level == 2) {
                damage = 85;
                range = 215.0f;
                attackCooldown = 1.7f;
                aoeRadius = 80.0f;
            } else if (level == 3) {
                damage = 120;
                range = 230.0f;
                attackCooldown = 1.6f;
                aoeRadius = 90.0f;
            }
            break;
    }

    return true;
}

void Tower::TriggerMuzzleFlash() {
    muzzleFlashTimer = 1.0f;
}

void Tower::Update(float deltaTime, std::vector<Enemy>& enemies, std::vector<Bullet>& bullets, const GameAudio& audio) {
    attackTimer -= deltaTime;

    // Decay muzzle flash in real time (1.0 -> 0.0 over ~0.12s) so the flash
    // stays visible for a perceptible moment and lines up with the shot.
    if (muzzleFlashTimer > 0.0f) muzzleFlashTimer -= deltaTime * 8.0f;
    if (muzzleFlashTimer < 0.0f) muzzleFlashTimer = 0.0f;

    // Cari enemy dalam range berdasarkan targeting mode
    Enemy* targetEnemy = nullptr;
    
    switch (targetingMode) {
        case TargetingMode::FIRST: {
            // First enemy in range (closest to goal - highest path progress)
            float maxPathProgress = -1.0f;
            for (Enemy& enemy : enemies) {
                if (!enemy.IsAlive()) continue;
                float distance = Vector2Distance(position, enemy.GetPosition());
                if (distance <= range) {
                    if (enemy.IsDying()) continue;
                    float pathProgress = enemy.GetPathProgress();
                    if (pathProgress > maxPathProgress) {
                        maxPathProgress = pathProgress;
                        targetEnemy = &enemy;
                    }
                }
            }
            break;
        }
        case TargetingMode::LAST: {
            // Last enemy in range (furthest from goal - lowest path progress)
            float minPathProgress = 999999.0f;
            for (Enemy& enemy : enemies) {
                if (!enemy.IsAlive()) continue;
                float distance = Vector2Distance(position, enemy.GetPosition());
                if (distance <= range) {
                    if (enemy.IsDying()) continue;
                    float pathProgress = enemy.GetPathProgress();
                    if (pathProgress < minPathProgress) {
                        minPathProgress = pathProgress;
                        targetEnemy = &enemy;
                    }
                }
            }
            break;
        }
        case TargetingMode::STRONGEST: {
            // Enemy with highest HP
            int maxHp = 0;
            for (Enemy& enemy : enemies) {
                if (!enemy.IsAlive()) continue;
                float distance = Vector2Distance(position, enemy.GetPosition());
                if (distance <= range) {
                    if (enemy.IsDying()) continue;
                    if (enemy.GetHp() > maxHp) {
                        maxHp = enemy.GetHp();
                        targetEnemy = &enemy;
                    }
                }
            }
            break;
        }
        case TargetingMode::CLOSEST: {
            // Enemy closest to tower
            float closestDist = range;
            for (Enemy& enemy : enemies) {
                if (!enemy.IsAlive()) continue;
                float distance = Vector2Distance(position, enemy.GetPosition());
                if (distance <= closestDist) {
                    if (enemy.IsDying()) continue;
                    closestDist = distance;
                    targetEnemy = &enemy;
                }
            }
            break;
        }
    }

    // Smooth rotation towards target
    if (targetEnemy != nullptr) {
        Vector2 toTarget = Vector2Normalize(
            Vector2Subtract(targetEnemy->GetPosition(), position));
        float desiredYaw = atan2f(toTarget.x, toTarget.y); // radians
        
        // Normalize angle difference for shortest path
        float diff = desiredYaw - currentYaw;
        while (diff > PI) diff -= 2.0f * PI;
        while (diff < -PI) diff += 2.0f * PI;
        
        float maxStep = rotationSpeed * DEG2RAD * deltaTime;
        if (fabsf(diff) < maxStep) {
            currentYaw = desiredYaw;
        } else {
            currentYaw += (diff > 0 ? maxStep : -maxStep);
        }
        
        // Update barrelDirection from currentYaw
        barrelDirection = {sinf(currentYaw), cosf(currentYaw)};
        targetYaw = desiredYaw;
    } else {
        // Idle: slowly return to default forward
        float diff = 0.0f - currentYaw;
        while (diff > PI) diff -= 2.0f * PI;
        while (diff < -PI) diff += 2.0f * PI;
        float maxStep = rotationSpeed * DEG2RAD * deltaTime * 0.3f;
        if (fabsf(diff) < maxStep) {
            currentYaw = 0.0f;
        } else {
            currentYaw += (diff > 0 ? maxStep : -maxStep);
        }
        barrelDirection = {sinf(currentYaw), cosf(currentYaw)};
    }

    // Clamp the cooldown debt. Without this the timer can drift far below
    // zero while the tower has no target, which would make the first shot
    // after re-acquiring a target feel "instant/uncanny".
    if (attackTimer < -attackCooldown) {
        attackTimer = -attackCooldown;
    }

    if (attackTimer > 0) {
        return;
    }

    // Jika ada target dan cooldown selesai, lepaskan tembakan
    if (targetEnemy != nullptr) {
        Vector2 direction = barrelDirection;

        TriggerMuzzleFlash();

        switch (type) {
            case TowerType::BASIC:
                bullets.emplace_back(position, direction, 500.0f, damage, 5.0f, YELLOW, false, 0.0f);
                PlaySound(audio.shootBasic);
                break;
            case TowerType::RAPID:
                bullets.emplace_back(position, direction, 550.0f, damage, 4.0f, SKYBLUE, false, 0.0f);
                PlaySound(audio.shootRapid);
                break;
            case TowerType::CANNON:
                bullets.emplace_back(position, direction, 450.0f, damage, 9.0f, RED, true, aoeRadius);
                PlaySound(audio.shootCannon);
                break;
        }

        // Carry over the overshoot so the effective fire rate matches the
        // declared cooldown instead of drifting slower frame by frame.
        attackTimer += attackCooldown;
        if (attackTimer > attackCooldown) attackTimer = attackCooldown;
    }
}

void Tower::Draw(bool isSelected) const {
    // Range circle indicator
    if (isSelected) {
        DrawCircle(position.x, position.y, range, Fade(YELLOW, 0.05f));
        DrawCircleLines(position.x, position.y, range, Fade(GOLD, 0.75f));
    } else {
        DrawCircleLines(position.x, position.y, range, Fade(WHITE, 0.20f));
    }

    float baseRadius = 18.0f;
    Color accentColor = Color{34, 197, 94, 255};

    switch (type) {
        case TowerType::BASIC:
            baseRadius = 18.0f;
            accentColor = Color{34, 197, 94, 255};
            break;
        case TowerType::RAPID:
            baseRadius = 16.0f;
            accentColor = Color{14, 165, 233, 255};
            break;
        case TowerType::CANNON:
            baseRadius = 21.0f;
            accentColor = Color{239, 68, 68, 255};
            break;
    }

    // Pedestal Drop Shadow & Outer Mechanical Rim
    DrawCircle(position.x, position.y + 3.0f, baseRadius + 3.0f, Fade(BLACK, 0.35f));
    DrawCircle(position.x, position.y, baseRadius + 2.0f, Color{15, 23, 42, 255}); // Dark slate chassis
    DrawCircleLines(position.x, position.y, baseRadius + 2.0f, Fade(accentColor, 0.5f));

    // Turret Barrel & Body
    switch (type) {
        case TowerType::BASIC: {
            // Main Barrel
            Vector2 barrelEnd = Vector2Add(position, Vector2Scale(barrelDirection, 23.0f));
            DrawLineEx(position, barrelEnd, 6.0f, Color{22, 101, 52, 255});
            DrawCircle(barrelEnd.x, barrelEnd.y, 3.5f, Color{74, 222, 128, 255}); // Glowing muzzle

            // Body
            DrawCircle(position.x, position.y, baseRadius, Color{34, 197, 94, 255});
            DrawCircle(position.x, position.y, 9.0f, Color{20, 83, 45, 255});
            DrawCircle(position.x, position.y, 4.5f, Color{187, 247, 208, 255}); // Core glow
            break;
        }
        case TowerType::RAPID: {
            // Twin Barrels
            Vector2 perp{-barrelDirection.y, barrelDirection.x};
            Vector2 b1Start = Vector2Add(position, Vector2Scale(perp, 4.0f));
            Vector2 b2Start = Vector2Subtract(position, Vector2Scale(perp, 4.0f));
            Vector2 b1End = Vector2Add(b1Start, Vector2Scale(barrelDirection, 21.0f));
            Vector2 b2End = Vector2Add(b2Start, Vector2Scale(barrelDirection, 21.0f));

            DrawLineEx(b1Start, b1End, 3.5f, Color{3, 105, 161, 255});
            DrawLineEx(b2Start, b2End, 3.5f, Color{3, 105, 161, 255});
            DrawCircle(b1End.x, b1End.y, 2.5f, Color{56, 189, 248, 255});
            DrawCircle(b2End.x, b2End.y, 2.5f, Color{56, 189, 248, 255});

            // Body
            DrawCircle(position.x, position.y, baseRadius, Color{14, 165, 233, 255});
            DrawCircle(position.x, position.y, 8.0f, Color{12, 74, 110, 255});
            DrawCircle(position.x, position.y, 4.0f, Color{224, 242, 254, 255}); // Core glow
            break;
        }
        case TowerType::CANNON: {
            // Heavy Barrel
            Vector2 barrelEnd = Vector2Add(position, Vector2Scale(barrelDirection, 27.0f));
            DrawLineEx(position, barrelEnd, 9.0f, Color{127, 29, 29, 255});
            DrawCircle(barrelEnd.x, barrelEnd.y, 5.0f, Color{248, 113, 113, 255}); // Glowing muzzle

            // Body
            DrawCircle(position.x, position.y, baseRadius, Color{239, 68, 68, 255});
            DrawCircle(position.x, position.y, 10.0f, Color{153, 27, 27, 255});
            DrawCircle(position.x, position.y, 5.0f, Color{254, 226, 226, 255}); // Core glow
            break;
        }
    }

    // Level Visual Indicator
    if (level == 2) {
        DrawCircleLines(position.x, position.y, baseRadius + 4.5f, Fade(Color{0, 220, 255, 255}, 0.85f));
    } else if (level == 3) {
        DrawCircleLines(position.x, position.y, baseRadius + 4.5f, Fade(GOLD, 0.9f));
        DrawCircleLines(position.x, position.y, baseRadius + 6.5f, Fade(GOLD, 0.7f));
    }

    // Selection Glow Indicator
    if (isSelected) {
        DrawCircleLines(position.x, position.y, baseRadius + 7.5f, Fade(GOLD, 0.95f));
        DrawCircleLines(position.x, position.y, baseRadius + 9.0f, Fade(YELLOW, 0.6f));
    }
}

void Tower::CycleTargetingMode() {
    switch (targetingMode) {
        case TargetingMode::FIRST: targetingMode = TargetingMode::LAST; break;
        case TargetingMode::LAST: targetingMode = TargetingMode::STRONGEST; break;
        case TargetingMode::STRONGEST: targetingMode = TargetingMode::CLOSEST; break;
        case TargetingMode::CLOSEST: targetingMode = TargetingMode::FIRST; break;
    }
}

const char* Tower::GetTargetingModeName() const {
    switch (targetingMode) {
        case TargetingMode::FIRST: return "FIRST";
        case TargetingMode::LAST: return "LAST";
        case TargetingMode::STRONGEST: return "STRONGEST";
        case TargetingMode::CLOSEST: return "CLOSEST";
    }
    return "FIRST";
}
