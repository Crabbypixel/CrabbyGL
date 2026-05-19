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
    unsigned int m_hotbarVAO = 0, m_hotbarVBO = 0, m_hotbarEBO = 0;
    unsigned int hotbarTexture = 0;
	glm::mat4 matProjection = glm::mat4(1.0f);
	Shader hotbarShader;

public:
	UIRenderer() = default;

	// Non-copyable, non-movable
	UIRenderer(const UIRenderer&) = delete;
	UIRenderer(UIRenderer&&) = delete;
	UIRenderer& operator=(const UIRenderer&) = delete;
	UIRenderer& operator=(UIRenderer&&) = delete;

	void Init(int screenWidhth, int screenHeight);
	void DrawHotbar() noexcept;
    ~UIRenderer() noexcept;
};