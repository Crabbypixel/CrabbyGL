#include "world/ChunkMeshBuilder.h"
#include "world/World.h"
#include <glm/glm.hpp>

// UV rect per face
struct UVRect { glm::vec2 min, max; };

// Atlas constants
static constexpr float TW = 16.0f / 256.0f;   // 0.0625 — tile width
static constexpr float TH = 16.0f / 256.0f;   // 0.0625 — tile height
static constexpr float TV0 = 1.0f - TH;       // 0.9375 — v bottom of tile row
static constexpr float TV1 = 1.0f;            // v top

// Get the position of texture block based on block type
static UVRect Tile(int i)
{
    return { { i * TW, TV0 }, { (i + 1) * TW, TV1 } };
}

// Default grass tint
static const glm::vec3 GRASS_TINT = { 0.55f, 0.78f, 0.28f };
//static const glm::vec3 GRASS_TINT = { 0.72f, 0.74f, 0.30f };

// Tile indices - block type
static constexpr int TEXTURE_DIRT = 0;
static constexpr int TEXTURE_GRASS_TOP = 1;
static constexpr int TEXTURE_GRASS_SIDE_OVERLAY = 2;
static constexpr int TEXTURE_STONE = 3;
static constexpr int TEXTURE_BEDROCK = 4;
static constexpr int TEXTURE_BRICK = 5;
static constexpr int TEXTURE_TREE_LOG_SIDES = 6;
static constexpr int TEXTURE_TREE_LOG_TOP = 7;
static constexpr int TEXTURE_TREE_LEAVES = 8;

// Face order: +Y -Y +X -X +Z -Z
static UVRect GetBlockFaceUV(BlockType type, int face)
{
    switch (type)
    {
    case BlockType::GRASS:
        if (face == 0) return Tile(TEXTURE_GRASS_TOP);    // +Y
        if (face == 1) return Tile(TEXTURE_DIRT);         // -Y
        return Tile(TEXTURE_DIRT);          // sides

    case BlockType::DIRT:
        return Tile(TEXTURE_DIRT);

    case BlockType::STONE:
        return Tile(TEXTURE_STONE);

    case BlockType::BEDROCK:
        return Tile(TEXTURE_BEDROCK);

    case BlockType::BRICK:
        return Tile(TEXTURE_BRICK);

    case BlockType::TREE_LOG:
        if (face == 0 || face == 1) return Tile(TEXTURE_TREE_LOG_TOP);      // +Y & -Y
        return Tile(TEXTURE_TREE_LOG_SIDES);                                // +X, -X, +Z, -Z

    case BlockType::TREE_LEAVES:
        return Tile(TEXTURE_TREE_LEAVES);

    default:
        return Tile(TEXTURE_DIRT);
    }
}

// 6 faces: +Y -Y +X -X +Z -Z
// Add this to the current face to go to the neighboring block to that face
// Each face = 4 verts -> 6 indices (2 tris) baked as 6 verts
static const glm::ivec3 NORMALS[6] = {
    { 0, 1, 0}, { 0,-1, 0},         // +Y & -Y
    { 1, 0, 0}, {-1, 0, 0},         // +X & -X
    { 0, 0, 1}, { 0, 0,-1}          // +Z & -Z
};

// Tangent directions along the face surface (local U axis)
static const glm::ivec3 TANGENT_U[6] = {
    {1, 0, 0}, {1, 0, 0},           // +Y & -Y
    {0, 0, 1}, {0, 0, 1},           // +X & -X
    {1, 0, 0}, {1, 0, 0},           // +Z & -Z
};

// Tangent directions along the face surface (local V axis)
static const glm::ivec3 TANGENT_V[6] = {
    {0, 0, 1}, {0, 0, 1},           // +Y & -Y
    {0, 1, 0}, {0, 1, 0},           // +X & -X
    {0, 1, 0}, {0, 1, 0},           // +Z & -Z
};

// Quad verts per face (local offsets from block origin)
static const glm::ivec3 FACE_VERTS[6][4] = {
    // +Y top
    {{0,1,0},{1,1,0},{1,1,1},{0,1,1}},
    // -Y bottom
    {{0,0,1},{1,0,1},{1,0,0},{0,0,0}},

    // +X right
    {{1,0,0},{1,0,1},{1,1,1},{1,1,0}},
    // -X left
    {{0,0,1},{0,0,0},{0,1,0},{0,1,1}},

    // +Z front
    {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},
    // -Z back
    {{1,0,0},{0,0,0},{0,1,0},{1,1,0}}
};

// Quad -> 2 tris (indices into 4-vert quad)
static const int TRI_IDX[6] = { 0,1,2, 0,2,3 };

static const int GetAOState(int side1, int side2, int corner) {
    if (side1 + side2 == 2)
        return 0;

    return 3 - (side1 + side2 + corner);
}

// We cannot access World's IsSolid(...) so we implement a similar function here
// (x, y, z) are local chunk pos, may be -1 or CX/CZ for AO neighbor samples
bool ChunkMeshBuilder::IsSolidLocal(const Chunk& chunk, int x, int y, int z, const Chunk* nPX, const Chunk* nNX, const Chunk* nPZ, const Chunk* nNZ, const Chunk* nPX_PZ, const Chunk* nPX_NZ, const Chunk* nNX_PZ, const Chunk* nNX_NZ)
{
    if (y < 0 || y >= CY)
        return false;

    if (x >= 0 && x < CX && z >= 0 && z < CZ)
        return chunk.GetUnchecked(x, y, z) != BlockType::AIR;

    // Side blocks
    if (nPX && x >= CX && z >= 0 && z < CZ)
        return nPX->GetUnchecked(0, y, z) != BlockType::AIR;                    // RIGHT
    else if (nNX && x < 0 && z >= 0 && z < CZ)
        return nNX->GetUnchecked(CX - 1, y, z) != BlockType::AIR;               // LEFT
    else if (nPZ && z >= CZ && x >= 0 && x < CX)
        return nPZ->GetUnchecked(x, y, 0) != BlockType::AIR;                    // FORWARD
    else if (nNZ && z < 0 && x >= 0 && x < CX)
        return nNZ->GetUnchecked(x, y, CZ - 1) != BlockType::AIR;               // BACK

    // Corner blocks
    else if (nPX_PZ && x >= CX && z >= CZ)
        return nPX_PZ->GetUnchecked(0, y, 0) != BlockType::AIR;                 // FORWARD-RIGHT
    else if (nPX_NZ && x >= CX && z < 0)
        return nPX_NZ->GetUnchecked(0, y, CZ - 1) != BlockType::AIR;            // BACK-RIGHT
    else if (nNX_PZ && x < 0 && z >= CZ)
        return nNX_PZ->GetUnchecked(CX - 1, y, 0) != BlockType::AIR;            // FORWARD-LEFT
    else if (nNX_NZ && x < 0 && z < 0)
        return nNX_NZ->GetUnchecked(CX - 1, y, CZ - 1) != BlockType::AIR;       // BACK-LEFT
    else
        return false;
}

void ChunkMeshBuilder::AddFace(std::vector<ChunkMesh::Vertex>& verts, const glm::ivec3& worldPos, const glm::ivec3& chunkLocalPos, Face face, BlockType type, const Chunk& chunk, const Chunk* nPX, const Chunk* nNX, const Chunk* nPZ, const Chunk* nNZ, const Chunk* nPX_PZ, const Chunk* nPX_NZ, const Chunk* nNX_PZ, const Chunk* nNX_NZ)
{

    bool isGrassSide = (type == BlockType::GRASS && face > 1);  // Only sides

    const UVRect baseRect = isGrassSide ? Tile(TEXTURE_DIRT) : GetBlockFaceUV(type, face);
    const UVRect overlayRect = isGrassSide ? Tile(TEXTURE_GRASS_SIDE_OVERLAY) : Tile(0);

    // Map quad corners to atlas sub-region
    glm::vec2 baseUVs[4] = {
        {baseRect.min.x, baseRect.min.y},   // v0 bottom-left
        {baseRect.max.x, baseRect.min.y},   // v1 bottom-right
        {baseRect.max.x, baseRect.max.y},   // v2 top-right
        {baseRect.min.x, baseRect.max.y},   // v3 top-left
    };

    glm::vec2 overlayUVs[4] = {
        {overlayRect.min.x, overlayRect.min.y},   // v0 bottom-left
        {overlayRect.max.x, overlayRect.min.y},   // v1 bottom-right
        {overlayRect.max.x, overlayRect.max.y},   // v2 top-right
        {overlayRect.min.x, overlayRect.max.y},   // v3 top-left
    };

    // Ambient Occlusion
    float ao[4];

    glm::ivec3 U = TANGENT_U[face];
    glm::ivec3 V = TANGENT_V[face];
    glm::ivec3 N = NORMALS[face];

    for (int i = 0; i < 4; i++)
    {
        glm::ivec3 v = FACE_VERTS[face][i];
        int du = (glm::dot(glm::vec3(v), glm::vec3(U)) > 0.5f) ? 1 : -1;
        int dv = (glm::dot(glm::vec3(v), glm::vec3(V)) > 0.5f) ? 1 : -1;

        // These are now in world coordinates
        glm::ivec3 base = chunkLocalPos + NORMALS[face];
        glm::ivec3 side1 = base + du * U;
        glm::ivec3 side2 = base + dv * V;
        glm::ivec3 corner = base + du * U + dv * V;

        int s1 = IsSolidLocal(chunk, side1.x, side1.y, side1.z, nPX, nNX, nPZ, nNZ, nPX_PZ, nPX_NZ, nNX_PZ, nNX_NZ);
        int s2 = IsSolidLocal(chunk, side2.x, side2.y, side2.z, nPX, nNX, nPZ, nNZ, nPX_PZ, nPX_NZ, nNX_PZ, nNX_NZ);
        int c = IsSolidLocal(chunk, corner.x, corner.y, corner.z, nPX, nNX, nPZ, nNZ, nPX_PZ, nPX_NZ, nNX_PZ, nNX_NZ);

        ao[i] = GetAOState(s1, s2, c);
    }

    int tri[6];

    if (ao[0] + ao[2] > ao[1] + ao[3])
    {
        // flipped
        int tmp[6] = { 0, 1, 3, 1, 2, 3 };
        memcpy(tri, tmp, sizeof(tri));
    }
    else
    {
        // normal
        int tmp[6] = { 0, 1, 2, 0, 2, 3 };
        memcpy(tri, tmp, sizeof(tri));
    }

    for (int i : tri)
    {
        verts.emplace_back(ChunkMesh::Vertex{
            worldPos + FACE_VERTS[face][i],
            baseUVs[i],                      // <- atlas sub-region now
            overlayUVs[i],
            NORMALS[face],
            worldPos,
            (type == BlockType::GRASS && face != Face::BOTTOM) ? GRASS_TINT : glm::vec3(1.0f),
            isGrassSide ? 1.0f : 0.0f,
            ao[i] / 3.0f
            });
    }
}

void ChunkMeshBuilder::Build(const Chunk& chunk, const Chunk* nPX, const Chunk* nNX, const Chunk* nPZ, const Chunk* nNZ, const Chunk* nPX_PZ, const Chunk* nPX_NZ, const Chunk* nNX_PZ, const Chunk* nNX_NZ, std::vector<ChunkMesh::Vertex>& outVertices)
{
    std::shared_lock lock(chunk.chunkMutex);
    std::shared_lock lockPX = nPX ? std::shared_lock(nPX->chunkMutex) : std::shared_lock<std::shared_mutex>{};
    std::shared_lock lockNX = nNX ? std::shared_lock(nNX->chunkMutex) : std::shared_lock<std::shared_mutex>{};
    std::shared_lock lockPZ = nPZ ? std::shared_lock(nPZ->chunkMutex) : std::shared_lock<std::shared_mutex>{};
    std::shared_lock lockNZ = nNZ ? std::shared_lock(nNZ->chunkMutex) : std::shared_lock<std::shared_mutex>{};
        
    outVertices.clear();

    // Chunk world coordinates (not global world coordinates)
    int wx0 = chunk.chunkPos.x * CX;
    int wz0 = chunk.chunkPos.y * CZ;

    // Iterate over every block
    for (int x = 0; x < CX; x++)
    {
        for (int y = 0; y < CY; y++)
        {
            for (int z = 0; z < CZ; z++)
            {
                BlockType blockType = chunk.Get(x, y, z);
                if (blockType == BlockType::AIR)        // If air, continue
                    continue;

                // Local world coordinates
                glm::ivec3 worldPos = glm::vec3(wx0 + x, y, wz0 + z);

                // Check all six faces
                for (int face = 0; face < 6; face++)
                {
                    int nx = x + NORMALS[face].x;
                    int ny = y + NORMALS[face].y;
                    int nz = z + NORMALS[face].z;

                    bool isNeighborSolid = false;

                    if (Chunk::InBounds(nx, ny, nz))
                    {
                        // Neighbor is inside this chunk — safe direct access
                        isNeighborSolid = chunk.GetUnchecked(nx, ny, nz) != BlockType::AIR;
                    }
                    else
                    {
                        // Out of chunk bounds — query neighbor chunk
                        if (nx < 0 && nNX) isNeighborSolid = nNX->GetUnchecked(CX - 1, ny, nz) != BlockType::AIR;
                        else if (nx >= CX && nPX) isNeighborSolid = nPX->GetUnchecked(0, ny, nz) != BlockType::AIR;
                        else if (nz < 0 && nNZ) isNeighborSolid = nNZ->GetUnchecked(nx, ny, CZ - 1) != BlockType::AIR;
                        else if (nz >= CZ && nPZ) isNeighborSolid = nPZ->GetUnchecked(nx, ny, 0) != BlockType::AIR;

                        // null neighbor -> isNeighborSolid stays false -> face renders (correct — exposed to unloaded chunk)
                    }

                    if (!isNeighborSolid)
                        AddFace(outVertices, worldPos, glm::ivec3{x, y, z}, (Face)face, blockType, chunk, nPX, nNX, nPZ, nNZ, nPX_PZ, nPX_NZ, nNX_PZ, nNX_NZ);
                }
            }
        }
    }
}