#include "world/WorldGen.h"

#include <cstring>
#include <cmath>
#include <algorithm>
#include <cassert>

// ─────────────────────────────────────────────────────────────────────────────
//  Static member definitions
// ─────────────────────────────────────────────────────────────────────────────
std::unique_ptr<OctaveNoise173> WorldGen::s_terrainNoise2;
std::unique_ptr<OctaveNoise173> WorldGen::s_terrainNoise3;
std::unique_ptr<OctaveNoise173> WorldGen::s_terrainNoise1;
std::unique_ptr<OctaveNoise173> WorldGen::s_sandGravelNoise;
std::unique_ptr<OctaveNoise173> WorldGen::s_stoneNoise;
std::unique_ptr<OctaveNoise173> WorldGen::s_terrainNoise4;
std::unique_ptr<OctaveNoise173> WorldGen::s_terrainNoise5;
std::unique_ptr<OctaveNoise173> WorldGen::s_tempNoise;
std::unique_ptr<OctaveNoise173> WorldGen::s_rainNoise;
std::unique_ptr<OctaveNoise173> WorldGen::s_detailNoise;

int64_t WorldGen::s_seed      = 0;
bool    WorldGen::s_initialised = false;

std::vector<double> WorldGen::s_tnBuf;
std::vector<double> WorldGen::s_sandBuf;
std::vector<double> WorldGen::s_gravelBuf;
std::vector<double> WorldGen::s_stoneBuf;
std::vector<double> WorldGen::s_tempBuf;
std::vector<double> WorldGen::s_rainBuf;
std::vector<double> WorldGen::s_noiseTmp;

// ─────────────────────────────────────────────────────────────────────────────
//  SetSeed  — call once at startup (or when a new world is created)
// ─────────────────────────────────────────────────────────────────────────────
void WorldGen::SetSeed(int64_t seed)
{
    s_seed = seed;

    // Java constructs noise generators from a seeded Random in order:
    // terrainNoise2(16), terrainNoise3(16), terrainNoise1(8),
    // sandGravel(4), stone(4), terrainNoise4(10), terrainNoise5(16), treeCount(8)
    // Temperature/rain use separate seeds: seed*9871, seed*39811, seed*543321
    JavaRandom rng(seed);
    s_terrainNoise2   = std::make_unique<OctaveNoise173>(rng, 16);
    s_terrainNoise3   = std::make_unique<OctaveNoise173>(rng, 16);
    s_terrainNoise1   = std::make_unique<OctaveNoise173>(rng,  8);
    s_sandGravelNoise = std::make_unique<OctaveNoise173>(rng,  4);
    s_stoneNoise      = std::make_unique<OctaveNoise173>(rng,  4);
    s_terrainNoise4   = std::make_unique<OctaveNoise173>(rng, 10);
    s_terrainNoise5   = std::make_unique<OctaveNoise173>(rng, 16);
    // treeCountNoise(8) — skip, we don't do tree population yet

    JavaRandom tempR (seed * 9871LL);
    JavaRandom rainR (seed * 39811LL);
    JavaRandom detR  (seed * 543321LL);
    s_tempNoise   = std::make_unique<OctaveNoise173>(tempR,  4);
    s_rainNoise   = std::make_unique<OctaveNoise173>(rainR,  4);
    s_detailNoise = std::make_unique<OctaveNoise173>(detR,   2);

    // Pre-size reusable buffers
    s_tnBuf    .assign(5*17*5,   0.0);
    s_sandBuf  .assign(16*16,    0.0);
    s_gravelBuf.assign(16*16,    0.0);
    s_stoneBuf .assign(16*16,    0.0);
    s_tempBuf  .assign(16*16,    0.0);
    s_rainBuf  .assign(16*16,    0.0);
    s_noiseTmp .assign(16*16,    0.0);

    s_initialised = true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Math helpers
// ─────────────────────────────────────────────────────────────────────────────
float WorldGen::jsin(float v) { return std::sin(v); }
float WorldGen::jcos(float v) { return std::cos(v); }
int   WorldGen::jfloor(double v) { int i=(int)v; return v<i?i-1:i; }

// ─────────────────────────────────────────────────────────────────────────────
//  Biome temperature / rain sampling
//  Mirrors WorldChunkManager173::getBiomeNoise()
// ─────────────────────────────────────────────────────────────────────────────
void WorldGen::sampleTempRain(int startX, int startZ,
                               int sizeX,  int sizeZ,
                               double* temp, double* rain,
                               double* noise3)
{
    // temperature noise  (e)
    s_tempNoise->generateNoise2D(temp,  startX, startZ, sizeX, sizeZ,
                                  0.02500000037252903, 0.02500000037252903, 0.25);
    // rain noise  (f)
    s_rainNoise->generateNoise2D(rain,  startX, startZ, sizeX, sizeZ,
                                  0.05000000074505806, 0.05000000074505806, 0.33333333333333333);
    // detail noise (g) — used for small perturbation
    s_detailNoise->generateNoise2D(noise3, startX, startZ, sizeX, sizeZ,
                                   0.25, 0.25, 0.5882352941176471);

    for (int i = 0; i < sizeX * sizeZ; ++i)
    {
        double d0 = noise3[i] * 1.1 + 0.5;
        constexpr double d1 = 0.01, d2 = 1.0 - d1;

        double t = (temp[i] * 0.15 + 0.7) * d2 + d0 * d1;
        constexpr double r1 = 0.002, r2 = 1.0 - r1;
        double r = (rain[i] * 0.15 + 0.5) * r2 + d0 * r1;

        t = 1.0 - (1.0-t)*(1.0-t);
        t = std::clamp(t, 0.0, 1.0);
        r = std::clamp(r, 0.0, 1.0);

        temp[i] = t;
        rain[i] = r;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  generateTerrainNoise
//  Mirrors ChunkProviderGenerate173::generateTerrainNoise()
//  Produces a 5×17×5 density array (xLen=5, yLen=17, zLen=5)
// ─────────────────────────────────────────────────────────────────────────────
void WorldGen::generateTerrainNoise(double* noise,
                                     int fromX, int fromZ,
                                     int xLen, int yLen, int zLen,
                                     double* temp, double* rain)
{
    // Working buffers (stack-allocate small ones)
    static thread_local std::vector<double> tn4Buf, tn5Buf, tn1Buf, tn2Buf, tn3Buf;
    int sz2D = xLen * zLen;
    int sz3D = xLen * yLen * zLen;

    tn4Buf.assign(sz2D, 0.0); tn5Buf.assign(sz2D, 0.0);
    tn1Buf.assign(sz3D, 0.0); tn2Buf.assign(sz3D, 0.0); tn3Buf.assign(sz3D, 0.0);

    constexpr double D0 = 684.412;

    // 2-D biome-scale noises
    s_terrainNoise4->generateNoise(tn4Buf.data(),
                                   fromX, 0, fromZ,
                                   xLen, 1, zLen,
                                   1.121, 1.0, 1.121);

    s_terrainNoise5->generateNoise(tn5Buf.data(),
                                   fromX, 0, fromZ,
                                   xLen, 1, zLen,
                                   200.0, 1.0, 200.0);

    // 3-D terrain noises
    s_terrainNoise1->generateNoise(tn1Buf.data(),
                                   fromX, 0, fromZ,
                                   xLen, yLen, zLen,
                                   D0/80.0, D0/160.0, D0/80.0);

    s_terrainNoise2->generateNoise(tn2Buf.data(),
                                   fromX, 0, fromZ,
                                   xLen, yLen, zLen,
                                   D0, D0, D0);

    s_terrainNoise3->generateNoise(tn3Buf.data(),
                                   fromX, 0, fromZ,
                                   xLen, yLen, zLen,
                                   D0, D0, D0);

    // The biome samples cover (xLen * 4) × (zLen * 4) blocks
    // but for the noise grid we only need one sample per cell.
    int i2 = 16 / xLen;  // = 16/4 = 4 blocks per cell
    int l1 = 0;  // 2-D index
    int k1 = 0;  // 3-D index

    for (int j2 = 0; j2 < xLen; ++j2)
    {
        int k2 = j2 * i2 + i2 / 2;

        for (int l2 = 0; l2 < zLen; ++l2)
        {
            int i3 = l2 * i2 + i2 / 2;

            double tmpT = temp[k2 * 16 + i3];
            double tmpR = rain[k2 * 16 + i3] * tmpT;
            double d4 = 1.0 - tmpR;
            d4 = d4 * d4 * d4 * d4;
            d4 = 1.0 - d4;

            double d5 = (tn4Buf[l1] + 256.0) / 512.0;
            d5 *= d4;
            if (d5 > 1.0) d5 = 1.0;

            double d6 = tn5Buf[l1] / 8000.0;
            if (d6 < 0.0) d6 = -d6 * 0.3;
            d6 = d6 * 3.0 - 2.0;
            if (d6 < 0.0)
            {
                d6 /= 2.0;
                if (d6 < -1.0) d6 = -1.0;
                d6 /= 1.4;
                d6 /= 2.0;
                d5 = 0.0;
            }
            else
            {
                if (d6 > 1.0) d6 = 1.0;
                d6 /= 8.0;
            }
            if (d5 < 0.0) d5 = 0.0;
            d5 += 0.5;
            d6 = d6 * (double)yLen / 16.0;
            double d7 = (double)yLen / 2.0 + d6 * 4.0;

            ++l1;

            for (int j3 = 0; j3 < yLen; ++j3)
            {
                double d9 = ((double)j3 - d7) * 12.0 / d5;
                if (d9 < 0.0) d9 *= 4.0;

                double d10 = tn2Buf[k1] / 512.0;
                double d11 = tn3Buf[k1] / 512.0;
                double d12 = (tn1Buf[k1] / 10.0 + 1.0) / 2.0;

                double d8;
                if      (d12 < 0.0) d8 = d10;
                else if (d12 > 1.0) d8 = d11;
                else                d8 = d10 + (d11 - d10) * d12;

                d8 -= d9;

                // Force top 3 slabs toward negative (seals the sky)
                if (j3 > yLen - 4)
                {
                    double t = (double)(j3 - (yLen - 4)) / 3.0;
                    d8 = d8 * (1.0 - t) + (-10.0) * t;
                }

                noise[k1++] = d8;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  fillBareTerrain
//  Trilinearly interpolates the 5×17×5 density grid down to 16×128×16 voxels.
//  Mirrors ChunkProviderGenerate173::generateBareTerrain()
//  Sea level = 64.  Water fills air below sea level.
// ─────────────────────────────────────────────────────────────────────────────
void WorldGen::fillBareTerrain(Chunk& chunk, int cx, int cz,
                                double* terrainNoise, double* temp)
{
    constexpr int SEA_LEVEL = 64;
    constexpr int b0 = 4;          // cells per chunk (4×4 horizontal)
    constexpr int b2 = 17;         // y cells
    constexpr int l  = b0 + 1;     // = 5

    for (int i1 = 0; i1 < b0; ++i1)
    {
        for (int j1 = 0; j1 < b0; ++j1)
        {
            for (int k1 = 0; k1 < 16; ++k1)   // 16 Y-slabs of 8 blocks each
            {
                constexpr double d0 = 0.125;   // 1/8

                double d1 = terrainNoise[((i1+0)*l + j1+0)*b2 + k1+0];
                double d2 = terrainNoise[((i1+0)*l + j1+1)*b2 + k1+0];
                double d3 = terrainNoise[((i1+1)*l + j1+0)*b2 + k1+0];
                double d4 = terrainNoise[((i1+1)*l + j1+1)*b2 + k1+0];

                double d5 = (terrainNoise[((i1+0)*l + j1+0)*b2 + k1+1] - d1) * d0;
                double d6 = (terrainNoise[((i1+0)*l + j1+1)*b2 + k1+1] - d2) * d0;
                double d7 = (terrainNoise[((i1+1)*l + j1+0)*b2 + k1+1] - d3) * d0;
                double d8 = (terrainNoise[((i1+1)*l + j1+1)*b2 + k1+1] - d4) * d0;

                for (int l1 = 0; l1 < 8; ++l1)
                {
                    constexpr double d9 = 0.25;
                    double d10 = d1, d11 = d2;
                    double d12 = (d3-d1)*d9, d13 = (d4-d2)*d9;

                    for (int i2 = 0; i2 < 4; ++i2)
                    {
                        constexpr double d14 = 0.25;
                        double d15 = d10;
                        double d16 = (d11-d10)*d14;

                        for (int k2 = 0; k2 < 4; ++k2)
                        {
                            int worldY = k1*8 + l1;
                            int bx = i1*4 + i2;
                            int bz = j1*4 + k2;

                            // Temperature from biome noise
                            double temperature = temp[(i1*4+i2)*16 + j1*4+k2];

                            BlockType bt = BlockType::AIR;

                            // Below sea level → water (or ice)
                            if (worldY < SEA_LEVEL)
                            {
                                if (temperature < 0.5 && worldY >= SEA_LEVEL - 1)
                                    bt = BlockType::ICE;  // add ICE to your BlockType if missing
                                else
                                    bt = BlockType::WATER;
                            }

                            // Solid if density > 0
                            if (d15 > 0.0)
                                bt = BlockType::STONE;

                            if (worldY >= 0 && worldY < CY)
                                chunk.blocks[bx][worldY][bz] = bt;

                            d15 += d16;
                        }
                        d10 += d12;
                        d11 += d13;
                    }
                    d1 += d5; d2 += d6; d3 += d7; d4 += d8;
                }
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  fillBiomeSurface
//  Converts raw stone near the surface into dirt/grass/sand/gravel,
//  and places bedrock at the bottom.
//  Mirrors ChunkProviderGenerate173::generateBiomeTerrain()
// ─────────────────────────────────────────────────────────────────────────────
void WorldGen::fillBiomeSurface(Chunk& chunk, int cx, int cz,
                                 double* sandNoise, double* gravelNoise,
                                 double* stoneNoise,
                                 JavaRandom& rng)
{
    constexpr int SEA_LEVEL = 64;

    for (int k = 0; k < 16; ++k)
    {
        for (int l = 0; l < 16; ++l)
        {
            bool hasSand   = sandNoise  [k + l*16] + rng.nextDouble()*0.2 > 0.0;
            bool hasGravel = gravelNoise[k + l*16] + rng.nextDouble()*0.2 > 3.0;
            int  stoneDep  = (int)(stoneNoise[k + l*16] / 3.0 + 3.0 + rng.nextDouble()*0.25);

            int   j1 = -1;   // depth counter (-1 = not yet in solid)
            BlockType top = BlockType::GRASS;
            BlockType fill = BlockType::DIRT;

            for (int k1 = CY-1; k1 >= 0; --k1)
            {
                // Randomised bedrock band
                if (k1 <= rng.nextInt(5))
                {
                    chunk.blocks[l][k1][k] = BlockType::BEDROCK;
                    continue;
                }

                BlockType cur = chunk.blocks[l][k1][k];

                if (cur == BlockType::AIR || cur == BlockType::WATER)
                {
                    j1 = -1;
                }
                else if (cur == BlockType::STONE)
                {
                    if (j1 == -1)
                    {
                        // Hit surface from above
                        if (stoneDep <= 0)
                        {
                            top  = BlockType::AIR;
                            fill = BlockType::STONE;
                        }
                        else if (k1 >= SEA_LEVEL - 4 && k1 <= SEA_LEVEL + 1)
                        {
                            top  = BlockType::GRASS;
                            fill = BlockType::DIRT;
                            if (hasGravel) { top = BlockType::AIR;   }
                            if (hasGravel) { fill = BlockType::GRAVEL; }
                            if (hasSand)   { top = BlockType::SAND;  }
                            if (hasSand)   { fill = BlockType::SAND; }
                        }

                        // Under water, replace air top with water
                        if (k1 < SEA_LEVEL && top == BlockType::AIR)
                            top = BlockType::WATER;

                        j1 = stoneDep;
                        chunk.blocks[l][k1][k] = (k1 >= SEA_LEVEL - 1) ? top : fill;
                    }
                    else if (j1 > 0)
                    {
                        --j1;
                        chunk.blocks[l][k1][k] = fill;
                        // Sand below sand → sandstone
                        if (j1 == 0 && fill == BlockType::SAND)
                        {
                            j1 = rng.nextInt(4);
                            fill = BlockType::SANDSTONE;
                        }
                    }
                }
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Cave carver
//  Mirrors MapGenBase173 / MapGenCaves173
// ─────────────────────────────────────────────────────────────────────────────

// One worm-tunnel segment (recursive, can branch once)
void WorldGen::caveSegment(Chunk& chunk, int cx, int cz,
                            double ox, double oy, double oz,
                            float size, float yaw, float pitch,
                            int step, int steps, double yScale,
                            JavaRandom& rng)
{
    double centerX = cx * 16 + 8.0;
    double centerZ = cz * 16 + 8.0;

    float yawDelta   = 0.0f;
    float pitchDelta = 0.0f;

    if (steps <= 0)
    {
        int r = 16 * 16 - 16;
        steps = r - rng.nextInt(r / 4);
    }

    bool branch = false;
    if (step == -1)
    {
        step  = steps / 2;
        branch = true;
    }

    int branchAt = rng.nextInt(steps / 2) + steps / 4;
    bool flatWorm = rng.nextInt(6) == 0;

    for (; step < steps; ++step)
    {
        float fc = jcos(pitch);
        float fs = jsin(pitch);

        double radius  = 1.5 + (double)(jsin((float)step * 3.14159265f / (float)steps) * size * 1.0f);
        double radiusY = radius * yScale;

        ox += (double)(jcos(yaw) * fc);
        oy += (double)fs;
        oz += (double)(jsin(yaw) * fc);

        pitch *= flatWorm ? 0.92f : 0.7f;
        pitch += pitchDelta * 0.1f;
        yaw   += yawDelta   * 0.1f;

        pitchDelta *= 0.9f;
        yawDelta   *= 0.75f;
        pitchDelta += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 2.0f;
        yawDelta   += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 4.0f;

        // Spawn branch at branchAt
        if (!branch && step == branchAt && size > 1.0f)
        {
            caveSegment(chunk, cx, cz, ox, oy, oz,
                        rng.nextFloat() * 0.5f + 0.5f,
                        yaw - 1.5707964f, pitch / 3.0f,
                        step, steps, 1.0, rng);
            caveSegment(chunk, cx, cz, ox, oy, oz,
                        rng.nextFloat() * 0.5f + 0.5f,
                        yaw + 1.5707964f, pitch / 3.0f,
                        step, steps, 1.0, rng);
            return;
        }

        if (branch && rng.nextInt(4) != 0) continue;

        // Culling — skip if segment is too far from chunk
        double dx = ox - centerX, dz = oz - centerZ;
        double stepsLeft = (double)(steps - step);
        double cullR = (double)(size + 2.0f + 16.0f);
        if (dx*dx + dz*dz - stepsLeft*stepsLeft > cullR*cullR)
            return;

        if (ox < centerX - 16.0 - radius*2.0 || oz < centerZ - 16.0 - radius*2.0 ||
            ox > centerX + 16.0 + radius*2.0 || oz > centerZ + 16.0 + radius*2.0)
            continue;

        // Bounding box clamped to chunk
        int x0 = std::max(0, jfloor(ox - radius) - cx*16 - 1);
        int x1 = std::min(16, jfloor(ox + radius) - cx*16 + 1);
        int y0 = std::max(1, jfloor(oy - radiusY) - 1);
        int y1 = std::min(CY-8, jfloor(oy + radiusY) + 1);
        int z0 = std::max(0, jfloor(oz - radius) - cz*16 - 1);
        int z1 = std::min(16, jfloor(oz + radius) - cz*16 + 1);

        // Water-adjacency check
        bool waterNear = false;
        for (int bx = x0; !waterNear && bx < x1; ++bx)
            for (int bz = z0; !waterNear && bz < z1; ++bz)
                for (int by = y1+1; !waterNear && by >= y0-1; --by)
                    if (by >= 0 && by < CY)
                        if (chunk.blocks[bx][by][bz] == BlockType::WATER)
                            waterNear = true;

        if (waterNear) continue;

        // Carve
        for (int bx = x0; bx < x1; ++bx)
        {
            double nx = ((double)(bx + cx*16) + 0.5 - ox) / radius;

            for (int bz = z0; bz < z1; ++bz)
            {
                double nz = ((double)(bz + cz*16) + 0.5 - oz) / radius;

                if (nx*nx + nz*nz >= 1.0) continue;

                bool grewGrass = false;

                for (int by = y1-1; by >= y0; --by)
                {
                    double ny = ((double)by + 0.5 - oy) / radiusY;

                    if (ny > -0.7 && nx*nx + ny*ny + nz*nz < 1.0)
                    {
                        BlockType bt = chunk.blocks[bx][by][bz];

                        if (bt == BlockType::GRASS)
                            grewGrass = true;

                        if (bt == BlockType::STONE || bt == BlockType::DIRT || bt == BlockType::GRASS)
                        {
                            if (by < 10)
                                chunk.blocks[bx][by][bz] = BlockType::LAVA;
                            else
                            {
                                chunk.blocks[bx][by][bz] = BlockType::AIR;
                                // Expose grass on ceiling
                                if (grewGrass && by > 0 && chunk.blocks[bx][by-1][bz] == BlockType::DIRT)
                                    chunk.blocks[bx][by-1][bz] = BlockType::GRASS;
                            }
                        }
                    }
                }
            }
        }
        if (branch) return;
    }
}

void WorldGen::carveCaves(Chunk& chunk, int cx, int cz, JavaRandom& rng)
{
    // How many cave systems originate in this chunk
    int count = rng.nextInt(rng.nextInt(rng.nextInt(40)+1)+1);
    if (rng.nextInt(15) != 0) count = 0;

    for (int n = 0; n < count; ++n)
    {
        double ox = (double)(cx*16 + rng.nextInt(16));
        double oy = (double)(rng.nextInt(rng.nextInt(CY-8)+8));
        double oz = (double)(cz*16 + rng.nextInt(16));

        int tunnels = 1;
        if (rng.nextInt(4) == 0)
        {
            // Room
            caveSegment(chunk, cx, cz, ox, oy, oz,
                        1.0f + rng.nextFloat() * 6.0f, 0.0f, 0.0f,
                        -1, -1, 0.5, rng);
            tunnels += rng.nextInt(4);
        }

        for (int t = 0; t < tunnels; ++t)
        {
            float yaw   = rng.nextFloat() * 3.14159265f * 2.0f;
            float pitch = (rng.nextFloat() - 0.5f) * 2.0f / 8.0f;
            float size  = rng.nextFloat() * 2.0f + rng.nextFloat();

            caveSegment(chunk, cx, cz, ox, oy, oz,
                        size, yaw, pitch, 0, 0, 1.0, rng);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Generate  — main entry point
// ─────────────────────────────────────────────────────────────────────────────
void WorldGen::Generate(Chunk& chunk, glm::ivec2 coord)
{
    if (!s_initialised)
        SetSeed(12345LL);   // default seed if none set

    const int cx = coord.x;
    const int cz = coord.y;

    // --- chunk-local RNG (matches Java per-chunk seed) --------------------
    JavaRandom rng(s_seed);
    int64_t a = rng.nextLong() / 2 * 2 + 1;
    int64_t b = rng.nextLong() / 2 * 2 + 1;
    rng.setSeed((int64_t)cx * a + (int64_t)cz * b ^ s_seed);

    // --- sample biome temperature / rain ---------------------------------
    double* temp   = s_tempBuf.data();
    double* rain   = s_rainBuf.data();
    double* noise3 = s_noiseTmp.data();

    sampleTempRain(cx*16, cz*16, 16, 16, temp, rain, noise3);

    // --- build 3-D density noise -----------------------------------------
    constexpr int XCELLS = 5, YCELLS = 17, ZCELLS = 5;
    double* tnoise = s_tnBuf.data();
    std::fill(tnoise, tnoise + XCELLS*YCELLS*ZCELLS, 0.0);

    generateTerrainNoise(tnoise, cx*4, cz*4, XCELLS, YCELLS, ZCELLS, temp, rain);

    // --- bare stone / water terrain  -------------------------------------
    fillBareTerrain(chunk, cx, cz, tnoise, temp);

    // --- biome surface (dirt / grass / sand etc.) ------------------------
    constexpr double D = 0.03125;
    s_sandGravelNoise->generateNoise(s_sandBuf.data(),
                                     (double)(cx*16), (double)(cz*16), 0.0,
                                     16, 16, 1, D, D, 1.0);
    s_sandGravelNoise->generateNoise(s_gravelBuf.data(),
                                     (double)(cx*16), 109.0134, (double)(cz*16),
                                     16, 1, 16, D, 1.0, D);
    s_stoneNoise->generateNoise(s_stoneBuf.data(),
                                (double)(cx*16), (double)(cz*16), 0.0,
                                16, 16, 1, D*2.0, D*2.0, D*2.0);

    fillBiomeSurface(chunk, cx, cz,
                     s_sandBuf.data(), s_gravelBuf.data(), s_stoneBuf.data(),
                     rng);

    // --- cave carving ----------------------------------------------------
    // Carve caves from a neighbourhood of chunks so tunnels cross borders.
    // In the original, MapGenBase iterates over a 17×17 chunk area.
    // Here we use the standard Beta radius of 8.
    constexpr int CAVE_RADIUS = 8;
    for (int dz = -CAVE_RADIUS; dz <= CAVE_RADIUS; ++dz)
    {
        for (int dx = -CAVE_RADIUS; dx <= CAVE_RADIUS; ++dx)
        {
            int ncx = cx + dx, ncz = cz + dz;
            // Seed the per-originating-chunk RNG
            JavaRandom caveRng(s_seed);
            int64_t ca = caveRng.nextLong() / 2 * 2 + 1;
            int64_t cb = caveRng.nextLong() / 2 * 2 + 1;
            caveRng.setSeed((int64_t)ncx * ca + (int64_t)ncz * cb ^ s_seed);

            carveCaves(chunk, cx, cz, caveRng);
        }
    }
}
