#pragma once
#include <glm/glm.hpp>

#include "world/BlockRegistry.h"

#include <vector>

class Chunk;
class ChunkMesh;
struct Vertex;
struct FaceCell;

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
        Chunk& chunk,
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
    using GridArray = std::array<std::array<FaceCell, CX>, CY>;     // For heap-allocating the 2D grid array

    static void AddTranslucentFace(
        std::vector<Vertex>& verts,
        const glm::ivec3& worldPos,
        const glm::ivec3& chunkLocalPos,
        int face,
        BlockType type,
        const Chunk& chunk,
        const Chunk* nPX, const Chunk* nNX,
        const Chunk* nPZ, const Chunk* nNZ,
        const Chunk* nPX_PZ, const Chunk* nPX_NZ,
        const Chunk* nNX_PZ, const Chunk* nNX_NZ
    );

    static void EmitCross(
        std::vector<Vertex>& verts,
        const glm::ivec3& worldPos,
        BlockType type
    );

    static void EmitGreedyQuad(
        int face, int layer,
        int row0, int col0, int H, int W,
        const GridArray& grid,
        int chunkWX, int chunkWZ,
        std::vector<Vertex>& out);

    static void BuildLayer(
        Chunk& chunk,
        const Chunk* nPX, const Chunk* nNX,
        const Chunk* nPZ, const Chunk* nNZ,
        const Chunk* nPX_PZ, const Chunk* nPX_NZ,
        const Chunk* nNX_PZ, const Chunk* nNX_NZ,
        int face, int layer, int chunkWX, int chunkWZ,
        std::vector<Vertex>& out
    );
};