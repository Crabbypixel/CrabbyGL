#pragma once

#include "Object.h"

class Sphere : public Object
{
public:
	static Model model;
	float radius;

	Sphere() = default;

	Sphere(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& color, float r, float mass) : Object(position, velocity, color, mass), radius{ r } {}

	void Init(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& color, float r, float mass)
	{
		this->position = position;
		this->initialVelocity = velocity;
		this->color = color;
		this->mass = mass;
		this->radius = radius;
	}

	void Draw(Shader& shader, const glm::mat4& matModel) override
	{
		shader.use();
		shader.setMat4("matModel", matModel);

		model.Draw(shader);
	}

	ObjectType GetType() const override { return ObjectType::Sphere; }
};

Model Sphere::model;