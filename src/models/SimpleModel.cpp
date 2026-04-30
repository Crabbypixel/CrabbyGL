#include "models/SimpleModel.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstring>

// Constructor
SimpleModel::SimpleModel(const std::string& objfilepath, const std::vector<std::string>&& texturePaths)
{
    LoadModel(vao, vbo, nr_indices, objfilepath);
    setTextures(texturePaths);
}

// Load model externally
bool SimpleModel::load(const std::string& objfilePath)
{
    return LoadModel(vao, vbo, nr_indices, objfilePath);
}

// Set textures
void SimpleModel::setTextures(const std::vector<std::string>& texturePaths)
{
    for (const auto& texturePath : texturePaths)
    {
        Texture2D texture;
        texture.loadTexture(texturePath.c_str());
        textures.push_back(texture);
    }

    // Bind textures
    for (unsigned int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        textures[i].bindTexture();
    }
}

void SimpleModel::setTextures(const std::vector<std::string>&& texturePaths)
{
    for (const auto& texturePath : texturePaths)
    {
        Texture2D texture;
        texture.loadTexture(texturePath.c_str());
        textures.push_back(texture);
    }

    // Bind textures
    for (unsigned int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        textures[i].bindTexture();
    }
}

// Bind textures
void SimpleModel::bindTextures()
{
    // Draw the model
    vao.bind();

    // Bind textures
    for (unsigned int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        textures[i].bindTexture();
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// Draw
void SimpleModel::draw()
{
    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, nr_indices);
}

// Destructor
SimpleModel::~SimpleModel()
{
    vbo.free();
    vao.free();
}

// Utility function to load models (written earlier so I'm lazy to properly integrate it in load() function :/
// TODO: Add texture functonality
bool SimpleModel::LoadModel(VertexArray& vao, VertexBuffer<float>& vbo, int& vertexCount, const std::string& modelFile)
{
    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec3> temp_normals;
    std::vector<glm::vec2> temp_textures;

    std::ifstream inputFileStream(modelFile);
    std::string line;
    std::stringstream ss;

    if (!inputFileStream.is_open())
    {
        std::cerr << "Failed to open object file: " << modelFile << std::endl;
        return false;
    }

    /** If the object file has "mtllib" on the third line, then the object file is
      * associated with some MTL file which we need to parse along with OBJ file */

      // Go to the third line
    std::string lineParser;

    // Read 3 lines
    for (int i = 0; i < 3; i++)
        std::getline(inputFileStream, lineParser);

    if (lineParser.find("mtllib") != std::string::npos)
    {
        struct VertexTexture
        {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 textures;
        };

        std::vector<VertexTexture> verticesTexture;

        while (std::getline(inputFileStream, line))
        {
            if (line.starts_with("vn"))
            {
                float f1, f2, f3;
                sscanf_s(line.c_str(), "%*s %f %f %f", &f1, &f2, &f3);
                temp_normals.push_back({ f1, f2, f3 });
            }
            else if (line.starts_with("vt"))
            {
                float f1, f2;
                sscanf_s(line.c_str(), "%*s %f %f", &f1, &f2);
                temp_textures.push_back({ f1, f2 });
            }
            else if (line.starts_with('v'))
            {
                float f1, f2, f3;
                sscanf_s(line.c_str(), "%*s %f %f %f", &f1, &f2, &f3);
                temp_positions.push_back({ f1, f2, f3 });
            }
            else if (line.starts_with('f'))
            {
                int v[3], n[3], t[3];

                sscanf_s(line.c_str(),
                    "%*s %d/%d/%d %d/%d/%d %d/%d/%d",
                    &v[0], &t[0], &n[0],
                    &v[1], &t[1], &n[1],
                    &v[2], &t[2], &n[2]);

                for (int i = 0; i < 3; i++)
                {
                    VertexTexture vertex;
                    vertex.position = temp_positions[v[i] - 1];
                    vertex.normal = temp_normals[n[i] - 1];
                    vertex.textures = temp_textures[t[i] - 1];

                    verticesTexture.push_back(vertex);
                }
            }
        }

        BufferLayout layout;

        vao.generate();
        vbo.generate(8);

        vbo.setBuffer(verticesTexture.size() * 8 * sizeof(float), verticesTexture.data());

        layout.setBufferLayout(vao, vbo, 3, BufferType::FLOAT);
        layout.setBufferLayout(vao, vbo, 3, BufferType::FLOAT);
        layout.setBufferLayout(vao, vbo, 2, BufferType::FLOAT);

        vertexCount = verticesTexture.size();
    }
    else
    {
        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 normal;
        };

        std::vector<Vertex> vertices;

        while (std::getline(inputFileStream, line))
        {
            if (line.starts_with("vn"))
            {
                float f1, f2, f3;
                sscanf_s(line.c_str(), "%*s %f %f %f", &f1, &f2, &f3);
                temp_normals.push_back({ f1, f2, f3 });
            }

            if (line.starts_with('v'))
            {
                float f1, f2, f3;
                sscanf_s(line.c_str(), "%*s %f %f %f", &f1, &f2, &f3);
                temp_positions.push_back({ f1, f2, f3 });
            }
            else if (line.starts_with('f'))
            {
                int v[3], n[3];

                sscanf_s(line.c_str(),
                    "%*s %d//%d %d//%d %d//%d",
                    &v[0], &n[0],
                    &v[1], &n[1],
                    &v[2], &n[2]);

                for (int i = 0; i < 3; i++)
                {
                    Vertex vertex;
                    vertex.position = temp_positions[v[i] - 1];
                    vertex.normal = temp_normals[n[i] - 1];

                    vertices.push_back(vertex);
                }
            }
        }

        BufferLayout layout;

        vao.generate();
        vbo.generate(6);

        vbo.setBuffer(vertices.size() * 6 * sizeof(float), vertices.data());

        layout.setBufferLayout(vao, vbo, 3, BufferType::FLOAT);
        layout.setBufferLayout(vao, vbo, 3, BufferType::FLOAT);

        vertexCount = vertices.size();
    }

    inputFileStream.close();
    return true;
}