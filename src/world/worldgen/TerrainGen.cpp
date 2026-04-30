#include "world/worldgen/TerrainGen.h"
#include "world/worldgen/NoiseSettings.h"
#include "stb/stb_perlin.h"
#include <cmath>
#include <algorithm>

using namespace WorldGenSettings;

static float Ridged(float x, float y, float z) {
    float n = stb_perlin_noise3(x, y, z, 0, 0, 0);
    return 1.0f - std::abs(n);
}

static float FBM(float x, float y, float z, int octaves) {
    float val = 0, amp = 1, tot = 0;
    for (int i = 0; i < octaves; i++) {
        val += stb_perlin_noise3(x, y, z, 0, 0, 0) * amp;
        tot += amp;
        x *= 2.1f; y *= 2.1f; z *= 2.1f;  // 2.1 instead of 2.0 — breaks tiling
        amp *= 0.5f;
    }
    return val / tot;
}

// Beta profile:
// - flat ocean floor
// - gentle plains just above sea
// - SUDDEN cliff (beta's signature)
// - small plateau on top
// - occasional peak
static float HeightSpline(float t)
{
    t = t * 0.5f + 0.5f;
    t = std::clamp(t, 0.0f, 1.0f);

    if (t < 0.25f) return -0.08f + t * 0.10f;          // ocean — below sea, slightly varied
    if (t < 0.42f) return -0.005f + (t - 0.25f) * 0.09f;   // plains — gently above sea
    if (t < 0.50f) return 0.010f + (t - 0.42f) * 1.80f;    // CLIFF — Beta's sharp wall, 8% range → big jump
    if (t < 0.72f) return 0.154f + (t - 0.50f) * 0.08f;    // plateau top — almost flat
    if (t < 0.85f) return 0.172f + (t - 0.72f) * 0.35f;    // mountain above plateau
    return 0.217f + (t - 0.85f) * 0.60f;                   // rare high peaks
}

float TerrainGen::GetDensity(float wx, float wy, float wz)
{
    float fx = wx * H_SCALE;
    float fz = wz * H_SCALE;

    // Domain warp — breaks uniformity, creates overhangs
    float warpX = FBM(fx * 0.8f, 0.3f, fz * 0.8f, 2) * WARP_STRENGTH;
    float warpZ = FBM(fx * 0.8f + 4.7f, 0.3f, fz * 0.8f + 4.7f, 2) * WARP_STRENGTH;
    float sfx = fx + warpX;
    float sfz = fz + warpZ;

    // Continentalness — slow, large scale (determines region type)
    float continent = FBM(sfx * 0.55f, 0.0f, sfz * 0.55f, 4);
    float contSpline = HeightSpline(continent);

    // Erosion mask — suppresses peaks in some regions → flat plains next to mountains
    // This is the KEY to Beta feel — contrast between flat and rugged
    float erosion = FBM(sfx * 0.40f + 8.3f, 0.0f, sfz * 0.40f + 8.3f, 2) * 0.5f + 0.5f;
    erosion = std::clamp(erosion, 0.0f, 1.0f);

    // Ridge — sharp mountain peaks, only where erosion is LOW
    float ridge = Ridged(sfx * 1.8f, 0.0f, sfz * 1.8f);
    ridge = ridge * ridge;                          // sharpen
    ridge *= (1.0f - erosion) * 0.06f;             // suppress in flat regions

    // Small detail — 3D so it creates overhangs
    float fy = wy * V_SCALE;
    float detail = FBM(sfx * 3.5f, fy * 1.5f, sfz * 3.5f, 2) * 0.025f;

    // Stone exposure noise — adds bumps that expose stone cliffs
    // Varies horizontally only, creates vertical stone faces
    float stoneNoise = FBM(sfx * 5.0f + 2.1f, 0.0f, sfz * 5.0f + 2.1f, 1) * 0.018f;

    float density = contSpline + ridge + detail + stoneNoise;

    // Y bias — density drops above SEA_LEVEL, rises below
    density -= (wy - SEA_LEVEL) * V_BIAS;

    return density;
}

void TerrainGen::Fill(Chunk& chunk, int wxBase, int wzBase)
{
    for (int x = 0; x < CX; x++)
        for (int z = 0; z < CZ; z++)
        {
            int wx = wxBase + x;
            int wz = wzBase + z;

            chunk.blocks[x][0][z] = BlockType::BEDROCK;

            int surfaceY = -1;
            bool prevSolid = false;
            int solidRun = 0;   // tracks consecutive solid blocks from top

            for (int y = CY - 1; y >= 1; y--)
            {
                float d = GetDensity((float)wx, (float)y, (float)wz);

                if (d > 0.0f) {
                    chunk.blocks[x][y][z] = BlockType::STONE;
                    if (surfaceY < 0) surfaceY = y;
                    solidRun++;
                }
                else {
                    chunk.blocks[x][y][z] = BlockType::AIR;
                    solidRun = 0;
                }
            }

            // Surface layers — grass/dirt only on top, stone exposed on cliffs
            if (surfaceY > 0 && surfaceY < CY - 1)
            {
                // Only place grass if there's air directly above (no overhang cap)
                if (chunk.blocks[x][surfaceY + 1 < CY ? surfaceY + 1 : surfaceY][z] == BlockType::AIR)
                {
                    chunk.blocks[x][surfaceY][z] = BlockType::GRASS;
                    for (int d = 1; d <= 3 && surfaceY - d >= 1; d++)
                        if (chunk.blocks[x][surfaceY - d][z] == BlockType::STONE)
                            chunk.blocks[x][surfaceY - d][z] = BlockType::DIRT;
                    // Only 3 dirt layers (was 4) — stone exposed sooner on steep slopes
                }
                // If overhang — stays STONE → visible cliff faces like Beta
            }
        }
}