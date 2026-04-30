#pragma once

#include "Mesh.h"
#include "rendering/Shader.h"

#include <vector>
#include <string>

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;
enum aiTextureType;

// Only declaration
unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma);

class Model
{
public:
    std::vector<Texture> textures_loaded;
    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection = false;

    Model() = default;

    void load(const std::string& path, bool gamma = false);
    void Draw(Shader& shader);

private:
    void LoadModel(const std::string& path);
    void ProcessNode(aiNode* node, const aiScene* scene);
    Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};