#include "rendering/ChunkMesh.h"
#include "rendering/Shader.h"

#include <glm/glm.hpp>

// Attrib layout mirrors Vertex struct exactly — offsets auto-computed with offsetof()
// Two new attribs (uvTileMin=2, uvTileMax=3) shift all old attribs by +2
// Shader must update layout(location=N) declarations to match
void ChunkMesh::Upload(const std::vector<Vertex>& vertices) noexcept
{
    vertexCount = (int)vertices.size();
    if (vertexCount == 0) { valid = false; return; }

    if (m_VAO == 0) {
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
    }

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

    // 0: pos (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    // 1: UV (vec2) — was baseUV, now tile-local 0..N
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, baseUV));
    glEnableVertexAttribArray(1);

	// 2: tileBase (uint8_t) - unsigned byte
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, tileBase));
    glEnableVertexAttribArray(2);

    // 3: tileOverlay (uint8_t) — unsigned byte
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, tileOverlay));
    glEnableVertexAttribArray(3);

	// 4: normal (uint8_t) - unsigned byte
    glVertexAttribIPointer(4, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(4);

    // 5: useOverlay (float)
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, useOverlay));
    glEnableVertexAttribArray(5);

    // 6: ao (float)
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, ao));
    glEnableVertexAttribArray(6);

    // 7: tint (vec3)
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tint));
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