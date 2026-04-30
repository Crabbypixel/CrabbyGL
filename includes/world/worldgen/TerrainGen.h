#pragma once
#include "world/Chunk.h"

class TerrainGen
{
public:
    static void Fill(Chunk& chunk, int wx, int wz);

private:
    static float GetDensity(float fx, float fy, float fz);
    static float GetSelector(float fx, float fy, float fz);
};