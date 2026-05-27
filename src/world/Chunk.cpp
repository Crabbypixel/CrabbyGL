#include "world/Chunk.h"

#include <vector>
#include <mutex>

constexpr bool Chunk::InBounds(int x, int y, int z) noexcept
{
	return (x >= 0 && x < CX) && (y >= 0 && y < CY) && (z >= 0 && z < CZ);
}

BlockType Chunk::Get(int x, int y, int z) const noexcept
{
	if (!InBounds(x, y, z))
		return BlockType::AIR;

	return blocks[x][y][z];
}

BlockType Chunk::GetUnchecked(int x, int y, int z) const
{
	return blocks[x][y][z];
}

void Chunk::Set(int x, int y, int z, BlockType type)
{
	if (!InBounds(x, y, z))
		return;

	std::unique_lock lock(chunkMutex);

	blocks[x][y][z] = type;
	dirty = true;
	modified = true;
}

void Chunk::SetUnchecked(int x, int y, int z, BlockType type)
{
	std::unique_lock lock(chunkMutex);

	blocks[x][y][z] = type;
	dirty = true;
	modified = true;
}
