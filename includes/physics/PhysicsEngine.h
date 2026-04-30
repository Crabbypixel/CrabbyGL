#pragma once

#include <glm/glm.hpp>

#include "Sphere.h"
#include "Cuboid.h"

#include <vector>
#include <memory>

class PhysicsEngine
{
public:
	const float G = 6.67430f;

	bool bStepOneFrame = false;
	bool bHasInitializedPhysics = false;
	bool bGravityEnabled = false;
	bool bPhysicsEnabled = false;

	// Entity storage
	//std::vector<Sphere> spheres;
	//std::vector<Cuboid> cuboids;
	std::vector<std::unique_ptr<Object>> objects;

	// Physics storage
	std::vector<std::pair<Sphere*, Sphere*>> collidingSpherePairs;
	std::vector<std::pair<Cuboid*, Cuboid*>> collidingCuboidPairs;
	std::vector<std::pair<Sphere*, Cuboid*>> collidingSphereCuboidPairs;

public:
	// Make the physics engine to only have a single instance
	static PhysicsEngine& Get()
	{
		static PhysicsEngine instance;
		return instance;
	}

	void AddSphere(const Sphere& sphere);
	void AddCuboid(const Cuboid& cuboid);
	void ClearAll();

	void Update(float dt);

	void ApplyGravity();

	void CheckSphereCollisions();
	void ResolveSphereCollisions(float dt);

	void CheckAABBCollisions();
	void ResolveAABBCollisions(float dt);

	void CheckSphereAABBCollisions();
	void ResolveSphereAABBCollisions(float dt);

	void UpdateSpherePositions(float dt);
	void UpdateCuboidPositions(float dt);

private:
	PhysicsEngine() {}
	PhysicsEngine(const PhysicsEngine&) = delete;
	void operator=(const PhysicsEngine&) = delete;
};

void PhysicsEngine::AddSphere(const Sphere& sphere)
{
	//spheres.emplace_back(sphere);
	objects.push_back(std::make_unique<Sphere>(sphere));
}

void PhysicsEngine::AddCuboid(const Cuboid& cuboid)
{
	//cuboids.emplace_back(cuboid);
	objects.push_back(std::make_unique<Cuboid>(cuboid));
}

void PhysicsEngine::ClearAll()
{
	//spheres.clear();
	//cuboids.clear();
	objects.clear();
}

void PhysicsEngine::Update(float dt)
{
	ApplyGravity();

	//CheckSphereCollisions();
	//ResolveSphereCollisions(dt);

	CheckAABBCollisions();
	//ResolveAABBCollisions(dt);

	//CheckSphereAABBCollisions();
	//ResolveSphereAABBCollisions(dt);

	//UpdateSpherePositions(dt);
	UpdateCuboidPositions(dt);
}

void PhysicsEngine::ApplyGravity()
{
	for (auto& object : objects)
		object->acceleration = glm::vec3(0.0f);

	if (bGravityEnabled)
	{
		for (int i = 0; i < objects.size(); i++)
		{
			for (int j = i + 1; j < objects.size(); j++)
			{
				Object* a = objects[i].get();
				Object* b = objects[j].get();

				glm::vec3 delta = b->position - a->position;
				float r = glm::length(delta);
				r = r * 100;
				if (r < 1.0f) continue; // skip too close

				glm::vec3 dir = delta / r;
				float F = G * a->mass * b->mass / (r * r);
				glm::vec3 force = F * dir;

				a->acceleration += force / a->mass;
				b->acceleration -= force / b->mass;
			}
		}
	}
}

void PhysicsEngine::CheckSphereCollisions()
{
	auto DoSpheresCollide = [](const glm::vec3& pos1, const glm::vec3& pos2, float r1, float r2) {
		return (pos1.x - pos2.x) * (pos1.x - pos2.x) + (pos1.y - pos2.y) * (pos1.y - pos2.y) + (pos1.z - pos2.z) * (pos1.z - pos2.z) <= (r1 + r2) * (r1 + r2);
		};

	collidingSpherePairs.clear();

	// Check for collisions
	for (int i = 0; i < objects.size(); i++)
	{
		if (objects[i]->GetType() != ObjectType::Sphere)
			continue;

		for (int j = i + 1; j < objects.size(); j++)
		{
			if (objects[j]->GetType() != ObjectType::Sphere)
				continue;

			Sphere* sphere1 = static_cast<Sphere*>(objects[i].get());
			Sphere* sphere2 = static_cast<Sphere*>(objects[j].get());

			if (DoSpheresCollide(sphere1->position, sphere2->position, sphere1->radius, sphere2->radius))
			{
				collidingSpherePairs.push_back(std::make_pair(sphere1, sphere2));

				float fDistance = glm::length(sphere1->position - sphere2->position);

				if (fDistance == 0.0f)
					continue;

				float fOverlap = (sphere1->radius + sphere2->radius - fDistance) * 0.5f;

				sphere1->position += fOverlap * (sphere1->position - sphere2->position) / fDistance;
				sphere2->position -= fOverlap * (sphere1->position - sphere2->position) / fDistance;
			}
		}
	}
}

void PhysicsEngine::ResolveSphereCollisions(float dt)
{
	// Elastic collision
	for (auto& pair : collidingSpherePairs)
	{
		Sphere* a = pair.first;
		Sphere* b = pair.second;

		glm::vec3 va = (a->position - a->previousPosition) / dt;
		glm::vec3 vb = (b->position - b->previousPosition) / dt;

		float m1 = a->mass;
		float m2 = b->mass;
		float totalMass = m1 + m2;

		// Vector between centres
		glm::vec3 normal = b->position - a->position;
		glm::vec3 unitNormal = glm::normalize(normal);

		float distance = glm::length(normal);

		if (distance == 0.0f)
			continue;

		glm::vec3 va_n = glm::dot(va, unitNormal) * unitNormal;
		glm::vec3 vb_n = glm::dot(vb, unitNormal) * unitNormal;

		glm::vec3 va_t = va - va_n;
		glm::vec3 vb_t = vb - vb_n;

		glm::vec3 va_n_after = (((m1 - m2) * va_n + 2 * m2 * vb_n)) / totalMass;
		glm::vec3 vb_n_after = ((2 * m1 * va_n + (m2 - m1) * vb_n)) / totalMass;

		glm::vec3 va_after = va_n_after + va_t;
		glm::vec3 vb_after = vb_n_after + vb_t;

		a->previousPosition = a->position - va_after * dt;
		b->previousPosition = b->position - vb_after * dt;
	}
}

void PhysicsEngine::UpdateSpherePositions(float dt)
{
	for (auto& object : objects)
	{
		if (object->GetType() != ObjectType::Sphere())
			continue;

		Sphere* sphere = static_cast<Sphere*>(object.get());
		// Clamp velocity to zero if change in pos is way too small
		glm::vec3 vel = (sphere->position - sphere->previousPosition) / dt;

		if (glm::length(vel) < 0.01f)
			sphere->previousPosition = sphere->position;

		sphere->UpdatePos(dt);
	}
}

void PhysicsEngine::CheckAABBCollisions()
{
	auto CheckAABBCollision = [](const Cuboid& a, const Cuboid& b) {
		glm::vec3 a_min = a.position - a.size * glm::vec3(0.5f);
		glm::vec3 a_max = a.position + a.size * glm::vec3(0.5f);

		glm::vec3 b_min = b.position - b.size * glm::vec3(0.5f);
		glm::vec3 b_max = b.position + b.size * glm::vec3(0.5f);

		return (a_min.x <= b_max.x && a_max.x >= b_min.x) &&
			(a_min.y <= b_max.y && a_max.y >= b_min.y) &&
			(a_min.z <= b_max.z && a_max.z >= b_min.z);
		};

	collidingCuboidPairs.clear();

	// Check for AABB collisions
	for (int i = 0; i < objects.size(); i++)
	{
		if (objects[i]->GetType() != ObjectType::Cuboid)
			continue;

		for (int j = i + 1; j < objects.size(); j++)
		{
			if (objects[j]->GetType() != ObjectType::Cuboid)
				continue;

			Cuboid* a = static_cast<Cuboid*>(objects[i].get());
			Cuboid* b = static_cast<Cuboid*>(objects[j].get());

			if (CheckAABBCollision(*a, *b))
			{
				std::cout << "aabb-aabb collision!\n";

				collidingCuboidPairs.push_back(std::make_pair(a, b));

				// Resolve positions
				glm::vec3 a_min = a->position - a->size * glm::vec3(0.5f);
				glm::vec3 a_max = a->position + a->size * glm::vec3(0.5f);

				glm::vec3 b_min = b->position - b->size * glm::vec3(0.5f);
				glm::vec3 b_max = b->position + b->size * glm::vec3(0.5f);

				glm::vec3 overlap = glm::vec3(0);
				overlap.x = std::min(a_max.x, b_max.x) - std::max(a_min.x, b_min.x);
				overlap.y = std::min(a_max.y, b_max.y) - std::max(a_min.y, b_min.y);
				overlap.z = std::min(a_max.z, b_max.z) - std::max(a_min.z, b_min.z);

				// Find min overlap
				float minOverlap = overlap.x;
				glm::vec3 axis(1, 0, 0);

				if (overlap.y < minOverlap)
				{
					minOverlap = overlap.y;
					axis = glm::vec3(0, 1, 0);
				}

				if (overlap.z < minOverlap)
				{
					minOverlap = overlap.z;
					axis = glm::vec3(0, 0, 1);
				}

				glm::vec3 direction = a->position - b->position;

				if (glm::dot(direction, axis) < 0.0f)
					axis *= -1.0f;

				glm::vec3 correction = axis * minOverlap * 0.5f;

				if (glm::dot(direction, axis) < 0.0f)
					correction *= -1.0f;

				if (a->bIsStatic && !b->bIsStatic)
				{
					b->position -= correction * 2.0f; // full push to dynamic object
				}
				else if (!a->bIsStatic && b->bIsStatic)
				{
					a->position += correction * 2.0f;
				}
				else
				{
					a->position += correction;
					b->position -= correction;
				}
			}
		}
	}
}

void PhysicsEngine::ResolveAABBCollisions(float dt)
{
	for (auto& pair : collidingCuboidPairs)
	{
		Cuboid* a = pair.first;
		Cuboid* b = pair.second;

		float m1 = a->mass;
		float m2 = b->mass;
		float totalMass = m1 + m2;

		// Calculate velocity
		glm::vec3 va = (a->position - a->previousPosition) / dt;
		glm::vec3 vb = (b->position - b->previousPosition) / dt;

		glm::vec3 normal = b->position - a->position;
		float dist = glm::length(normal);
		if (dist == 0.0f) continue;

		glm::vec3 unitNormal = glm::normalize(normal);

		// Decay vectors into normal and tangential directions
		glm::vec3 va_n = glm::dot(va, unitNormal) * unitNormal;
		glm::vec3 vb_n = glm::dot(vb, unitNormal) * unitNormal;

		glm::vec3 va_t = va - va_n;
		glm::vec3 vb_t = vb - vb_n;

		// Perform elastic collision physics on normal vectors
		glm::vec3 va_n_after = (((m1 - m2) * va_n + 2 * m2 * vb_n)) / totalMass;
		glm::vec3 vb_n_after = ((2 * m1 * va_n + (m2 - m1) * vb_n)) / totalMass;

		// Combine the vectors to get the final velocity vector
		glm::vec3 va_after = va_n_after + va_t;
		glm::vec3 vb_after = vb_n_after + vb_t;

		a->previousPosition = a->position - va_after * dt;
		b->previousPosition = b->position - vb_after * dt;
	}
}

void PhysicsEngine::UpdateCuboidPositions(float dt)
{
	// Update cuboid pos
	for (auto& object : objects)
	{
		if (object->GetType() != ObjectType::Cuboid)
			continue;

		glm::vec3 vel = (object->position - object->previousPosition) / dt;

		if (glm::length(vel) < 0.01f)
			object->previousPosition = object->position;

		object->UpdatePos(dt);
	}
}

void PhysicsEngine::CheckSphereAABBCollisions()
{
	auto DoSpheresAABBCollide = [](const Cuboid& cuboid, const Sphere& sphere) {
		glm::vec3 min = cuboid.position - 0.5f * cuboid.size;
		glm::vec3 max = cuboid.position + 0.5f * cuboid.size;

		glm::vec3 closestPoint = glm::clamp(sphere.position, min, max);
		float distance = glm::length(closestPoint - sphere.position);
		
		return distance < sphere.radius;
	};

	collidingSphereCuboidPairs.clear();

	for (int i = 0; i < objects.size(); i++)
	{
		for (int j = i+1; j < objects.size(); j++)
		{
			Cuboid* cuboid = nullptr;
			Sphere* sphere = nullptr;

			if (objects[i]->GetType() == ObjectType::Cuboid && objects[j]->GetType() == ObjectType::Sphere)
			{
				cuboid = static_cast<Cuboid*>(objects[i].get());
				sphere = static_cast<Sphere*>(objects[j].get());
			}
			else if (objects[i]->GetType() == ObjectType::Sphere && objects[j]->GetType() == ObjectType::Cuboid)
			{
				sphere = static_cast<Sphere*>(objects[i].get());
				cuboid = static_cast<Cuboid*>(objects[j].get());
			}
			else
			{
				continue;
			}

			if (!cuboid || !sphere)
				continue;

			glm::vec3 min = cuboid->position - 0.5f * cuboid->size;
			glm::vec3 max = cuboid->position + 0.5f * cuboid->size;

			glm::vec3 closestPoint = glm::clamp(sphere->position, min, max);
			glm::vec3 delta = sphere->position - closestPoint;
			float distance = glm::length(delta);

			if (distance == 0.0f)
				continue;

			if (distance < sphere->radius)
			{
				std::cout << "sphere-aabb collisions!\n";

				collidingSphereCuboidPairs.push_back(std::make_pair(sphere, cuboid));

				float overlap = sphere->radius - distance;
				glm::vec3 pushDir = glm::normalize(delta);

				sphere->position += 0.5f * pushDir * overlap;
				cuboid->position -= 0.5f * pushDir * overlap;
			}
		}
	}
}

void PhysicsEngine::ResolveSphereAABBCollisions(float dt)
{
	for (auto& pair : collidingSphereCuboidPairs)
	{
		Sphere* sphere = pair.first;
		Cuboid* cuboid = pair.second;

		float m_sphere = sphere->mass;
		float m_cuboid = cuboid->mass;
		float totalMass = m_sphere + m_cuboid;

		glm::vec3 min = cuboid->position - 0.5f * cuboid->size;
		glm::vec3 max = cuboid->position + 0.5f * cuboid->size;

		glm::vec3 closestPoint = glm::clamp(sphere->position, min, max);

		// Calculate velocity
		glm::vec3 v_sphere = (sphere->position - sphere->previousPosition) / dt;
		glm::vec3 v_cuboid = (cuboid->position - cuboid->previousPosition) / dt;

		glm::vec3 normal = sphere->position - closestPoint;
		float dist = glm::length(normal);
		
		if (dist == 0.0f) continue;

		glm::vec3 unitNormal = glm::normalize(normal);

		// Decay vectors into normal and tangential directions
		glm::vec3 vn_sphere = glm::dot(v_sphere, unitNormal) * unitNormal;
		glm::vec3 vn_cuboid = glm::dot(v_cuboid, unitNormal) * unitNormal;

		glm::vec3 vt_sphere = v_sphere - vn_sphere;
		glm::vec3 vt_cuboid = v_cuboid - vn_cuboid;

		// Perform elastic collision physics on normal vectors
		glm::vec3 vn_sphere_after = (((m_sphere - m_cuboid) * vn_sphere + 2 * m_cuboid * vn_cuboid)) / totalMass;
		glm::vec3 vn_cuboid_after = ((2 * m_sphere * vn_sphere + (m_cuboid - m_sphere) * vn_cuboid)) / totalMass;

		// Combine the vectors to get the final velocity vector
		glm::vec3 v_sphere_final = vn_sphere_after + vt_sphere;
		glm::vec3 v_cuboid_final = vn_cuboid_after + vt_cuboid;

		sphere->previousPosition = sphere->position - v_sphere_final * dt;
		cuboid->previousPosition = cuboid->position - v_cuboid_final * dt;
	}
}