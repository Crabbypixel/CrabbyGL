#pragma once

#include "Object.h"

class Cuboid : public Object
{
public:
	static Model model;
	glm::vec3 size;
	bool bIsStatic = false;

	Cuboid() = default;

	Cuboid(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& color, const glm::vec3& size, float mass) : Object(position, velocity, color, mass), size{ size } {}

	void Init(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& color, const glm::vec3& size, float mass)
	{
		this->position = position;
		this->initialVelocity = velocity;
		this->color = color;
		this->mass = mass;
		this->size = size;
	}

	void Draw(Shader& shader, const glm::mat4& matModel) override
	{
		shader.use();
		shader.setMat4("matModel", matModel);

		model.Draw(shader);
	}

	ObjectType GetType() const override { return ObjectType::Cuboid; }
};

Model Cuboid::model;