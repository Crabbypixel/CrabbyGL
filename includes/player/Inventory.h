#pragma once
#include "world/BlockRegistry.h"
#include "world/BlockType.h"

#include <glm/glm.hpp>

#include <array>
#include <string>

struct ItemStack
{
	BlockType type = BlockType::AIR;
	uint8_t count = 0;		// Not that important
	[[nodiscard]] bool IsEmpty() const noexcept { return count == 0 || type == BlockType::AIR; }
};

class Inventory
{
public:
	static constexpr int HOTBAR_SIZE = 9;
	static constexpr int TOTAL_SIZE = 36;
	static constexpr int MAX_STACK = 64;

	Inventory() = default;
	~Inventory() = default;

	[[nodiscard]] const ItemStack& GetDragItem() const noexcept { return m_dragItem; };
	[[nodiscard]] BlockType GetHeldBlock() const noexcept { return m_slots[m_hotbarIndex].IsEmpty() ? BlockType::AIR : m_slots[m_hotbarIndex].type; };
	[[nodiscard]] int GetHotbarIndex() const noexcept { return m_hotbarIndex; };
	[[nodiscard]] const ItemStack& At(int i) const noexcept { assert(i >= 0 && i < TOTAL_SIZE); return m_slots[i]; };

	[[nodiscard]] bool IsOpen() const noexcept { return m_isOpen; };
	void Open() noexcept { m_isOpen = true; };
	void Close() noexcept { m_isOpen = false; };
	void Toggle() noexcept { m_isOpen = !m_isOpen; };

	bool AddBlock(BlockType type);
	void RemoveFromSlot(int index);
	void Scroll(int delta) noexcept;
	void ClickSlot(int index, bool rightClick) noexcept;
	void Dump() noexcept;

	bool Load(const std::string& filepath);
	bool Save();

private:
	std::array<ItemStack, TOTAL_SIZE> m_slots{};
	ItemStack m_dragItem{};
	int m_hotbarIndex = 0;
	bool m_isOpen = false;
	
	std::string inventorySaveFilePath;
};