#pragma once
#include <cstdint>

enum class BlockType : uint8_t
{
	/* 0*/	AIR,
	/* 1*/	DIRT,
	/* 2*/	GRASS_BLOCK,
	/* 3*/	STONE,
	/* 4*/	BEDROCK,
	/* 5*/	BRICK,
	/* 6*/	TREE_LOG_Y,
	/* 7*/	TREE_LEAVES,
	/* 8*/	COBBLESTONE,
	/* 9*/	PLANK,
	/*10*/	SAND,
	/*11*/	GRAVEL,
	/*12*/	GLASS,
	/*13*/	GOLD_ORE,
	/*14*/	IRON_ORE,
	/*15*/	COAL_ORE,
	/*16*/	DIAMOND_ORE,
	/*17*/	SMOOTH_STONE,
	/*18*/	SAPLING,
	/*19*/	ROSE,
	/*20*/	DANDELION,
	/*21*/	BROWN_MUSHROOM,
	/*22*/	RED_MUSHROOM,
	/*23*/  GRASS,
	/*24*/  TREE_LOG_X,
	/*25*/  TREE_LOG_Z,
	MAX_VALUE			// Ending sentinel - not a real block type, used for validation and iteration
};
