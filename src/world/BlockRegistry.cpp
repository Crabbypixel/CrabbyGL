#include "world/BlockRegistry.h"

using namespace Tiles;

// +Y -Y +X -X +Z -Z
const BlockDef BLOCK_DEFS[] =
{
/* 0*/	{ "Air", { 0, 0, 0, 0, 0, 0 }, -1, { 1, 1, 1 }, BLOCK_TRANSPARENT, 0.0f, false },
/* 1*/	{ "Dirt", { DIRT, DIRT, DIRT, DIRT, DIRT, DIRT }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
/* 2*/	{ "Grass", { GRASS_TOP, DIRT, DIRT, DIRT, DIRT, DIRT }, GRASS_OVERLAY, { 0.55f, 0.78f, 0.28f }, BLOCK_OPAQUE | BLOCK_SOLID, 0.6f, true },
/* 3*/	{ "Stone", { STONE, STONE, STONE, STONE, STONE, STONE }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
/* 4*/	{ "Bedrock", { BEDROCK, BEDROCK, BEDROCK, BEDROCK, BEDROCK, BEDROCK }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 10000.0f, false },
/* 5*/	{ "Brick", { BRICK, BRICK, BRICK, BRICK, BRICK, BRICK }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
/* 6*/	{ "Log", { TREE_LOG_TOP, TREE_LOG_TOP, TREE_LOG_SIDES, TREE_LOG_SIDES, TREE_LOG_SIDES, TREE_LOG_SIDES }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
/* 7*/	{ "Leaves", { TREE_LEAVES, TREE_LEAVES, TREE_LEAVES, TREE_LEAVES, TREE_LEAVES, TREE_LEAVES }, -1, { 1, 1, 1 }, BLOCK_TRANSLUCENT | BLOCK_SOLID, 10000.0f, false },
/* 8*/	{ "Cobblestone", { COBBLESTONE, COBBLESTONE, COBBLESTONE, COBBLESTONE, COBBLESTONE, COBBLESTONE }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
/* 9*/	{ "Planks", { PLANK, PLANK, PLANK, PLANK, PLANK, PLANK }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
/*10*/	{ "Sand", { SAND, SAND, SAND, SAND, SAND, SAND }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
/*11*/	{ "Gravel", { GRAVEL, GRAVEL, GRAVEL, GRAVEL, GRAVEL, GRAVEL }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
/*12*/	{ "Glass", { GLASS, GLASS, GLASS, GLASS, GLASS, GLASS }, -1, { 1, 1, 1 }, BLOCK_TRANSLUCENT | BLOCK_SOLID, 10000.0f, false },
/*13*/	{ "Gold ore", { ORE_GOLD, ORE_GOLD, ORE_GOLD, ORE_GOLD, ORE_GOLD, ORE_GOLD }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
/*14*/	{ "Iron ore", { ORE_IRON, ORE_IRON, ORE_IRON, ORE_IRON, ORE_IRON, ORE_IRON }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
/*15*/	{ "Coal ore", { ORE_COAL, ORE_COAL, ORE_COAL, ORE_COAL, ORE_COAL, ORE_COAL }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
/*16*/	{ "Diamond ore", { ORE_DIAMOND, ORE_DIAMOND, ORE_DIAMOND, ORE_DIAMOND, ORE_DIAMOND, ORE_DIAMOND }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
/*17*/	{ "Smooth stone", { SMOOTH_STONE, SMOOTH_STONE, SMOOTH_STONE, SMOOTH_STONE, SMOOTH_STONE, SMOOTH_STONE }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },

/*18*/  { "Sapling",    {SAPLING,   SAPLING,   SAPLING,   SAPLING,   SAPLING,   SAPLING   }, -1, {1,1,1}, BLOCK_TRANSPARENT | BLOCK_CROSS, 0.0f, false },
/*19*/  { "Rose",       {ROSE,      ROSE,      ROSE,      ROSE,      ROSE,      ROSE      }, -1, {1,1,1}, BLOCK_TRANSPARENT | BLOCK_CROSS, 0.0f, false },
/*20*/  { "Dandelion",  {DANDELION, DANDELION, DANDELION, DANDELION, DANDELION, DANDELION }, -1, {1,1,1}, BLOCK_TRANSPARENT | BLOCK_CROSS, 0.0f, false },
/*21*/  { "Brown mushroom",  {BROWN_MUSHROOM, BROWN_MUSHROOM, BROWN_MUSHROOM, BROWN_MUSHROOM, BROWN_MUSHROOM, BROWN_MUSHROOM }, -1, {1,1,1}, BLOCK_TRANSPARENT | BLOCK_CROSS, 0.0f, false },
/*22*/  { "Red mushroom",  {RED_MUSHROOM, RED_MUSHROOM, RED_MUSHROOM, RED_MUSHROOM, RED_MUSHROOM, RED_MUSHROOM }, -1, {1,1,1}, BLOCK_TRANSPARENT | BLOCK_CROSS, 0.0f, false },

};

const BlockDef& GetDef(BlockType t)
{
	return BLOCK_DEFS[(uint8_t)t];
}

bool IsCross(BlockType t)
{
	return GetDef(t).flags & BLOCK_CROSS;
}

bool IsOpaque(BlockType t)
{
	return GetDef(t).flags & BLOCK_OPAQUE;
}

bool IsTransparent(BlockType t)
{
	return GetDef(t).flags & BLOCK_TRANSPARENT;
}

bool IsSolid(BlockType t)
{
	return GetDef(t).flags & BLOCK_SOLID;
}

bool IsTranslucent(BlockType t)
{
	return GetDef(t).flags & BLOCK_TRANSLUCENT;
}
