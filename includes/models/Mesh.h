#pragma once

#include <glm/glm.hpp>

#include "rendering/Shader.h"

#include <vector>
#include <string>

// Maximum number of bones affecting a vertex (for animation)
#define MAX_BONE_INFLUENCE 4

struct Vertex {
    glm::vec3 vPosition;
    glm::vec3 fNormal;
    glm::vec2 vTexCoords;

    glm::vec3 vTangent;    // Tangent (for normal mapping)
    glm::vec3 vBitangent;  // Bitangent (for normal mapping)

    int   m_BoneIDs[MAX_BONE_INFLUENCE]; // Bone indices
    float m_Weights[MAX_BONE_INFLUENCE]; // Bone weights
};

// Texture structure
struct Texture {
    unsigned int id;     // OpenGL texture ID
    std::string type;    // texture_diffuse, texture_specular, etc.
    std::string path;
};

// Mesh class
class Mesh {
public:
    // Public mesh data (used by renderer / model)
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

    unsigned int VAO; // Vertex Array Object

    // Constructor
    Mesh(const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices,
        const std::vector<Texture>& textures);

    // Draw mesh using given shader
    void Draw(Shader& shader);

private:
    unsigned int VBO, EBO; // Buffers

    // Internal function to setup OpenGL buffers
    void setupMesh();
};