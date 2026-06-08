#pragma once
#include "world/Chunk.h"
#include <queue>

class World;

struct LightNode
{
	LightNode(uint16_t index, Chunk* chunk) : index(index), chunk(chunk) {}

	uint16_t index;
	Chunk* chunk;
};

struct LightRemovalNode
{
	LightRemovalNode(uint16_t index, uint8_t v, Chunk* chunk) : index(index), val(v), chunk(chunk) {}

	uint16_t index;
	uint8_t val;
	Chunk* chunk;
};

class LightingSystem
{
private:
	World* m_world = nullptr;
	std::queue<LightNode> m_lightBFSQueue;
	std::queue<LightRemovalNode> m_lightRemovalBFSQueue;

	// Non-copyable, non-movable
	LightingSystem(const LightingSystem&) = delete;
	LightingSystem(LightingSystem&&) = delete;
	LightingSystem& operator=(const LightingSystem&) = delete;
	LightingSystem& operator=(LightingSystem&&) = delete;

	// Light accessors
	int  GetTorchLight(Chunk* chunk, int x, int y, int z);
	void SetTorchLight(Chunk* chunk, int x, int y, int z, int val);
	int  GetSunlight(Chunk* chunk, int x, int y, int z);
	void SetSunlight(Chunk* chunk, int x, int y, int z, int val);

	// BFS passes:
	void PropagateTorch();
	void RemoveTorch();

public:
	LightingSystem() = default;
	void Init(World* world) { m_world = world; }
	void InitChunkLight(Chunk* chunk);

	void NotifyBlockPlaced(int wx, int wy, int wz, BlockType type);
	void NotifyBlockRemoved(int wx, int wy, int wz);
	void Update();
};