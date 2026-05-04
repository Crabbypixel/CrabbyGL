#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb/stb_perlin.h"

#include "rendering/ChunkMesh.h"
#include "world/Chunk.h"
#include "world/Raycast.h"
#include "world/World.h"
#include "world/BlockRegistry.h"
#include "world/ChunkMeshBuilder.h"
#include "rendering/Shader.h"

World::World()
{
    chunks.reserve(1000);
    //WorldGen::SetSeed(1337);   // MUST be first
    m_saveWorker = std::thread(&World::SaveWorkerLoop, this);
}

// ───── Coord helpers ─────────────────────────────────────────────────
// Get chunk coord from world coords
glm::ivec2 World::ChunkCoord(float worldX, float worldZ)
{
    return {
        (int)std::floor(worldX / (float)CX),
        (int)std::floor(worldZ / (float)CZ)
    };
}

// Get local chunk coord from world coords
glm::ivec3 World::ChunkLocalCoord(int worldX, int worldY, int worldZ)
{
    return {
        (worldX % CX + CX) % CX,
        worldY,
        (worldZ % CZ + CZ) % CZ
    };
}

// ───── Chunk access ──────────────────────────────────────────────────
Chunk* World::GetChunk(int worldX, int worldZ)
{
    auto it = chunks.find(ChunkCoord(worldX, worldZ));
    return (it != chunks.end()) ? it->second.get() : nullptr;
}

const Chunk* World::GetChunk(int worldX, int worldZ) const
{
    auto it = chunks.find(ChunkCoord(worldX, worldZ));
    return (it != chunks.end()) ? it->second.get() : nullptr;
}

// ───── Block access ──────────────────────────────────────────────────
bool World::IsSolid(int worldX, int worldY, int worldZ) const
{
    return ::IsSolid(GetBlock(worldX, worldY, worldZ));
}

BlockType World::GetBlock(int worldX, int worldY, int worldZ) const
{
    if (worldY < 0 || worldY >= CY)
        return BlockType::AIR;

    const Chunk* chunk = GetChunk(worldX, worldZ);
    if (!chunk)
        return BlockType::AIR;

    auto l = ChunkLocalCoord(worldX, worldY, worldZ);
    return chunk->Get(l.x, l.y, l.z);
}

void World::SetBlock(int worldX, int worldY, int worldZ, BlockType type)
{
    if (worldY < 0 || worldY >= CY)
        return;

    Chunk* chunk = GetChunk(worldX, worldZ);
    if (!chunk)
        return;

    auto l = ChunkLocalCoord(worldX, worldY, worldZ);
    chunk->SetUnchecked(l.x, l.y, l.z, type);
}

// ───── World generation ────────────────────────────────────────────────────
float World::GetTerrainHeight(int wx, int wz)
{
    const float MIN_H = 20.0f;
    const float MAX_H = 120.0f;

    float fx = wx * 0.01f;
    float fz = wz * 0.01f;

    float n = stb_perlin_noise3(fx, 0.0f, fz, 0, 0, 0) * 0.5f + 0.5f;
    n += (stb_perlin_noise3(fx * 2.0f, 0.0f, fz * 2.0f, 0, 0, 0) * 0.5f + 0.5f) * 0.50f;
    n += (stb_perlin_noise3(fx * 4.0f, 0.0f, fz * 4.0f, 0, 0, 0) * 0.5f + 0.5f) * 0.25f;
    n /= 1.75f;
    n = pow(n, 2.0f);

    return MIN_H + n * (MAX_H - MIN_H);
}

// Invoked by worker
//void World::FillChunkData(Chunk& chunk, glm::ivec2 coord)
//{
//    if (LoadChunkFromDisk(chunk, coord))
//    {
//        chunk.modified = false;
//        return;
//    }
//
//    chunk.chunkPos = coord;
//
//    WorldGen::Generate(chunk, coord);
//
//    chunk.dirty = true;
//    chunk.modified = false;
//}

//// ORIGINAL
void World::FillChunkData(Chunk& chunk, glm::ivec2 coord)
{
    // Try disk first
    if (LoadChunkFromDisk(chunk, coord))
    {
        chunk.modified = false;
        return;
    }

    // Fresh Perlin gen
    chunk.chunkPos = coord;
    int cx = coord.x;
    int cz = coord.y;

    for (int x = 0; x < CX; x++)
    {
        for (int z = 0; z < CZ; z++)
        {
            int worldX = cx * CX + x;
            int worldZ = cz * CZ + z;
            int height = (int)GetTerrainHeight(worldX, worldZ);

            float n = (height - 20.0f) / (100.0f - 20.0f);
            int   thickness = 4 + (int)((1.0f - n) * 10.0f);
            int   base = std::max(1, height - thickness);

            chunk.blocks[x][0][z] = BlockType::BEDROCK;
            for (int y = 1; y < base; y++) chunk.blocks[x][y][z] = BlockType::STONE;
            for (int y = base; y < height; y++) chunk.blocks[x][y][z] = BlockType::DIRT;
            chunk.blocks[x][height][z] = BlockType::GRASS;

            if (cx * CX + x == 0 || cz * CZ + z == 0)
                chunk.blocks[x][height + 1][z] = BlockType::BRICK;

            float cave = stb_perlin_noise3(worldX * 0.05f, chunk.chunkPos.y * 0.1f, worldZ * 0.05f, 0, 0, 0);
            if (cave > 0.3f && chunk.chunkPos.y > 5 && chunk.chunkPos.y < height - 3)
                chunk.blocks[x][chunk.chunkPos.y][z] = BlockType::AIR;
        }
    }   

    chunk.dirty = true;
    chunk.modified = false;
}

//void World::FillChunkData(Chunk& chunk, glm::ivec2 coord)
//{
//    if (LoadChunkFromDisk(chunk, coord)) { chunk.modified = false; return; }
//
//    chunk.chunkPos = coord;
//    int cx = coord.x, cz = coord.y;
//
//    // Noise scales — tune these
//    constexpr float H_SCALE = 0.005f;   // horizontal frequency
//    constexpr float V_SCALE = 0.010f;   // vertical frequency (tighter = more layered)
//    constexpr float SEA_LEVEL = 64.0f;
//    constexpr float V_BIAS = 0.025f;  // how fast density drops with height
//
//    for (int x = 0; x < CX; x++)
//        for (int z = 0; z < CZ; z++)
//        {
//            int wx = cx * CX + x;
//            int wz = cz * CZ + z;
//
//            // Bedrock
//            chunk.blocks[x][0][z] = BlockType::BEDROCK;
//
//            // Surface tracking for grass placement
//            int surfaceY = -1;
//
//            for (int y = CY - 1; y >= 1; y--)
//            {
//                float fx = wx * H_SCALE;
//                float fy = y * V_SCALE;
//                float fz = wz * H_SCALE;
//
//                // Low terrain noise
//                float low = stb_perlin_noise3(fx, fy, fz, 0, 0, 0);
//                low += stb_perlin_noise3(fx * 2.0f, fy * 2.0f, fz * 2.0f, 0, 0, 0) * 0.5f;
//                low += stb_perlin_noise3(fx * 4.0f, fy * 4.0f, fz * 4.0f, 0, 0, 0) * 0.25f;
//                low /= 1.75f;
//
//                // High terrain noise (different offset = different pattern)
//                float high = stb_perlin_noise3(fx + 100.0f, fy, fz + 100.0f, 0, 0, 0);
//                high += stb_perlin_noise3(fx * 2.0f + 100.0f, fy * 2.0f, fz * 2.0f + 100.0f, 0, 0, 0) * 0.5f;
//                high /= 1.5f;
//
//                // Selector — blends between low and high
//                float sel = stb_perlin_noise3(fx * 0.5f, fy * 0.5f, fz * 0.5f, 0, 0, 0);
//                sel = (sel + 1.0f) * 0.5f;   // 0..1
//                sel = sel * sel;               // bias toward low terrain (flat majority)
//
//                float density = low + sel * (high - low);
//
//                // Y bias — pulls density down with height, creates natural ceiling
//                density -= (y - SEA_LEVEL) * V_BIAS;
//
//                if (density > 0.0f)
//                {
//                    // Determine block type — will be patched to DIRT/GRASS after
//                    chunk.blocks[x][y][z] = BlockType::STONE;
//                    if (surfaceY < 0) surfaceY = y;   // first solid from top
//                }
//                else
//                {
//                    chunk.blocks[x][y][z] = BlockType::AIR;
//                }
//            }
//
//            // Surface pass — replace top layers with dirt + grass
//            if (surfaceY > 0)
//            {
//                chunk.blocks[x][surfaceY][z] = BlockType::GRASS;
//                for (int d = 1; d <= 3 && surfaceY - d >= 1; d++)
//                    if (chunk.blocks[x][surfaceY - d][z] == BlockType::STONE)
//                        chunk.blocks[x][surfaceY - d][z] = BlockType::DIRT;
//            }
//
//            // Cave carve — inside density loop is better but this works
//            for (int y = 1; y < CY - 4; y++)
//            {
//                if (chunk.blocks[x][y][z] == BlockType::AIR) continue;
//                float cave = stb_perlin_noise3(wx * 0.05f, y * 0.08f, wz * 0.05f, 0, 0, 0);
//                if (cave > 0.35f)
//                    chunk.blocks[x][y][z] = BlockType::AIR;
//            }
//        }
//
//    chunk.dirty = true;
//    chunk.modified = false;
//}

//void World::FillChunkData(Chunk& chunk, glm::ivec2 coord)
//{
//    if (LoadChunkFromDisk(chunk, coord)) { chunk.modified = false; return; }
//
//    chunk.chunkPos = coord;
//    WorldGen::Generate(chunk, coord);
//
//    chunk.dirty = true;
//    chunk.modified = false;
//}

// ───── File IO ──────────────────────────────────────────────────
std::string World::ChunkFilePath(glm::ivec2 coord)
{
    return "saves/" + std::to_string(coord.x) + "_" + std::to_string(coord.y) + ".bin";
}

void World::SaveChunk(const Chunk& chunk)
{
    std::filesystem::create_directories("saves");
    auto path = ChunkFilePath(chunk.chunkPos);

    FILE* f = nullptr;
    fopen_s(&f, path.c_str(), "wb");

    if (!f)
        return;

    size_t bytesWritten = fwrite(chunk.blocks, sizeof(chunk.blocks), 1, f) * sizeof(chunk.blocks);

    if (bytesWritten != sizeof(chunk.blocks))
        std::cout << "Error writing to chunk " << path.c_str() << ".\n";
    fclose(f);
}

bool World::LoadChunkFromDisk(Chunk& chunk, glm::ivec2& coord)
{
    auto path = ChunkFilePath(coord);

    FILE* f = nullptr;
    fopen_s(&f, path.c_str(), "rb");
    if (!f)
        return false;

    size_t bytesRead = fread(chunk.blocks, sizeof(chunk.blocks), 1, f) * sizeof(chunk.blocks);

    if (bytesRead != sizeof(chunk.blocks))
        std::cout << "Chunk file " << path.c_str() << " is corrupted.\n";

    fclose(f);

    chunk.chunkPos = coord;
    chunk.dirty = true;

    return true;
}

void World::UnloadChunks()
{
    // Save modified chunks
    for (auto& [coord, chunk] : chunks)
    {
        if (chunk->modified)
        {
            SaveChunk(*chunk);
            chunk->modified = false;
        }
    }

    // Unload chunks from memory
    for (auto& [coord, chunk] : chunks)
    {
        m_chunkMeshes[coord].Destroy();
        m_chunkMeshes.erase(coord);
    }

    chunks.clear();
    m_chunkMeshes.clear();

    std::cout << "World saved and unloaded.\n";

}

// ───── Raycast CRUD ──────────────────────────────────────────────────
bool World::PlaceBlock(const RaycastHit& hit, BlockType type)
{
    if (!hit.hit)
        return false;

    glm::ivec3 target = hit.blockPos + hit.normal;

    if (IsSolid(target.x, target.y, target.z))
        return false;

    SetBlock(target.x, target.y, target.z, type);

    MarkNeighborChunksDirty(hit.blockPos.x, hit.blockPos.y, hit.blockPos.z);

    return true;
}

bool World::BreakBlock(const RaycastHit& hit)
{
    if (!hit.hit)
        return false;

    const BlockType& blockType = GetBlock(hit.blockPos.x, hit.blockPos.y, hit.blockPos.z);
    auto flags = GetDef(blockType).flags;

    //if (!(flags & BLOCK_SOLID) && !(flags & BLOCK_CROSS))
    //    return false;

    SetBlock(hit.blockPos.x, hit.blockPos.y, hit.blockPos.z, BlockType::AIR);

    MarkNeighborChunksDirty(hit.blockPos.x, hit.blockPos.y, hit.blockPos.z);

    return true;
}

void World::MarkNeighborChunksDirty(int wx, int wy, int wz)
{
    glm::vec3 localPos = ChunkLocalCoord(wx, wy, wz);
    int lx = localPos.x;
    int lz = localPos.z;

    if (lx == 0) { Chunk* c = GetChunk(wx - 1, wz); if (c) c->dirty = true; }
    if (lx == CX - 1) { Chunk* c = GetChunk(wx + 1, wz); if (c) c->dirty = true; }
    if (lz == 0) { Chunk* c = GetChunk(wx, wz - 1); if (c) c->dirty = true; }
    if (lz == CZ - 1) { Chunk* c = GetChunk(wx, wz + 1); if (c) c->dirty = true; }
}

// ───── Sync ──────────────────────────────────────────────────────────
void World::SyncRenderer()
{
    auto getNeighbor = [&](glm::ivec2 c) -> Chunk* {
        auto it = chunks.find(c);
        return it != chunks.end() ? it->second.get() : nullptr;
        };

    static const glm::ivec2 ND[4] = { {1,0},{-1,0},{0,1},{0,-1} };

    // Phase 1: Enqueue dirty chunks to mesh workers
    {
        std::lock_guard<std::mutex> lockQ(m_meshQueueMutex);
        std::lock_guard<std::mutex> lockR(m_meshRefMutex);

        for (auto& [chunkPos, chunk] : chunks)
        {
            // Only mesh dirty chunks
            if (!chunk->dirty)
                continue;

            // Skip if already present
            if (m_meshRefCount.count(chunkPos))
                continue;

            // Protect coord (+ 4 neighbors) from unload/deletion later by the main thread
            m_meshRefCount[chunkPos]++;
            for (auto& nd : ND)
                m_meshRefCount[chunkPos + nd]++;    // We check if this coord is present in worker thread, no need to check now

            MeshJob job;
            job.coord = chunkPos;
            job.chunk = chunk.get();
            job.nPX = getNeighbor(chunkPos + glm::ivec2{ 1,  0 });
            job.nNX = getNeighbor(chunkPos + glm::ivec2{ -1,  0 });
            job.nPZ = getNeighbor(chunkPos + glm::ivec2{ 0,  1 });
            job.nNZ = getNeighbor(chunkPos + glm::ivec2{ 0, -1 });
            job.nPX_PZ = getNeighbor(chunkPos + glm::ivec2{ 1,  1 });                  // (+X, +Z)
            job.nPX_NZ = getNeighbor(chunkPos + glm::ivec2{ 1, -1 });                  // (+X, -Z)
            job.nNX_PZ = getNeighbor(chunkPos + glm::ivec2{ -1,  1 });                  // (-X, +Z)
            job.nNX_NZ = getNeighbor(chunkPos + glm::ivec2{ -1, -1 });                  // (-X, -Z)
            m_meshQueue.push(job);

            chunk->dirty = false;
        }
    }
    m_meshQueueCV.notify_all();

    // Phase 2: Drain mesh staging & GPU upload
    // Move the meshes from the staging region to local main thread memory
    std::unordered_map<glm::ivec2, std::vector<Vertex>, IVec2Hash> ready;
    {
        std::lock_guard<std::mutex> lock(m_meshStagingMutex);
        ready.swap(m_meshStaging);
    }

    // Actually upload meshes to the GPU
    for (auto& [coord, verts] : ready)
    {
        auto it = m_chunkMeshes.find(coord);
        if (it != m_chunkMeshes.end())
        {
            it->second.Build(verts);
        }
    }
}

// ───── Textures & Draw ───────────────────────────────────────────────
void World::SetChunkShader(Shader& shader)
{
    m_chunkShader = &shader;
    shader.use();
    shader.setInt("u_atlas", 2);   // single sampler, slot 2
}

void World::LoadAtlasTexture(const char* path)
{
    stbi_set_flip_vertically_on_load(true);   // GL origin = bottom-left

    int w, h, channels;
    unsigned char* data = stbi_load(path, &w, &h, &channels, 0);
    if (!data) {
        std::cout << "Atlas load failed: " << path << '\n';
        return;
    }

    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    GLenum fmt = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Nearest-neighbor — keeps pixel art crisp
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 2);
    //glTexParameter f(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -1.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(data);

    m_atlasTexture = id;
}

void World::DrawAll(const glm::mat4& proj, const glm::mat4& view)
{
    if (!m_chunkShader)
        return;

    m_chunkShader->use();

    // Bind textures once — shared across all chunk draw calls
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m_atlasTexture);

    // Extract frustum planes
    m_frustum.Extract(proj * view);

    // Draw all meshes (computed earlier)
    for (auto& [chunkPos, mesh] : m_chunkMeshes)
    {
        glm::vec3 minP = { chunkPos.x * CX,    0,  chunkPos.y * CZ };
        glm::vec3 maxP = { chunkPos.x * CX + CX, CY, chunkPos.y * CZ + CZ };

        // Implement frustum culling
        if (!m_frustum.ContainsAABB(minP, maxP)) continue;

        mesh.Draw();
    }
}

// ───── Frustum Culling ───────────────────────────────────────────────
void Frustum::Extract(const glm::mat4& vp)
{
    planes[0] = glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]); // left
    planes[1] = glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]); // right
    planes[2] = glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]); // bottom
    planes[3] = glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]); // top
    planes[4] = glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]); // near
    planes[5] = glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]); // far

    for (auto& p : planes) p /= glm::length(glm::vec3(p));
}

bool Frustum::ContainsAABB(const glm::vec3& minP, const glm::vec3& maxP) const
{
    for (const auto& plane : planes)
    {
        glm::vec3 pv = {
            plane.x >= 0 ? maxP.x : minP.x,
            plane.y >= 0 ? maxP.y : minP.y,
            plane.z >= 0 ? maxP.z : minP.z
        };
        if (glm::dot(glm::vec3(plane), pv) + plane.w < 0.0f)
            return false;
    }
    return true;
}

// ───── Infinite World ───────────────────────────────────────────────
void World::UpdateChunkStreaming(const glm::vec3& playerPos)
{
    glm::ivec2 playerChunkCoord = ChunkCoord(playerPos.x, playerPos.z);

    if (playerChunkCoord == m_lastPlayerChunk)
    {
        bool hasAllChunksLoaded = true;
        std::lock_guard<std::mutex> lk(m_chunkLoadQueuedMutex);
        {
            for (int dx = -m_viewDist; dx <= m_viewDist && hasAllChunksLoaded; dx++)
            {
                for (int dz = -m_viewDist; dz <= m_viewDist && hasAllChunksLoaded; dz++)
                {
                    if (dx * dx + dz * dz > m_viewDist * m_viewDist)
                        continue;

                    glm::ivec2 c = { playerChunkCoord.x + dx, playerChunkCoord.y + dz };

                    // If player chunk is NOT found in both core chunk data or queued data for loading -> not loaded
                    // Else, proceed with load/unload below
                    if (!chunks.count(c) && !m_chunkLoadQueued.count(c))
                        hasAllChunksLoaded = false;
                }
            }
            if (hasAllChunksLoaded)
            {
                return;
            }
        }
    }
    else
    {
        m_lastPlayerChunk = playerChunkCoord;
    }

    // Make list of chunks to unload
    std::vector<glm::ivec2> chunksToUnload;
    for (auto& [chunkCoord, _] : chunks)
    {
        glm::ivec2 d = chunkCoord - playerChunkCoord;

        if (d.x * d.x + d.y * d.y > m_unloadDist * m_unloadDist)
            chunksToUnload.push_back(chunkCoord);
    }

    // Actually unload
    for (auto& chunkCoord : chunksToUnload)
    {
        // Guard check - if chunkCoord is also present in meshRefCount, don't unload
        {
            std::lock_guard<std::mutex> lock(m_meshRefMutex);
            if (m_meshRefCount.count(chunkCoord))
                continue;       // Deferred - retry this chunk coord next time when worker is done
        }

        // Else, proceed with unloading (unload only if chunk is modified)
        auto it = chunks.find(chunkCoord);
        if (it != chunks.end() && it->second->modified)
        {
            // Move the chunk to the unloading queue, save worker thread will unload
            std::lock_guard<std::mutex> lk(m_saveMutex);
            m_saveQueue.push(std::move(it->second));
            m_saveCV.notify_all();
        }

        // Remove chunk from memory
        m_chunkMeshes[chunkCoord].Destroy();
        m_chunkMeshes.erase(chunkCoord);
        chunks.erase(chunkCoord);
    }

    // Make list of chunks to load
    std::vector<glm::ivec2> chunksToLoad;
    for (int dx = -m_viewDist; dx <= m_viewDist; dx++)
    {
        for (int dz = -m_viewDist; dz <= m_viewDist; dz++)
        {
            if (dx * dx + dz * dz > m_viewDist * m_viewDist)
                continue;

            glm::ivec2 chunkCoord = { playerChunkCoord.x + dx, playerChunkCoord.y + dz };

            // If chunk coord isn't present, load it
            if (!chunks.count(chunkCoord))
                chunksToLoad.push_back(chunkCoord);
        }
    }

    // Prioritize chunks close to the player - sort the list from nearest to load first then at the end
    std::sort(chunksToLoad.begin(), chunksToLoad.end(), [&](const glm::ivec2& a, const glm::ivec2& b) {
        glm::ivec2 da = a - playerChunkCoord, db = b - playerChunkCoord;
        return da.x * da.x + da.y * da.y < db.x * db.x + db.y * db.y;
        });

    // Actually load
    {
        std::lock_guard<std::mutex> lkQ(m_genChunkLoadQueueMutex);
        std::lock_guard<std::mutex> lkS(m_chunkLoadQueuedMutex);

        for (auto& chunkCoord : chunksToLoad)
        {
            // If chunks is already queued for loading then skip
            if (m_chunkLoadQueued.count(chunkCoord))
                continue;

            // Queue the coord of the to-be-loaded chunk
            m_genChunkLoadQueue.push(chunkCoord);
            m_chunkLoadQueued.insert(chunkCoord);

            // Mark neighboring chunks dirty because faces at chunk borders depend on adjacent chunk data
            // When a chunk changes, neighbors may need to rebuild meshes for correct face culling
            // Hence mark them dirty so the renderer will re-build the neighbor chunk meshes
            auto it = chunks.find(chunkCoord + glm::ivec2{ 1,0 }); if (it != chunks.end()) it->second->dirty = true;
            it = chunks.find(chunkCoord + glm::ivec2{ 0,1 }); if (it != chunks.end()) it->second->dirty = true;
            it = chunks.find(chunkCoord + glm::ivec2{ -1,0 }); if (it != chunks.end()) it->second->dirty = true;
            it = chunks.find(chunkCoord + glm::ivec2{ 0,-1 }); if (it != chunks.end()) it->second->dirty = true;
        }
    }
    m_genChunkLoadQueueCV.notify_all();
}

void World::CommitGeneratedChunks()
{
    // Move the chunks from the staging region (done by loading workers)
    // to local main thread memory - directly accessing staging region leads to data races
    std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, IVec2Hash> queued;
    {
        std::lock_guard<std::mutex> lk(m_chunkLoadStagingMutex);
        queued.swap(m_chunkLoadStaging);
    }

    // Actually move to the core chunk data
    // Mark the chunks dirty for meshing
    for (auto& [coord, chunkPtr] : queued)
    {
        chunks[coord] = std::move(chunkPtr);
        m_chunkMeshes[coord];
        chunks[coord]->dirty = true;

        /*
        * THIS IS VERY IMPORTANT AS - IT HAS RESULTED IN A SERIOUS BUG
        * !!! Queue cleanup must happen ONLY on the main thread after promotion.
        *
        * Context:
        * - m_chunkLoadQueued tracks chunk coords that are currently scheduled or in-flight.
        * - Gen workers:
        *     1) generate chunk data
        *     2) push result -> m_chunkLoadStaging (unique_ptr<Chunk>)
        *     3) DO NOT erase from m_chunkLoadQueued
        *
        * Why NOT erase in worker thread?
        * There exists a critical race window:
        *
        *   Worker thread:
        *     push to staging
        *     erase coord from m_chunkLoadQueued   <- (BAD if done here)
        *
        *   Main thread (same frame):
        *     chunks.count(coord) == 0      (not promoted yet)
        *     m_chunkLoadQueued.count(coord) == 0
        *     → re-enqueues SAME coord
        *
        * Result:
        * - Same chunk generated twice
        * - Two unique_ptr<Chunk> created for same coord
        * - One overwrites the other during promotion
        * - Leads to use-after-free / dangling pointer / undefined behavior
        *
        * Correct ordering (this block enforces it):
        *   Worker:   generate -> push to staging (coord remains in queued set)
        *   Main:     promote staging -> insert into `chunks`
        *             THEN erase coord from m_chunkLoadQueued  <- SAFE POINT
        *
        * Guarantee:
        * - coord remains "reserved" until it is fully visible in `chunks`
        * - prevents duplicate generation
        * - closes the race window between staging and promotion
        *
        * Summary:
        * m_chunkLoadQueued is not just a queue — it is a "reservation set".
        * Removal must be delayed until AFTER successful promotion on the main thread.
        */
        {
            std::lock_guard<std::mutex> lk(m_chunkLoadQueuedMutex);
            m_chunkLoadQueued.erase(coord);
        }
    }
}

// ───── Multithreading ───────────────────────────────────────────────
void World::StartChunkLoadWorkers(int count)
{
    m_shutdown = false;
    for (int i = 0; i < count; i++)
        m_chunkLoadWorkers.emplace_back([this] { ChunkLoadWorkerLoop(); });
}


void World::StopAllWorkers()
{
    // Initiate shutdown
    m_shutdown = true;

    // Wake all sleeping workers so they exit
    m_genChunkLoadQueueCV.notify_all();
    m_saveCV.notify_all();
    m_meshQueueCV.notify_all();

    // Join the save worker
    if (m_saveWorker.joinable())
    {
        m_saveWorker.join();
        std::cout << "Save worker joined.\n";
    }

    // Join mesh workers
    for (auto& t : m_meshWorkers)
    {
        if (t.joinable())
            t.join();
    }

    m_meshWorkers.clear();

    // Join chunk load workers
    for (auto& t : m_chunkLoadWorkers)
    {
        if (t.joinable())
            t.join();
    }

    m_chunkLoadWorkers.clear();
}

void World::ChunkLoadWorkerLoop()
{
    while (true)
    {
        // Get the coord of the chunk to be loaded safely
        glm::ivec2 coord;
        {
            std::unique_lock<std::mutex> lk(m_genChunkLoadQueueMutex);
            m_genChunkLoadQueueCV.wait(lk, [&] {         // Wakeup when queue is NOT empty or when shutdown triggered
                return !m_genChunkLoadQueue.empty() || m_shutdown;
                });

            if (m_shutdown && m_genChunkLoadQueue.empty())
                break;

            coord = m_genChunkLoadQueue.front();
            m_genChunkLoadQueue.pop();
        }

        // Fill into worker-local chunk
        auto chunk = std::make_unique<Chunk>();
        chunk->chunkPos = coord;

        // No need to lock this, no shared resource used in this function
        FillChunkData(*chunk, coord);

        // Move the chunk to staged section and remove the coord from queue
        {
            std::lock_guard<std::mutex> lk(m_chunkLoadStagingMutex);
            m_chunkLoadStaging[coord] = std::move(chunk);        // Move semantics, O(1) operation
        }
    }
}

void World::SaveWorkerLoop()
{
    while (true)
    {
        std::unique_ptr<Chunk> chunk;

        // Safely get the chunk from the save queue
        {
            std::unique_lock<std::mutex> lk(m_saveMutex);
            m_saveCV.wait(lk, [&] {
                return !m_saveQueue.empty() || m_shutdown;
                });

            if (m_shutdown && m_saveQueue.empty())
                break;

            std::cout << "Saving...\n";

            chunk = std::move(m_saveQueue.front());
            m_saveQueue.pop();
        }

        // Save the chunk to disk
        SaveChunk(*chunk);
    }
}

void World::MeshWorkerLoop()
{
    std::vector<Vertex> verts;
    verts.reserve(CX * CY * CZ * 3);

    while (true)
    {
        // Contains information about chunk coord, pointers to 
        // current chunk and all its 4 neighbors 
        // (no need to access map - reduced hashing)
        MeshJob job;

        {
            std::unique_lock<std::mutex> lock(m_meshQueueMutex);
            m_meshQueueCV.wait(lock, [&] {          // Wait until mesh queue is empty or shutdown is NOT triggered
                return !m_meshQueue.empty() || m_shutdown;
                });

            if (m_shutdown && m_meshQueue.empty())
                break;

            // Pop out a mesh job from the queue
            // for further processing - generate mesh
            job = m_meshQueue.front();
            m_meshQueue.pop();
        }

        // Now coord is owned, build the vertices
        {
            verts.clear();

            if (job.chunk)
                ChunkMeshBuilder::Build(
                    *job.chunk,
                    job.nPX, job.nNX,
                    job.nPZ, job.nNZ,
                    job.nPX_PZ, job.nPX_NZ,
                    job.nNX_PZ, job.nNX_NZ,
                    verts);
        }

        // Push verts to staging
        {
            std::lock_guard<std::mutex> lock(m_meshStagingMutex);
            //m_meshStaging[job.coord] = (verts);
            m_meshStaging[job.coord] = std::move(verts);
        }

        // Decrement refcounts - so that the chunk can be unloaded (unguard now)
        {
            static const glm::ivec2 ND[4] = { {1,0},{-1,0},{0,1},{0,-1} };
            std::lock_guard<std::mutex> lock(m_meshRefMutex);

            // Decrement coord itself
            m_meshRefCount[job.coord]--;
            if (m_meshRefCount[job.coord] == 0)
                m_meshRefCount.erase(job.coord);

            // Decrement neighbors
            for (auto& nd : ND)
            {
                glm::ivec2 nb = job.coord + nd;
                auto it = m_meshRefCount.find(nb);
                if (it != m_meshRefCount.end())
                {
                    (it->second)--;
                    if (it->second <= 0)
                        m_meshRefCount.erase(it->first);
                }
            }
        }
    }
}

void World::StartMeshWorkers(int count = 2)
{
    for (int i = 0; i < count; i++)
        m_meshWorkers.emplace_back([this] { MeshWorkerLoop(); });
}