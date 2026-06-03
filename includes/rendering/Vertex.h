#pragma once

#include <glm/glm.hpp>

struct Vertex
{
    glm::vec3 pos;          // World-space vertex position
    glm::vec2 baseUV;       // Greedy-mesh UV coordinates

    uint8_t   tileBase;     // Atlas tile for base texture
    uint8_t   tileOverlay;  // Atlas tile for overlay texture

    uint8_t   packed;       // [0-2] normal, [3] overlay, [4-5] AO

    uint32_t  tint;         // RGBA8 tint color
};