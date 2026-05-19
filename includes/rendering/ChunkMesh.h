#pragma once
#include <glad/glad.h>
#include <vector>

#include "rendering/Vertex.h"

class ChunkMesh
{
public:
    ChunkMesh() = default;

    void Upload(const std::vector<Vertex>& vertices) noexcept;
    void Draw() const;
    void Destroy() noexcept;

    int  vertexCount = 0;
    bool valid = false;             // has uploaded data?

private:
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
};