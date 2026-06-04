#include "world/Chunk.h"

#include <vector>
#include <mutex>
#include <fstream>

static int GetIndex(int x, int y, int z)
{
	// YZX ordering!!!
	return (y * CX * CZ) + (x * CZ) + z;
}

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
	// YZX ordering!!!
	return blocks[GetIndex(x, y, z)];
}

void Chunk::SetUnchecked(int x, int y, int z, BlockType type)
{
	// YZX ordering!!!
	blocks[GetIndex(x, y, z)] = type;

	dirty = true;
	modified = true;
	aoDirty = true;
}

void Chunk::Serialize(std::ofstream& f) const
{
	if (!f)
		return;

	f.write(reinterpret_cast<const char*>(blocks.data()), sizeof(blocks));
}

bool Chunk::Deserialize(std::ifstream& f)
{
	if (!f)
		return false;

	f.read(reinterpret_cast<char*>(blocks.data()), sizeof(blocks));
	return f.gcount() == sizeof(blocks);

	// present: blocks[z + CZ * y + (CZ * CY) * x]
	// convert: blocks[y * (CX * CZ) + x * CZ + z]
	// ! Convert to [y][x][z]
	//for (int x = 0; x < CX; ++x)
	//{
	//	for (int y = 0; y < CY; ++y)
	//	{
	//		for (int z = 0; z < CZ; ++z)
	//		{
	//			int presentIndex = z + CZ * y + (CZ * CY) * x;
	//			int newIndex = y * (CX * CZ) + x * CZ + z;
	//			blocks[newIndex] = temp[presentIndex];
	//		}
	//	}
	//}

	//return f.gcount() == sizeof(blocks);
}
