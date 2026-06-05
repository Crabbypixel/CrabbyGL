#include "world/LightingSystem.h"

void LightingSystem::NotifyBlockPlaced(int wx, int wy, int wz, BlockType type)
{

}

void LightingSystem::NotifyBlockRemoved(int wx, int wy, int wz)
{

}

void LightingSystem::Update(World& world)
{

}

void LightingSystem::PropagateTorch(World& world)
{
	// Call this when a torch is placed

}

void LightingSystem::RemoveTorch(World& world)
{

}

// Get bits 0000XXXX
int LightingSystem::GetTorchLight(Chunk* chunk, int x, int y, int z)
{
	if (!chunk)
		return 0;

	return chunk->lightMap[x][y][z] & 0xf;
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

static constexpr uint16_t Encode(uint8_t x, uint8_t y, uint8_t z) noexcept
{
	assert(x >= 0 && x <= CX);
	assert(y >= 0 && y <= CY);
	assert(z >= 0 && z <= CZ);

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