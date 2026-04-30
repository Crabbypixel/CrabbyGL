#pragma once

#include <glad/glad.h>
#include <string>

class Texture2D
{
private:
    unsigned int m_TextureID;
    int m_width, m_height;
    unsigned char* data;
    int m_nrChannels;

public:
    Texture2D() = default;
    ~Texture2D();

    void load(GLenum wrapType, GLint minFilter, GLint magFilter,
        const std::string textureFile,
        GLint internalFormat, GLenum format);

    unsigned int getTextureID() const;

    void bindTexture() const;

    void loadTexture(char const* path);
};