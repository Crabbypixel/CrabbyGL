#pragma once

#include "rendering/VertexArray.h"
#include "rendering/VertexBuffer.h"
#include "rendering/Shader.h"
#include "rendering/BufferLayout.h"
#include "rendering/Texture2D.h"

#include <vector>
#include <string>

class SimpleModel
{
private:
    VertexArray vao;
    VertexBuffer<float> vbo;
    std::vector<Texture2D> textures;
    int nr_indices = 0;

public:
    //glm::mat4 matModel = glm::mat4(1.0f);

    SimpleModel() = default;
    SimpleModel(const std::string& objfilepath, const std::vector<std::string>&& texturePaths);

    // Load the object file
    bool load(const std::string& objfilePath);

    // Set textures to the model, if any
    // For now, this function is just a placeholder and does nothing
    void setTextures(const std::vector<std::string>& texturePaths);
    void setTextures(const std::vector<std::string>&& texturePaths);

    // Bind textures by using a function, as doing them for every draw call is inefficient.
    void bindTextures();

    // Function which draws the model onto the screen. Make sure to bind shaders before calling this function.
    void draw();

    // Destructor
    ~SimpleModel();

private:
    // Utility function to load model
    bool LoadModel(VertexArray& vao, VertexBuffer<float>& vbo, int& vertexCount, const std::string& modelFile);
};