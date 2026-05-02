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
	AIR,
	DIRT,
	GRASS,
	STONE,
	BEDROCK,
	BRICK,
	TREE_LOG,
	TREE_LEAVES,
	COBBLESTONE,
	PLANK,
	SAND,
	GRAVEL,
	GLASS,
	GOLD_ORE,
	IRON_ORE,
	COAL_ORE,
	DIAMOND_ORE,
	SMOOTH_STONE,
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