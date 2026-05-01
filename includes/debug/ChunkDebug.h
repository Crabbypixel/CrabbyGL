#pragma once
#include "rendering/Shader.h"

#include <glad/glad.h>

#include <string>

class ChunkDebug
{
public:
    void Init(const std::string& shaderFile);
    void DrawChunkBoundary(const glm::vec3& playerPos);
    void Destroy();

    bool visible = false;

private:
    GLuint m_VAO = 0, m_VBO = 0;
    int    m_lineCount = 0;
    Shader m_shader;
    unsigned int boxVAO, boxVBO;
};