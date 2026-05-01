#include <glm/glm.hpp>

#include "world/BlockRegistry.h"
#include "world/Chunk.h"
#include "rendering/ChunkMesh.h"
#include "world/ChunkMeshBuilder.h"
#include "world/World.h"

// UV rect per face
struct UVRect { glm::vec2 min, max; };

// Atlas constants
static constexpr float TILE_WIDTH = 16.0f;
static constexpr float TILE_HEIGHT = 16.0f;
static constexpr float TEXTURE_MAP_WIDTH = 256.0f;
static constexpr float TEXTURE_MAP_HEIGHT = 256.0f;
static constexpr float SCREEN_TILE_WIDTH = TILE_WIDTH / TEXTURE_MAP_WIDTH;   // 0.0625 — tile width
static constexpr float SCREEN_TILE_HEIGHT = TILE_HEIGHT / TEXTURE_MAP_HEIGHT;   // 0.0625 — tile height

// Get the position of texture block based on block type
static UVRect Tile(int i)
{
    const int SIZE = 16;
    int col = i % SIZE;
    int row = i / SIZE;

    float u0 = col * SCREEN_TILE_WIDTH;
    float u1 = (col + 1) * SCREEN_TILE_WIDTH;

    float v0 = 1.0f - (row + 1) * SCREEN_TILE_HEIGHT;
    float v1 = 1.0f - row * SCREEN_TILE_HEIGHT;

    return { { u0, v0 }, { u1, v1 } };
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
        return IsOpaque(chunk.GetUnchecked(x, y, z));

    // Side blocks
    if (nPX && x >= CX && z >= 0 && z < CZ)
        return IsOpaque(nPX->GetUnchecked(0, y, z));                    // RIGHT
    else if (nNX && x < 0 && z >= 0 && z < CZ)
        return IsOpaque(nNX->GetUnchecked(CX - 1, y, z));               // LEFT
    else if (nPZ && z >= CZ && x >= 0 && x < CX)
        return IsOpaque(nPZ->GetUnchecked(x, y, 0));                    // FORWARD
    else if (nNZ && z < 0 && x >= 0 && x < CX)
        return IsOpaque(nNZ->GetUnchecked(x, y, CZ - 1));               // BACK

    // Corner blocks
    else if (nPX_PZ && x >= CX && z >= CZ)
        return IsOpaque(nPX_PZ->GetUnchecked(0, y, 0));                 // FORWARD-RIGHT
    else if (nPX_NZ && x >= CX && z < 0)
        return IsOpaque(nPX_NZ->GetUnchecked(0, y, CZ - 1));            // BACK-RIGHT
    else if (nNX_PZ && x < 0 && z >= CZ)
        return IsOpaque(nNX_PZ->GetUnchecked(CX - 1, y, 0));            // FORWARD-LEFT
    else if (nNX_NZ && x < 0 && z < 0)
        return IsOpaque(nNX_NZ->GetUnchecked(CX - 1, y, CZ - 1));       // BACK-LEFT
    else
        return false;
}

void ChunkMeshBuilder::AddFace(std::vector<Vertex>& verts, const glm::ivec3& worldPos, const glm::ivec3& chunkLocalPos, Face face, BlockType type, const Chunk& chunk, const Chunk* nPX, const Chunk* nNX, const Chunk* nPZ, const Chunk* nNZ, const Chunk* nPX_PZ, const Chunk* nPX_NZ, const Chunk* nNX_PZ, const Chunk* nNX_NZ)
{
    const BlockDef& blockInfo = GetDef(type);

    // Overlay logic (only for side faces like grass)
    bool useOverlay = blockInfo.useOverlay && face > 1;

    // Base texture
    const UVRect baseRect = Tile(blockInfo.faces[face]);
    // Overlay texture
    const UVRect overlayRect = useOverlay ? Tile(blockInfo.overlay) : UVRect{ {0,0},{0,0} };

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
        verts.emplace_back(Vertex{
            worldPos + FACE_VERTS[face][i],
            baseUVs[i],                      // <- atlas sub-region now
            overlayUVs[i],
            NORMALS[face],
            worldPos,
            blockInfo.tint,
            useOverlay ? 1.0f : 0.0f,
            ao[i] / 3.0f
            });
    }
}

void ChunkMeshBuilder::Build(const Chunk& chunk, const Chunk* nPX, const Chunk* nNX, const Chunk* nPZ, const Chunk* nNZ, const Chunk* nPX_PZ, const Chunk* nPX_NZ, const Chunk* nNX_PZ, const Chunk* nNX_NZ, std::vector<Vertex>& outVertices)
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
                if (IsTransparent(blockType))        // If air, continue
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
                        isNeighborSolid = IsOpaque(chunk.GetUnchecked(nx, ny, nz));
                    }
                    else
                    {
                        // Out of chunk bounds — query neighbor chunk
                        if (nx < 0 && nNX) isNeighborSolid = IsOpaque(nNX->GetUnchecked(CX - 1, ny, nz));
                        else if (nx >= CX && nPX) isNeighborSolid = IsOpaque(nPX->GetUnchecked(0, ny, nz));
                        else if (nz < 0 && nNZ) isNeighborSolid = IsOpaque(nNZ->GetUnchecked(nx, ny, CZ - 1));
                        else if (nz >= CZ && nPZ) isNeighborSolid = IsOpaque(nPZ->GetUnchecked(nx, ny, 0));

                        // null neighbor -> isNeighborSolid stays false -> face renders (correct — exposed to unloaded chunk)
                    }

                    if (!isNeighborSolid)
                        AddFace(outVertices, worldPos, glm::ivec3{x, y, z}, (Face)face, blockType, chunk, nPX, nNX, nPZ, nNZ, nPX_PZ, nPX_NZ, nNX_PZ, nNX_NZ);
                }
            }
        }
    }
}