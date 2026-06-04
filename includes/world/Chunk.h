#pragma once
#include "world/BlockType.h"

#include <glm/glm.hpp>
#include <cstdint>
#include <atomic>
#include <shared_mutex>
#include <array>

constexpr int CX = 16;
constexpr int CY = 256;
constexpr int CZ = 16;
constexpr int CHUNK_VOLUME = CX * CY * CZ;

class Chunk
{
private:
	// Core block data
	// Linearized access for better cache performance in meshing and serialization
	// ** Z is the fastest moving axis, then X, then Y (YZX order) to optimize for horizontal locality during meshing **
	std::array<BlockType, CHUNK_VOLUME> blocks{};

	// Index - YZX ordering
	[[nodiscard]] static constexpr int GetIndex(int x, int y, int z) noexcept
	{
		return (y * CX * CZ) + (x * CZ) + z;
	}

public:
	Chunk() = default;

	Chunk(const Chunk&) = delete;
	Chunk(Chunk&&) = delete;
	Chunk& operator=(const Chunk&) = delete;
	Chunk& operator=(Chunk&&) = delete;

	mutable std::shared_mutex chunkMutex;

	// Cached AO values for each block face (6 faces per block) to avoid redundant AO calculations during meshing
	uint8_t aoCache[CX][CY][CZ][6];

	glm::ivec2 chunkPos;

	// CRITICAL: Main thread owns the world state, workers CANNOT modify
	bool dirty = true;				// Needs mesh rebuild
	bool modified = false;			// Has unsaved changes (never cleared except after SaveChunkToDisk)
	std::atomic<bool> aoDirty = true;			// For computing AO values

	[[nodiscard]] BlockType Get(int x, int y, int z) const noexcept;
	[[nodiscard]] BlockType GetUnchecked(int x, int y, int z) const;
	void Set(int x, int y, int z, BlockType type);
	void SetUnchecked(int x, int y, int z, BlockType type);

	void Serialize(std::ofstream& f) const;
	[[nodiscard]] bool Deserialize(std::ifstream& f);

	// Range check
	[[nodiscard]] static constexpr bool InBounds(int x, int y, int z) noexcept;
};