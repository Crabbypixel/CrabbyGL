#include "world/worldgen/CaveGen.h"
#include "world/worldgen/NoiseSettings.h"
#include "stb/stb_perlin.h"
#include <cmath>
#include <algorithm>

using namespace WorldGenSettings;

// Carve a sphere of air at world pos (wx,wy,wz) with given radius
static void CarveSphere(Chunk& chunk, int wxBase, int wzBase,
    float cx, float cy, float cz, float radius)
{
    int r = (int)std::ceil(radius);
    int ix = (int)cx, iy = (int)cy, iz = (int)cz;

    for (int dx = -r; dx <= r; dx++)
        for (int dy = -r; dy <= r; dy++)
            for (int dz = -r; dz <= r; dz++)
            {
                // Ellipse — caves taller than wide feels more natural
                float dist = (dx * dx + dy * dy * 1.5f + dz * dz);
                if (dist > radius * radius) continue;

                int bx = ix + dx - wxBase;
                int by = iy + dy;
                int bz = iz + dz - wzBase;

                if (bx < 0 || bx >= CX) continue;
                if (bz < 0 || bz >= CZ) continue;
                if (by <= 1 || by >= CY - 1) continue;

                if (chunk.blocks[bx][by][bz] == BlockType::BEDROCK) continue;

                chunk.blocks[bx][by][bz] = BlockType::AIR;
            }
}

void CaveGen::CarveWorm(Chunk& chunk, int wxBase, int wzBase,
    float ox, float oy, float oz,
    float dirX, float dirY, float dirZ,
    int steps, float radius)
{
    float x = ox, y = oy, z = oz;

    for (int i = 0; i < steps; i++)
    {
        // Sample noise to steer worm — different offsets per axis
        float t = i * 0.18f;
        float nx = stb_perlin_noise3(x * 0.04f + 0.0f, t, z * 0.04f + 0.0f, 0, 0, 0);
        float ny = stb_perlin_noise3(x * 0.04f + 3.7f, t, z * 0.04f + 3.7f, 0, 0, 0) * 0.4f; // less vertical wander
        float nz = stb_perlin_noise3(x * 0.04f + 8.3f, t, z * 0.04f + 8.3f, 0, 0, 0);

        // Blend noise into direction — worm curves gradually
        dirX = dirX * 0.8f + nx * 0.2f;
        dirY = dirY * 0.8f + ny * 0.2f;
        dirZ = dirZ * 0.8f + nz * 0.2f;

        // Normalize direction
        float len = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
        if (len > 0.001f) { dirX /= len; dirY /= len; dirZ /= len; }

        // Step forward
        x += dirX * 1.5f;
        y += dirY * 1.5f;
        z += dirZ * 1.5f;

        // Clamp Y — no surface caves, no basement caves
        if (y < CAVE_MIN_Y + 2) { y = CAVE_MIN_Y + 2; dirY = std::abs(dirY); }
        if (y > CAVE_MAX_Y - 2) { y = CAVE_MAX_Y - 2; dirY = -std::abs(dirY); }

        // Vary radius slightly — caves pulse width
        float r = radius + stb_perlin_noise3(x * 0.05f, i * 0.1f, z * 0.05f, 0, 0, 0) * 1.2f;
        r = std::clamp(r, 1.0f, radius + 1.5f);

        // Only carve if worm is near this chunk (avoid wasted work)
        float chunkCX = wxBase + CX * 0.5f;
        float chunkCZ = wzBase + CZ * 0.5f;
        float chunkDist = (x - chunkCX) * (x - chunkCX) + (z - chunkCZ) * (z - chunkCZ);
        if (chunkDist < (CX + r + 4) * (CX + r + 4))
            CarveSphere(chunk, wxBase, wzBase, x, y, z, r);
    }
}

void CaveGen::Carve(Chunk& chunk, int wxBase, int wzBase)
{
    // ── Worm caves — primary Beta-like tunnels ─────────────────
    // Seed worms from a grid of origins so caves connect across chunks
    // Each cell spawns 0-2 worms depending on noise

    const int GRID = 48;   // worm origin every 48 blocks
    const int RANGE = 2;   // check ±2 grid cells around chunk

    int gridX0 = (int)std::floor((float)wxBase / GRID) - RANGE;
    int gridZ0 = (int)std::floor((float)wzBase / GRID) - RANGE;

    for (int gx = gridX0; gx <= gridX0 + RANGE * 2 + 1; gx++)
        for (int gz = gridZ0; gz <= gridZ0 + RANGE * 2 + 1; gz++)
        {
            // Deterministic seed per grid cell — same world = same caves
            float cellSeed = stb_perlin_noise3(gx * 0.31f, 7.3f, gz * 0.31f, 0, 0, 0);

            // ~70% of cells spawn a worm
            if (cellSeed < -0.40f) continue;

            // Worm origin — center of grid cell + noise offset
            float ox = gx * GRID + stb_perlin_noise3(gx * 0.7f, 0.0f, gz * 0.7f, 0, 0, 0) * GRID * 0.4f;
            float oz = gz * GRID + stb_perlin_noise3(gx * 0.7f, 1.0f, gz * 0.7f, 0, 0, 0) * GRID * 0.4f;
            float oy = CAVE_MIN_Y + (stb_perlin_noise3(gx * 0.5f, 2.0f, gz * 0.5f, 0, 0, 0) * 0.5f + 0.5f)
                * (CAVE_MAX_Y - CAVE_MIN_Y);

            // Initial direction — random per cell
            float dx = stb_perlin_noise3(gx * 1.3f, 3.0f, gz * 1.3f, 0, 0, 0);
            float dy = stb_perlin_noise3(gx * 1.3f, 4.0f, gz * 1.3f, 0, 0, 0) * 0.3f;
            float dz = stb_perlin_noise3(gx * 1.3f, 5.0f, gz * 1.3f, 0, 0, 0);

            float baseRadius = 2.5f + (stb_perlin_noise3(gx * 0.9f, 6.0f, gz * 0.9f, 0, 0, 0) * 0.5f + 0.5f) * 2.0f;

            CarveWorm(chunk, wxBase, wzBase, ox, oy, oz, dx, dy, dz, 120, baseRadius);

            // ~30% of cells spawn a second branching worm
            if (cellSeed > 0.30f)
            {
                float dx2 = stb_perlin_noise3(gx * 1.7f, 7.0f, gz * 1.7f, 0, 0, 0);
                float dz2 = stb_perlin_noise3(gx * 1.7f, 8.0f, gz * 1.7f, 0, 0, 0);
                CarveWorm(chunk, wxBase, wzBase, ox, oy, oz, dx2, 0.0f, dz2, 80, baseRadius * 0.7f);
            }
        }

    // ── Large caverns — rare open rooms ────────────────────────
    float cavernNoise = stb_perlin_noise3(wxBase * 0.008f, 9.0f, wzBase * 0.008f, 0, 0, 0);
    if (cavernNoise > 0.55f)
    {
        float cx = wxBase + CX * 0.5f;
        float cz = wzBase + CZ * 0.5f;
        float cy = CAVE_MIN_Y + 8 + (stb_perlin_noise3(wxBase * 0.02f, 10.0f, wzBase * 0.02f, 0, 0, 0) * 0.5f + 0.5f) * 20.0f;
        float r = 6.0f + cavernNoise * 8.0f;
        CarveSphere(chunk, wxBase, wzBase, cx, cy, cz, r);
    }
}