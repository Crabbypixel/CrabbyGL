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
	static constexpr float SLOT_SPACING = 40.5f;
	static constexpr float INVENTORY_SCALE = 2.0f;

	// Inventory screen constants
	static constexpr float INV_SLOT_SIZE = 18.0f;	// Total slot stride including the boundary
	static constexpr float INV_SLOT_START_X = 8.0f; // Start of first slot from the left side (in texture space)

	// Texture Y pos from top for each region (Texture coords start from 0, 0 at top-left corner)
	static constexpr float INV_TEX_Y = 82.0f;
	static constexpr float HOTBAR_TEX_Y = 140.0f;

	// Hotbar constants
	static constexpr float HOTBAR_SLOT_Y = 24.0f;
	static constexpr float HOTBAR_Y = 20.0f;
	static constexpr float HOTBAR_CURSOR_Y = 16.0f;

	glm::mat4 matProjection = glm::mat4(1.0f);

	// Hotbar variables
    unsigned int m_hotbarVAO = 0, m_hotbarVBO = 0, m_hotbarEBO = 0;
    unsigned int m_hotbarTexture = 0;
	Shader hotbarShader;
	const int HOTBAR_TEX_W = 364, HOTBAR_TEX_H = 40;

	// Hotbar selector variables
	unsigned int m_hotbarCursorVAO = 0, m_hotbarCursorVBO = 0, m_hotbarCursorEBO = 0;
	unsigned int m_hotbarCursorTexture = 0;
	Shader hotbarCursorShader;
	const int HOTBAR_CURSOR_W = 44, HOTBAR_CURSOR_H = 44;

	// Inventory
	unsigned int m_inventoryVAO = 0, m_inventoryVBO = 0, m_inventoryEBO = 0;
	unsigned int m_inventoryTexture = 0;
	Shader inventoryShader;
	const int INV_TEX_W = 176, INV_TEX_H = 166;

	// Debug rects
	unsigned int m_debugRectVAO = 0, m_debugRectVBO = 0;
	Shader debugRectShader;

	int screenWidth = 0, screenHeight = 0;

	void InitHotbar();
	void InitHotbarCursor();
	void InitInventory();
	void InitDebugRect();

public:
	UIRenderer() = default;
	~UIRenderer() noexcept;

	// Non-copyable, non-movable
	UIRenderer(const UIRenderer&) = delete;
	UIRenderer(UIRenderer&&) = delete;
	UIRenderer& operator=(const UIRenderer&) = delete;
	UIRenderer& operator=(UIRenderer&&) = delete;

	[[nodiscard]] glm::vec2 GetInventorySlotPos(int slotIndex) const noexcept;
	[[nodiscard]] glm::vec2 GetHotbarSlotPos(int slotIndex) const noexcept;

	[[nodiscard]] int GetMouseInventorySlot(float mouseX, float mouseY) const noexcept;

	void Init(int screenWidth, int screenHeight);
	void DrawHotbar() noexcept;
	void DrawHotbarCursor(int index) noexcept;
	void DrawInventory() noexcept;

	// Temporary debug
	void DrawDebugRect(float x, float y, float w, float h, const glm::vec4& color);
};