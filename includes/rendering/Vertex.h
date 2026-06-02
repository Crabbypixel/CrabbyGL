#pragma once

#include <glm/glm.hpp>

struct Vertex
{
    glm::vec3 pos;          // worldPos + FACE_VERTS[face][k]
    glm::vec2 baseUV;       // tile-local 0..N
    uint8_t   tileBase;
    uint8_t   tileOverlay;

    // uint8_t packed;

    uint8_t   normal;           // 3 bits
    float     useOverlay;       // 1 bit (bool)
    float     ao;               // 2 bits

    glm::vec3 tint;         
};

// Efficient ver
struct Vertex2
{
    glm::vec3 pos;
    glm::vec2 baseUV;
    uint8_t tileBase;
    uint8_t tileOverlay;
	uint8_t packed;     // normal(3b) + ao(2b) + overlayUse(1b) + spare(2b) = 8 bits (1 byte)
    uint8_t tint[3];
};