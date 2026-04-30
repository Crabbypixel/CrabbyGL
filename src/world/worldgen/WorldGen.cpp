#include "world/worldgen/WorldGen.h"
#include "world/worldgen/TerrainGen.h"
#include "world/worldgen/CaveGen.h"
#include "world/worldgen/NoiseSettings.h"

void WorldGen::Generate(Chunk& chunk, glm::ivec2 coord)
{
    int wx = coord.x * CX;
    int wz = coord.y * CZ;

    TerrainGen::Fill(chunk, wx, wz);
    CaveGen::Carve(chunk, wx, wz);
    // OreGen::Place(chunk, wx, wz);     ← uncomment when ready
    // StructureGen::Place(chunk, wx, wz);
}