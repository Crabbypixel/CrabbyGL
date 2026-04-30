#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "rendering/Shader.h"

class ChunkMesh
{
public:
    struct Vertex {
        glm::vec3 pos;              // world position of vertex
        glm::vec2 baseUV;           // base UV
        glm::vec2 overlayUV;        // overlay UV
        glm::vec3 normal;           // face normal
        glm::vec3 blockOrigin;      // integer world position
        glm::vec3 tint;             // Tint for greyshade textures
        float useOverlay;           // 1.0 for grass, 0.0 for others
        float ao;
    };

    void Build(const std::vector<Vertex>& vertices);
    void Draw() const;
    void Destroy();

    int  vertexCount = 0;
    bool valid = false;             // has uploaded data?

private:
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
};