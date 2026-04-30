#pragma once
#include <glm/glm.hpp>
#include "world/World.h"

class Player
{
public:
    glm::vec3 pos = glm::vec3(0.0f, 10.0f, 0.0f);  // feet
    glm::vec3 vel = glm::vec3(0.0f);
    bool onGround = false;
    bool canFly = true;

    // Dimensions
    static constexpr float HALF_W = 0.30f;   // AABB half-width (total 0.6)
    static constexpr float HEIGHT = 1.80f;   // AABB full height
    static constexpr float EYE_OFF = 1.60f;   // eye height above feet
    static constexpr float SPEED = 10.0f;
    static constexpr float JUMP_VEL = 8.0f;
    static constexpr float GRAVITY = -24.0f;
    static constexpr float MAX_FALL = -50.0f;

    void Update(
        float dt,
        const glm::vec3& camFront,
        bool fwd, bool back, bool left, bool right,
        bool jump, bool crouch, bool booster,
        const World& world
    );

    glm::vec3 EyePos() const { return pos + glm::vec3(0.0f, EYE_OFF, 0.0f); }

private:
    void  ResolveX(const World& world);
    void  ResolveY(const World& world);
    void  ResolveZ(const World& world);
};