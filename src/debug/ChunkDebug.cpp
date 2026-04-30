#include "debug/ChunkDebug.h"

#include "rendering/Shader.h"
#include <vector>

#define CHUNK_SIZE 16

void ChunkDebug::Init(const std::string& shaderFile)
{
    m_shader.load(shaderFile);
    glUniformBlockBinding(m_shader.getID(), glGetUniformBlockIndex(m_shader.getID(), "Matrices"), 0);
}

void ChunkDebug::DrawChunkBoundary(const glm::vec3& playerPos)
{
    if (!visible) return;

    // Which chunk is player in?
    int cx = (int)std::floor(playerPos.x / 16.0f);
    int cz = (int)std::floor(playerPos.z / 16.0f);

    float x0 = (float)(cx * 16);
    float x1 = x0 + 16.0f;
    float z0 = (float)(cz * 16);
    float z1 = z0 + 16.0f;
    float y0 = 0.0f;
    float y1 = 256.0f;

    float verts[] = {
        // Bottom face
        x0,y0,z0,  x1,y0,z0,
        x1,y0,z0,  x1,y0,z1,
        x1,y0,z1,  x0,y0,z1,
        x0,y0,z1,  x0,y0,z0,

        // Top face
        x0,y1,z0,  x1,y1,z0,
        x1,y1,z0,  x1,y1,z1,
        x1,y1,z1,  x0,y1,z1,
        x0,y1,z1,  x0,y1,z0,

        // Vertical edges
        x0,y0,z0,  x0,y1,z0,
        x1,y0,z0,  x1,y1,z0,
        x1,y0,z1,  x1,y1,z1,
        x0,y0,z1,  x0,y1,z1,
    };

    // Lazy init box VAO once
    if (boxVAO == 0)
    {
        glGenVertexArrays(1, &boxVAO);
        glGenBuffers(1, &boxVBO);
        glBindVertexArray(boxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    // Update verts every frame (player moves)
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_shader.use();
    m_shader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));

    glBindVertexArray(boxVAO);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 24);
    glLineWidth(1.0f);
    glBindVertexArray(0);
}

void ChunkDebug::Destroy()
{
    if (boxVAO) glDeleteVertexArrays(1, &boxVAO);
    if (boxVBO) glDeleteBuffers(1, &boxVBO);
}