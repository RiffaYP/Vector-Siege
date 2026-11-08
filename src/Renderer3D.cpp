#include "Renderer3D.h"
#include "Enemy.h"
#include "Tower.h"
#include "Bullet.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

Vector3 Renderer3D::To3D(Vector2 p2, float height) {
    return Vector3{ (p2.x - 640.0f) * 0.045f, height, (p2.y - 360.0f) * 0.045f };
}

Vector2 Renderer3D::To2D(Vector3 p3) {
    return Vector2{ (p3.x / 0.045f) + 640.0f, (p3.z / 0.045f) + 360.0f };
}

// ---------------------------------------------------------------------------
// Alien Crystal Energy Cluster (Replaces Pine Trees)
// ---------------------------------------------------------------------------
void Renderer3D::DrawCrystalCluster(Vector3 pos, float scale) {
    // Base glow ring on ground
    DrawCylinder(Vector3{pos.x, pos.y + 0.01f, pos.z}, 0.55f * scale, 0.55f * scale, 0.025f, 8, Color{0, 155, 180, 55});

    // Main tall prismatic spire (5-sided)
    DrawCylinder(Vector3{pos.x, pos.y, pos.z}, 0.0f, 0.22f * scale, 2.1f * scale, 5, Color{14, 100, 130, 210});
    DrawCylinderWires(Vector3{pos.x, pos.y, pos.z}, 0.0f, 0.22f * scale, 2.1f * scale, 5, Color{56, 189, 248, 255});

    // Secondary spire – left-back
    DrawCylinder(Vector3{pos.x - 0.28f * scale, pos.y, pos.z + 0.20f * scale},
                 0.0f, 0.16f * scale, 1.5f * scale, 5, Color{56, 150, 185, 185});

    // Secondary spire – right-front
    DrawCylinder(Vector3{pos.x + 0.30f * scale, pos.y, pos.z - 0.25f * scale},
                 0.0f, 0.18f * scale, 1.65f * scale, 5, Color{80, 200, 230, 160});

    // Violet accent micro-crystal
    DrawCylinder(Vector3{pos.x + 0.38f * scale, pos.y, pos.z + 0.22f * scale},
                 0.0f, 0.09f * scale, 0.90f * scale, 4, Color{160, 80, 240, 185});
}

// ---------------------------------------------------------------------------
// Sci-Fi Tech Outpost Node (Replaces Boulders)
// ---------------------------------------------------------------------------
void Renderer3D::DrawTechOutpost(Vector3 pos, float scale) {
    // Armored base slab
    DrawCube(Vector3{pos.x, pos.y + 0.10f * scale, pos.z}, 1.0f * scale, 0.20f * scale, 1.0f * scale, Color{28, 38, 58, 255});
    DrawCubeWires(Vector3{pos.x, pos.y + 0.10f * scale, pos.z}, 1.0f * scale, 0.20f * scale, 1.0f * scale, Color{0, 160, 190, 140});

    // Central pillar
    DrawCube(Vector3{pos.x, pos.y + 0.55f * scale, pos.z}, 0.34f * scale, 0.72f * scale, 0.34f * scale, Color{38, 55, 82, 255});

    // Sensor dish top
    DrawCube(Vector3{pos.x, pos.y + 0.98f * scale, pos.z}, 0.52f * scale, 0.16f * scale, 0.52f * scale, Color{15, 23, 42, 255});
    DrawCubeWires(Vector3{pos.x, pos.y + 0.98f * scale, pos.z}, 0.52f * scale, 0.16f * scale, 0.52f * scale, Color{0, 200, 225, 180});

    // Antenna nub
    DrawCylinder(Vector3{pos.x, pos.y + 1.10f * scale, pos.z}, 0.0f, 0.08f * scale, 0.22f * scale, 6, Color{0, 195, 220, 255});
}

void Renderer3D::DrawTerrain(const std::vector<Vector2>& path, float globalTime) {
    // =================================================================
    // 1. SCI-FI ARENA BASE — deep space navy composite ground
    // =================================================================
    DrawCube(Vector3{0.0f, -0.602f, 0.0f}, 66.0f, 1.196f, 42.0f, Color{12, 20, 34, 255});

    // Large tech floor panels with alternating tones
    for (int x = -14; x <= 14; x += 2) {
        for (int z = -9; z <= 9; z += 2) {
            Color panelCol = ((x + z) % 4 == 0)
                ? Color{18, 30, 50, 255}
                : Color{14, 24, 42, 255};
            DrawCube(Vector3{(float)x * 2.1f, 0.003f, (float)z * 2.1f}, 2.02f, 0.005f, 2.02f, panelCol);
        }
    }

    // Sparse tech guidance grid (very subtle, low alpha)
    for (int x = -28; x <= 28; x += 6) {
        DrawCube(Vector3{(float)x, 0.010f, 0.0f}, 0.04f, 0.004f, 40.0f, Color{0, 90, 130, 38});
    }
    for (int z = -18; z <= 18; z += 6) {
        DrawCube(Vector3{0.0f, 0.010f, (float)z}, 64.0f, 0.004f, 0.04f, Color{0, 90, 130, 38});
    }

    // =================================================================
    // 2. ENERGY LOGISTIC LANE — sci-fi path with animated flow
    // =================================================================
    const float dashSpacing = 1.2f;
    // Scroll speed: positive = toward path end (direction enemies walk)
    const float dashScroll = fmodf(globalTime * 1.8f, dashSpacing);

    const float laneHalf = 1.20f;
    const float accentHalf = 0.88f;
    const int segCount = (int)path.size() - 1;

    std::vector<Vector2> segDir(segCount > 0 ? segCount : 0);
    std::vector<float> turnAngle(path.size(), 0.0f);
    for (int i = 0; i < segCount; i++) {
        float dx = path[i + 1].x - path[i].x;
        float dz = path[i + 1].y - path[i].y;
        float l = sqrtf(dx * dx + dz * dz);
        segDir[i] = (l > 1e-5f) ? Vector2{dx / l, dz / l} : Vector2{0.0f, 0.0f};
        if (i > 0) {
            float dot = segDir[i - 1].x * segDir[i].x + segDir[i - 1].y * segDir[i].y;
            dot = fmaxf(-1.0f, fminf(1.0f, dot));
            turnAngle[i] = acosf(dot);
        }
    }
    auto cross2 = [](Vector2 a, Vector2 b) { return a.x * b.y - a.y * b.x; };
    // Local +X axis of a segment (in world XZ), matching the render rotation.
    auto sideDir = [](Vector2 d) { return Vector2{d.y, -d.x}; };

    auto DrawFlatStrip = [](Vector3 a, Vector3 b, float width, float y,
                            Color color) {
        float dx = b.x - a.x;
        float dz = b.z - a.z;
        float len = sqrtf(dx * dx + dz * dz);
        if (len < 1e-4f) return;
        float mx = (a.x + b.x) * 0.5f;
        float mz = (a.z + b.z) * 0.5f;
        float angle = atan2f(dx, dz) * RAD2DEG;
        rlPushMatrix();
        rlTranslatef(mx, 0.0f, mz);
        rlRotatef(angle, 0.0f, 1.0f, 0.0f);
        DrawCube(Vector3{0.0f, y, 0.0f}, width, 0.014f, len, color);
        rlPopMatrix();
    };

    for (int i = 0; i < segCount; i++) {
        Vector3 pStart = To3D(path[i], 0.0f);
        Vector3 pEnd   = To3D(path[i + 1], 0.0f);
        Vector3 mid    = Vector3Scale(Vector3Add(pStart, pEnd), 0.5f);
        Vector3 diff   = Vector3Subtract(pEnd, pStart);
        float   len    = Vector3Length(diff);
        float   angle  = atan2f(diff.x, diff.z) * RAD2DEG;

        rlPushMatrix();
        rlTranslatef(mid.x, 0.0f, mid.z);
        rlRotatef(angle, 0.0f, 1.0f, 0.0f);

        // Dark asphalt lane surface (fills the intersections)
        DrawCube(Vector3{0.0f, 0.038f, 0.0f}, 2.40f, 0.076f, len, Color{22, 28, 40, 255});

        for (int s = -1; s <= 1; s += 2) {
            // Outer-corner borders are miter-clipped so they meet cleanly
            float edgeStartClip = 0.0f, edgeEndClip = 0.0f;
            if (i > 0 && s * cross2(segDir[i - 1], segDir[i]) < 0.0f)
                edgeStartClip = laneHalf * tanf(turnAngle[i] * 0.5f);
            if (i < segCount - 1 && s * cross2(segDir[i], segDir[i + 1]) < 0.0f)
                edgeEndClip = laneHalf * tanf(turnAngle[i + 1] * 0.5f);

            float edgeLen = len - edgeStartClip - edgeEndClip;
            if (edgeLen > 0.05f) {
                float edgeCenter = (edgeStartClip - edgeEndClip) * 0.5f;
                DrawCube(Vector3{(float)s * laneHalf, 0.080f, edgeCenter},
                         0.10f, 0.014f, edgeLen, Color{0, 220, 245, 255});
            }

            float accentStartClip = 0.0f, accentEndClip = 0.0f;
            if (i > 0 && s * cross2(segDir[i - 1], segDir[i]) < 0.0f)
                accentStartClip = accentHalf * tanf(turnAngle[i] * 0.5f);
            if (i < segCount - 1 && s * cross2(segDir[i], segDir[i + 1]) < 0.0f)
                accentEndClip = accentHalf * tanf(turnAngle[i + 1] * 0.5f);

            float accentLen = len - accentStartClip - accentEndClip;
            if (accentLen > 0.05f) {
                float accentCenter = (accentStartClip - accentEndClip) * 0.5f;
                DrawCube(Vector3{(float)s * accentHalf, 0.076f, accentCenter},
                         0.05f, 0.008f, accentLen, Color{0, 165, 190, 130});
            }
        }

        // Animated center dashes — energy flowing toward enemies
        int dashCount = (int)(len / dashSpacing) + 1;
        float halfLen = len * 0.5f;
        for (int d = 0; d < dashCount; d++) {
            float dz = -halfLen + dashScroll + (float)d * dashSpacing;
            // Keep dashes clear of the junction squares
            if (dz < -halfLen + 0.65f || dz > halfLen - 0.65f) continue;
            DrawCube(Vector3{0.0f, 0.078f, dz}, 0.14f, 0.010f, 0.55f, Color{0, 200, 235, 150});
        }

        rlPopMatrix();
    }

    // Inner-corner borders: L-shaped connectors through each bend, so edge
    // strips never overlap the perpendicular lane.
    for (int i = 1; i < segCount; i++) {
        Vector3 corner = To3D(path[i], 0.0f);
        Vector2 nPrev = sideDir(segDir[i - 1]);
        Vector2 nNext = sideDir(segDir[i]);
        for (int s = -1; s <= 1; s += 2) {
            if (s * cross2(segDir[i - 1], segDir[i]) <= 0.0f) continue;

            Vector3 edgeA{corner.x + (float)s * nPrev.x * laneHalf, 0.080f,
                          corner.z + (float)s * nPrev.y * laneHalf};
            Vector3 edgeB{corner.x + (float)s * nNext.x * laneHalf, 0.080f,
                          corner.z + (float)s * nNext.y * laneHalf};
            DrawFlatStrip(edgeA, corner, 0.10f, 0.082f, Color{0, 220, 245, 255});
            DrawFlatStrip(corner, edgeB, 0.10f, 0.082f, Color{0, 220, 245, 255});

            Vector3 accentA{corner.x + (float)s * nPrev.x * accentHalf, 0.076f,
                            corner.z + (float)s * nPrev.y * accentHalf};
            Vector3 accentB{corner.x + (float)s * nNext.x * accentHalf, 0.076f,
                            corner.z + (float)s * nNext.y * accentHalf};
            DrawFlatStrip(accentA, corner, 0.05f, 0.078f, Color{0, 165, 190, 130});
            DrawFlatStrip(corner, accentB, 0.05f, 0.078f, Color{0, 165, 190, 130});
        }
    }

    // =================================================================
    // 3. ALIEN CRYSTAL CLUSTERS (Sci-Fi Vegetation)
    // =================================================================
    static const Vector3 crystals[] = {
        {-28.0f, 0.0f, -14.0f}, {-24.0f, 0.0f, -11.5f}, {-26.5f, 0.0f, -6.5f},
        {-18.5f, 0.0f, -15.0f}, {-10.5f, 0.0f, -14.5f}, { 5.5f,  0.0f, -15.5f},
        { 12.5f, 0.0f, -14.5f}, { 22.5f, 0.0f, -14.5f}, { 27.0f, 0.0f, -11.0f},
        {-28.0f, 0.0f,  12.5f}, {-22.5f, 0.0f,  14.5f}, {-14.5f, 0.0f,  15.5f},
        { -2.5f, 0.0f,  13.5f}, { 14.5f, 0.0f,  14.5f}, { 20.5f, 0.0f,  13.5f},
        {-12.5f, 0.0f, -4.5f}, {  0.5f, 0.0f,  -4.5f}, { 12.5f, 0.0f,   2.5f}
    };
    for (const auto& c : crystals) DrawCrystalCluster(c, 1.1f);

    // =================================================================
    // 4. TECH OUTPOST NODES (Sci-Fi Rock Structures)
    // =================================================================
    static const Vector3 outposts[] = {
        {-21.5f, 0.0f, -13.5f}, { 8.5f,  0.0f, -13.5f}, { 25.5f, 0.0f, -12.5f},
        {-16.5f, 0.0f,  13.5f}, { 2.5f,  0.0f,  14.5f}, { 23.5f, 0.0f,  12.5f},
        { -9.0f, 0.0f,  -2.5f}, { 16.5f, 0.0f,  -2.5f}
    };
    for (const auto& o : outposts) DrawTechOutpost(o, 1.0f);

    // =================================================================
    // 5. SPAWN PORTAL & BASE FORTRESS
    // =================================================================
    DrawSpawnPortal(path.front(), globalTime);
    DrawBaseFortress(path.back(), globalTime);
}

// ---------------------------------------------------------------------------
// SCI-FI WARP PORTAL — Spawn point for enemy waves
// ---------------------------------------------------------------------------
void Renderer3D::DrawSpawnPortal(Vector2 startPathPos, float globalTime) {
    Vector3 pos = To3D(startPathPos, 0.0f);

    // Ground emanation pad
    float padPulse = 0.80f + 0.20f * sinf(globalTime * 2.5f);
    DrawCylinder(Vector3{pos.x, 0.04f, pos.z}, 2.35f, 2.35f, 0.08f, 16, Color{10, 16, 28, 255});
    DrawCylinderWires(Vector3{pos.x, 0.04f, pos.z},
                      2.35f * padPulse, 2.35f * padPulse, 0.08f, 16, Color{160, 80, 245, 200});

    // Left & right arch pillars
    DrawCube(Vector3{pos.x - 1.65f, 1.90f, pos.z}, 0.24f, 3.80f, 0.24f, Color{22, 28, 48, 255});
    DrawCubeWires(Vector3{pos.x - 1.65f, 1.90f, pos.z}, 0.24f, 3.80f, 0.24f, Color{160, 80, 245, 180});
    DrawCube(Vector3{pos.x + 1.65f, 1.90f, pos.z}, 0.24f, 3.80f, 0.24f, Color{22, 28, 48, 255});
    DrawCubeWires(Vector3{pos.x + 1.65f, 1.90f, pos.z}, 0.24f, 3.80f, 0.24f, Color{160, 80, 245, 180});

    // Top horizontal crossbar
    DrawCube(Vector3{pos.x, 3.90f, pos.z}, 3.90f, 0.28f, 0.24f, Color{32, 18, 62, 255});
    DrawCubeWires(Vector3{pos.x, 3.90f, pos.z}, 3.90f, 0.28f, 0.24f, Color{192, 130, 255, 255});

    // Outer spinning ring (2 crossing bars rotate around Y axis)
    rlPushMatrix();
    rlTranslatef(pos.x, 2.80f, pos.z);
    rlRotatef(fmodf(globalTime * 55.0f, 360.0f), 0.0f, 1.0f, 0.0f);
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 4.60f, 0.13f, 0.13f, Color{160, 80, 245, 255});
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 0.13f, 0.13f, 4.60f, Color{0, 210, 245, 255});
    rlPopMatrix();

    // Counter-rotating inner ring
    rlPushMatrix();
    rlTranslatef(pos.x, 1.95f, pos.z);
    rlRotatef(fmodf(-globalTime * 85.0f, 360.0f), 0.0f, 1.0f, 0.0f);
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 3.10f, 0.09f, 0.09f, Color{56, 189, 248, 220});
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 0.09f, 0.09f, 3.10f, Color{192, 132, 252, 220});
    rlPopMatrix();

    // Inner energy gate (pulsating color)
    float gp = 0.55f + 0.45f * sinf(globalTime * 4.2f);
    Color gateCol = { (unsigned char)(80 * gp), (unsigned char)(22 * gp), (unsigned char)(140 * gp), 145 };
    DrawCube(Vector3{pos.x, 1.90f, pos.z}, 2.85f, 3.30f, 0.10f, gateCol);
    DrawCubeWires(Vector3{pos.x, 1.90f, pos.z}, 2.85f, 3.30f, 0.10f, Color{160, 80, 245, 200});

    // Central vortex orb
    float orbR = 0.40f + 0.10f * sinf(globalTime * 5.5f);
    DrawSphere(Vector3{pos.x, 1.95f, pos.z}, orbR,          Color{192, 132, 252, 205});
    DrawSphere(Vector3{pos.x, 1.95f, pos.z}, orbR * 0.55f,  Color{255, 255, 255, 225});
    DrawSphereWires(Vector3{pos.x, 1.95f, pos.z}, orbR * 1.25f, 8, 8, Color{160, 80, 245, 90});

    // Floating apex gem (slowly rotates)
    rlPushMatrix();
    rlTranslatef(pos.x, 4.45f, pos.z);
    rlRotatef(fmodf(globalTime * 40.0f, 360.0f), 0.0f, 1.0f, 0.0f);
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 0.38f, 0.52f, 0.38f, Color{192, 132, 252, 255});
    DrawCubeWires(Vector3{0.0f, 0.0f, 0.0f}, 0.38f, 0.52f, 0.38f, Color{255, 255, 255, 200});
    rlPopMatrix();
}

// ---------------------------------------------------------------------------
// COMMAND BUNKER — Player base at end of enemy path
// ---------------------------------------------------------------------------
void Renderer3D::DrawBaseFortress(Vector2 endPathPos, float globalTime) {
    Vector3 pos = To3D(endPathPos, 0.0f);

    // Armored landing platform (tier 0)
    DrawCube(Vector3{pos.x, 0.12f, pos.z}, 6.20f, 0.24f, 6.20f, Color{20, 30, 48, 255});
    DrawCubeWires(Vector3{pos.x, 0.12f, pos.z}, 6.20f, 0.24f, 6.20f, Color{0, 150, 185, 120});

    // Corner defense nodes
    const float cn = 2.55f;
    Color nodeCol = {28, 44, 72, 255};
    Color nodeWire = {0, 200, 225, 150};
    DrawCube(Vector3{pos.x - cn, 0.66f, pos.z - cn}, 0.70f, 1.08f, 0.70f, nodeCol);
    DrawCubeWires(Vector3{pos.x - cn, 0.66f, pos.z - cn}, 0.70f, 1.08f, 0.70f, nodeWire);
    DrawCube(Vector3{pos.x + cn, 0.66f, pos.z - cn}, 0.70f, 1.08f, 0.70f, nodeCol);
    DrawCubeWires(Vector3{pos.x + cn, 0.66f, pos.z - cn}, 0.70f, 1.08f, 0.70f, nodeWire);
    DrawCube(Vector3{pos.x - cn, 0.66f, pos.z + cn}, 0.70f, 1.08f, 0.70f, nodeCol);
    DrawCubeWires(Vector3{pos.x - cn, 0.66f, pos.z + cn}, 0.70f, 1.08f, 0.70f, nodeWire);
    DrawCube(Vector3{pos.x + cn, 0.66f, pos.z + cn}, 0.70f, 1.08f, 0.70f, nodeCol);
    DrawCubeWires(Vector3{pos.x + cn, 0.66f, pos.z + cn}, 0.70f, 1.08f, 0.70f, nodeWire);

    // Main fortified body (tier 1)
    DrawCube(Vector3{pos.x, 1.48f, pos.z}, 4.10f, 2.72f, 4.10f, Color{26, 40, 64, 255});
    DrawCubeWires(Vector3{pos.x, 1.48f, pos.z}, 4.10f, 2.72f, 4.10f, Color{28, 55, 85, 255});

    // Cyan armor band — 4 faces
    DrawCube(Vector3{pos.x, 1.48f, pos.z - 2.07f}, 3.70f, 0.30f, 0.04f, Color{0, 200, 225, 255});
    DrawCube(Vector3{pos.x, 1.48f, pos.z + 2.07f}, 3.70f, 0.30f, 0.04f, Color{0, 200, 225, 255});
    DrawCube(Vector3{pos.x - 2.07f, 1.48f, pos.z}, 0.04f, 0.30f, 3.70f, Color{0, 200, 225, 255});
    DrawCube(Vector3{pos.x + 2.07f, 1.48f, pos.z}, 0.04f, 0.30f, 3.70f, Color{0, 200, 225, 255});

    // Upper command deck (tier 2)
    DrawCube(Vector3{pos.x, 3.02f, pos.z}, 2.90f, 0.68f, 2.90f, Color{34, 52, 84, 255});
    DrawCubeWires(Vector3{pos.x, 3.02f, pos.z}, 2.90f, 0.68f, 2.90f, Color{0, 175, 210, 175});

    // Antenna column (tier 3)
    DrawCylinder(Vector3{pos.x, 3.42f, pos.z}, 0.38f, 0.58f, 0.65f, 8, Color{42, 65, 100, 255});
    DrawCylinder(Vector3{pos.x, 4.35f, pos.z}, 0.22f, 0.32f, 1.55f, 8, Color{26, 40, 64, 255});
    DrawCylinderWires(Vector3{pos.x, 4.35f, pos.z}, 0.24f, 0.34f, 1.55f, 8, Color{0, 180, 215, 195});

    // Rotating radar dish
    rlPushMatrix();
    rlTranslatef(pos.x, 5.20f, pos.z);
    rlRotatef(fmodf(globalTime * 38.0f, 360.0f), 0.0f, 1.0f, 0.0f);
    DrawCube(Vector3{0.35f, 0.0f, 0.0f}, 0.78f, 0.10f, 0.78f, Color{0, 180, 215, 255});
    DrawCube(Vector3{0.0f,  0.0f, 0.0f}, 0.08f, 0.08f, 0.90f, Color{0, 220, 248, 180});
    rlPopMatrix();

    // Top antenna spike
    DrawCylinder(Vector3{pos.x, 5.52f, pos.z}, 0.0f, 0.065f, 0.88f, 6, Color{0, 200, 228, 255});

    // Central pulsating energy shield core
    float cp = 0.50f + 0.50f * sinf(globalTime * 3.8f);
    float coreR = 0.58f + 0.12f * cp;
    Color coreCol = {
        0,
        (unsigned char)(175 + (int)(45 * cp)),
        (unsigned char)(215 + (int)(25 * cp)),
        (unsigned char)(155 + (int)(45 * cp))
    };
    DrawSphere(Vector3{pos.x, 1.48f, pos.z}, coreR, coreCol);
    DrawSphereWires(Vector3{pos.x, 1.48f, pos.z}, coreR * 1.18f, 8, 8,
                    Color{0, 200, 235, (unsigned char)(75 + (int)(90 * cp))});

    // Pulsating shield ring
    rlPushMatrix();
    rlTranslatef(pos.x, 1.48f, pos.z);
    rlRotatef(fmodf(globalTime * 22.0f, 360.0f), 0.0f, 1.0f, 0.0f);
    Color ringCol = {0, 215, 245, (unsigned char)(95 + (int)(85 * cp))};
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 1.65f, 0.09f, 0.09f, ringCol);
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 0.09f, 0.09f, 1.65f, ringCol);
    rlPopMatrix();
}

void Renderer3D::DrawRangeIndicator3D(Vector2 center2D, float range2D, Color color, bool fill) {
    Vector3 c3 = To3D(center2D, 0.06f);
    float r3 = range2D * 0.045f;
    const int segs = 48;

    // Optional translucent disc so the covered area reads clearly (used for
    // the tower placement preview).
    if (fill) {
        Color fillCol = Fade(color, 0.06f);
        for (int i = 0; i < segs; i++) {
            float a1 = (float)i * (2.0f * PI / (float)segs);
            float a2 = (float)(i + 1) * (2.0f * PI / (float)segs);
            Vector3 p1 = { c3.x + cosf(a1) * r3, c3.y, c3.z + sinf(a1) * r3 };
            Vector3 p2 = { c3.x + cosf(a2) * r3, c3.y, c3.z + sinf(a2) * r3 };
            DrawTriangle3D(c3, p1, p2, fillCol);
        }
    }

    for (int i = 0; i < segs; i++) {
        float a1 = (float)i * (2.0f * PI / (float)segs);
        float a2 = (float)(i + 1) * (2.0f * PI / (float)segs);
        Vector3 p1 = { c3.x + cosf(a1) * r3, c3.y, c3.z + sinf(a1) * r3 };
        Vector3 p2 = { c3.x + cosf(a2) * r3, c3.y, c3.z + sinf(a2) * r3 };
        DrawLine3D(p1, p2, color);
    }
}

void Renderer3D::DrawTowerPreview3D(Vector2 pos2D, int towerType, float range2D, bool canBuild) {
    Color col = canBuild ? Color{34, 197, 94, 220} : Color{239, 68, 68, 220};
    
    // Range indicator with translucent fill so the covered area is obvious
    DrawRangeIndicator3D(pos2D, range2D, col, true);
    DrawRangeIndicator3D(pos2D, range2D * 0.3f, Fade(col, 0.15f));

    Vector3 pos = To3D(pos2D, 0.0f);
    Color tint = canBuild ? Color{34, 197, 94, 130} : Color{239, 68, 68, 130};
    Color wireTint = canBuild ? Color{74, 222, 128, 220} : Color{248, 113, 113, 220};
    
    // Subtle ghost effect
    float ghostAlpha = 0.5f;
    Color ghostTint = Fade(tint, ghostAlpha);
    Color ghostWireTint = Fade(wireTint, ghostAlpha);

    // Holographic preview of chosen tower type
    if (towerType == 0) {
        // Basic Tower Preview (Green defense turret)
        DrawCylinder(Vector3{pos.x, 0.10f, pos.z}, 1.25f, 1.40f, 0.20f, 8, ghostTint);
        DrawCylinderWires(Vector3{pos.x, 0.10f, pos.z}, 1.25f, 1.40f, 0.20f, 8, ghostWireTint);
        DrawCylinder(Vector3{pos.x, 0.55f, pos.z}, 0.70f, 0.85f, 0.65f, 8, ghostTint);
        DrawCube(Vector3{pos.x, 1.30f, pos.z}, 0.85f, 0.55f, 1.05f, ghostTint);
        DrawCubeWires(Vector3{pos.x, 1.30f, pos.z}, 0.85f, 0.55f, 1.05f, ghostWireTint);
        DrawCylinderEx(Vector3{pos.x, 1.30f, pos.z + 0.50f}, Vector3{pos.x, 1.30f, pos.z + 1.40f}, 0.11f, 0.10f, 8, ghostTint);
        DrawCylinderWires(Vector3{pos.x, 1.30f, pos.z + 0.95f}, 0.12f, 0.12f, 0.90f, 8, ghostWireTint);
    } else if (towerType == 1) {
        // Rapid Tower Preview (Blue twin-barrel turret)
        DrawCylinder(Vector3{pos.x, 0.10f, pos.z}, 1.15f, 1.30f, 0.20f, 6, ghostTint);
        DrawCylinderWires(Vector3{pos.x, 0.10f, pos.z}, 1.15f, 1.30f, 0.20f, 6, ghostWireTint);
        DrawCylinder(Vector3{pos.x, 0.52f, pos.z}, 0.60f, 0.78f, 0.65f, 6, ghostTint);
        DrawCube(Vector3{pos.x, 1.25f, pos.z - 0.05f}, 0.68f, 0.46f, 0.90f, ghostTint);
        DrawCubeWires(Vector3{pos.x, 1.25f, pos.z - 0.05f}, 0.68f, 0.46f, 0.90f, ghostWireTint);
        DrawCylinderEx(Vector3{pos.x - 0.20f, 1.25f, pos.z + 0.45f}, Vector3{pos.x - 0.20f, 1.25f, pos.z + 1.30f}, 0.065f, 0.060f, 8, ghostTint);
        DrawCylinderEx(Vector3{pos.x + 0.20f, 1.25f, pos.z + 0.45f}, Vector3{pos.x + 0.20f, 1.25f, pos.z + 1.30f}, 0.065f, 0.060f, 8, ghostTint);
    } else if (towerType == 2) {
        // Cannon Tower Preview (Red heavy siege turret)
        DrawCube(Vector3{pos.x, 0.14f, pos.z}, 3.0f, 0.28f, 3.0f, ghostTint);
        DrawCubeWires(Vector3{pos.x, 0.14f, pos.z}, 3.0f, 0.28f, 3.0f, ghostWireTint);
        DrawCylinder(Vector3{pos.x, 0.62f, pos.z}, 1.00f, 1.20f, 0.65f, 8, ghostTint);
        DrawCube(Vector3{pos.x, 1.45f, pos.z}, 1.35f, 0.65f, 1.40f, ghostTint);
        DrawCubeWires(Vector3{pos.x, 1.45f, pos.z}, 1.35f, 0.65f, 1.40f, ghostWireTint);
        DrawCylinderEx(Vector3{pos.x, 1.45f, pos.z + 0.70f}, Vector3{pos.x, 1.45f, pos.z + 1.95f}, 0.22f, 0.19f, 12, ghostTint);
        DrawCube(Vector3{pos.x, 1.45f, pos.z + 2.05f}, 0.48f, 0.44f, 0.30f, ghostWireTint);
    }
    
    // Placement validation feedback particles
    if (!canBuild) {
        // Red warning particles when can't place
        for (int i = 0; i < 8; i++) {
            float angle = (float)i * (360.0f / 8.0f) * DEG2RAD + GetTime() * 2.0f;
            float r = range2D * 0.045f * 0.5f;
            Vector3 pPos = {pos.x + cosf(angle) * r, 0.2f, pos.z + sinf(angle) * r};
            DrawSphere(pPos, 0.08f, Fade(RED, 0.5f + 0.3f * sinf(GetTime() * 5.0f + i)));
        }
    }
}

void Renderer3D::DrawBasicTower3D(Vector3 pos, float yaw, int level, bool isSelected, float globalTime, float muzzleFlash) {
    // ------------------------------------------------------------------------
    // STATIC BASE & PEDESTAL (Does not rotate with target)
    // ------------------------------------------------------------------------
    // Contact drop shadow on ground
    DrawCylinder(Vector3{pos.x, 0.015f, pos.z + 0.12f}, 1.50f, 1.50f, 0.01f, 16, Color{0, 0, 0, 75});

    // Octagonal ground base plate
    DrawCylinder(Vector3{pos.x, 0.10f, pos.z}, 1.25f, 1.40f, 0.20f, 8, Color{15, 23, 42, 255});
    DrawCylinderWires(Vector3{pos.x, 0.10f, pos.z}, 1.25f, 1.40f, 0.20f, 8, Color{34, 197, 94, 140});
    DrawCylinder(Vector3{pos.x, 0.21f, pos.z}, 1.05f, 1.05f, 0.04f, 8, Color{30, 41, 59, 255});

    // Pedestal column with vertical structural ribs
    DrawCylinder(Vector3{pos.x, 0.55f, pos.z}, 0.70f, 0.85f, 0.65f, 8, Color{51, 65, 85, 255});
    DrawCube(Vector3{pos.x, 0.55f, pos.z - 0.75f}, 0.24f, 0.50f, 0.18f, Color{71, 85, 105, 255});
    DrawCube(Vector3{pos.x, 0.55f, pos.z + 0.75f}, 0.24f, 0.50f, 0.18f, Color{71, 85, 105, 255});
    DrawCube(Vector3{pos.x - 0.75f, 0.55f, pos.z}, 0.18f, 0.50f, 0.24f, Color{71, 85, 105, 255});
    DrawCube(Vector3{pos.x + 0.75f, 0.55f, pos.z}, 0.18f, 0.50f, 0.24f, Color{71, 85, 105, 255});

    // Swivel collar bearing ring
    DrawCylinder(Vector3{pos.x, 0.90f, pos.z}, 0.78f, 0.78f, 0.12f, 12, Color{15, 23, 42, 255});
    DrawCylinderWires(Vector3{pos.x, 0.90f, pos.z}, 0.79f, 0.79f, 0.12f, 12, Color{34, 197, 94, 255});

    // Static Upgrades on Pedestal
    if (level >= 2) {
        // 4 corner reinforcement struts
        DrawCube(Vector3{pos.x - 1.0f, 0.28f, pos.z - 1.0f}, 0.35f, 0.35f, 0.35f, Color{100, 116, 139, 255});
        DrawCube(Vector3{pos.x + 1.0f, 0.28f, pos.z - 1.0f}, 0.35f, 0.35f, 0.35f, Color{100, 116, 139, 255});
        DrawCube(Vector3{pos.x - 1.0f, 0.28f, pos.z + 1.0f}, 0.35f, 0.35f, 0.35f, Color{100, 116, 139, 255});
        DrawCube(Vector3{pos.x + 1.0f, 0.28f, pos.z + 1.0f}, 0.35f, 0.35f, 0.35f, Color{100, 116, 139, 255});
    }
    if (level == 3) {
        // Glowing green energy conduits on pedestal
        DrawCylinder(Vector3{pos.x, 0.55f, pos.z}, 0.82f, 0.82f, 0.08f, 8, Color{74, 222, 128, 255});
    }

    // Selection glow: pulsing energy ring at the base when this tower is
    // selected. Makes the selected tower readable on the body itself, not
    // just via the corner brackets drawn in DrawTower3D().
    if (isSelected) {
        float pulse = 0.5f + 0.5f * sinf(globalTime * 5.0f);
        float ringR = 1.55f + 0.10f * pulse;
        DrawCylinder(Vector3{pos.x, 0.22f, pos.z}, ringR, ringR, 0.02f, 24,
                     Fade(Color{250, 204, 21, 255}, 0.15f + 0.20f * pulse));
        DrawCylinderWires(Vector3{pos.x, 0.24f, pos.z}, ringR, ringR, 0.02f, 24,
                          Fade(Color{250, 204, 21, 255}, 0.55f + 0.35f * pulse));
    }

    // ------------------------------------------------------------------------
    // ROTATING TURRET & BARREL (Yaw tracks target direction)
    // ------------------------------------------------------------------------
    rlPushMatrix();
    rlTranslatef(pos.x, 0.96f, pos.z);
    rlRotatef(yaw, 0.0f, 1.0f, 0.0f);

    // Main pod chassis
    DrawCube(Vector3{0.0f, 0.35f, -0.05f}, 0.85f, 0.55f, 1.05f, Color{30, 41, 59, 255});
    DrawCubeWires(Vector3{0.0f, 0.35f, -0.05f}, 0.85f, 0.55f, 1.05f, Color{15, 23, 42, 255});

    // Top sloped cowl
    DrawCube(Vector3{0.0f, 0.62f, -0.10f}, 0.65f, 0.12f, 0.75f, Color{51, 65, 85, 255});

    // Green sci-fi cheek armor plates
    DrawCube(Vector3{-0.44f, 0.35f, -0.05f}, 0.08f, 0.40f, 0.90f, Color{34, 197, 94, 255});
    DrawCube(Vector3{ 0.44f, 0.35f, -0.05f}, 0.08f, 0.40f, 0.90f, Color{34, 197, 94, 255});

    // Targeting sensor optic dome
    DrawCylinder(Vector3{0.0f, 0.70f, 0.15f}, 0.12f, 0.15f, 0.16f, 8, Color{15, 23, 42, 255});
    DrawSphere(Vector3{0.0f, 0.76f, 0.15f}, 0.13f, Color{74, 222, 128, 255});

    // Single Short/Medium Barrel
    float barrelLen = (level == 1) ? 1.40f : (level == 2) ? 1.60f : 1.75f;
    float bStart = 0.58f;
    float bEnd = barrelLen;
    DrawCube(Vector3{0.0f, 0.34f, 0.52f}, 0.36f, 0.36f, 0.22f, Color{15, 23, 42, 255});
    DrawCube(Vector3{0.0f, 0.48f, 0.52f}, 0.26f, 0.06f, 0.24f, Color{34, 197, 94, 255});

    DrawCylinderEx(Vector3{0.0f, 0.34f, bStart}, Vector3{0.0f, 0.34f, bEnd}, 0.11f, 0.10f, 8, Color{51, 65, 85, 255});
    DrawCylinderEx(Vector3{0.0f, 0.34f, bEnd - 0.12f}, Vector3{0.0f, 0.34f, bEnd + 0.02f}, 0.13f, 0.13f, 8, Color{15, 23, 42, 255});
    DrawCylinderEx(Vector3{0.0f, 0.34f, bEnd}, Vector3{0.0f, 0.34f, bEnd + 0.03f}, 0.11f, 0.08f, 8, Color{34, 197, 94, 255});

    // Muzzle flash - Enhanced with multiple layers
    if (muzzleFlash > 0.0f) {
        Vector3 flashPos = {0.0f, 0.34f, bEnd + 0.05f};
        // Outer glow
        float outerSize = 0.25f + 0.15f * muzzleFlash;
        DrawSphere(flashPos, outerSize, Fade(Color{100, 200, 255, 255}, muzzleFlash * 0.3f));
        // Middle ring
        float midSize = 0.18f + 0.12f * muzzleFlash;
        DrawSphere(flashPos, midSize, Fade(Color{200, 240, 255, 255}, muzzleFlash * 0.6f));
        // Inner core
        float coreSize = 0.10f + 0.08f * muzzleFlash;
        DrawSphere(flashPos, coreSize, Fade(Color{255, 255, 255, 255}, muzzleFlash));
        // Tiny bright center
        DrawSphere(flashPos, coreSize * 0.4f, Fade(Color{255, 255, 255, 255}, 1.0f));
    }

    // Level 3 extra detail: Dual sensor antennae
    if (level == 3) {
        DrawCube(Vector3{-0.28f, 0.65f, -0.30f}, 0.06f, 0.30f, 0.06f, Color{203, 213, 225, 255});
        DrawCube(Vector3{ 0.28f, 0.65f, -0.30f}, 0.06f, 0.30f, 0.06f, Color{203, 213, 225, 255});
    }

    rlPopMatrix();
}

void Renderer3D::DrawRapidTower3D(Vector3 pos, float yaw, int level, bool isSelected, float globalTime, float muzzleFlash) {
    // ------------------------------------------------------------------------
    // STATIC BASE & PEDESTAL (Hexagonal aerodynamic riser)
    // ------------------------------------------------------------------------
    // Contact drop shadow on ground
    DrawCylinder(Vector3{pos.x, 0.015f, pos.z + 0.10f}, 1.40f, 1.40f, 0.01f, 16, Color{0, 0, 0, 75});

    // Hexagonal base plate
    DrawCylinder(Vector3{pos.x, 0.10f, pos.z}, 1.15f, 1.30f, 0.20f, 6, Color{15, 23, 42, 255});
    DrawCylinderWires(Vector3{pos.x, 0.10f, pos.z}, 1.15f, 1.30f, 0.20f, 6, Color{14, 165, 233, 160});
    DrawCylinder(Vector3{pos.x, 0.21f, pos.z}, 0.95f, 0.95f, 0.04f, 6, Color{30, 41, 59, 255});

    // Tapered hexagonal pedestal
    DrawCylinder(Vector3{pos.x, 0.52f, pos.z}, 0.60f, 0.78f, 0.65f, 6, Color{30, 41, 59, 255});
    // Cyan traverse bearing ring
    DrawCylinder(Vector3{pos.x, 0.88f, pos.z}, 0.70f, 0.70f, 0.12f, 12, Color{14, 165, 233, 255});
    DrawCylinderWires(Vector3{pos.x, 0.88f, pos.z}, 0.71f, 0.71f, 0.12f, 12, Color{56, 189, 248, 255});

    // Static Upgrades on Pedestal
    if (level >= 2) {
        DrawCylinder(Vector3{pos.x - 0.75f, 0.35f, pos.z}, 0.14f, 0.14f, 0.40f, 6, Color{14, 165, 233, 255});
        DrawCylinder(Vector3{pos.x + 0.75f, 0.35f, pos.z}, 0.14f, 0.14f, 0.40f, 6, Color{14, 165, 233, 255});
    }
    if (level == 3) {
        DrawCylinder(Vector3{pos.x, 0.35f, pos.z - 0.75f}, 0.14f, 0.14f, 0.40f, 6, Color{56, 189, 248, 255});
        DrawCylinder(Vector3{pos.x, 0.35f, pos.z + 0.75f}, 0.14f, 0.14f, 0.40f, 6, Color{56, 189, 248, 255});
    }

    // Selection glow (pulsing base ring) - see DrawBasicTower3D for details.
    if (isSelected) {
        float pulse = 0.5f + 0.5f * sinf(globalTime * 5.0f);
        float ringR = 1.55f + 0.10f * pulse;
        DrawCylinder(Vector3{pos.x, 0.22f, pos.z}, ringR, ringR, 0.02f, 24,
                     Fade(Color{250, 204, 21, 255}, 0.15f + 0.20f * pulse));
        DrawCylinderWires(Vector3{pos.x, 0.24f, pos.z}, ringR, ringR, 0.02f, 24,
                          Fade(Color{250, 204, 21, 255}, 0.55f + 0.35f * pulse));
    }

    // ------------------------------------------------------------------------
    // ROTATING TURRET & DUAL BARRELS (Compact, distinct wedge silhouette)
    // ------------------------------------------------------------------------
    rlPushMatrix();
    rlTranslatef(pos.x, 0.94f, pos.z);
    rlRotatef(yaw, 0.0f, 1.0f, 0.0f);

    // Central narrow pod
    DrawCube(Vector3{0.0f, 0.30f, -0.10f}, 0.68f, 0.46f, 0.90f, Color{30, 41, 59, 255});
    DrawCubeWires(Vector3{0.0f, 0.30f, -0.10f}, 0.68f, 0.46f, 0.90f, Color{15, 23, 42, 255});

    // Beveled top spine with cyan stripe
    DrawCube(Vector3{0.0f, 0.54f, -0.05f}, 0.28f, 0.08f, 0.80f, Color{14, 165, 233, 255});

    // Angled cheek armor plates with cyan vents
    DrawCube(Vector3{-0.36f, 0.30f, -0.10f}, 0.08f, 0.34f, 0.75f, Color{56, 189, 248, 255});
    DrawCube(Vector3{ 0.36f, 0.30f, -0.10f}, 0.08f, 0.34f, 0.75f, Color{56, 189, 248, 255});

    // High-rate sensor bar on top
    DrawCube(Vector3{0.0f, 0.60f, -0.15f}, 0.48f, 0.10f, 0.18f, Color{186, 230, 253, 255});

    // Rear ammo magazine box
    DrawCube(Vector3{0.0f, 0.28f, -0.52f}, 0.52f, 0.38f, 0.24f, Color{15, 23, 42, 255});

    // DUAL PARALLEL BARRELS (Clearly visible twin-gun configuration)
    float barrelLen = (level == 1) ? 1.30f : (level == 2) ? 1.48f : 1.62f;
    float bRadius = 0.065f;
    float bStart = 0.48f;
    float bEnd = barrelLen;

    // Left barrel
    DrawCube(Vector3{-0.20f, 0.28f, 0.42f}, 0.16f, 0.20f, 0.22f, Color{15, 23, 42, 255});
    DrawCylinderEx(Vector3{-0.20f, 0.28f, bStart}, Vector3{-0.20f, 0.28f, bEnd}, bRadius, bRadius - 0.005f, 8, Color{51, 65, 85, 255});
    DrawCylinderEx(Vector3{-0.20f, 0.28f, bEnd - 0.10f}, Vector3{-0.20f, 0.28f, bEnd + 0.02f}, bRadius + 0.025f, bRadius + 0.025f, 8, Color{14, 165, 233, 255});

    // Right barrel
    DrawCube(Vector3{ 0.20f, 0.28f, 0.42f}, 0.16f, 0.20f, 0.22f, Color{15, 23, 42, 255});
    DrawCylinderEx(Vector3{ 0.20f, 0.28f, bStart}, Vector3{ 0.20f, 0.28f, bEnd}, bRadius, bRadius - 0.005f, 8, Color{51, 65, 85, 255});
    DrawCylinderEx(Vector3{ 0.20f, 0.28f, bEnd - 0.10f}, Vector3{ 0.20f, 0.28f, bEnd + 0.02f}, bRadius + 0.025f, bRadius + 0.025f, 8, Color{14, 165, 233, 255});

    // Level 2 & 3: Twin-barrel stabilizer bridge
    if (level >= 2) {
        DrawCube(Vector3{0.0f, 0.28f, 0.95f}, 0.38f, 0.06f, 0.12f, Color{56, 189, 248, 255});
    }
    if (level == 3) {
        DrawCube(Vector3{0.0f, 0.28f, 1.30f}, 0.38f, 0.06f, 0.12f, Color{186, 230, 253, 255});
    }

    // Muzzle flash (both barrels) - Enhanced
    if (muzzleFlash > 0.0f) {
        // Left barrel
        Vector3 lPos = {-0.20f, 0.28f, bEnd + 0.05f};
        float outerSize = 0.18f + 0.12f * muzzleFlash;
        DrawSphere(lPos, outerSize, Fade(Color{56, 189, 248, 255}, muzzleFlash * 0.3f));
        float midSize = 0.14f + 0.10f * muzzleFlash;
        DrawSphere(lPos, midSize, Fade(Color{186, 230, 253, 255}, muzzleFlash * 0.6f));
        float coreSize = 0.08f + 0.06f * muzzleFlash;
        DrawSphere(lPos, coreSize, Fade(Color{255, 255, 255, 255}, muzzleFlash));
        DrawSphere(lPos, coreSize * 0.4f, Fade(Color{255, 255, 255, 255}, 1.0f));
        
        // Right barrel
        Vector3 rPos = {0.20f, 0.28f, bEnd + 0.05f};
        DrawSphere(rPos, outerSize, Fade(Color{56, 189, 248, 255}, muzzleFlash * 0.3f));
        DrawSphere(rPos, midSize, Fade(Color{186, 230, 253, 255}, muzzleFlash * 0.6f));
        DrawSphere(rPos, coreSize, Fade(Color{255, 255, 255, 255}, muzzleFlash));
        DrawSphere(rPos, coreSize * 0.4f, Fade(Color{255, 255, 255, 255}, 1.0f));
    }

    rlPopMatrix();
}

void Renderer3D::DrawCannonTower3D(Vector3 pos, float yaw, int level, bool isSelected, float globalTime, float muzzleFlash) {
    // ------------------------------------------------------------------------
    // STATIC BASE & PEDESTAL (Heavy Fortified Outriggers)
    // ------------------------------------------------------------------------
    // Contact drop shadow on ground
    DrawCylinder(Vector3{pos.x, 0.015f, pos.z + 0.15f}, 1.90f, 1.90f, 0.01f, 16, Color{0, 0, 0, 85});

    // Heavy square base plate
    DrawCube(Vector3{pos.x, 0.14f, pos.z}, 3.0f, 0.28f, 3.0f, Color{15, 23, 42, 255});
    DrawCubeWires(Vector3{pos.x, 0.14f, pos.z}, 3.0f, 0.28f, 3.0f, Color{239, 68, 68, 140});
    DrawCube(Vector3{pos.x, 0.28f, pos.z}, 2.6f, 0.06f, 2.6f, Color{30, 41, 59, 255});

    // 4 Corner Hydraulic Stabilizer Footpads
    DrawCube(Vector3{pos.x - 1.30f, 0.20f, pos.z - 1.30f}, 0.42f, 0.35f, 0.42f, Color{71, 85, 105, 255});
    DrawCube(Vector3{pos.x + 1.30f, 0.20f, pos.z - 1.30f}, 0.42f, 0.35f, 0.42f, Color{71, 85, 105, 255});
    DrawCube(Vector3{pos.x - 1.30f, 0.20f, pos.z + 1.30f}, 0.42f, 0.35f, 0.42f, Color{71, 85, 105, 255});
    DrawCube(Vector3{pos.x + 1.30f, 0.20f, pos.z + 1.30f}, 0.42f, 0.35f, 0.42f, Color{71, 85, 105, 255});

    // Heavy cylindrical mount column
    DrawCylinder(Vector3{pos.x, 0.62f, pos.z}, 1.00f, 1.20f, 0.65f, 8, Color{51, 65, 85, 255});
    // Heavy traverse ring with red hazard border
    DrawCylinder(Vector3{pos.x, 0.98f, pos.z}, 1.15f, 1.15f, 0.14f, 16, Color{15, 23, 42, 255});
    DrawCylinderWires(Vector3{pos.x, 0.98f, pos.z}, 1.16f, 1.16f, 0.14f, 16, Color{239, 68, 68, 255});

    // Static Upgrades on Pedestal
    if (level >= 2) {
        DrawCube(Vector3{pos.x - 1.25f, 0.42f, pos.z}, 0.25f, 0.35f, 0.85f, Color{239, 68, 68, 255});
        DrawCube(Vector3{pos.x + 1.25f, 0.42f, pos.z}, 0.25f, 0.35f, 0.85f, Color{239, 68, 68, 255});
    }
    if (level == 3) {
        DrawCylinder(Vector3{pos.x - 1.15f, 0.50f, pos.z}, 0.24f, 0.24f, 0.45f, 8, Color{249, 115, 22, 255});
        DrawCylinder(Vector3{pos.x + 1.15f, 0.50f, pos.z}, 0.24f, 0.24f, 0.45f, 8, Color{249, 115, 22, 255});
    }

    // Selection glow (pulsing base ring) - see DrawBasicTower3D for details.
    if (isSelected) {
        float pulse = 0.5f + 0.5f * sinf(globalTime * 5.0f);
        float ringR = 1.85f + 0.10f * pulse;   // cannon has a wider footprint
        DrawCylinder(Vector3{pos.x, 0.22f, pos.z}, ringR, ringR, 0.02f, 24,
                     Fade(Color{250, 204, 21, 255}, 0.15f + 0.20f * pulse));
        DrawCylinderWires(Vector3{pos.x, 0.24f, pos.z}, ringR, ringR, 0.02f, 24,
                          Fade(Color{250, 204, 21, 255}, 0.55f + 0.35f * pulse));
    }

    // ------------------------------------------------------------------------
    // ROTATING HEAVY TURRET & MASSIVE CANNON (Artillery gunhouse)
    // ------------------------------------------------------------------------
    rlPushMatrix();
    rlTranslatef(pos.x, 1.05f, pos.z);
    rlRotatef(yaw, 0.0f, 1.0f, 0.0f);

    // Main casemate citadel
    DrawCube(Vector3{0.0f, 0.40f, -0.05f}, 1.35f, 0.65f, 1.40f, Color{30, 41, 59, 255});
    DrawCubeWires(Vector3{0.0f, 0.40f, -0.05f}, 1.35f, 0.65f, 1.40f, Color{15, 23, 42, 255});

    // Sloped upper armor plate
    DrawCube(Vector3{0.0f, 0.74f, -0.15f}, 1.05f, 0.20f, 1.05f, Color{51, 65, 85, 255});

    // Red armor side skirts
    DrawCube(Vector3{-0.72f, 0.40f, -0.05f}, 0.12f, 0.50f, 1.25f, Color{239, 68, 68, 255});
    DrawCube(Vector3{ 0.72f, 0.40f, -0.05f}, 0.12f, 0.50f, 1.25f, Color{239, 68, 68, 255});

    // Red front armor faceplates
    DrawCube(Vector3{-0.45f, 0.42f, 0.62f}, 0.32f, 0.45f, 0.12f, Color{220, 38, 38, 255});
    DrawCube(Vector3{ 0.45f, 0.42f, 0.62f}, 0.32f, 0.45f, 0.12f, Color{220, 38, 38, 255});

    // Rear counterweight & blast exhaust bustle
    DrawCube(Vector3{0.0f, 0.40f, -0.80f}, 1.15f, 0.55f, 0.40f, Color{15, 23, 42, 255});
    // Orange rangefinder optic bar on roof
    DrawCube(Vector3{0.0f, 0.88f, 0.15f}, 0.75f, 0.08f, 0.14f, Color{249, 115, 22, 255});

    // ONE MASSIVE HEAVY CANNON BARREL
    float barrelLen = (level == 1) ? 1.85f : (level == 2) ? 2.10f : 2.30f;
    float barrelRad = 0.20f;
    float bStart = 0.78f;
    float bEnd = barrelLen;

    // Heavy central mantlet
    DrawCube(Vector3{0.0f, 0.40f, 0.68f}, 0.58f, 0.50f, 0.32f, Color{15, 23, 42, 255});
    DrawCube(Vector3{0.0f, 0.40f, 0.68f}, 0.62f, 0.38f, 0.28f, Color{239, 68, 68, 255});

    // Hydraulic recoil cylinders on top of mantlet
    DrawCylinderEx(Vector3{-0.22f, 0.60f, 0.60f}, Vector3{-0.22f, 0.60f, 1.15f}, 0.065f, 0.065f, 8, Color{148, 163, 184, 255});
    DrawCylinderEx(Vector3{ 0.22f, 0.60f, 0.60f}, Vector3{ 0.22f, 0.60f, 1.15f}, 0.065f, 0.065f, 8, Color{148, 163, 184, 255});

    // Main heavy barrel tube along +Z
    DrawCylinderEx(Vector3{0.0f, 0.40f, bStart}, Vector3{0.0f, 0.40f, bEnd}, barrelRad, barrelRad - 0.02f, 12, Color{35, 39, 47, 255});
    // Thermal shroud sleeve
    DrawCylinderEx(Vector3{0.0f, 0.40f, 0.80f}, Vector3{0.0f, 0.40f, 1.30f}, barrelRad + 0.04f, barrelRad + 0.04f, 8, Color{71, 85, 105, 255});

    // Massive slotted muzzle brake at the tip
    DrawCube(Vector3{0.0f, 0.40f, bEnd + 0.12f}, 0.48f, 0.44f, 0.30f, Color{220, 38, 38, 255});
    DrawCubeWires(Vector3{0.0f, 0.40f, bEnd + 0.12f}, 0.48f, 0.44f, 0.30f, Color{249, 115, 22, 255});
    DrawCylinderEx(Vector3{0.0f, 0.40f, bEnd - 0.05f}, Vector3{0.0f, 0.40f, bEnd + 0.28f}, 0.13f, 0.13f, 8, Color{15, 23, 42, 255});

    // Level 3 extra detail: Side thermal dissipation fins
    if (level == 3) {
        DrawCube(Vector3{-0.82f, 0.42f, 0.10f}, 0.08f, 0.30f, 0.60f, Color{249, 115, 22, 255});
        DrawCube(Vector3{ 0.82f, 0.42f, 0.10f}, 0.08f, 0.30f, 0.60f, Color{249, 115, 22, 255});
    }

    // Muzzle flash - Enhanced for cannon
    if (muzzleFlash > 0.0f) {
        Vector3 flashPos = {0.0f, 0.40f, bEnd + 0.30f};
        // Outer fireball
        float outerSize = 0.35f + 0.20f * muzzleFlash;
        DrawSphere(flashPos, outerSize, Fade(Color{249, 115, 22, 255}, muzzleFlash * 0.25f));
        // Middle explosion
        float midSize = 0.25f + 0.15f * muzzleFlash;
        DrawSphere(flashPos, midSize, Fade(Color{255, 140, 30, 255}, muzzleFlash * 0.5f));
        // Inner core
        float coreSize = 0.15f + 0.10f * muzzleFlash;
        DrawSphere(flashPos, coreSize, Fade(Color{255, 220, 100, 255}, muzzleFlash * 0.8f));
        // Blinding center
        DrawSphere(flashPos, coreSize * 0.3f, Fade(Color{255, 255, 255, 255}, 1.0f));
    }

    rlPopMatrix();
}

void Renderer3D::DrawTower3D(const Tower& tower, bool isSelected, float globalTime) {
    Vector3 pos = To3D(tower.GetPosition(), 0.0f);
    TowerType type = tower.GetType();
    int level = tower.GetLevel();
    Vector2 bDir = tower.GetBarrelDirection();
    float yaw = atan2f(bDir.x, bDir.y) * RAD2DEG;
    float flash = tower.GetMuzzleFlashTimer();

    if (isSelected) {
        // Futuristic range indicator - subtle, no pulse
        DrawRangeIndicator3D(tower.GetPosition(), tower.GetRange(), Color{250, 204, 21, 180});
        DrawRangeIndicator3D(tower.GetPosition(), tower.GetRange() * 0.5f, Color{250, 204, 21, 60});

        // Corner selection target brackets around base
        float bSize = (type == TowerType::CANNON) ? 1.70f : 1.45f;
        Color selCol = Color{250, 204, 21, 240};
        DrawCube(Vector3{pos.x - bSize, 0.05f, pos.z - bSize}, 0.35f, 0.06f, 0.10f, selCol);
        DrawCube(Vector3{pos.x - bSize, 0.05f, pos.z - bSize}, 0.10f, 0.06f, 0.35f, selCol);
        DrawCube(Vector3{pos.x + bSize, 0.05f, pos.z - bSize}, 0.35f, 0.06f, 0.10f, selCol);
        DrawCube(Vector3{pos.x + bSize, 0.05f, pos.z - bSize}, 0.10f, 0.06f, 0.35f, selCol);
        DrawCube(Vector3{pos.x - bSize, 0.05f, pos.z + bSize}, 0.35f, 0.06f, 0.10f, selCol);
        DrawCube(Vector3{pos.x - bSize, 0.05f, pos.z + bSize}, 0.10f, 0.06f, 0.35f, selCol);
        DrawCube(Vector3{pos.x + bSize, 0.05f, pos.z + bSize}, 0.35f, 0.06f, 0.10f, selCol);
        DrawCube(Vector3{pos.x + bSize, 0.05f, pos.z + bSize}, 0.10f, 0.06f, 0.35f, selCol);
    }

    switch (type) {
        case TowerType::BASIC:
            DrawBasicTower3D(pos, yaw, level, isSelected, globalTime, flash);
            break;
        case TowerType::RAPID:
            DrawRapidTower3D(pos, yaw, level, isSelected, globalTime, flash);
            break;
        case TowerType::CANNON:
            DrawCannonTower3D(pos, yaw, level, isSelected, globalTime, flash);
            break;
    }
}

void Renderer3D::DrawEnemy3D(const Enemy& enemy, float globalTime) {
    if (!enemy.IsAlive()) return;

    Vector3 pos = To3D(enemy.GetPosition(), 0.0f);
    EnemyType type = enemy.GetType();
    float wt = enemy.GetWalkTimer();
    float facing = enemy.GetFacingAngle();
    float lean = enemy.GetLeanAngle();  // Add getter
    float bob = enemy.GetBobOffset();   // Add getter
    float hitFlash = enemy.GetHitFlashTimer(); // Add getter
    // Konversi arah gerak 2D ke sudut rotasi yaw 3D
    float yaw = atan2f(cosf(facing), sinf(facing)) * RAD2DEG;

    rlPushMatrix();
    rlTranslatef(pos.x, 0.0f, pos.z);
    rlRotatef(yaw, 0.0f, 1.0f, 0.0f);

    // Apply lean (roll) and bob (vertical offset)
    rlPushMatrix();
    if (lean != 0.0f) {
        rlRotatef(lean * RAD2DEG, 0.0f, 0.0f, 1.0f);
    }
    rlTranslatef(0.0f, bob, 0.0f);

    // Hit flash intensity (0-1)
    float hitFlashIntensity = 0.0f;
    if (hitFlash > 0.0f) {
        hitFlashIntensity = hitFlash * 10.0f;
        if (hitFlashIntensity > 1.0f) hitFlashIntensity = 1.0f;
    }

    // ============================================================
    // MUSUH 1: NORMAL — ASSAULT DRONE (Merah, armor berlapis)
    // Dari atas: segi enam gepeng + 4 thruster di pojok + sensor ring
    // ============================================================
    if (type == EnemyType::NORMAL) {
        float bob   = 0.06f * sinf(wt * 8.0f);           // bobbing vertikal
        float sRing = fmodf(globalTime * 140.0f, 360.0f); // sensor ring rotation

        // Landing struts / kaki pendek (4 sudut)
        DrawCube(Vector3{-0.28f, 0.06f, -0.28f}, 0.10f, 0.12f, 0.10f, Color{80, 20, 20, 255});
        DrawCube(Vector3{ 0.28f, 0.06f, -0.28f}, 0.10f, 0.12f, 0.10f, Color{80, 20, 20, 255});
        DrawCube(Vector3{-0.28f, 0.06f,  0.28f}, 0.10f, 0.12f, 0.10f, Color{80, 20, 20, 255});
        DrawCube(Vector3{ 0.28f, 0.06f,  0.28f}, 0.10f, 0.12f, 0.10f, Color{80, 20, 20, 255});

        // Badan utama: cakram armor berlapis (terlihat dari atas sebagai hexagon)
        DrawCylinder(Vector3{0.0f, 0.18f + bob, 0.0f}, 0.52f, 0.62f, 0.18f, 6, Color{170, 28, 28, 255});
        DrawCylinder(Vector3{0.0f, 0.30f + bob, 0.0f}, 0.40f, 0.40f, 0.10f, 6, Color{220, 38, 38, 255});
        DrawCylinderWires(Vector3{0.0f, 0.30f + bob, 0.0f}, 0.42f, 0.42f, 0.10f, 6, Color{248, 113, 113, 200});

        // Panel samping merah gelap (armor plate kiri-kanan)
        DrawCube(Vector3{-0.55f, 0.22f + bob, 0.0f}, 0.14f, 0.20f, 0.42f, Color{140, 20, 20, 255});
        DrawCube(Vector3{ 0.55f, 0.22f + bob, 0.0f}, 0.14f, 0.20f, 0.42f, Color{140, 20, 20, 255});

        // Thruster pods di 4 sudut (lingkaran kecil)
        DrawCylinder(Vector3{-0.48f, 0.14f + bob, -0.48f}, 0.10f, 0.12f, 0.14f, 6, Color{60, 10, 10, 255});
        DrawCylinder(Vector3{ 0.48f, 0.14f + bob, -0.48f}, 0.10f, 0.12f, 0.14f, 6, Color{60, 10, 10, 255});
        DrawCylinder(Vector3{-0.48f, 0.14f + bob,  0.48f}, 0.10f, 0.12f, 0.14f, 6, Color{60, 10, 10, 255});
        DrawCylinder(Vector3{ 0.48f, 0.14f + bob,  0.48f}, 0.10f, 0.12f, 0.14f, 6, Color{60, 10, 10, 255});
        // Thruster glow
        DrawCylinder(Vector3{-0.48f, 0.07f + bob, -0.48f}, 0.07f, 0.07f, 0.04f, 6, Color{255, 100, 30, 200});
        DrawCylinder(Vector3{ 0.48f, 0.07f + bob, -0.48f}, 0.07f, 0.07f, 0.04f, 6, Color{255, 100, 30, 200});
        DrawCylinder(Vector3{-0.48f, 0.07f + bob,  0.48f}, 0.07f, 0.07f, 0.04f, 6, Color{255, 100, 30, 200});
        DrawCylinder(Vector3{ 0.48f, 0.07f + bob,  0.48f}, 0.07f, 0.07f, 0.04f, 6, Color{255, 100, 30, 200});

        // Core energi merah berdenyut di tengah atas
        float cp = 0.5f + 0.5f * sinf(globalTime * 6.0f);
        DrawSphere(Vector3{0.0f, 0.44f + bob, 0.0f}, 0.13f + 0.03f * cp, Color{255, 80, 80, 230});
        DrawSphere(Vector3{0.0f, 0.44f + bob, 0.0f}, 0.07f,             Color{255, 220, 220, 255});

        // Sensor ring berputar (terlihat dari atas sebagai garis silang)
        rlPushMatrix();
        rlTranslatef(0.0f, 0.46f + bob, 0.0f);
        rlRotatef(sRing, 0.0f, 1.0f, 0.0f);
        DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 0.90f, 0.04f, 0.04f, Color{255, 130, 130, 200});
        DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 0.04f, 0.04f, 0.90f, Color{255, 160, 80,  180});
        rlPopMatrix();
    }

    // ============================================================
    // MUSUH 2: FAST — SCOUT DRONE (Kuning/Amber, ramping & aerodinamis)
    // Dari atas: badan lonjong tipis + 2 sirip samping membentang
    // ============================================================
    else if (type == EnemyType::FAST) {
        float bob  = 0.07f * sinf(wt * 14.0f);          // cepat bobbing
        float tilt = 6.0f  * sinf(globalTime * 5.5f);   // tilt kiri-kanan ringan

        // Tilt ringan saat melayang cepat
        rlPushMatrix();
        rlRotatef(tilt, 0.0f, 0.0f, 1.0f);

        // Badan utama: lonjong tipis ke arah gerak (+Z = forward)
        DrawCube(Vector3{0.0f, 0.26f + bob, 0.0f}, 0.28f, 0.18f, 0.72f, Color{180, 120, 0, 255});
        // Lapisan armor atas (warna amber lebih cerah)
        DrawCube(Vector3{0.0f, 0.34f + bob, 0.0f}, 0.20f, 0.06f, 0.60f, Color{234, 179, 8, 255});

        // Sirip kiri & kanan membentang (terlihat dari atas seperti sayap)
        DrawCube(Vector3{-0.58f, 0.24f + bob, 0.06f}, 0.55f, 0.07f, 0.38f, Color{200, 140, 0, 255});
        DrawCube(Vector3{ 0.58f, 0.24f + bob, 0.06f}, 0.55f, 0.07f, 0.38f, Color{200, 140, 0, 255});
        // Ujung sirip (warna lebih gelap)
        DrawCube(Vector3{-0.88f, 0.24f + bob, 0.12f}, 0.10f, 0.06f, 0.18f, Color{140, 90, 0, 255});
        DrawCube(Vector3{ 0.88f, 0.24f + bob, 0.12f}, 0.10f, 0.06f, 0.18f, Color{140, 90, 0, 255});

        // Ekor stabilizer (belakang, -Z)
        DrawCube(Vector3{-0.22f, 0.28f + bob, -0.42f}, 0.06f, 0.16f, 0.12f, Color{160, 100, 0, 255});
        DrawCube(Vector3{ 0.22f, 0.28f + bob, -0.42f}, 0.06f, 0.16f, 0.12f, Color{160, 100, 0, 255});

        // Hidung runcing (depan, +Z)
        DrawCylinder(Vector3{0.0f, 0.26f + bob, 0.36f}, 0.0f, 0.12f, 0.28f, 6, Color{245, 200, 20, 255});

        // Core energi amber kecil
        float cp = 0.5f + 0.5f * sinf(globalTime * 9.0f);
        DrawSphere(Vector3{0.0f, 0.33f + bob, 0.0f}, 0.09f + 0.02f * cp, Color{255, 210, 50, 230});
        DrawSphere(Vector3{0.0f, 0.33f + bob, 0.0f}, 0.04f,              Color{255, 255, 200, 255});

        // Thruster glow di belakang
        DrawCylinder(Vector3{-0.10f, 0.26f + bob, -0.37f}, 0.05f, 0.05f, 0.06f, 6, Color{255, 180, 30, 200});
        DrawCylinder(Vector3{ 0.10f, 0.26f + bob, -0.37f}, 0.05f, 0.05f, 0.06f, 6, Color{255, 180, 30, 200});

        rlPopMatrix();
    }

    // ============================================================
    // MUSUH 3: TANK — HEAVY MECH (Ungu, besar, track berjalan)
    // Dari atas: kotak besar + 4 track di pojok + turret atas
    // ============================================================
    else if (type == EnemyType::TANK) {
        float bob   = 0.04f * sinf(wt * 5.0f);           // lambat & berat
        float tread = fmodf(wt * 3.5f, 1.0f);            // animasi track

        // Landasan track / underbody (rendah ke tanah)
        DrawCube(Vector3{0.0f, 0.14f, 0.0f}, 1.10f, 0.28f, 1.30f, Color{40, 20, 65, 255});

        // 4 track / roda mekanik di pojok
        DrawCylinder(Vector3{-0.52f, 0.14f, -0.58f}, 0.20f, 0.20f, 0.28f, 8, Color{55, 30, 85, 255});
        DrawCylinder(Vector3{ 0.52f, 0.14f, -0.58f}, 0.20f, 0.20f, 0.28f, 8, Color{55, 30, 85, 255});
        DrawCylinder(Vector3{-0.52f, 0.14f,  0.58f}, 0.20f, 0.20f, 0.28f, 8, Color{55, 30, 85, 255});
        DrawCylinder(Vector3{ 0.52f, 0.14f,  0.58f}, 0.20f, 0.20f, 0.28f, 8, Color{55, 30, 85, 255});
        // Track ring aksen ungu terang
        DrawCylinderWires(Vector3{-0.52f, 0.14f, -0.58f}, 0.22f, 0.22f, 0.28f, 8, Color{160, 80, 240, 200});
        DrawCylinderWires(Vector3{ 0.52f, 0.14f, -0.58f}, 0.22f, 0.22f, 0.28f, 8, Color{160, 80, 240, 200});
        DrawCylinderWires(Vector3{-0.52f, 0.14f,  0.58f}, 0.22f, 0.22f, 0.28f, 8, Color{160, 80, 240, 200});
        DrawCylinderWires(Vector3{ 0.52f, 0.14f,  0.58f}, 0.22f, 0.22f, 0.28f, 8, Color{160, 80, 240, 200});

        // Bright tread markers that orbit the wheels so the tracks visibly roll.
        for (int w = 0; w < 4; w++) {
            Vector3 wheelPos = {
                (w % 2 == 0) ? -0.52f : 0.52f,
                0.14f,
                (w < 2) ? -0.58f : 0.58f
            };
            float a = tread * 2.0f * PI + (float)w * 1.3f;
            Vector3 markerPos{wheelPos.x + cosf(a) * 0.18f,
                              wheelPos.y + sinf(a) * 0.18f,
                              wheelPos.z};
            DrawSphere(markerPos, 0.035f, Color{200, 130, 255, 255});
        }

        // Badan utama: armor tebal berbentuk kotak besar
        DrawCube(Vector3{0.0f, 0.54f + bob, 0.0f}, 0.95f, 0.50f, 1.08f, Color{75, 35, 115, 255});
        DrawCubeWires(Vector3{0.0f, 0.54f + bob, 0.0f}, 0.95f, 0.50f, 1.08f, Color{130, 60, 200, 180});

        // Armor samping (shield plating)
        DrawCube(Vector3{-0.56f, 0.50f + bob, 0.0f}, 0.16f, 0.42f, 0.90f, Color{90, 40, 140, 255});
        DrawCube(Vector3{ 0.56f, 0.50f + bob, 0.0f}, 0.16f, 0.42f, 0.90f, Color{90, 40, 140, 255});

        // Turret bulat di atas badan
        DrawCylinder(Vector3{0.0f, 0.84f + bob, 0.0f}, 0.30f, 0.38f, 0.28f, 8, Color{100, 45, 155, 255});
        DrawCylinderWires(Vector3{0.0f, 0.84f + bob, 0.0f}, 0.32f, 0.40f, 0.28f, 8, Color{190, 100, 255, 180});

        // Laras senjata pendek (mengarah ke depan +Z)
        DrawCube(Vector3{0.0f, 0.88f + bob, 0.48f}, 0.16f, 0.16f, 0.38f, Color{55, 25, 85, 255});
        DrawCubeWires(Vector3{0.0f, 0.88f + bob, 0.48f}, 0.16f, 0.16f, 0.38f, Color{160, 80, 240, 200});

        // Core energi ungu berdenyut
        float cp = 0.5f + 0.5f * sinf(globalTime * 4.5f);
        DrawSphere(Vector3{0.0f, 0.98f + bob, 0.0f}, 0.14f + 0.04f * cp, Color{180, 80, 255, 220});
        DrawSphere(Vector3{0.0f, 0.98f + bob, 0.0f}, 0.07f,              Color{240, 200, 255, 255});

        // Antena di atas turret
        DrawCylinder(Vector3{-0.18f, 1.15f + bob, 0.0f}, 0.0f, 0.035f, 0.28f, 5, Color{200, 120, 255, 255});
        DrawCylinder(Vector3{ 0.18f, 1.15f + bob, 0.0f}, 0.0f, 0.035f, 0.28f, 5, Color{200, 120, 255, 255});
    }

    // ============================================================
    // MUSUH 4: BOSS — COLOSSUS (Ungu gelap/magenta, sangat besar)
    // Dari atas: badan heksagonal lebar + 6 armor plate melayang
    // + core besar berdenyut + orbit ring
    // ============================================================
    else if (type == EnemyType::BOSS) {
        float bob      = 0.09f * sinf(wt * 3.5f);
        float orbitAng = fmodf(globalTime * 70.0f, 360.0f);   // orbit armor plates
        float corePulse= 0.5f + 0.5f * sinf(globalTime * 4.0f);

        // -- BASE CHASSIS --
        // Platform ground-level yang lebar (terlihat dari atas sebagai blok besar)
        DrawCube(Vector3{0.0f, 0.18f, 0.0f}, 1.80f, 0.36f, 1.80f, Color{38, 15, 65, 255});
        DrawCubeWires(Vector3{0.0f, 0.18f, 0.0f}, 1.80f, 0.36f, 1.80f, Color{180, 0, 180, 160});

        // 4 kaki hidraulik besar di pojok
        DrawCylinder(Vector3{-0.78f, 0.18f, -0.78f}, 0.18f, 0.22f, 0.36f, 8, Color{55, 20, 90, 255});
        DrawCylinder(Vector3{ 0.78f, 0.18f, -0.78f}, 0.18f, 0.22f, 0.36f, 8, Color{55, 20, 90, 255});
        DrawCylinder(Vector3{-0.78f, 0.18f,  0.78f}, 0.18f, 0.22f, 0.36f, 8, Color{55, 20, 90, 255});
        DrawCylinder(Vector3{ 0.78f, 0.18f,  0.78f}, 0.18f, 0.22f, 0.36f, 8, Color{55, 20, 90, 255});
        // Kaki glow aksen magenta
        DrawCylinderWires(Vector3{-0.78f, 0.18f, -0.78f}, 0.20f, 0.24f, 0.36f, 8, Color{220, 0, 180, 200});
        DrawCylinderWires(Vector3{ 0.78f, 0.18f, -0.78f}, 0.20f, 0.24f, 0.36f, 8, Color{220, 0, 180, 200});
        DrawCylinderWires(Vector3{-0.78f, 0.18f,  0.78f}, 0.20f, 0.24f, 0.36f, 8, Color{220, 0, 180, 200});
        DrawCylinderWires(Vector3{ 0.78f, 0.18f,  0.78f}, 0.20f, 0.24f, 0.36f, 8, Color{220, 0, 180, 200});

        // -- MAIN BODY --
        // Badan utama berbentuk heksagonal tinggi, besar
        DrawCylinder(Vector3{0.0f, 0.72f + bob, 0.0f}, 0.80f, 0.95f, 0.68f, 6, Color{55, 15, 95, 255});
        DrawCylinderWires(Vector3{0.0f, 0.72f + bob, 0.0f}, 0.82f, 0.97f, 0.68f, 6, Color{200, 0, 200, 180});

        // Armor chest plate depan
        DrawCube(Vector3{0.0f, 0.76f + bob, 0.60f}, 0.70f, 0.52f, 0.16f, Color{70, 20, 120, 255});
        DrawCubeWires(Vector3{0.0f, 0.76f + bob, 0.60f}, 0.70f, 0.52f, 0.16f, Color{220, 80, 220, 200});

        // Bahu besar kiri & kanan (terlihat dari atas seperti pelindung samping)
        float sB = sinf(globalTime * 2.8f) * 0.06f;
        DrawCube(Vector3{-1.05f, 0.95f + bob + sB,  0.0f}, 0.40f, 0.55f, 0.65f, Color{80, 20, 130, 255});
        DrawCubeWires(Vector3{-1.05f, 0.95f + bob + sB, 0.0f}, 0.40f, 0.55f, 0.65f, Color{200, 80, 240, 180});
        DrawCube(Vector3{ 1.05f, 0.95f + bob - sB,  0.0f}, 0.40f, 0.55f, 0.65f, Color{80, 20, 130, 255});
        DrawCubeWires(Vector3{ 1.05f, 0.95f + bob - sB, 0.0f}, 0.40f, 0.55f, 0.65f, Color{200, 80, 240, 180});

        // Pelindung bahu ujung
        DrawCube(Vector3{-1.32f, 0.95f + bob + sB, 0.0f}, 0.18f, 0.40f, 0.50f, Color{110, 30, 170, 255});
        DrawCube(Vector3{ 1.32f, 0.95f + bob - sB, 0.0f}, 0.18f, 0.40f, 0.50f, Color{110, 30, 170, 255});

        // -- HEAD / SENSOR DOME --
        DrawCylinder(Vector3{0.0f, 1.48f + bob, 0.0f}, 0.38f, 0.48f, 0.38f, 6, Color{65, 18, 110, 255});
        DrawCylinderWires(Vector3{0.0f, 1.48f + bob, 0.0f}, 0.40f, 0.50f, 0.38f, 6, Color{220, 0, 220, 200});
        // Sensor mata merah magenta (dua titik terang dari atas)
        DrawSphere(Vector3{-0.18f, 1.60f + bob, 0.36f}, 0.08f, Color{255, 50, 220, 255});
        DrawSphere(Vector3{ 0.18f, 1.60f + bob, 0.36f}, 0.08f, Color{255, 50, 220, 255});

        // Tanduk mekanis di atas kepala (antena / horn)
        DrawCylinder(Vector3{-0.22f, 1.88f + bob, 0.0f}, 0.0f, 0.06f, 0.42f, 5, Color{220, 0, 200, 255});
        DrawCylinder(Vector3{ 0.22f, 1.88f + bob, 0.0f}, 0.0f, 0.06f, 0.42f, 5, Color{220, 0, 200, 255});
        // Tanduk tengah lebih besar
        DrawCylinder(Vector3{0.0f, 1.88f + bob, 0.18f}, 0.0f, 0.08f, 0.55f, 5, Color{255, 80, 255, 255});

        // Struktur punggung (dari atas terlihat sebagai tonjolan belakang)
        DrawCube(Vector3{-0.30f, 1.10f + bob, -0.55f}, 0.18f, 0.45f, 0.28f, Color{90, 25, 145, 255});
        DrawCube(Vector3{ 0.30f, 1.10f + bob, -0.55f}, 0.18f, 0.45f, 0.28f, Color{90, 25, 145, 255});
        DrawCylinder(Vector3{0.0f, 1.35f + bob, -0.55f}, 0.0f, 0.14f, 0.35f, 5, Color{200, 0, 200, 220});

        // -- CORE ENERGI BESAR berdenyut --
        float cR = 0.22f + 0.07f * corePulse;
        Color coreCol = {(unsigned char)(180 + (int)(60 * corePulse)), 0, (unsigned char)(200 + (int)(40 * corePulse)), (unsigned char)(200 + (int)(55 * corePulse))};
        DrawSphere(Vector3{0.0f, 0.78f + bob, 0.48f}, cR, coreCol);
        DrawSphere(Vector3{0.0f, 0.78f + bob, 0.48f}, cR * 0.5f, Color{255, 180, 255, 255});
        DrawSphereWires(Vector3{0.0f, 0.78f + bob, 0.48f}, cR * 1.4f, 7, 7,
                        Color{220, 0, 220, (unsigned char)(80 + (int)(80 * corePulse))});

        // -- ORBIT RING (armor plate melayang berputar di sekitar badan) --
        rlPushMatrix();
        rlTranslatef(0.0f, 0.90f + bob, 0.0f);
        rlRotatef(orbitAng, 0.0f, 1.0f, 0.0f);
        // 2 armor plate orbit berlawanan arah
        DrawCube(Vector3{ 1.10f, 0.0f, 0.0f}, 0.30f, 0.22f, 0.55f, Color{120, 30, 180, 240});
        DrawCubeWires(Vector3{ 1.10f, 0.0f, 0.0f}, 0.30f, 0.22f, 0.55f, Color{220, 0, 220, 200});
        DrawCube(Vector3{-1.10f, 0.0f, 0.0f}, 0.30f, 0.22f, 0.55f, Color{120, 30, 180, 240});
        DrawCubeWires(Vector3{-1.10f, 0.0f, 0.0f}, 0.30f, 0.22f, 0.55f, Color{220, 0, 220, 200});
        rlPopMatrix();

        // Orbit ring kedua (dibalik, lebih kecil, lebih cepat)
        rlPushMatrix();
        rlTranslatef(0.0f, 1.05f + bob, 0.0f);
        rlRotatef(-orbitAng * 1.4f, 0.0f, 1.0f, 0.0f);
        DrawCube(Vector3{0.0f, 0.0f,  0.85f}, 0.42f, 0.14f, 0.18f, Color{200, 0, 200, 210});
        DrawCube(Vector3{0.0f, 0.0f, -0.85f}, 0.42f, 0.14f, 0.18f, Color{200, 0, 200, 210});
        rlPopMatrix();
    }

    // Hit flash DISABLED (was distracting)
    // if (hitFlashIntensity > 0.0f) {
    //     rlSetBlendMode(BLEND_ADDITIVE);
    //     float flashSize = 0.5f + 0.3f * hitFlashIntensity;
    //     unsigned char flashAlpha = (unsigned char)(180 * hitFlashIntensity);
    //     DrawSphere(Vector3{0.0f, 0.3f, 0.0f}, flashSize, Color{255, 255, 255, flashAlpha});
    //     rlSetBlendMode(BLEND_ALPHA);
    // }

    rlPopMatrix(); // Close lean/bob transform
    rlPopMatrix(); // Close yaw transform
}

static void DrawRing3D(Vector3 center, float innerR, float outerR, Color color);

void Renderer3D::DrawBullet3D(const Bullet& bullet) {
    if (!bullet.IsActive()) return;

    Vector3 pos = To3D(bullet.GetPosition(), 1.4f);
    Vector2 dir = bullet.GetDirection();
    float yaw = atan2f(dir.x, dir.y) * RAD2DEG;
    Color bulletColor = bullet.GetColor();
    const auto& trail = bullet.GetTrailPositions();

    // Draw 3D trail from actual trail positions
    if (!trail.empty()) {
        for (size_t i = 0; i < trail.size(); i++) {
            Vector3 trailPos3D = To3D(trail[i], 1.4f);
            float alpha = (float)i / (float)trail.size();
            float trailSize = 0.15f * alpha;
            Color trailCol = Fade(bulletColor, alpha * 0.5f);
            DrawSphere(trailPos3D, trailSize, trailCol);
        }
    }

    if (bullet.IsAoE()) {
        // Cannon: Heavy High-Explosive Shell (Dense core with glowing crimson/orange tracer)
        DrawSphere(pos, 0.35f, Color{30, 30, 35, 255});
        DrawSphere(pos, 0.24f, Color{239, 68, 68, 255});
        DrawSphereWires(pos, 0.37f, 8, 8, Color{249, 115, 22, 255});
        
        // Trailing embers (time from the global clock, not per-bullet state,
        // so multiple shells don't accelerate the animation).
        float emberTime = (float)GetTime();
        for (int i = 0; i < 3; i++) {
            float angle = emberTime * 5.0f + i * 2.0f;
            Vector3 emberPos = {pos.x + sinf(angle) * 0.15f, pos.y - 0.1f - i * 0.08f, pos.z + cosf(angle) * 0.15f};
            DrawSphere(emberPos, 0.08f, Fade(Color{249, 115, 22, 255}, 0.7f - i * 0.2f));
        }
    } else if (bulletColor.b > 200) {
        // Rapid: High-Velocity Plasma Bolt (Cyan glow with white core)
        DrawSphere(pos, 0.20f, Color{56, 189, 248, 255});
        DrawSphere(pos, 0.12f, Color{255, 255, 255, 255});
        DrawSphereWires(pos, 0.26f, 6, 6, Color{186, 230, 253, 220});
    } else {
        // Basic: Kinetic Sentry Tracer Round (High-speed slug with yellow/green energy)
        rlPushMatrix();
        rlTranslatef(pos.x, pos.y, pos.z);
        rlRotatef(yaw, 0.0f, 1.0f, 0.0f);
        DrawCylinderEx(Vector3{0.0f, 0.0f, -0.25f}, Vector3{0.0f, 0.0f, 0.35f}, 0.06f, 0.06f, 6, Color{250, 204, 21, 255});
        DrawSphere(Vector3{0.0f, 0.0f, 0.35f}, 0.08f, Color{255, 255, 255, 255});
        rlPopMatrix();
    }
}

// Helper: Draw 3D ring on ground plane
static void DrawRing3D(Vector3 center, float innerR, float outerR, Color color) {
    const int segs = 32;
    for (int i = 0; i < segs; i++) {
        float a1 = (float)i * (2.0f * PI / (float)segs);
        float a2 = (float)(i + 1) * (2.0f * PI / (float)segs);
        Vector3 p1i = {center.x + cosf(a1) * innerR, center.y, center.z + sinf(a1) * innerR};
        Vector3 p2i = {center.x + cosf(a2) * innerR, center.y, center.z + sinf(a2) * innerR};
        Vector3 p1o = {center.x + cosf(a1) * outerR, center.y, center.z + sinf(a1) * outerR};
        Vector3 p2o = {center.x + cosf(a2) * outerR, center.y, center.z + sinf(a2) * outerR};
        DrawTriangle3D(p1i, p2i, p1o, color);
        DrawTriangle3D(p2i, p2o, p1o, color);
    }
}

void Renderer3D::DrawExplosion3D(const Explosion& exp) {
    // progress: 0 at spawn -> 1 at expiry.
    float progress = 1.0f - (exp.timer / exp.maxLifetime);
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    // A degenerate (zero radius) explosion would flicker for one frame right
    // before it is erased; skip it entirely instead.
    float radius3D = exp.radius * 0.045f * progress;
    if (radius3D <= 0.001f) return;

    // Lift the fireball so the bright core sits above the floor instead of
    // being buried inside the terrain (it used to be drawn at y = 0.15).
    Vector3 pos = To3D(exp.position, 0.15f + radius3D * 0.55f);
    float r = radius3D;

    // Brightness: full white-hot at spawn, then fade out as it expands.
    float alpha = 1.0f - progress;

    // Kubah gelombang ledakan api
    DrawSphere(pos, r, Fade(Color{249, 115, 22, 255}, alpha));
    DrawSphereWires(pos, r * 1.1f, 8, 8, Fade(Color{239, 68, 68, 255}, alpha));

    // Inner fireball
    DrawSphere(pos, r * 0.6f, Fade(Color{255, 180, 30, 255}, alpha));

    // Shockwave ring stays glued to the ground plane (y = small offset).
    Vector3 ringPos = To3D(exp.position, 0.06f);
    float ringAlpha = alpha * 0.8f;
    DrawRing3D(ringPos, r * 0.3f, r * 1.3f, Fade(Color{249, 115, 22, 255}, ringAlpha));
    DrawRing3D(ringPos, r * 0.5f, r * 1.1f, Fade(Color{255, 200, 50, 255}, ringAlpha * 0.6f));

    // Rising ember particles
    float emberTime = (float)GetTime();
    for (int i = 0; i < 8; i++) {
        float angle = emberTime * 2.0f + i * (360.0f / 8.0f) * DEG2RAD;
        float h = progress * 1.5f + sinf(emberTime * 3.0f + i) * 0.3f;
        Vector3 emberPos = {pos.x + cosf(angle) * r * 0.7f, h, pos.z + sinf(angle) * r * 0.7f};
        DrawSphere(emberPos, 0.06f, Fade(Color{255, 140, 20, 255}, alpha * 0.8f));
    }
}
