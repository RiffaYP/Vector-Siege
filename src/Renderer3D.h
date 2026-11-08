#pragma once

#include "raylib.h"
#include <vector>

class Enemy;
class Tower;
class Bullet;
struct Explosion;

class Renderer3D {
public:
    // Coordinate conversions between 2D logic space and 3D world space
    static Vector3 To3D(Vector2 p2, float height = 0.0f);
    static Vector2 To2D(Vector3 p3);

    // 3D Scene Rendering
    static void DrawTerrain(const std::vector<Vector2>& path, float globalTime);
    static void DrawBaseFortress(Vector2 endPathPos, float globalTime);
    static void DrawSpawnPortal(Vector2 startPathPos, float globalTime);

    static void DrawRangeIndicator3D(Vector2 center2D, float range2D, Color color, bool fill = false);
    static void DrawTowerPreview3D(Vector2 pos2D, int towerType, float range2D, bool canBuild);

    static void DrawTower3D(const Tower& tower, bool isSelected, float globalTime);
    static void DrawBasicTower3D(Vector3 pos, float yaw, int level, bool isSelected, float globalTime, float muzzleFlash = 0.0f);
    static void DrawRapidTower3D(Vector3 pos, float yaw, int level, bool isSelected, float globalTime, float muzzleFlash = 0.0f);
    static void DrawCannonTower3D(Vector3 pos, float yaw, int level, bool isSelected, float globalTime, float muzzleFlash = 0.0f);
    static void DrawEnemy3D(const Enemy& enemy, float globalTime);
    static void DrawBullet3D(const Bullet& bullet);
    static void DrawExplosion3D(const Explosion& exp);

    // Helpers
    static void DrawCrystalCluster(Vector3 pos, float scale = 1.0f);
    static void DrawTechOutpost(Vector3 pos, float scale = 1.0f);
};
