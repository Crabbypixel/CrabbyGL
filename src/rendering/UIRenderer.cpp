#include "stb/stb_image.h"

#include <glad/glad.h>

#include <iostream>

#include "rendering/UIRenderer.h"

void UIRenderer::Init(int screenWidth, int screenHeight)
{
	this->screenWidth = screenWidth;
	this->screenHeight = screenHeight;

	matProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight, -1.0f, 1.0f);

	InitInventory();
	InitHotbar();
	InitHotbarCursor();
	InitDebugRect();
}

glm::vec2 UIRenderer::GetInventorySlotPos(int slotIndex) const noexcept
{
	// Scaled panel origin (bottom-left in screen space, Y-up ortho)
	float inventorySlotScreenPosX = (screenWidth - INV_TEX_W * INVENTORY_SCALE) * 0.5f;
	float inventorySlotScreenPosY = (screenHeight - INV_TEX_H * INVENTORY_SCALE) * 0.5f;

	// Texture Y -> Screen Y conversion (stbi flips, texture Y starts from top, screen Y starts from bottom)
	auto texYToScreenY = [&](float texY) {
		return inventorySlotScreenPosY + (INV_TEX_H - texY - INV_SLOT_SIZE) * INVENTORY_SCALE;
	};

	if (slotIndex < 9)
	{
		// Hotbar slots 0-8
		return {
			inventorySlotScreenPosX + (INV_SLOT_START_X + slotIndex * INV_SLOT_SIZE) * INVENTORY_SCALE,
			texYToScreenY(HOTBAR_TEX_Y)
		};
	}

	// Main inventory slots 9-35 (3 rows of 9)
	int inventoryIndex = slotIndex - 9;
	int row = inventoryIndex / 9;
	int col = inventoryIndex % 9;

	return {
		inventorySlotScreenPosX + (INV_SLOT_START_X + col * INV_SLOT_SIZE) * INVENTORY_SCALE,
		texYToScreenY(INV_TEX_Y + row * INV_SLOT_SIZE)
	};
}

glm::vec2 UIRenderer::GetHotbarSlotPos(int slotIndex) const noexcept
{
	float hotbarSlotScreenPosX = (screenWidth - HOTBAR_TEX_W) * 0.5f;
	float hotbarSlotScreenPosY = HOTBAR_SLOT_Y;

	float hotbarSlotPosX = hotbarSlotScreenPosX + 4.0f + (slotIndex * SLOT_SPACING);

	return { hotbarSlotPosX, hotbarSlotScreenPosY };
}

int UIRenderer::GetMouseInventorySlot(float mouseX, float mouseY) const noexcept
{
	constexpr float SLOT_SIZE = 32;

	for (int i = 0; i < 36; i++)
	{
		glm::vec2 pos = GetInventorySlotPos(i);

		bool inside =
			mouseX >= pos.x && mouseX < pos.x + SLOT_SIZE
			&& mouseY >= pos.y && mouseY < pos.y + SLOT_SIZE;

		if (inside)
			return i;
	}

	return -1;
}

void UIRenderer::InitDebugRect()
{
	glGenVertexArrays(1, &m_debugRectVAO);
	glGenBuffers(1, &m_debugRectVBO);

	glBindVertexArray(m_debugRectVAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_debugRectVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, nullptr, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	debugRectShader.load("assets/shaders/DebugRect.glsl");
}

void UIRenderer::InitHotbar()
{
	float w = (float)HOTBAR_TEX_W;
	float h = (float)HOTBAR_TEX_H;

	float x = (screenWidth - HOTBAR_TEX_W) * 0.5f;
	float y = HOTBAR_Y;

	float hotbarVertices[] =
	{
		x + w, y + h, 0.0f,   1.0f, 1.0f,
		x + w, y,     0.0f,   1.0f, 0.0f,
		x,     y,     0.0f,   0.0f, 0.0f,
		x,     y + h, 0.0f,   0.0f, 1.0f
	};

	unsigned int hotbarIndices[] = {
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};

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
	glGenTextures(1, &m_hotbarTexture);
	glBindTexture(GL_TEXTURE_2D, m_hotbarTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Load and generate texture
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

void UIRenderer::InitHotbarCursor()
{
	float w = (float)HOTBAR_CURSOR_W;
	float h = (float)HOTBAR_CURSOR_H;

	float x = (screenWidth - HOTBAR_CURSOR_W) * 0.5f;
	float y = HOTBAR_CURSOR_Y;

	float hotbarCursorVertices[] =
	{
		x + w, y + h, 0.0f,   1.0f,1.0f,
		x + w, y,     0.0f,   1.0f,0.0f,
		x,     y,     0.0f,   0.0f,0.0f,
		x,     y + h, 0.0f,   0.0f,1.0f
	};

	unsigned int hotbarCursorIndices[] = {
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};
	
	// Load
	glGenVertexArrays(1, &m_hotbarCursorVAO);
	glGenBuffers(1, &m_hotbarCursorVBO);
	glGenBuffers(1, &m_hotbarCursorEBO);

	glBindVertexArray(m_hotbarCursorVAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_hotbarCursorVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(hotbarCursorVertices), hotbarCursorVertices, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_hotbarCursorEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(hotbarCursorIndices), hotbarCursorIndices, GL_DYNAMIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Textures
	glGenTextures(1, &m_hotbarCursorTexture);
	glBindTexture(GL_TEXTURE_2D, m_hotbarCursorTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Load and generate texture
	int width, height, nrChannels;
	unsigned char* data = stbi_load("assets/textures/hotbar_selector.png", &width, &height, &nrChannels, 0);

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
	hotbarCursorShader.load("assets/shaders/Hotbar_Selector.glsl");
}

void UIRenderer::InitInventory()
{
	float w = (float)INV_TEX_W * INVENTORY_SCALE;
	float h = (float)INV_TEX_H * INVENTORY_SCALE;

	int x = (screenWidth - (int)w) / 2;
	int y = (screenHeight - (int)h) / 2;

	float inventoryVertices[] =
	{
		(float)x + w, (float)y + h, 0.0f,   1.0f, 1.0f,
		(float)x + w, (float)y,     0.0f,   1.0f, 0.0f,
		(float)x,     (float)y,     0.0f,   0.0f, 0.0f,
		(float)x,     (float)y + h, 0.0f,   0.0f, 1.0f
	};

	unsigned int inventoryIndices[] = {
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};

	// Load
	glGenVertexArrays(1, &m_inventoryVAO);
	glGenBuffers(1, &m_inventoryVBO);
	glGenBuffers(1, &m_inventoryEBO);

	glBindVertexArray(m_inventoryVAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_inventoryVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(inventoryVertices), inventoryVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_inventoryEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(inventoryIndices), inventoryIndices, GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Textures
	glGenTextures(1, &m_inventoryTexture);
	glBindTexture(GL_TEXTURE_2D, m_inventoryTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Load and generate texture
	int width, height, nrChannels;
	unsigned char* data = stbi_load("assets/textures/player_inventory.png", &width, &height, &nrChannels, 0);

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		//glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	// Load shader
	inventoryShader.load("assets/shaders/Inventory.glsl");
}

void UIRenderer::DrawHotbar() noexcept
{
	hotbarShader.use();
	hotbarShader.setInt("hotbarTexture", 2);
	hotbarShader.setMat4("matProjection", matProjection);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_hotbarTexture);

	glBindVertexArray(m_hotbarVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void UIRenderer::DrawHotbarCursor(int index) noexcept
{
	float hotbarX = (screenWidth - HOTBAR_TEX_W) * 0.5f;

	float x = hotbarX - 4.0f + (index * SLOT_SPACING);
	float y = HOTBAR_CURSOR_Y;

	float w = (float)HOTBAR_CURSOR_W;
	float h = (float)HOTBAR_CURSOR_H;

	float verts[] =
	{
		x + w, y + h, 0.0f, 1.0f, 1.0f,
		x + w, y,     0.0f, 1.0f, 0.0f,
		x,     y,     0.0f, 0.0f, 0.0f,
		x,     y + h, 0.0f, 0.0f, 1.0f
	};

	glBindBuffer(GL_ARRAY_BUFFER, m_hotbarCursorVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

	hotbarCursorShader.use();
	hotbarCursorShader.setInt("hotbarSelectorTexture", 3);
	hotbarCursorShader.setMat4("matProjection", matProjection);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_hotbarCursorTexture);

	glBindVertexArray(m_hotbarCursorVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void UIRenderer::DrawInventory() noexcept
{
	inventoryShader.use();
	inventoryShader.setInt("inventoryTexture", 4);
	inventoryShader.setMat4("matProjection", matProjection);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, m_inventoryTexture);

	glBindVertexArray(m_inventoryVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void UIRenderer::DrawDebugRect(float x, float y, float w, float h, const glm::vec4& color)
{
	float verts[] =
	{
		x,     y,
		x + w,   y,
		x + w,   y + h,

		x,     y,
		x + w,   y + h,
		x,     y + h
	};

	debugRectShader.use();

	debugRectShader.setMat4("matProjection", matProjection);
	debugRectShader.setVec4("color", color);

	glBindVertexArray(m_debugRectVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_debugRectVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
}

UIRenderer::~UIRenderer() noexcept
{
	glDeleteVertexArrays(1, &m_hotbarVAO);
	glDeleteBuffers(1, &m_hotbarVBO);
	glDeleteBuffers(1, &m_hotbarEBO);
	glDeleteTextures(1, &m_hotbarTexture);

	glDeleteVertexArrays(1, &m_hotbarCursorVAO);
	glDeleteBuffers(1, &m_hotbarCursorVBO);
	glDeleteBuffers(1, &m_hotbarCursorEBO);
	glDeleteTextures(1, &m_hotbarCursorTexture);

	glDeleteVertexArrays(1, &m_inventoryVAO);
	glDeleteBuffers(1, &m_inventoryVBO);
	glDeleteBuffers(1, &m_inventoryEBO);
	glDeleteTextures(1, &m_inventoryTexture);

	glDeleteVertexArrays(1, &m_debugRectVAO);
	glDeleteBuffers(1, &m_debugRectVBO);
}