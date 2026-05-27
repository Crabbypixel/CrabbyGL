#pragma once

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;               // world position of vertex
    glm::vec2 baseUV;            // base UV
    glm::vec2 overlayUV;         // overlay UV
    glm::vec3 normal;            // face normal
    glm::ivec3 blockOrigin;      // integer world position
    glm::vec3 tint;              // Tint for greyshade textures
    float useOverlay;            // 1.0 for grass, 0.0 for others
    float ao;                    // ambient occlusion
};