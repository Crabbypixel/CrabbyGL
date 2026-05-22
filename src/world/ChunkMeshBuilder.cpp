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
static constexpr float SCREEN_TILE_WIDTH = TILE_WIDTH / TEXTURE_MAP_WIDTH;      // 16/256 = 0.0625 — tile width
static constexpr float SCREEN_TILE_HEIGHT = TILE_HEIGHT / TEXTURE_MAP_HEIGHT;   // 16/256 = 0.0625 — tile height

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
static constexpr glm::ivec3 NORMALS[6] = {
    { 0, 1, 0}, { 0,-1, 0},         // +Y & -Y
    { 1, 0, 0}, {-1, 0, 0},         // +X & -X
    { 0, 0, 1}, { 0, 0,-1}          // +Z & -Z
};

// Tangent directions along the face surface (local U axis)
static constexpr glm::ivec3 TANGENT_U[6] = {
    {1, 0, 0}, {1, 0, 0},           // +Y & -Y
    {0, 0, 1}, {0, 0, 1},           // +X & -X
    {1, 0, 0}, {1, 0, 0},           // +Z & -Z
};

// Tangent directions along the face surface (local V axis)
static constexpr glm::ivec3 TANGENT_V[6] = {
    {0, 0, 1}, {0, 0, 1},           // +Y & -Y
    {0, 1, 0}, {0, 1, 0},           // +X & -X
    {0, 1, 0}, {0, 1, 0},           // +Z & -Z
};

// Quad verts per face (local offsets from block origin)
static constexpr glm::ivec3 FACE_VERTS[6][4] = {
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

// Quad -> 2 tris (convert indices into 4-vert quad)
static constexpr int TRI_IDX[6] = { 0,1,2, 0,2,3 };

static constexpr int GetAOState(int side1, int side2, int corner) noexcept
{
    if (side1 + side2 == 2)
        return 0;

    return 3 - (side1 + side2 + corner);
}

// We cannot access World's IsSolid(...) so we implement a similar function here
// (x, y, z) are local chunk pos, may be -1 or CX/CZ for AO neighbor samples
// Use GetUnchecked whenever possible so as to avoid redundant bounds checks
bool ChunkMeshBuilder::IsSolidLocal(const Chunk& chunk, int x, int y, int z, const Chunk* nPX, const Chunk* nNX, const Chunk* nPZ, const Chunk* nNZ, const Chunk* nPX_PZ, const Chunk* nPX_NZ, const Chunk* nNX_PZ, const Chunk* nNX_NZ)
{
    if (y < 0 || y >= CY)
        return false;

    if (x >= 0 && x < CX && z >= 0 && z < CZ)
        return IsOpaque(chunk.GetUnchecked(x, y, z));

    // Side chunks
    if (nPX && x >= CX && z >= 0 && z < CZ)
        return IsOpaque(nPX->GetUnchecked(0, y, z));                    // RIGHT
    else if (nNX && x < 0 && z >= 0 && z < CZ)
        return IsOpaque(nNX->GetUnchecked(CX - 1, y, z));               // LEFT
    else if (nPZ && z >= CZ && x >= 0 && x < CX)
        return IsOpaque(nPZ->GetUnchecked(x, y, 0));                    // FORWARD
    else if (nNZ && z < 0 && x >= 0 && x < CX)
        return IsOpaque(nNZ->GetUnchecked(x, y, CZ - 1));               // BACK

    // Corner chunks
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

void ChunkMeshBuilder::EmitCross(std::vector<Vertex>& verts, const glm::ivec3& worldPos, BlockType type)
{
    const BlockDef& crossItem = GetDef(type);
    UVRect uv = Tile(crossItem.faces[0]);
    glm::vec3 tint = crossItem.tint;

    // Map quad corners to atlas sub-region
    glm::vec2 uvs[4] = {
        { uv.min.x, uv.min.y },   // v0 bottom-left
        { uv.max.x, uv.min.y },   // v1 bottom-right
        { uv.max.x, uv.max.y },   // v2 top-right
        { uv.min.x, uv.max.y },   // v3 top-left
    };

    glm::vec2 noOverlay[4] = { {0, 0} };

    static const glm::ivec3 CROSS_VERTS1[4] = { {0,0,1},{1,0,0},{1,1,0},{0,1,1} };
    static const glm::ivec3 CROSS_VERTS2[4] = { {0,0,0},{1,0,1},{1,1,1},{0,1,0} };
    static const glm::ivec3 DUMMY_NORMAL = { 0, 1, 0 };  // lighting unused for cross

    // Forward + reversed winding -> double-sided
    constexpr int FWD[6] = { 0,1,2, 0,2,3 };
    constexpr int REV[6] = { 0,2,1, 0,3,2 };

    auto emit = [&](const glm::ivec3 quad[4], const int idx[6]) {
        for (int i = 0; i < 6; i++)
            verts.emplace_back(Vertex{
                worldPos + quad[idx[i]],
                uvs[idx[i]],
                noOverlay[idx[i]],
                DUMMY_NORMAL,
                worldPos,
                tint,
                0.0f,   // no overlay
                0.6f    // ao = full bright
            });
        };

    emit(CROSS_VERTS1, FWD);  emit(CROSS_VERTS1, REV);
    emit(CROSS_VERTS2, FWD);  emit(CROSS_VERTS2, REV);
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
    float ao[4] = {};

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
        std::copy(tmp, tmp + 6, tri);
    }
    else
    {
        // normal
        int tmp[6] = { 0, 1, 2, 0, 2, 3 };
        std::copy(tmp, tmp + 6, tri);
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


// Builds mesh for chunks
void ChunkMeshBuilder::Build(const Chunk& chunk, const Chunk* nPX, const Chunk* nNX, const Chunk* nPZ, const Chunk* nNZ, const Chunk* nPX_PZ, const Chunk* nPX_NZ, const Chunk* nNX_PZ, const Chunk* nNX_NZ, std::vector<Vertex>& outVertices)
{
    std::shared_lock lock(chunk.chunkMutex);
    std::shared_lock lockPX = nPX ? std::shared_lock(nPX->chunkMutex) : std::shared_lock<std::shared_mutex>{};
    std::shared_lock lockNX = nNX ? std::shared_lock(nNX->chunkMutex) : std::shared_lock<std::shared_mutex>{};
    std::shared_lock lockPZ = nPZ ? std::shared_lock(nPZ->chunkMutex) : std::shared_lock<std::shared_mutex>{};
    std::shared_lock lockNZ = nNZ ? std::shared_lock(nNZ->chunkMutex) : std::shared_lock<std::shared_mutex>{};
	std::shared_lock lockPX_PZ = nPX_PZ ? std::shared_lock(nPX_PZ->chunkMutex) : std::shared_lock<std::shared_mutex>{};
    std::shared_lock lockPX_NZ = nPX_NZ ? std::shared_lock(nPX_NZ->chunkMutex) : std::shared_lock<std::shared_mutex>{};
    std::shared_lock lockNX_PZ = nNX_PZ ? std::shared_lock(nNX_PZ->chunkMutex) : std::shared_lock<std::shared_mutex>{};
    std::shared_lock lockNX_NZ = nNX_NZ ? std::shared_lock(nNX_NZ->chunkMutex) : std::shared_lock<std::shared_mutex>{};

    outVertices.clear();

    // Chunk world coordinates (not global world coordinates)
    int chunk_wx0 = chunk.chunkPos.x * CX;
    int chunk_wz0 = chunk.chunkPos.y * CZ;

    // Iterate over every block
    for (int x = 0; x < CX; x++)
    {
        for (int y = 0; y < CY; y++)
        {
            for (int z = 0; z < CZ; z++)
            {
                BlockType blockType = chunk.GetUnchecked(x, y, z);
                if (blockType == BlockType::AIR) [[likely]]     // If air, continue
                    continue;

                // Cross item
				else if (GetDef(blockType).flags & BLOCK_CROSS) [[unlikely]]
                {
                    // Local world coordinates
                    glm::ivec3 worldPos = glm::ivec3(chunk_wx0 + x, y, chunk_wz0 + z);
                    EmitCross(outVertices, worldPos, blockType);
                    continue;
                }

                // For rendering faces of translucent objects
				else if (IsTranslucent(blockType)) [[unlikely]]
                {
                    // Local world coordinates
                    glm::ivec3 worldPos = glm::ivec3(chunk_wx0 + x, y, chunk_wz0 + z);

                    // Check all six faces
                    for (int face = 0; face < 6; face++)
                    {
                        int nx = x + NORMALS[face].x;
                        int ny = y + NORMALS[face].y;
                        int nz = z + NORMALS[face].z;

                        bool shouldRenderFace = true;

                        if (Chunk::InBounds(nx, ny, nz))
                        {
                            // Neighbor is inside this chunk — safe direct access
                            auto neighbor = chunk.GetUnchecked(nx, ny, nz);

                            bool isTranslucent = IsTranslucent(neighbor);
                            bool isSolid = IsSolid(neighbor);

                            // Render the face if the neighbor is solid, or if it is translucent of the same type.
                            // Do NOT render if the neighbor is translucent and a different type (e.g., glass vs leaves).
                            shouldRenderFace = !((isSolid || isTranslucent) && !(isTranslucent && neighbor != blockType));
                        }
                        else
                        {
                            BlockType neighbor;
                            bool hasNeighbor = true;

                            // Fetch neighbor from adjacent chunk
                            if (nx < 0 && nNX)
                                neighbor = nNX->GetUnchecked(CX - 1, ny, nz);
                            else if (nx >= CX && nPX)
                                neighbor = nPX->GetUnchecked(0, ny, nz);
                            else if (nz < 0 && nNZ)
                                neighbor = nNZ->GetUnchecked(nx, ny, CZ - 1);
                            else if (nz >= CZ && nPZ)
                                neighbor = nPZ->GetUnchecked(nx, ny, 0);
                            else
                                hasNeighbor = false;

                            // Do the same here
                            if (hasNeighbor)
                            {
                                bool isTranslucent = IsTranslucent(neighbor);
                                bool isSolid = IsSolid(neighbor);

                                shouldRenderFace = !((isSolid || isTranslucent) &&
                                    !(isTranslucent && neighbor != blockType));
                            }
                            else
                            {
                                // No neighbor chunk -> face is exposed
                                shouldRenderFace = true;
                            }
                        }

                        if (shouldRenderFace)
                            AddFace(outVertices, worldPos, glm::ivec3{ x, y, z }, (Face)face, blockType, chunk, nPX, nNX, nPZ, nNZ, nPX_PZ, nPX_NZ, nNX_PZ, nNX_NZ);
                    }
                }

                // For rendering faces of opaque objects
                else
                {
                    // Local world coordinates
                    glm::ivec3 worldPos = glm::ivec3(chunk_wx0 + x, y, chunk_wz0 + z);

                    // Check all six faces
                    for (int face = 0; face < 6; face++)
                    {
                        int nx = x + NORMALS[face].x;
                        int ny = y + NORMALS[face].y;
                        int nz = z + NORMALS[face].z;

                        bool shouldRenderFace = true;

                        if (Chunk::InBounds(nx, ny, nz))
                        {
                            // Neighbor is inside this chunk — safe direct access
							shouldRenderFace = !IsOpaque(chunk.GetUnchecked(nx, ny, nz));
                        }
                        else
                        {
                            // Out of chunk bounds — query neighbor chunk
                            if (nx < 0 && nNX) shouldRenderFace = !IsOpaque(nNX->GetUnchecked(CX - 1, ny, nz));
                            else if (nx >= CX && nPX) shouldRenderFace = !IsOpaque(nPX->GetUnchecked(0, ny, nz));
                            else if (nz < 0 && nNZ) shouldRenderFace = !IsOpaque(nNZ->GetUnchecked(nx, ny, CZ - 1));
                            else if (nz >= CZ && nPZ) shouldRenderFace = !IsOpaque(nPZ->GetUnchecked(nx, ny, 0));
                        }

                        if (shouldRenderFace)
                            AddFace(outVertices, worldPos, glm::ivec3{ x, y, z }, (Face)face, blockType, chunk, nPX, nNX, nPZ, nNZ, nPX_PZ, nPX_NZ, nNX_PZ, nNX_NZ);
                    }
                }
            }
        }
    }
}