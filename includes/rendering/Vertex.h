#pragma once

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;          // worldPos + FACE_VERTS[face][k]
    glm::vec2 baseUV;       // tile-local 0..N
    glm::vec2 uvTileMin;    // baseRect.min  <- was missing here
    glm::vec2 uvTileMax;    // baseRect.max  <- was missing here
    glm::vec2 overlayUV;    // overlay atlas coords
    glm::vec3 normal;       // face normal
    //glm::vec3 blockOrigin;  // worldPos
    glm::vec3 tint;         
    float     useOverlay;   
    float     ao;         
};