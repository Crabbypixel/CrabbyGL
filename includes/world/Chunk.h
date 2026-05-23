#pragma once
#include "world/BlockType.h"

#include <glm/glm.hpp>
#include <cstdint>
#include <atomic>
#include <shared_mutex>

constexpr int CX = 16;
constexpr int CY = 256;
constexpr int CZ = 16;

class Chunk
{
public:
	Chunk() { memset(blocks, 0, sizeof(blocks)); }

	Chunk(const Chunk&) = delete;
	Chunk(Chunk&&) = delete;
	Chunk& operator=(const Chunk&) = delete;
	Chunk& operator=(Chunk&&) = delete;

	mutable std::shared_mutex chunkMutex;

	// Core block data
	/*
	  * TODO: Make this private and only accessible via Get / Set, but
	  * that would require a lot of code changes so maybe later
	
	  * TODO: Maybe linearize this into a 1D array for better cache performance, but 
	  * that would require changing the block access code everywhere so maybe later
	*/
	BlockType blocks[CX][CY][CZ];

	glm::ivec2 chunkPos;
	std::atomic<bool> dirty{ true };						// Needs mesh rebuild
	std::atomic<bool> modified{ false };					// Has unsaved changes (never cleared except after SaveChunkToDisk)

	[[nodiscard]] BlockType Get(int x, int y, int z) const noexcept;
	[[nodiscard]] BlockType GetUnchecked(int x, int y, int z) const;
	void Set(int x, int y, int z, BlockType type);
	void SetUnchecked(int x, int y, int z, BlockType type);

	// Range check
	[[nodiscard]] static constexpr bool InBounds(int x, int y, int z) noexcept;
};