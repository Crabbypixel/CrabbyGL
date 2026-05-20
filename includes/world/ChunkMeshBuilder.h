#pragma once
#include <vector>

class Chunk;
class ChunkMesh;
struct Vertex;

class ChunkMeshBuilder
{
public:
    ChunkMeshBuilder() = default;

    // Non-copyable, non-movable
	ChunkMeshBuilder(const ChunkMeshBuilder&) = delete;
	ChunkMeshBuilder(ChunkMeshBuilder&&) = delete;
	ChunkMeshBuilder& operator=(const ChunkMeshBuilder&) = delete;
	ChunkMeshBuilder& operator=(ChunkMeshBuilder&&) = delete;

    // Builds mesh for chunk, queries world for cross-chunk neighbors
    // The pointers point to neighboring chunks, caching for performance (no need of map lookups)
    static void Build(
        const Chunk& chunk,
        const Chunk* nPX, const Chunk* nNX,
        const Chunk* nPZ, const Chunk* nNZ,
        const Chunk* nPX_PZ, const Chunk* nPX_NZ,
        const Chunk* nNX_PZ, const Chunk* nNX_NZ,
        std::vector<Vertex>& outVertices
    );

private:
    enum Face {
        TOP = 0,
        BOTTOM = 1,
        POS_X = 2,
        NEG_X = 3,
        POS_Z = 4,
        NEG_Z = 5
    };

    static void AddFace(
        std::vector<Vertex>& verts,
        const glm::ivec3& pos,
        const glm::ivec3& chunkLocalPos,
        Face face,
        BlockType type, const Chunk& chunk,
        const Chunk* nPX, const Chunk* nNX,
        const Chunk* nPZ, const Chunk* nNZ,
        const Chunk* nPX_PZ, const Chunk* nPX_NZ,
        const Chunk* nNX_PZ, const Chunk* nNX_NZ
    );

    [[nodiscard]]
    static bool IsSolidLocal(const Chunk& chunk,
        int x, int y, int z,
        const Chunk* nPX, const Chunk* nNX,
        const Chunk* nPZ, const Chunk* nNZ,
        const Chunk* nPX_PZ, const Chunk* nPX_NZ,
        const Chunk* nNX_PZ, const Chunk* nNX_NZ
    );

    static void EmitCross(std::vector<Vertex>& verts,
        const glm::ivec3& worldPos,
        BlockType type
    );
};