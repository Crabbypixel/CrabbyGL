#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <iostream>
#include <atomic>
#include <shared_mutex>

constexpr int CX = 16;
constexpr int CY = 256;
constexpr int CZ = 16;

enum class BlockType : uint8_t
{
	AIR = 0,
	DIRT = 1,
	GRASS = 2,
	STONE = 3,
	BEDROCK = 4,
	BRICK = 5,
	TREE_LOG = 6,
	TREE_LEAVES = 7,
	COBBLESTONE = 8
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