#include "physics/WorldPhysics.h"
#include "world/BlockType.h"
#include "world/World.h"
#include "world/BlockRegistry.h"

void WorldPhysics::Update(float dt, World& world)
{
	m_accumulator += dt;

	while (m_accumulator >= TICK_DT)
	{
		Tick(world);
		m_accumulator -= TICK_DT;
	}
}

void WorldPhysics::NotifyBlockChanged(int wx, int wy, int wz)
{
	m_nextDirtyBlocks.insert({ wx, wy, wz });
	m_nextDirtyBlocks.insert({ wx, wy + 1, wz });
}

void WorldPhysics::Tick(World& world)
{
	// Process each dirty block and determine if it needs to fall due to gravity, and if so, 
	// update the world and mark any affected blocks as dirty for the next tick
	for (const glm::ivec3& pos : m_dirtyBlocks)
	{
		// Get the block type at the current position
		BlockType type = world.GetBlock(pos.x, pos.y, pos.z);

		// Check if gravity block
		if (!IsGravityBlock(type))
			continue;

		// Prevent falling into the void
		if (pos.y <= 0)
			continue;

		// Get the block type below
		glm::ivec3 belowPos = { pos.x, pos.y - 1, pos.z };
		BlockType belowType = world.GetBlock(belowPos.x, belowPos.y, belowPos.z);

		// If the block below is air, fall down else continue
		if (belowType != BlockType::AIR)
			continue;

		// The below block is air, so move the gravity block down by one and mark the new position dirty for the next tick
		world.SetBlock(pos.x, pos.y, pos.z, BlockType::AIR);
		world.SetBlock(belowPos.x, belowPos.y, belowPos.z, type);

		// Falling blocks can trigger additional falling blocks above them on subsequent ticks.
		// Mark the source position dirty so the block above gets re-evaluated,
		// and mark the block below the new position dirty for future gravity updates.
		m_nextDirtyBlocks.insert({ pos.x, pos.y + 1, pos.z });
		m_nextDirtyBlocks.insert({ pos.x, pos.y - 1, pos.z });
		m_nextDirtyBlocks.insert({ pos.x + 1, pos.y, pos.z });
		m_nextDirtyBlocks.insert({ pos.x - 1, pos.y, pos.z });
		m_nextDirtyBlocks.insert({ pos.x, pos.y, pos.z + 1 });
		m_nextDirtyBlocks.insert({ pos.x, pos.y, pos.z - 1 });
	}

	std::swap(m_dirtyBlocks, m_nextDirtyBlocks);
	m_nextDirtyBlocks.clear();
}

bool WorldPhysics::IsGravityBlock(BlockType type) noexcept
{
	return type == BlockType::SAND || type == BlockType::GRAVEL;
}