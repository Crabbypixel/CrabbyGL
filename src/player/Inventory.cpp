#include "player/Inventory.h"

#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>

// Updates hotbar cursor
void Inventory::Scroll(int delta) noexcept
{
	m_hotbarIndex = (m_hotbarIndex + delta + HOTBAR_SIZE) % HOTBAR_SIZE;
}

bool Inventory::AddBlock(BlockType type)
{
	// Try adding to the current hotbar index
	if (m_slots[m_hotbarIndex].IsEmpty())
	{
		m_slots[m_hotbarIndex] = { type, 1 };
		return true;
	}

	// If already present
	for (auto& slot :m_slots)
	{
		if (slot.type == type)
			return true;
	}

	// If not present, add new
	for (auto& slot : m_slots)
	{
		if (slot.IsEmpty())
		{
			slot.type = type;
			slot.count = 1;
			return true;
		}
	}

	return false;	// full
}

void Inventory::RemoveFromSlot(int index)
{
	if (index < 0 || index > TOTAL_SIZE)
		return;

	m_slots[index] = {};
}

void Inventory::ClickSlot(int index, bool rightClick) noexcept
{
	if (index < 0 || index > TOTAL_SIZE)
		return;

	std::swap(m_dragItem, m_slots[index]);
}

void Inventory::Dump() noexcept
{
	// Dump held item
	if (!m_dragItem.IsEmpty())
	{
		AddBlock(m_dragItem.type);
		m_dragItem = {};
	}
}

bool Inventory::Load(const std::string& filepath)
{
	inventorySaveFilePath = filepath;

	std::ifstream f(filepath, std::ios::binary);

	if (!f)
		return false;

	f.read(reinterpret_cast<char*>(m_slots.data()), TOTAL_SIZE * sizeof(ItemStack));
	return f.good();
}

bool Inventory::Save()
{
	std::ofstream f(inventorySaveFilePath, std::ios::binary);

	if (!f)
		return false;

	f.write(reinterpret_cast<char*>(m_slots.data()), TOTAL_SIZE * sizeof(ItemStack));
	return f.good();
}