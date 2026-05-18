#pragma once
#include <glm/glm.hpp>
#include <unordered_set>

class World;
enum class BlockType : uint8_t;

class WorldPhysics
{
public:
	static constexpr float TICK_RATE = 20.0f;		            // 20 ticks per second
	static constexpr float TICK_DT = 1.0f / TICK_RATE;

	// Call every frame with real delta time, but physics will only update at fixed intervals (TICK_DT)
	void Update(float dt, World& world);

	// Call when any block is placed or broken by the player
	void NotifyBlockChanged(int wx, int wy, int wz);

private:
	float m_accumulator = 0.0f;		// Accumulated time since last physics update

    // Boost-combine hash for glm::ivec3
    // Simple XOR hashes cluster badly — positions on the same axis collide
    struct IVec3Hash
    {
        size_t operator()(const glm::ivec3& v) const
        {
            size_t h = 0;
            auto combine = [&](int n)
            {
                // Spreads bits across the full size_t range
                // Breaks the symmetry that XOR lacks
                h ^= std::hash<int>()(n) + 0x9e3779b9u + (h << 6) + (h >> 2);
            };

            combine(v.x);
            combine(v.y);
            combine(v.z);

            return h;
        }
    };

    using DirtySet = std::unordered_set<glm::ivec3, IVec3Hash>;
	DirtySet m_dirtyBlocks;		// Set of block positions that changed since last physics update
	DirtySet m_nextDirtyBlocks;	// Set of block positions that will be dirty in the next physics update

	// Runs one tick of physics updates, processing all blocks in m_dirtyBlocks and 
    // updating m_nextDirtyBlocks with any blocks that need to be re-evaluated on the next tick
    void Tick(World& world);

	// Returns true if the block type is affected by gravity (e.g. sand, gravel)
	static bool IsGravityBlock(BlockType type);
};