#pragma once
#include <glad/glad.h>
#include <vector>

#include "rendering/Vertex.h"

class ChunkMesh
{
public:
    void Upload(const std::vector<Vertex>& vertices);
    void Draw() const;
    void Destroy();

    int  vertexCount = 0;
    bool valid = false;             // has uploaded data?

private:
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
};