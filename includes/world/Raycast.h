#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <limits>

class World;

struct RaycastHit
{
	bool hit = false;
	glm::ivec3 blockPos = glm::ivec3(0);
	glm::ivec3 normal = glm::ivec3(0);
	int index = -1;
};

RaycastHit RaycastDDA(const glm::vec3& origin, const glm::vec3& direction, const World& world, float maxDistance = 6.0f);