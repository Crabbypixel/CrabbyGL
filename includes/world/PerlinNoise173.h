#pragma once
#include <cmath>
#include <vector>
#include <random>

class PerlinNoise173 {
public:
    int p[512];
    double offsetX, offsetY, offsetZ;

    PerlinNoise173(std::mt19937_64 rng) {
        // Random offsets per instance — this is what makes each
        // noise object unique, matching Notch's original design
        auto randDouble = [&]() {
            return std::uniform_real_distribution<double>(0.0, 256.0)(rng);
            };
        offsetX = randDouble();
        offsetY = randDouble();
        offsetZ = randDouble();

        for (int i = 0; i < 256; p[i] = i++) {}

        for (int i = 0; i < 256; ++i) {
            int j = std::uniform_int_distribution<int>(i, 255)(rng);
            std::swap(p[i], p[j]);
            p[i + 256] = p[i];
        }
    }

    // Lerp
    double lerp(double t, double a, double b) const {
        return a + t * (b - a);
    }

    // 2D gradient
    double grad2(int hash, double x, double z) const {
        int h = hash & 15;
        double u = (1 - ((h & 8) >> 3)) * x;
        double v = h < 4 ? 0.0 : (h != 12 && h != 14 ? z : x);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    // 3D gradient
    double grad3(int hash, double x, double y, double z) const {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h != 12 && h != 14 ? z : x);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    // Fade curve — Ken Perlin's 6t^5 - 15t^4 + 10t^3
    double fade(double t) const {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

    // Single 3D sample
    double sample(double x, double y, double z) const {
        x += offsetX; y += offsetY; z += offsetZ;

        int ix = (int)std::floor(x) & 255;
        int iy = (int)std::floor(y) & 255;
        int iz = (int)std::floor(z) & 255;

        double fx = x - std::floor(x);
        double fy = y - std::floor(y);
        double fz = z - std::floor(z);

        double u = fade(fx), v = fade(fy), w = fade(fz);

        int A = p[ix] + iy, AA = p[A] + iz, AB = p[A + 1] + iz;
        int B = p[ix + 1] + iy, BA = p[B] + iz, BB = p[B + 1] + iz;

        return lerp(w,
            lerp(v,
                lerp(u, grad3(p[AA], fx, fy, fz),
                    grad3(p[BA], fx - 1.0, fy, fz)),
                lerp(u, grad3(p[AB], fx, fy - 1.0, fz),
                    grad3(p[BB], fx - 1.0, fy - 1.0, fz))),
            lerp(v,
                lerp(u, grad3(p[AA + 1], fx, fy, fz - 1.0),
                    grad3(p[BA + 1], fx - 1.0, fy, fz - 1.0)),
                lerp(u, grad3(p[AB + 1], fx, fy - 1.0, fz - 1.0),
                    grad3(p[BB + 1], fx - 1.0, fy - 1.0, fz - 1.0))));
    }

    // 2D sample
    double sample2D(double x, double z) const {
        return sample(x, 0.0, z);
    }

    // ── Batch fill — THE important one ──────────────────────────────
    // Fills output[] with noise for a sizeX * sizeY * sizeZ grid
    // startX/Y/Z: world offset
    // scaleX/Y/Z: frequency per axis
    // amplitude:  divisor (output += sample / amplitude)
    //
    // This is how Beta terrain worked — fill 3D noise buffers
    // at LOW resolution (5x33x5), then trilinearly interpolate
    // up to full chunk resolution (16x256x16). Fast + smooth.
    void fill(double* out,
        double startX, double startY, double startZ,
        int sizeX, int sizeY, int sizeZ,
        double scaleX, double scaleY, double scaleZ,
        double amplitude) const
    {
        double invAmp = 1.0 / amplitude;

        if (sizeY == 1) {
            // 2D path — no Y iteration
            int idx = 0;
            for (int ix = 0; ix < sizeX; ++ix) {
                double x = (startX + ix) * scaleX + offsetX;
                int X = (int)std::floor(x); x -= X; X &= 255;
                double u = fade(x);

                for (int iz = 0; iz < sizeZ; ++iz) {
                    double z = (startZ + iz) * scaleZ + offsetZ;
                    int Z = (int)std::floor(z); z -= Z; Z &= 255;
                    double w = fade(z);

                    int A = p[X] + 0, AA = p[A] + Z;
                    int B = p[X + 1] + 0, BA = p[B] + Z;

                    double r0 = lerp(u, grad2(p[AA], x, z),
                        grad2(p[BA], x - 1.0, z));
                    double r1 = lerp(u, grad2(p[AA + 1], x, z - 1.0),
                        grad2(p[BA + 1], x - 1.0, z - 1.0));

                    out[idx++] += lerp(w, r0, r1) * invAmp;
                }
            }
        }
        else {
            // 3D path — full trilinear
            int idx = 0;
            int prevIY = -1;
            double a0, a1, a2, a3; // cached XZ plane values
            a0 = a1 = a2 = a3 = 0.0;

            for (int ix = 0; ix < sizeX; ++ix) {
                double x = (startX + ix) * scaleX + offsetX;
                int X = (int)std::floor(x); x -= X; X &= 255;
                double u = fade(x);

                for (int iz = 0; iz < sizeZ; ++iz) {
                    double z = (startZ + iz) * scaleZ + offsetZ;
                    int Z = (int)std::floor(z); z -= Z; Z &= 255;
                    double w = fade(z);

                    for (int iy = 0; iy < sizeY; ++iy) {
                        double y = (startY + iy) * scaleY + offsetY;
                        int Y = (int)std::floor(y); y -= Y; Y &= 255;
                        double v = fade(y);

                        // Only recompute XZ hash plane when Y integer changes
                        if (iy == 0 || Y != prevIY) {
                            prevIY = Y;
                            int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
                            int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;
                            a0 = lerp(u, grad3(p[AA], x, y, z), grad3(p[BA], x - 1.0, y, z));
                            a1 = lerp(u, grad3(p[AB], x, y - 1.0, z), grad3(p[BB], x - 1.0, y - 1.0, z));
                            a2 = lerp(u, grad3(p[AA + 1], x, y, z - 1.0), grad3(p[BA + 1], x - 1.0, y, z - 1.0));
                            a3 = lerp(u, grad3(p[AB + 1], x, y - 1.0, z - 1.0), grad3(p[BB + 1], x - 1.0, y - 1.0, z - 1.0));
                        }

                        double r = lerp(w, lerp(v, a0, a1), lerp(v, a2, a3));
                        out[idx++] += r * invAmp;
                    }
                }
            }
        }
    }
};