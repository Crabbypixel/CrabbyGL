#include "rendering/ChunkMesh.h"
#include "rendering/Shader.h"

#include <glm/glm.hpp>

void ChunkMesh::Upload(const std::vector<Vertex>& vertices) noexcept
{
    vertexCount = (int)vertices.size();
    if (vertexCount == 0) { valid = false; Destroy(); return; }

    if (m_VAO == 0)
    {
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
    }

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
    
    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    // baseUV
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, baseUV));
    glEnableVertexAttribArray(1);
    // overlayUV
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, overlayUV));
    glEnableVertexAttribArray(2);
    // normal
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(3);
    // blockOrigin
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, blockOrigin));
    glEnableVertexAttribArray(4);
    // tint
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tint));
    glEnableVertexAttribArray(5);
    // useOverlay
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, useOverlay));
    glEnableVertexAttribArray(6);
    // AO
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, ao));
    glEnableVertexAttribArray(7);

    glBindVertexArray(0);
    valid = true;
}

void ChunkMesh::Draw() const
{
    if (!valid || vertexCount == 0)
        return;
    
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

void ChunkMesh::Destroy() noexcept
{
    if (m_VAO) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
    if (m_VBO) { glDeleteBuffers(1, &m_VBO);      m_VBO = 0; }
    valid = false;
}
