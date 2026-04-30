#pragma once

#include <glm/glm.hpp>

#include "world/Chunk.h"
#include "rendering/ChunkMesh.h"
#include "world/ChunkMeshBuilder.h"
#include "Raycast.h"
//#include "world/PerlinNoise173.h"
//#include "world/WorldGen.h"

#include <cmath>
#include <array>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <mutex>
#include <thread>
#include <atomic>
#include <shared_mutex>
#include <condition_variable>

static struct IVec2Hash
{
    size_t operator()(const glm::ivec2& v) const
    {
        size_t h1 = std::hash<int>()(v.x);
        size_t h2 = std::hash<int>()(v.y);
        return h1 ^ (h2 * 2654435761u);
    }
};

static struct Frustum
{
    std::array<glm::vec4, 6> planes;

    void Extract(const glm::mat4& vp);
    bool ContainsAABB(const glm::vec3& min, const glm::vec3& max) const;
};

static struct MeshJob
{ 
    glm::ivec2 coord;
    Chunk* chunk;

    // Pointers for chunks neighboring the current chunk
    // This prevents this class to access the thread & time critical World object
    // Decouples the mesh worker from the world state
    Chunk* nPX = nullptr;       // +X neighbor
    Chunk* nNX = nullptr;       // -X neighbor
    Chunk* nPZ = nullptr;       // +Z neighbor
    Chunk* nNZ = nullptr;       // -Z neighbor

    Chunk* nPX_PZ = nullptr;    // (+X, +Z)
    Chunk* nPX_NZ = nullptr;    // (+X, -Z)
    Chunk* nNX_PZ = nullptr;    // (-X, +Z)
    Chunk* nNX_NZ = nullptr;    // (-X, -Z)
};

// In World.h or a TerrainGen struct
//struct TerrainGen {
//    PerlinNoise173 lowNoise1, lowNoise2;
//    PerlinNoise173 highNoise1, highNoise2;
//    PerlinNoise173 selector;
//
//    TerrainGen(uint64_t worldSeed)
//        : lowNoise1(makeRng(worldSeed, 1)),
//        lowNoise2(makeRng(worldSeed, 2)),
//        highNoise1(makeRng(worldSeed, 3)),
//        highNoise2(makeRng(worldSeed, 4)),
//        selector(makeRng(worldSeed, 5))
//    {
//    }
//
//private:
//    static std::mt19937_64 makeRng(uint64_t seed, int salt) {
//        return std::mt19937_64(seed + salt * 0x9e3779b97f4a7c15ULL);
//    }
//};

class World
{
public:
    // Core data
    std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, IVec2Hash> chunks;

    // Constructor
    World();

    // Block access
    BlockType GetBlock(int worldX, int worldY, int worldZ) const;
    void SetBlock(int worldX, int worldY, int worldZ, BlockType type);
    bool IsSolid(int worldX, int worldY, int worldZ) const;

    // Player interaction
    bool PlaceBlock(const RaycastHit& hit, BlockType type);
    bool BreakBlock(const RaycastHit& hit);

    // Rendering
    void SyncRenderer();
    void SetChunkShader(Shader& shader);
    void LoadAtlasTexture(const char* path);
    void DrawAll(const glm::mat4& proj, const glm::mat4& view);

    // Streaming
    void UpdateChunkStreaming(const glm::vec3& playerPos);

    // Save chunks
    void UnloadChunks();

    // Helpers
    static glm::ivec2 ChunkCoord(float worldX, float worldZ);
    static glm::ivec3 ChunkLocalCoord(int worldX, int worldY, int worldZ);

    void CommitGeneratedChunks();  // called on each frame, main thread only

    // Workers
    void StartChunkLoadWorkers(int count);
    void StopAllWorkers();
    void StartMeshWorkers(int count);

private:
    // Internal chunk access
    Chunk* GetChunk(int worldX, int worldZ);
    const Chunk* GetChunk(int worldX, int worldZ) const;

    // Rendering data
    unsigned int m_atlasTexture = 0;
    Frustum m_frustum;
    std::unordered_map<glm::ivec2, ChunkMesh, IVec2Hash> m_chunkMeshes;
    Shader* m_chunkShader = nullptr;

    // Chunk updates
    void MarkNeighborChunksDirty(int wx, int wy, int wz);

    // Returns height at location using Perlin noise
    static float GetTerrainHeight(int wx, int wz);

    // Streaming / disk
    static std::string ChunkFilePath(glm::ivec2 coord);
    static void SaveChunk(const Chunk& chunk);
    static bool LoadChunkFromDisk(Chunk& chunk, glm::ivec2& coord);

    // Streaming state
    int m_viewDist = 8;
    int m_unloadDist = 12;
    glm::ivec2 m_lastPlayerChunk = { INT_MAX, INT_MAX };

    // Multithreading
    std::atomic<bool> m_shutdown{ false };

    // ──────── Load workers ────────
    std::vector<std::thread> m_chunkLoadWorkers;

    // Job queue - main thread pushes coords to load, workers pop
    std::queue<glm::ivec2> m_genChunkLoadQueue;
    std::mutex m_genChunkLoadQueueMutex;
    std::condition_variable m_genChunkLoadQueueCV;

    // Staging - workers push, main promotes
    std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, IVec2Hash> m_chunkLoadStaging;
    std::mutex m_chunkLoadStagingMutex;

    // In-flight set - prevents duplicate queuing
    std::unordered_set<glm::ivec2, IVec2Hash> m_chunkLoadQueued;
    std::mutex m_chunkLoadQueuedMutex;

    // Pure CPU task - worker-safe, no data races
    void FillChunkData(Chunk& chunk, glm::ivec2 coord);
    void ChunkLoadWorkerLoop();

    // ──────── Save worker ────────
    // Save IO threading
    std::queue<std::unique_ptr<Chunk>> m_saveQueue;
    std::mutex m_saveMutex;
    std::condition_variable m_saveCV;
    std::thread m_saveWorker;
    void SaveWorkerLoop();

    // ──────── Mesh worker ────────
    // Staging region
    std::unordered_map<glm::ivec2, std::vector<ChunkMesh::Vertex>, IVec2Hash> m_meshStaging;
    std::mutex m_meshStagingMutex;

    // Mesh job queue
    std::queue<MeshJob> m_meshQueue;
    std::mutex m_meshQueueMutex;
    std::condition_variable m_meshQueueCV;

    // In-flight mesh coord set - main thread must NOT unload these
    std::unordered_map<glm::ivec2, int, IVec2Hash> m_meshRefCount;
    std::mutex m_meshRefMutex;

    // Mesh workers
    std::vector<std::thread> m_meshWorkers;
    void MeshWorkerLoop();
};