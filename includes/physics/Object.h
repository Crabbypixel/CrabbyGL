#pragma once

#include "models/SimpleModel.h"
#include "models/AssimpModelLoader.h"

enum class ObjectType
{
	Sphere,
	Cuboid
};

class Object
{
public:
	bool bFirst = true;

	glm::vec3 previousPosition;
	glm::vec3 position;
	glm::vec3 initialVelocity;
	glm::vec3 acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 color;

	float mass;

	Object() = default;
	Object(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& color, float mass) : position{ position }, initialVelocity{ velocity }, color{ color }, mass{ mass } {}
	
	void Init(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& color, float mass)
	{
		this->position = position;
		this->initialVelocity = velocity;
		this->color = color;
		this->mass = mass;
	}

	void UpdatePos(float dt)
	{
		if (bFirst)
		{
			previousPosition = position - initialVelocity * dt;
			bFirst = false;
		}
		else
		{
			glm::vec3 temp = position;
			position += (position - previousPosition) + acceleration * dt * dt;
			previousPosition = temp;
		}
	}

	virtual void Draw(Shader& shader, const glm::mat4& matModel) = 0;
	virtual ObjectType GetType() const = 0;
	virtual ~Object() = default;
};