#pragma once
#include <glm/glm.hpp>
#include <cstdint>

enum class BlockType : uint8_t;
struct BlockInstance;

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
	constexpr int COBBLESTONE = 9;

	constexpr int PLANK = 10;
	constexpr int SAND = 16;
	constexpr int GRAVEL = 17;
	constexpr int GLASS = 11;
	constexpr int SMOOTH_STONE = 12;
	constexpr int ORE_GOLD = 32;
	constexpr int ORE_IRON = 33;
	constexpr int ORE_COAL = 34;
	constexpr int ORE_DIAMOND = 35;

	constexpr int SAPLING = 13;
	constexpr int ROSE = 14;
	constexpr int DANDELION = 15;
	constexpr int BROWN_MUSHROOM = 31;
	constexpr int RED_MUSHROOM = 30;
	constexpr int GRASS = 29;
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
	BLOCK_TRANSLUCENT = 1 << 3,
	BLOCK_EMISSIVE = 1 << 4,
	BLOCK_CROSS = 1 << 5
};

const BlockDef& GetDef(BlockType t);
bool IsOpaque(BlockType t);
bool IsTransparent(BlockType t);
bool IsSolid(BlockType t);
bool IsTranslucent(BlockType t);