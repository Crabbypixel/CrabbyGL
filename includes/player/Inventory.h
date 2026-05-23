#pragma once
#include "world/BlockRegistry.h"
#include "world/BlockType.h"
#include <glm/glm.hpp>

struct ItemStack
{
	BlockType type = BlockType::AIR;
	uint8_t count = 0;
	bool IsEmpty() const { return count == 0 || type == BlockType::AIR; }
};

class Inventory
{
public:

};