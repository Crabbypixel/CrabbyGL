#include "world/LightingSystem.h"
#include "world/BlockRegistry.h"
#include "world/World.h"

#include <iostream>
#include <mutex>

// Index helpers
static constexpr uint16_t Encode(uint8_t x, uint8_t y, uint8_t z) noexcept
{
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

void LightingSystem::InitChunkLight(Chunk* chunk)
{
	if (!chunk)
		return;

	for(int y = 0; y < CY; ++y)
	for(int x = 0; x < CX; ++x)
	for(int z = 0; z < CZ; ++z)
	{
		int lightValue = GetDef(chunk->Get(x, y, z)).lightEmission;
		if (lightValue > 0)
		{
			SetTorchLight(chunk, x, y, z, lightValue);
			m_lightBFSQueue.emplace(Encode(x, y, z), chunk);
		}
	}

	// Check if the chunk's neighbors edge blocks have light -> seed BFS
	// Neighbor bleed-in
	static const struct {
		glm::ivec2 ND;          // Neighbor chunk direction offset
		glm::ivec2 borderPos;   // Target coordinate inside the neighboring chunk
		glm::ivec2 edgePos;     // Boundary coordinate inside the current chunk
	} NEIGHBORS[] = {
		// -1 means that index is run by a loop variable
		{ {  1,  0 }, { 0,      -1 }, { CX - 1, -1     } },        // +X neighbor
		{ { -1,  0 }, { CX - 1, -1 }, { 0,      -1     } },        // -X neighbor
		{ {  0,  1 }, { -1,     0  }, { -1,     CZ - 1 } },        // +Z neighbor
		{ {  0, -1 }, { -1, CZ - 1 }, { -1,     0      } }         // -Z neighbor
	};

	for (const auto& side : NEIGHBORS)
	{
		const glm::ivec2 pos = chunk->chunkPos;
		const glm::ivec2 nPos = pos + side.ND;
		Chunk* neighborChunk = m_world->GetChunk(nPos.x * CX, nPos.y * CZ);

		if (!neighborChunk)
			continue;

		// Traverse layer by layer (increasing height)
		// The inner (i) loop is only for +/- X/Z so it always runs 16 times
		for(int y = 0; y < CY; ++y)
		for(int i = 0; i < CX; ++i)
		{
			// Border pos - coord inside neighboring chunk
			glm::ivec3 borderPos = (side.borderPos.x != -1) ? glm::ivec3(side.borderPos.x, y, i) : glm::ivec3(i, y, side.borderPos.y);
			glm::ivec3 edgePos = (side.edgePos.x != -1) ? glm::ivec3(side.edgePos.x, y, i) : glm::ivec3(i, y, side.edgePos.y);

			int borderLight = GetTorchLight(neighborChunk, borderPos.x, borderPos.y, borderPos.z);
			int incomingLight = borderLight - 1;
			int edgeLight = GetTorchLight(chunk, edgePos.x, edgePos.y, edgePos.z);

			if (borderLight <= 1)
				continue;

			if (edgeLight < incomingLight)
			{
				SetTorchLight(chunk, edgePos.x, edgePos.y, edgePos.z, incomingLight);
				m_lightBFSQueue.emplace(Encode(edgePos.x, edgePos.y, edgePos.z), chunk);
			}
		}
	}
}

void LightingSystem::NotifyBlockPlaced(int wx, int wy, int wz, BlockType type)
{
	int emission = GetDef(type).lightEmission;

	glm::ivec3 local = World::ChunkLocalCoord(wx, wy, wz);
	Chunk* chunk = m_world->GetChunk(wx, wz);

	if (!chunk)
		return;

	if (emission > 0)
	{
		SetTorchLight(chunk, local.x, local.y, local.z, emission);
		m_lightBFSQueue.emplace(Encode(local.x, local.y, local.z), chunk);
	}

	else if (IsOpaque(type))
	{
		int existingLightLevel = GetTorchLight(chunk, local.x, local.y, local.z);
		SetTorchLight(chunk, local.x, local.y, local.z, 0);
		m_lightRemovalBFSQueue.emplace(Encode(local.x, local.y, local.z), existingLightLevel, chunk);
	}
}

void LightingSystem::NotifyBlockRemoved(int wx, int wy, int wz)
{
	Chunk* chunk = m_world->GetChunk(wx, wz);
	if (!chunk)
	{
		std::cerr << "NotifyBlockRemoved() - invalid chunk\n";
		return;
	}

	glm::ivec3 local = World::ChunkLocalCoord(wx, wy, wz);

	uint16_t index = Encode(local.x, local.y, local.z);
	int lightValue = GetTorchLight(chunk, local.x, local.y, local.z);

	m_lightRemovalBFSQueue.emplace(index, lightValue, chunk);
	SetTorchLight(chunk, local.x, local.y, local.z, 0);
}

void LightingSystem::Update()
{
	RemoveTorch();
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
		{
			std::cerr << "PropagateTorch() - invalid chunk\n";
			return;
		}
		glm::ivec3 local = { DecodeX(index), DecodeY(index), DecodeZ(index) };

		// Get light level of this pos
		int lightLevel = GetTorchLight(chunk, local.x, local.y, local.z);

		// Look at all adjacent voxels to this position
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
			glm::ivec3 adjacentBlockPos = local + ND;
			Chunk* adjacentBlockChunk = chunk;

			// Resolve if the block is in adjacent chunk
			if (!Chunk::InBounds(adjacentBlockPos.x, adjacentBlockPos.y, adjacentBlockPos.z))
			{
				// Allowing blocks outside vertical world limit makes query into the same chunk, so don't allow
				if (adjacentBlockPos.y < 0 || adjacentBlockPos.y >= CY)
					continue;

				glm::ivec2 neighboringBlockChunkPos = chunk->chunkPos + glm::ivec2(ND.x, ND.z);
				adjacentBlockChunk = m_world->GetChunk(neighboringBlockChunkPos.x * CX, neighboringBlockChunkPos.y * CZ);

				if (!adjacentBlockChunk)
					continue;

				// Get local block pos
				if		(adjacentBlockPos.x == -1) adjacentBlockPos.x = CX - 1;
				else if (adjacentBlockPos.x == CX) adjacentBlockPos.x = 0;
				else if (adjacentBlockPos.z == -1) adjacentBlockPos.z = CZ - 1;
				else if (adjacentBlockPos.z == CZ) adjacentBlockPos.z = 0;
			}

			// Change light levels and add into queue
			// Only propagate light into non-opaque blocks
			if (!IsOpaque(adjacentBlockChunk->Get(adjacentBlockPos.x, adjacentBlockPos.y, adjacentBlockPos.z))
				&& GetTorchLight(adjacentBlockChunk, adjacentBlockPos.x, adjacentBlockPos.y, adjacentBlockPos.z) + 2 <= lightLevel)
			{
				// Set torch light of adjacent block
				SetTorchLight(adjacentBlockChunk, adjacentBlockPos.x, adjacentBlockPos.y, adjacentBlockPos.z, lightLevel - 1);

				// Add adjacent block into queue
				m_lightBFSQueue.emplace(Encode(adjacentBlockPos.x, adjacentBlockPos.y, adjacentBlockPos.z), adjacentBlockChunk);
			}
		}
	}
}

void LightingSystem::RemoveTorch()
{
	while (!m_lightRemovalBFSQueue.empty())
	{
		uint16_t index = 0;
		int lightLevel = 0;
		Chunk* chunk = nullptr;
		{
			LightRemovalNode& node = m_lightRemovalBFSQueue.front();
			index = node.index;
			lightLevel = node.val;
			chunk = node.chunk;
			m_lightRemovalBFSQueue.pop();
		}

		if (!chunk)
		{
			std::cerr << "RemoveTorch() - invalid chunk\n";
			return;
		}

		glm::ivec3 local = { DecodeX(index), DecodeY(index), DecodeZ(index) };

		static const glm::ivec3 NEIGHBORS[] = {
			{1, 0, 0}, {-1, 0, 0},	// +X, -X
			{0, 0, 1}, { 0, 0,-1},	// +Z, -Z
			{0, 1, 0}, { 0,-1, 0},	// +Y, -Y
		};

		for (const auto& ND : NEIGHBORS)
		{
			glm::ivec3 adjacentBlockPos = local + ND;
			Chunk* adjacentBlockChunk = chunk;

			// Resolve if the block is in adjacent chunk
			if (!Chunk::InBounds(adjacentBlockPos.x, adjacentBlockPos.y, adjacentBlockPos.z))
			{
				if (adjacentBlockPos.y < 0 || adjacentBlockPos.y >= CY)
					continue;

				glm::ivec2 neighboringBlockChunkPos = chunk->chunkPos + glm::ivec2(ND.x, ND.z);
				adjacentBlockChunk = m_world->GetChunk(neighboringBlockChunkPos.x * CX, neighboringBlockChunkPos.y * CZ);
				
				if (!adjacentBlockChunk)
					continue;

				// Get local block pos
				if		(adjacentBlockPos.x == -1) adjacentBlockPos.x = CX - 1;
				else if (adjacentBlockPos.x == CX) adjacentBlockPos.x = 0;
				else if (adjacentBlockPos.z == -1) adjacentBlockPos.z = CZ - 1;
				else if (adjacentBlockPos.z == CZ) adjacentBlockPos.z = 0;
			}

			int neighborLevel = GetTorchLight(adjacentBlockChunk, adjacentBlockPos.x, adjacentBlockPos.y, adjacentBlockPos.z);
			
			if (neighborLevel != 0 && neighborLevel < lightLevel)
			{
				// Set adjacent block light level
				SetTorchLight(adjacentBlockChunk, adjacentBlockPos.x, adjacentBlockPos.y, adjacentBlockPos.z, 0);
				m_lightRemovalBFSQueue.emplace(Encode(adjacentBlockPos.x, adjacentBlockPos.y, adjacentBlockPos.z), neighborLevel, adjacentBlockChunk);
			}
			else if(neighborLevel >= lightLevel)
			{
				// Kind of becomes a light source, propagate light
				m_lightBFSQueue.emplace(Encode(adjacentBlockPos.x, adjacentBlockPos.y, adjacentBlockPos.z), adjacentBlockChunk);
			}
		}
	}
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
	
	std::unique_lock lock(chunk->chunkMutex);

	chunk->lightMap[x][y][z] = (chunk->lightMap[x][y][z] & 0xF0) | static_cast<uint8_t>(val);
	chunk->dirty = true;
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

	std::unique_lock lock(chunk->chunkMutex);
	chunk->lightMap[x][y][z] = (chunk->lightMap[x][y][z] & 0xF) | (static_cast<uint8_t>(val) << 4);
}