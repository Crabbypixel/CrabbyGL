#include "world/Chunk.h"

#include <vector>
#include <mutex>
#include <fstream>

constexpr bool Chunk::InBounds(int x, int y, int z) noexcept
{
	return (x >= 0 && x < CX) && (y >= 0 && y < CY) && (z >= 0 && z < CZ);
}

BlockType Chunk::Get(int x, int y, int z) const noexcept
{
	if (!InBounds(x, y, z))
		return BlockType::AIR;

	std::shared_lock lock(chunkMutex);

	return GetUnchecked(x, y, z);
}

void Chunk::Set(int x, int y, int z, BlockType type)
{
	if (!InBounds(x, y, z))
		return;
	
	std::unique_lock lock(chunkMutex);

	SetUnchecked(x, y, z, type);
}

BlockType Chunk::GetUnchecked(int x, int y, int z) const
{
	return blocks[x][y][z];
}

void Chunk::SetUnchecked(int x, int y, int z, BlockType type)
{
	blocks[x][y][z] = type;

	dirty = true;
	modified = true;
	aoDirty = true;
}

void Chunk::Serialize(std::ofstream& f) const
{
	if (!f)
		return;

	f.write(reinterpret_cast<const char*>(blocks), sizeof(blocks));
}

bool Chunk::Deserialize(std::ifstream& f)
{
	if (!f)
		return false;

	f.read(reinterpret_cast<char*>(blocks), sizeof(blocks));
	return f.gcount() == sizeof(blocks);
}
