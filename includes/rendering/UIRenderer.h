#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "rendering/VertexArray.h"
#include "rendering/VertexBuffer.h"
#include "rendering/BufferLayout.h"
#include "rendering/Shader.h"

class UIRenderer
{
private:
    unsigned int m_hotbarVAO, m_hotbarVBO, m_hotbarEBO;
    unsigned int hotbarTexture;
	glm::mat4 matProjection;
	Shader hotbarShader;

public:
	void Init(int screenWidht, int screenHeight);
	void DrawHotbar();
    ~UIRenderer();
};