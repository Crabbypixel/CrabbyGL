#include "stb/stb_image.h"

#include <glad/glad.h>

#include <iostream>

#include "rendering/UIRenderer.h"

void UIRenderer::Init(int screenWidth, int screenHeight)
{
	float x = (screenWidth - 364.0f) * 0.5f;
	float y = 20.0f;

	float w = 364.0f;
	float h = 44.0f;

	float hotbarVertices[] =
	{
		x + w, y + h, 0.0f,   1,1,
		x + w, y,     0.0f,   1,0,
		x,     y,     0.0f,   0,0,
		x,     y + h, 0.0f,   0,1
	};

	unsigned int hotbarIndices[] = {
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};


	matProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight, -1.0f, 1.0f);

	// Load
	glGenVertexArrays(1, &m_hotbarVAO);
	glGenBuffers(1, &m_hotbarVBO);
	glGenBuffers(1, &m_hotbarEBO);

	glBindVertexArray(m_hotbarVAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_hotbarVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(hotbarVertices), hotbarVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_hotbarEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(hotbarIndices), hotbarIndices, GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Textures
	glGenTextures(1, &hotbarTexture);
	glBindTexture(GL_TEXTURE_2D, hotbarTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Load and generate the texture
	int width, height, nrChannels;
	unsigned char* data = stbi_load("assets/textures/hotbar.png", &width, &height, &nrChannels, 0);

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	// Load shader
	hotbarShader.load("assets/shaders/Hotbar.glsl");
}

void UIRenderer::DrawHotbar() noexcept
{
	hotbarShader.use();
	hotbarShader.setInt("hotbarTexture", 2);
	hotbarShader.setMat4("matProjection", matProjection);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, hotbarTexture);

	glBindVertexArray(m_hotbarVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

UIRenderer::~UIRenderer() noexcept
{
	glDeleteVertexArrays(1, &m_hotbarVAO);
	glDeleteBuffers(1, &m_hotbarVBO);
	glDeleteBuffers(1, &m_hotbarEBO);
}