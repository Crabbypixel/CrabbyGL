#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "rendering/VertexArray.h"
#include "rendering/VertexBuffer.h"
#include "rendering/BufferLayout.h"
#include "rendering/Shader.h"

#include <array>

class Inventory;

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
	static constexpr float HOTBAR_CURSOR_Y = 17.0f;

	glm::mat4 matProjection = glm::mat4(1.0f);

	// Hotbar variables
    unsigned int m_hotbarVAO = 0, m_hotbarVBO = 0, m_hotbarEBO = 0;
    unsigned int m_hotbarTexture = 0;
	Shader hotbarShader;
	static constexpr int HOTBAR_TEX_W = 364, HOTBAR_TEX_H = 40;

	// Hotbar selector variables
	unsigned int m_hotbarCursorVAO = 0, m_hotbarCursorVBO = 0, m_hotbarCursorEBO = 0;
	unsigned int m_hotbarCursorTexture = 0;
	Shader hotbarCursorShader;
	static constexpr int HOTBAR_CURSOR_W = 44, HOTBAR_CURSOR_H = 44;

	// Inventory
	unsigned int m_inventoryVAO = 0, m_inventoryVBO = 0, m_inventoryEBO = 0;
	unsigned int m_inventoryTexture = 0;
	Shader inventoryShader;
	static constexpr int INV_TEX_W = 176, INV_TEX_H = 166;

	// Debug rects
	unsigned int m_debugRectVAO = 0, m_debugRectVBO = 0;
	Shader debugRectShader;

	// Icons
	unsigned int m_iconVAO = 0, m_iconVBO = 0;
	unsigned int m_iconTexture = 0;
	Shader iconShader;

	// Font
	unsigned int m_asciiVAO = 0, m_asciiVBO = 0;
	unsigned int m_asciiTexture = 0;
	Shader asciiShader;

	static int screenWidth, screenHeight;

	void InitHotbar();
	void InitHotbarCursor();
	void InitInventory();
	void InitDebugRect();
	void InitIcons();
	void InitASCII();

public:
	UIRenderer() = default;
	~UIRenderer() noexcept;

	// Non-copyable, non-movable
	UIRenderer(const UIRenderer&) = delete;
	UIRenderer(UIRenderer&&) = delete;
	UIRenderer& operator=(const UIRenderer&) = delete;
	UIRenderer& operator=(UIRenderer&&) = delete;

	[[nodiscard]] static glm::vec2 GetInventorySlotPos(int slotIndex) noexcept;
	[[nodiscard]] static glm::vec2 GetHotbarSlotPos(int slotIndex) noexcept;
	[[nodiscard]] static int GetMouseInventorySlot(float mouseX, float mouseY) noexcept;

	void Init(int screenWidth, int screenHeight);
	void LoadIcons(const char* path);
	void LoadASCII(const char* path);

	void DrawHotbar() noexcept;
	void DrawHotbarCursor(int index) noexcept;
	void DrawInventory() noexcept;
	void DrawTexturedQuad(float x, float y, float w, float h, glm::vec2 uvMin, glm::vec2 uvMax, unsigned int texID) noexcept;
	void DrawInventoryIcons(const Inventory& inv) noexcept;
	void DrawHotbarIcons(const Inventory& inv) noexcept;
	void DrawHeldItem(const Inventory& inv, float mouseX, float mouseY) noexcept;
	void DrawDebugRect(float x, float y, float w, float h, const glm::vec4& color) noexcept;
	void DrawText(float x, float y, float scale, const std::string& text, glm::vec4 color) noexcept;
	void DrawTextBold(float x, float y, float scale, const std::string& text, glm::vec4 color) noexcept;
};