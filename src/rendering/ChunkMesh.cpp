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

    // 2: uvTileMin (vec2) - tile bottom-left
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uvTileMin));
    glEnableVertexAttribArray(2);

    // 3: uvTileMax (vec2) — atlas tile top-right
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uvTileMax));
    glEnableVertexAttribArray(3);

    // 4: overlayTileMin (vec2)
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, overlayTileMin));
    glEnableVertexAttribArray(4);

    // 5: overlayTileMax (vec2)
	glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, overlayTileMax));
    glEnableVertexAttribArray(5);

    // 6: normal (vec3)
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(6);

    // 7: tint (vec3)
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tint));
    glEnableVertexAttribArray(7);

    // 8: useOverlay (float)
    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, useOverlay));
    glEnableVertexAttribArray(8);

    // 9: ao (float)
    glVertexAttribPointer(9, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, ao));
    glEnableVertexAttribArray(9);

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