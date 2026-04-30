#pragma once
#include "world/Chunk.h"
#include <glm/glm.hpp>

class CaveGen {
public:
    static void Carve(Chunk& chunk, int wxBase, int wzBase);

private:
    // Worm caves — traces a path through chunk space
    static void CarveWorm(Chunk& chunk, int wxBase, int wzBase,
        float ox, float oy, float oz,
        float dirX, float dirY, float dirZ,
        int steps, float radius);
};