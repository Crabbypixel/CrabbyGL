#pragma once

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;          // worldPos + FACE_VERTS[face][k]
    glm::vec2 baseUV;       // tile-local 0..N
    glm::vec2 uvTileMin;    
    glm::vec2 uvTileMax;    
    glm::vec2 overlayTileMin;
    glm::vec2 overlayTileMax;
    glm::vec3 normal;
    glm::vec3 tint;         
    float     useOverlay;   
    float     ao;         
};