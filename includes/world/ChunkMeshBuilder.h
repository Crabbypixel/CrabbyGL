#pragma once
class Chunk;
class ChunkMesh;

class ChunkMeshBuilder
{
public:
    // Builds mesh for chunk, queries world for cross-chunk neighbors
    // The pointers point to neighboring chunks, caching for performance (no need of map lookups)
    static void Build(
        const Chunk& chunk,
        const Chunk* nPX, const Chunk* nNX,
        const Chunk* nPZ, const Chunk* nNZ,
        const Chunk* nPX_PZ, const Chunk* nPX_NZ,
        const Chunk* nNX_PZ, const Chunk* nNX_NZ,
        std::vector<ChunkMesh::Vertex>& outVertices
    );

private:
    static enum Face {
        TOP = 0,
        BOTTOM = 1,
        POS_X = 2,
        NEG_X = 3,
        POS_Z = 4,
        NEG_Z = 5
    };

    static void AddFace(
        std::vector<ChunkMesh::Vertex>& verts,
        const glm::ivec3& pos,
        const glm::ivec3& chunkLocalPos,
        Face face,
        BlockType type, const Chunk& chunk,
        const Chunk* nPX, const Chunk* nNX,
        const Chunk* nPZ, const Chunk* nNZ,
        const Chunk* nPX_PZ, const Chunk* nPX_NZ,
        const Chunk* nNX_PZ, const Chunk* nNX_NZ
    );

    static bool IsSolidLocal(const Chunk& chunk,
        int x, int y, int z,
        const Chunk* nPX, const Chunk* nNX,
        const Chunk* nPZ, const Chunk* nNZ,
        const Chunk* nPX_PZ, const Chunk* nPX_NZ,
        const Chunk* nNX_PZ, const Chunk* nNX_NZ
    );
};