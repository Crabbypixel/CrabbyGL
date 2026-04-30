#pragma once
#include "world/Chunk.h"

class WorldGen
{
public:
    // Single entry point — World::FillChunkData calls only this
    static void Generate(Chunk& chunk, glm::ivec2 coord);

private:
    static void PlaceTerrain(Chunk& chunk, int wx, int wz);
    //static void CarveCaves(Chunk& chunk, int wx, int wz);
   // static void PlaceOres(Chunk& chunk, int wx, int wz);    // stub for now
};