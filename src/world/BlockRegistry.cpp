#include "world/BlockRegistry.h"

using namespace Tiles;

// +Y -Y +X -X +Z -Z
const BlockDef BLOCK_DEFS[] =
{
	{ "air", { 0, 0, 0, 0, 0, 0 }, -1, { 1, 1, 1 }, BLOCK_TRANSPARENT, 0.0f, false },
	{ "dirt", { DIRT, DIRT, DIRT, DIRT, DIRT, DIRT }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
	{ "grass", { GRASS_TOP, DIRT, DIRT, DIRT, DIRT, DIRT }, GRASS_OVERLAY, { 0.55f, 0.78f, 0.28f }, BLOCK_OPAQUE | BLOCK_SOLID, 0.6f, true },
	{ "stone", { STONE, STONE, STONE, STONE, STONE, STONE }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
	{ "bedrock", { BEDROCK, BEDROCK, BEDROCK, BEDROCK, BEDROCK, BEDROCK }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 10000.0f, false },
	{ "brick", { BRICK, BRICK, BRICK, BRICK, BRICK, BRICK }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
	{ "log", { TREE_LOG_TOP, TREE_LOG_TOP, TREE_LOG_SIDES, TREE_LOG_SIDES, TREE_LOG_SIDES, TREE_LOG_SIDES }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
	{ "leaves", { TREE_LEAVES, TREE_LEAVES, TREE_LEAVES, TREE_LEAVES, TREE_LEAVES, TREE_LEAVES }, -1, { 1, 1, 1 }, BLOCK_TRANSPARENT | BLOCK_SOLID, 10000.0f, false },
	{ "cobblestone", { COBBLESTONE, COBBLESTONE, COBBLESTONE, COBBLESTONE, COBBLESTONE, COBBLESTONE }, -1, { 1, 1, 1 }, BLOCK_OPAQUE | BLOCK_SOLID, 1.5f, false },
};

const BlockDef& GetDef(BlockType t)
{
	return BLOCK_DEFS[(uint8_t)t];
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
