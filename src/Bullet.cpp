#include "Bullet.h"
#include "Enemy.h"
#include "raymath.h"

Bullet::Bullet(Vector2 startPos, Vector2 dir, float spd, int dmg, float rad, Color col, bool aoe, float aoeRad)
    : position(startPos), direction(dir), speed(spd), damage(dmg),
      radius(rad), color(col), active(true), isAoE(aoe), aoeRadius(aoeRad),
      age(0.0f) {
  trailPositions.push_back(startPos);
}

void Bullet::Update(float deltaTime) {
    if (!active) return;

    // Expire shots that fly off the map without hitting anything, otherwise
    // missed bullets travel forever (memory/CPU leak).
    age += deltaTime;
    if (age >= MAX_LIFETIME) {
      active = false;
      return;
    }
    // Hard safety net: cull anything far outside the playfield.
    if (position.x < -200.0f || position.x > 1480.0f ||
        position.y < -200.0f || position.y > 920.0f) {
      active = false;
      return;
    }

    // Add current position to trail
    trailPositions.push_back(position);
    if (trailPositions.size() > MAX_TRAIL_LENGTH) {
      trailPositions.erase(trailPositions.begin());
    }

    position.x += direction.x * speed * deltaTime;
    position.y += direction.y * speed * deltaTime;
}

static void SpawnImpactFX(std::vector<Particle>& particles, Vector2 pos, Color col, bool isAoE) {
    int count = isAoE ? 18 : 10;
    float speed = isAoE ? 140.0f : 100.0f;
    
    for (int i = 0; i < count; i++) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float spd = (float)GetRandomValue((int)(speed * 0.3f), (int)speed);
        float lifetime = 0.3f + (float)GetRandomValue(0, 25) / 100.0f;
        
        Color pCol = col;
        if (isAoE && i % 3 == 0) pCol = Color{249, 115, 22, 255};
        if (!isAoE && col.b > 200 && i % 2 == 0) pCol = Color{186, 230, 253, 255};
        if (!isAoE && col.r > 200 && col.g > 150 && i % 2 == 0) pCol = Color{255, 255, 200, 255};
        
        particles.push_back({
            pos,
            Vector2{cosf(angle) * spd, sinf(angle) * spd},
            pCol,
            (float)GetRandomValue(isAoE ? 3 : 2, isAoE ? 6 : 4),
            lifetime,
            lifetime
        });
    }
    
    // Add directional impact ring
    for (int i = 0; i < 8; i++) {
        float angle = i * (2.0f * PI / 8.0f);
        particles.push_back({
            pos,
            Vector2{cosf(angle) * speed * 0.6f, sinf(angle) * speed * 0.6f},
            Fade(col, 0.8f),
            2.0f,
            0.15f,
            0.15f
        });
    }
}

static void SpawnDeathParticles(std::vector<Particle>& particles, Vector2 pos, EnemyType type, Color dmgColor) {
    int count = (type == EnemyType::BOSS) ? 32 : (type == EnemyType::TANK ? 20 : 14);
    float baseSpeed = (type == EnemyType::BOSS) ? 160.0f : 120.0f;

    // Determine death effect color palette based on damage type
    bool isPlasma = (dmgColor.b > 200 && dmgColor.g > 150);
    bool isExplosion = (dmgColor.r > 200 && dmgColor.g > 100 && dmgColor.b < 100);
    bool isKinetic = (dmgColor.r > 200 && dmgColor.g > 150 && dmgColor.b < 100);

    Color pColor = (type == EnemyType::BOSS) ? Color{168, 85, 247, 255} :
                   (type == EnemyType::TANK) ? Color{100, 116, 139, 255} :
                   (type == EnemyType::FAST) ? Color{249, 115, 22, 255} : Color{239, 68, 68, 255};

    // Override with damage-type colors for more variation
    if (isPlasma) pColor = Color{56, 189, 248, 255};
    else if (isExplosion) pColor = Color{249, 115, 22, 255};
    else if (isKinetic) pColor = Color{250, 204, 21, 255};

    for (int i = 0; i < count; i++) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float spd = (float)GetRandomValue((int)(baseSpeed * 0.2f), (int)baseSpeed);
        float lifetime = 0.4f + (float)GetRandomValue(0, 40) / 100.0f;
        
        Color c = pColor;
        // Add damage-type specific sparkle
        if (isPlasma && i % 3 == 0) c = Color{186, 230, 253, 255};
        else if (isExplosion && i % 3 == 0) c = Color{255, 200, 100, 255};
        else if (isKinetic && i % 3 == 0) c = Color{255, 255, 200, 255};
        else if (type == EnemyType::BOSS && i % 2 == 0) c = Color{250, 204, 21, 255};

        particles.push_back({
            pos,
            Vector2{cosf(angle) * spd, sinf(angle) * spd},
            c,
            (float)GetRandomValue(3, 6),
            lifetime,
            lifetime
        });
    }

    // Add directional burst lines
    for (int i = 0; i < 6; i++) {
        float angle = i * (2.0f * PI / 6.0f);
        particles.push_back({
            pos,
            Vector2{cosf(angle) * baseSpeed * 0.8f, sinf(angle) * baseSpeed * 0.8f},
            Fade(pColor, 0.9f),
            2.5f,
            0.2f,
            0.2f
        });
    }
}

static void SpawnBountyText(std::vector<FloatingText>& floatingTexts, Vector2 pos, int reward) {
    FloatingText ft;
    ft.position = pos;
    TextCopy(ft.text, TextFormat("+$%d", reward));
    ft.color = Color{250, 204, 21, 255}; // Gold
    ft.lifetime = 0.9f;
    ft.maxLifetime = 0.9f;
    ft.floatSpeed = 35.0f;
    floatingTexts.push_back(ft);
}

void Bullet::CheckCollision(std::vector<Enemy>& enemies, int& playerMoney,
                            std::vector<Explosion>& explosions,
                            std::vector<Particle>& particles,
                            std::vector<FloatingText>& floatingTexts,
                            const GameAudio& audio,
                            std::function<void(Vector2, bool, bool, Color)> onHit) {
    if (!active) return;

    for (Enemy& enemy : enemies) {
        if (!enemy.IsAlive() || enemy.IsDying() || !active) {
            continue;
        }

        float distance = Vector2Distance(position, enemy.GetPosition());
        if (distance < (radius + enemy.GetRadius())) {
            active = false;
            Vector2 impactPos = position;

            // Spawn impact FX
            SpawnImpactFX(particles, impactPos, color, isAoE);

            // Applies damage with crit roll and awards the bounty exactly once,
            // only when the hit actually transitions the enemy into its death
            // state. Enemies already dying/dead are skipped, so a single bullet
            // (or overlapping AoE blasts) can never pay the reward twice.
            auto ApplyDamage = [&](Enemy& target) {
                if (!target.IsAlive() || target.IsDying()) return;
                bool wasDying = target.IsDying();
                bool isCrit = (GetRandomValue(0, 99) < 15);
                int finalDmg = isCrit ? damage * 2 : damage;
                target.TakeDamage(finalDmg);

                bool killed = (!wasDying && target.IsDying());
                if (killed) {
                    playerMoney += target.GetReward();
                    SpawnBountyText(floatingTexts, target.GetPosition(), target.GetReward());
                    SpawnDeathParticles(particles, target.GetPosition(), target.GetType(), color);
                    PlaySound(audio.enemyDeath);
                }
                if (onHit) onHit(target.GetPosition(), killed, isCrit, color);

                if (isCrit) {
                    FloatingText cft;
                    cft.position = target.GetPosition();
                    cft.position.y -= 22.0f;
                    TextCopy(cft.text, "CRIT!");
                    cft.color = Color{255, 255, 120, 255};
                    cft.lifetime = 1.0f;
                    cft.maxLifetime = 1.0f;
                    cft.floatSpeed = 40.0f;
                    floatingTexts.push_back(cft);
                }
            };

            if (isAoE) {
                // Tambahkan efek visual ledakan pada titik impact
                explosions.push_back({impactPos, aoeRadius, 0.25f, 0.25f});
                PlaySound(audio.explosion);

                // Spawn fiery ember particles around explosion
                SpawnImpactFX(particles, impactPos, Color{249, 115, 22, 255}, true);

                // Berikan damage ke seluruh musuh hidup dalam radius AoE
                for (Enemy& target : enemies) {
                    float distToImpact = Vector2Distance(impactPos, target.GetPosition());
                    if (distToImpact <= aoeRadius) {
                        ApplyDamage(target);
                    }
                }
            } else {
                ApplyDamage(enemy);
            }

            break; // Bullet meledak/hancur, keluar dari loop
        }
    }
}

