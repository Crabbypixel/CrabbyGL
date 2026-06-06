#include "world/LightingSystem.h"
#include "world/BlockRegistry.h"
#include "world/World.h"

#include <iostream>

// Index helpers
static constexpr uint16_t Encode(uint8_t x, uint8_t y, uint8_t z) noexcept
{
	assert(x >= 0 && x < CX);
	assert(y >= 0 && y < CY);
	assert(z >= 0 && z < CZ);

	return ((uint16_t)x << 12) | ((uint16_t)y << 4) | (uint16_t)z;
}

static constexpr int DecodeX(uint16_t index) noexcept
{
	return (index >> 12) & 0xF;
}

static constexpr int DecodeY(uint16_t index) noexcept
{
	return (index >> 4) & 0xFF;
}

static constexpr int DecodeZ(uint16_t index) noexcept
{
	return (index) & 0xF;
}

void LightingSystem::NotifyBlockPlaced(int wx, int wy, int wz, BlockType type)
{
	int emission = GetDef(type).lightEmission;

	if (emission > 0)
	{
		glm::ivec3 localCoord = World::ChunkLocalCoord(wx, wy, wz);
		Chunk* chunk = m_world->GetChunk(wx, wz);

		if (!chunk)
			return;

		uint16_t index = Encode(localCoord.x, localCoord.y, localCoord.z);

		SetTorchLight(chunk, localCoord.x, localCoord.y, localCoord.z, emission);

		m_lightBFSQueue.emplace(index, chunk);
	}
}

void LightingSystem::NotifyBlockRemoved(int wx, int wy, int wz)
{

}

void LightingSystem::Update()
{
	PropagateTorch();
}

void LightingSystem::PropagateTorch()
{
	while (!m_lightBFSQueue.empty())
	{
		uint16_t index = 0;
		Chunk* chunk = nullptr;
		{
			LightNode& node = m_lightBFSQueue.front();
			index = node.index;
			chunk = node.chunk;
			m_lightBFSQueue.pop();
		}

		if (!chunk)
			return;

		glm::ivec3 local = { DecodeX(index), DecodeY(index), DecodeZ(index) };

		// Get light level of this pos
		int lightLevel = GetTorchLight(chunk, local.x, local.y, local.z);

		// Look at all neighboring voxels to this position
		// If its light level is 2 or more levels less than
		// to the current one, add them to the queue

		// Check all adjacent neighbors: +X, -X, +Z, -Z, +Y, -Y
		static const glm::ivec3 NEIGHBORS[] = {
			{1, 0, 0}, {-1, 0, 0},	// +X, -X
			{0, 0, 1}, {0, 0, -1},	// +Z, -Z
			{0, 1, 0}, {0, -1, 0},	// +Y, -Y
		};

		// Do bounds checking and if X is less than 0, query -X 
		for (const auto& ND : NEIGHBORS)
		{
			glm::vec3 adjacentBlock = local + ND;
			if (Chunk::InBounds(adjacentBlock.x, adjacentBlock.y, adjacentBlock.z))
			{
				// Only propagate light into non-opaque blocks
				if (!IsOpaque(chunk->Get(adjacentBlock.x, adjacentBlock.y, adjacentBlock.z)) && GetTorchLight(chunk, adjacentBlock.x, adjacentBlock.y, adjacentBlock.z) + 2 <= lightLevel)
				{
					// Set light level
					SetTorchLight(chunk, adjacentBlock.x, adjacentBlock.y, adjacentBlock.z, lightLevel - 1);

					// Add neighboring block into queue
					uint16_t neighboringIndex = Encode(adjacentBlock.x, adjacentBlock.y, adjacentBlock.z);
					m_lightBFSQueue.emplace(neighboringIndex, chunk);
				}
			}
			else
			{
				// Allowing blocks outside vertical world limit makes query into the same chunk, so don't allow
				if (adjacentBlock.y < 0 || adjacentBlock.y >= CY)
					continue;

				glm::ivec2 neighboringChunkPos = chunk->chunkPos + glm::ivec2(ND.x, ND.z);
				Chunk* neighboringChunk = m_world->GetChunk(neighboringChunkPos.x, neighboringChunkPos.y);

				if (!neighboringChunk)
					continue;

				// Get local block pos
				if (adjacentBlock.x == -1)		adjacentBlock.x = CX;
				else if (adjacentBlock.x == CX) adjacentBlock.x = 0;
				else if (adjacentBlock.z == -1) adjacentBlock.z = CZ;
				else if (adjacentBlock.z == CZ) adjacentBlock.z = 0;
			}
		}
	}
}

void LightingSystem::RemoveTorch()
{

}

// Get bits 0000XXXX
int LightingSystem::GetTorchLight(Chunk* chunk, int x, int y, int z)
{
	if (!chunk)
		return 0;

	return chunk->lightMap[x][y][z] & 0xF;
}

// Set bits 0000XXXX
void LightingSystem::SetTorchLight(Chunk* chunk, int x, int y, int z, int val)
{
	if (!chunk)
		return;

	chunk->lightMap[x][y][z] = (chunk->lightMap[x][y][z] & 0xF0) | static_cast<uint8_t>(val);
}
// Get bits XXXX0000
int LightingSystem::GetSunlight(Chunk* chunk, int x, int y, int z)
{
	if (!chunk)
		return 0;

	return (chunk->lightMap[x][y][z] >> 4) & 0xF;
}

// Set bits XXXX0000
void LightingSystem::SetSunlight(Chunk* chunk, int x, int y, int z, int val)
{
	if (!chunk)
		return;

	chunk->lightMap[x][y][z] = (chunk->lightMap[x][y][z] & 0xF) | (static_cast<uint8_t>(val) << 4);
}