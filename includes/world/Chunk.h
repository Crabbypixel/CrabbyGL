#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <atomic>
#include <shared_mutex>

constexpr int CX = 16;
constexpr int CY = 256;
constexpr int CZ = 16;

enum class BlockType : uint8_t
{
	/* 0*/	AIR,
	/* 1*/	DIRT,
	/* 2*/	GRASS,
	/* 3*/	STONE,
	/* 4*/	BEDROCK,
	/* 5*/	BRICK,
	/* 6*/	TREE_LOG,
	/* 7*/	TREE_LEAVES,
	/* 8*/	COBBLESTONE,
	/* 9*/	PLANK,
	/*10*/	SAND,
	/*11*/	GRAVEL,
	/*12*/	GLASS,
	/*13*/	GOLD_ORE,
	/*14*/	IRON_ORE,
	/*15*/	COAL_ORE,
	/*16*/	DIAMOND_ORE,
	/*17*/	SMOOTH_STONE,
	/*18*/	SAPLING,
	/*19*/	ROSE,
	/*20*/	DANDELION,
	/*21*/	BROWN_MUSHROOM,
	/*22*/	RED_MUSHROOM
};

struct BlockInstance
{
	glm::vec3 pos;
	BlockType type;
};

class Chunk
{
public:
	Chunk() = default;

	Chunk(const Chunk&) = delete;
	Chunk& operator=(const Chunk&) = delete;

	mutable std::shared_mutex chunkMutex;
	BlockType blocks[CX][CY][CZ];
	glm::ivec2 chunkPos;
	std::atomic<bool> dirty{ true };						// Needs mesh rebuild
	std::atomic<bool> modified{ false };					// Has unsaved changes (never cleared except after SaveChunk)

	BlockType Get(int x, int y, int z) const;
	BlockType GetUnchecked(int x, int y, int z) const;
	void Set(int x, int y, int z, BlockType type);
	void SetUnchecked(int x, int y, int z, BlockType type);

	// Range check
	static bool InBounds(int x, int y, int z);
};