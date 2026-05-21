#pragma once
#include <glad/glad.h>
#include <vector>

#include "rendering/Vertex.h"

class ChunkMesh
{
public:
    ChunkMesh() = default;
	~ChunkMesh() noexcept { Destroy(); }

	// No copying or moving - OpenGL resources should be unique and not duplicated
	ChunkMesh(const ChunkMesh&) = delete;
	ChunkMesh& operator=(const ChunkMesh&) = delete;
	ChunkMesh(ChunkMesh&&) = delete;
    ChunkMesh& operator=(ChunkMesh&&) = delete; // map[key] uses default-construct then assign — keep deleted for safety

    void Upload(const std::vector<Vertex>& vertices) noexcept;
    void Draw() const;
    void Destroy() noexcept;

    int  vertexCount = 0;
    bool valid = false;             // has uploaded data?

private:
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
};