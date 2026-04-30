#pragma once

#include <cstdint>
#include <memory>
#include <random>
#include <vector>
#include <cmath>

#include <glm/glm.hpp>

// Forward-declare your Chunk type — adjust include path as needed
#include "world/Chunk.h"   // provides Chunk, BlockType, CX, CY, CZ

// ─────────────────────────────────────────────────────────────────────────────
//  Java-faithful LCG (same multiplier / addend as java.util.Random)
// ─────────────────────────────────────────────────────────────────────────────
class JavaRandom
{
public:
    explicit JavaRandom(int64_t seed = 0) { setSeed(seed); }

    void setSeed(int64_t seed)
    {
        m_seed = (seed ^ 0x5DEECE66DLL) & ((1LL << 48) - 1);
    }

    int32_t nextInt()
    {
        m_seed = (m_seed * 0x5DEECE66DLL + 0xBLL) & ((1LL << 48) - 1);
        return (int32_t)(m_seed >> 16);
    }

    int32_t nextInt(int32_t bound)
    {
        if (bound <= 0) return 0;
        if ((bound & -bound) == bound)          // power of two
        {
            m_seed = (m_seed * 0x5DEECE66DLL + 0xBLL) & ((1LL << 48) - 1);
            return (int32_t)((bound * (m_seed >> 17)) >> 31);
        }
        int32_t bits, val;
        do {
            m_seed = (m_seed * 0x5DEECE66DLL + 0xBLL) & ((1LL << 48) - 1);
            bits = (int32_t)(m_seed >> 17);
            val = bits % bound;
        } while (bits - val + (bound - 1) < 0);
        return val;
    }

    int64_t nextLong()
    {
        return ((int64_t)nextInt() << 32) + nextInt();
    }

    float nextFloat()
    {
        m_seed = (m_seed * 0x5DEECE66DLL + 0xBLL) & ((1LL << 48) - 1);
        return (float)((int32_t)(m_seed >> 24)) / (float)(1 << 24);
    }

    double nextDouble()
    {
        m_seed = (m_seed * 0x5DEECE66DLL + 0xBLL) & ((1LL << 48) - 1);
        int64_t hi = m_seed >> 22;
        m_seed = (m_seed * 0x5DEECE66DLL + 0xBLL) & ((1LL << 48) - 1);
        int64_t lo = m_seed >> 27;
        return (double)((hi << 27) + lo) / (double)(1LL << 53);
    }

private:
    int64_t m_seed;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Faithful port of NoiseGeneratorPerlin173  (Ken Perlin improved noise)
// ─────────────────────────────────────────────────────────────────────────────
class PerlinNoise173
{
public:
    double offsetX, offsetY, offsetZ;

    explicit PerlinNoise173(JavaRandom& rng)
    {
        offsetX = rng.nextDouble() * 256.0;
        offsetY = rng.nextDouble() * 256.0;
        offsetZ = rng.nextDouble() * 256.0;

        for (int i = 0; i < 256; ++i) perm[i] = i;
        for (int i = 0; i < 256; ++i)
        {
            int j = rng.nextInt(256 - i) + i;
            int t = perm[i]; perm[i] = perm[j]; perm[j] = t;
            perm[i + 256] = perm[i];
        }
    }

    // Full 3-D sample
    double sample(double x, double y, double z) const
    {
        x += offsetX; y += offsetY; z += offsetZ;
        int xi = ifloor(x), yi = ifloor(y), zi = ifloor(z);
        double xf = x - xi, yf = y - yi, zf = z - zi;
        xi &= 255; yi &= 255; zi &= 255;
        double u = fade(xf), v = fade(yf), w = fade(zf);

        int A  = perm[xi]   + yi, AA = perm[A]   + zi, AB = perm[A+1] + zi;
        int B  = perm[xi+1] + yi, BA = perm[B]   + zi, BB = perm[B+1] + zi;

        return lerp(w,
            lerp(v,
                lerp(u, grad(perm[AA],   xf,   yf,   zf),
                         grad(perm[BA],   xf-1, yf,   zf)),
                lerp(u, grad(perm[AB],   xf,   yf-1, zf),
                         grad(perm[BB],   xf-1, yf-1, zf))),
            lerp(v,
                lerp(u, grad(perm[AA+1], xf,   yf,   zf-1),
                         grad(perm[BA+1], xf-1, yf,   zf-1)),
                lerp(u, grad(perm[AB+1], xf,   yf-1, zf-1),
                         grad(perm[BB+1], xf-1, yf-1, zf-1))));
    }

    // 2-D sample (z=0)
    double sample2D(double x, double z) const { return sample(x, 0.0, z); }

    // Bulk fill matching Java's `a(double[] buf, ...)` signature used by
    // NoiseGeneratorOctaves173 — writes *adds* amplitude-scaled noise into buf.
    void addNoise(double* buf,
                  double startX, double startY, double startZ,
                  int sizeX,   int sizeY,   int sizeZ,
                  double scaleX, double scaleY, double scaleZ,
                  double amplitude) const
    {
        if (sizeY == 1)
        {
            // 2-D fast path (Java's if(var9==1) branch)
            int idx = 0;
            for (int ix = 0; ix < sizeX; ++ix)
            {
                double px = (startX + ix) * scaleX + offsetX;
                int xi = ifloor(px); double xf = px - xi; xi &= 255;
                double u = fade(xf);

                for (int iz = 0; iz < sizeZ; ++iz)
                {
                    double pz = (startZ + iz) * scaleZ + offsetZ;
                    int zi = ifloor(pz); double zf = pz - zi; zi &= 255;
                    double w = fade(zf);

                    int A = perm[xi] + 0, AA = perm[A] + zi;
                    int B = perm[xi+1] + 0, BA = perm[B] + zi;

                    double n = lerp(u,
                        lerp(w, grad2(perm[AA], xf, zf), grad2(perm[AA+1], xf, zf-1)),
                        lerp(w, grad2(perm[BA], xf-1, zf), grad2(perm[BA+1], xf-1, zf-1)));

                    buf[idx++] += n / amplitude;
                }
            }
        }
        else
        {
            // Full 3-D path
            int idx = 0;
            for (int ix = 0; ix < sizeX; ++ix)
            {
                double px = (startX + ix) * scaleX + offsetX;
                int xi = ifloor(px); double xf = px - xi; xi &= 255;
                double u = fade(xf);

                for (int iz = 0; iz < sizeZ; ++iz)
                {
                    double pz = (startZ + iz) * scaleZ + offsetZ;
                    int zi = ifloor(pz); double zf = pz - zi; zi &= 255;
                    double w = fade(zf);

                    int prevYi = -1;
                    double aa=0, ab=0, ba=0, bb=0; // cached Y-layer values

                    for (int iy = 0; iy < sizeY; ++iy)
                    {
                        double py = (startY + iy) * scaleY + offsetY;
                        int yi = ifloor(py); double yf = py - yi; yi &= 255;

                        if (iy == 0 || yi != prevYi)
                        {
                            prevYi = yi;
                            int A  = perm[xi]   + yi, AA2 = perm[A]   + zi, AB2 = perm[A+1] + zi;
                            int B  = perm[xi+1] + yi, BA2 = perm[B]   + zi, BB2 = perm[B+1] + zi;

                            aa = lerp(u, grad(perm[AA2],   xf,   yf,   zf), grad(perm[BA2],   xf-1, yf,   zf));
                            ab = lerp(u, grad(perm[AB2],   xf,   yf-1, zf), grad(perm[BB2],   xf-1, yf-1, zf));
                            ba = lerp(u, grad(perm[AA2+1], xf,   yf,   zf-1), grad(perm[BA2+1], xf-1, yf,   zf-1));
                            bb = lerp(u, grad(perm[AB2+1], xf,   yf-1, zf-1), grad(perm[BB2+1], xf-1, yf-1, zf-1));
                        }

                        double v2 = fade(yf);
                        double n = lerp(w, lerp(v2, aa, ab), lerp(v2, ba, bb));
                        buf[idx++] += n / amplitude;
                    }
                }
            }
        }
    }

private:
    int perm[512];

    static double fade(double t) { return t*t*t*(t*(t*6-15)+10); }
    static double lerp(double t, double a, double b) { return a + t*(b-a); }
    static int    ifloor(double v) { int i=(int)v; return v<i?i-1:i; }

    static double grad(int h, double x, double y, double z)
    {
        h &= 15;
        double u = h<8 ? x : y;
        double v = h<4 ? y : (h==12||h==14 ? x : z);
        return ((h&1)?-u:u) + ((h&2)?-v:v);
    }

    // 2-D gradient (matches Java's 2-arg `a()`)
    static double grad2(int h, double x, double z)
    {
        h &= 15;
        double u = (1-((h&8)>>3))*x;
        double v = h<4 ? 0.0 : (h==12||h==14 ? x : z);
        return ((h&1)?-u:u) + ((h&2)?-v:v);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  OctaveNoise (NoiseGeneratorOctaves173)
// ─────────────────────────────────────────────────────────────────────────────
class OctaveNoise173
{
public:
    OctaveNoise173(JavaRandom& rng, int octaves)
    {
        for (int i = 0; i < octaves; ++i)
            m_octaves.emplace_back(rng);
    }

    // Fills / adds to buf — matches the Java 10-arg generateNoise signature
    double* generateNoise(double* buf,
                          double startX, double startY, double startZ,
                          int sizeX, int sizeY, int sizeZ,
                          double scaleX, double scaleY, double scaleZ) const
    {
        int total = sizeX * sizeY * sizeZ;
        if (!buf) { buf = new double[total]; }
        std::fill(buf, buf + total, 0.0);

        double amp = 1.0;
        for (auto& p : m_octaves)
        {
            p.addNoise(buf,
                       startX, startY, startZ,
                       sizeX, sizeY, sizeZ,
                       scaleX*amp, scaleY*amp, scaleZ*amp, amp);
            amp /= 2.0;
        }
        return buf;
    }

    // 2-arg convenience matching Java's generateNoise(buf,x,z,sizeX,sizeZ,sx,sz,?)
    double* generateNoise2D(double* buf,
                            int startX, int startZ,
                            int sizeX,  int sizeZ,
                            double scaleX, double scaleZ, double /*ignored*/) const
    {
        return generateNoise(buf,
                             (double)startX, 10.0, (double)startZ,
                             sizeX, 1, sizeZ,
                             scaleX, 1.0, scaleZ);
    }

    double sampleForCoord(double x, double z) const
    {
        double v = 0.0; double amp = 1.0;
        for (auto& p : m_octaves)
        {
            v += p.sample2D(x*amp, z*amp) / amp;
            amp /= 2.0;
        }
        return v;
    }

    int octaveCount() const { return (int)m_octaves.size(); }

private:
    std::vector<PerlinNoise173> m_octaves;
};

// ─────────────────────────────────────────────────────────────────────────────
//  WorldGen  — call WorldGen::Generate(chunk, coord) from FillChunkData
// ─────────────────────────────────────────────────────────────────────────────
class WorldGen
{
public:
    // Seed should come from your world.  Default 0 gives a fixed world.
    static void SetSeed(int64_t seed);

    // Main entry — fills chunk block data with Beta 1.73-style terrain
    static void Generate(Chunk& chunk, glm::ivec2 coord);

private:
    // ── noise generators (created once from seed) ────────────────────────
    static std::unique_ptr<OctaveNoise173> s_terrainNoise2;   // 16 oct
    static std::unique_ptr<OctaveNoise173> s_terrainNoise3;   // 16 oct
    static std::unique_ptr<OctaveNoise173> s_terrainNoise1;   //  8 oct
    static std::unique_ptr<OctaveNoise173> s_sandGravelNoise; //  4 oct
    static std::unique_ptr<OctaveNoise173> s_stoneNoise;      //  4 oct
    static std::unique_ptr<OctaveNoise173> s_terrainNoise4;   // 10 oct
    static std::unique_ptr<OctaveNoise173> s_terrainNoise5;   // 16 oct

    static int64_t s_seed;
    static bool    s_initialised;

    // ── internal passes ──────────────────────────────────────────────────
    static void generateTerrainNoise(double* noise,
                                     int fromX, int fromZ,
                                     int xLen, int yLen, int zLen,
                                     double* temp, double* rain);

    static void fillBareTerrain(Chunk& chunk, int cx, int cz,
                                double* terrainNoise, double* temp);

    static void fillBiomeSurface(Chunk& chunk, int cx, int cz,
                                 double* sandNoise, double* gravelNoise,
                                 double* stoneNoise,
                                 JavaRandom& rng);

    static void carveCaves(Chunk& chunk, int cx, int cz, JavaRandom& rng);
    static void caveSegment(Chunk& chunk, int cx, int cz,
                            double ox, double oy, double oz,
                            float size, float yaw, float pitch,
                            int step, int steps, double yScale,
                            JavaRandom& rng);

    // ── biome temperature / rain (NoiseGeneratorOctaves2 equivalent) ─────
    static void sampleTempRain(int startX, int startZ,
                               int sizeX, int sizeZ,
                               double* temp, double* rain,
                               double* noise3);

    static std::unique_ptr<OctaveNoise173> s_tempNoise;   // 4 oct  (e)
    static std::unique_ptr<OctaveNoise173> s_rainNoise;   // 4 oct  (f)
    static std::unique_ptr<OctaveNoise173> s_detailNoise; // 2 oct  (g)

    // temporary buffers — allocated once
    static std::vector<double> s_tnBuf;         // 5*17*5 density grid
    static std::vector<double> s_sandBuf;
    static std::vector<double> s_gravelBuf;
    static std::vector<double> s_stoneBuf;
    static std::vector<double> s_tempBuf;
    static std::vector<double> s_rainBuf;
    static std::vector<double> s_noiseTmp;

    static float jsin(float v);
    static float jcos(float v);
    static int   jfloor(double v);
};
