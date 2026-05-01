#pragma once
#include <cstdint>
#include "world/Chunk.h"

namespace Tiles
{
	constexpr int DIRT = 0;
	constexpr int GRASS_TOP = 1;
	constexpr int GRASS_OVERLAY = 2;
	constexpr int STONE = 3;
	constexpr int BEDROCK = 4;
	constexpr int BRICK = 5;
	constexpr int TREE_LOG_SIDES = 6;
	constexpr int TREE_LOG_TOP = 7;
	constexpr int TREE_LEAVES = 8;
	constexpr int COBBLESTONE = 16;
}

struct BlockDef
{
	const char* name;
	int faces[6];
	int overlay = -1;
	glm::vec3 tint;
	uint16_t flags;
	float hardness;
	bool useOverlay;
};

enum BlockFlags : uint16_t
{
	BLOCK_OPAQUE = 1 << 0,
	BLOCK_SOLID = 1 << 1,
	BLOCK_TRANSPARENT = 1 << 2,
	BLOCK_EMISSIVE = 1 << 3
};

const BlockDef& GetDef(BlockType t);
bool IsOpaque(BlockType t);
bool IsTransparent(BlockType t);
bool IsSolid(BlockType t);