#include "player/Player.h"
#include "world/World.h"

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

// ── helpers ───────────────────────────────────────────────────────────

// ── axis-separated resolution ─────────────────────────────────────────
// Each axis resolved independently to prevent corner-locking
void Player::ResolveX(const World& world)
{
    const float e = 0.001f;
    // Epsilon contracts Y and Z — avoids sampling floor/wall blocks at exact boundaries
    int y0 = (int)std::floor(pos.y + e);
    int y1 = (int)std::floor(pos.y + HEIGHT - e);
    int z0 = (int)std::floor(pos.z - HALF_W + e);
    int z1 = (int)std::floor(pos.z + HALF_W - e);

    if (vel.x > 0.0f)
    {
        float px1 = pos.x + HALF_W;
        int   bx = (int)std::floor(px1);
        float push = 0.0f;
        for (int y = y0; y <= y1; y++)
            for (int z = z0; z <= z1; z++)
            {
                if (!world.IsSolid(bx, y, z)) continue;
                float pen = px1 - (float)bx;
                if (pen > 0.0f) push = std::min(push, -pen);
            }
        pos.x += push;
        if (push != 0.0f) vel.x = 0.0f;
    }
    else if (vel.x < 0.0f)
    {
        float px0 = pos.x - HALF_W;
        int   bx = (int)std::floor(px0);
        float push = 0.0f;
        for (int y = y0; y <= y1; y++)
            for (int z = z0; z <= z1; z++)
            {
                if (!world.IsSolid(bx, y, z)) continue;
                float pen = (float)(bx + 1) - px0;
                if (pen > 0.0f) push = std::max(push, pen);
            }
        pos.x += push;
        if (push != 0.0f) vel.x = 0.0f;
    }
}

void Player::ResolveZ(const World& world)
{
    const float e = 0.001f;
    int x0 = (int)std::floor(pos.x - HALF_W + e);
    int x1 = (int)std::floor(pos.x + HALF_W - e);
    int y0 = (int)std::floor(pos.y + e);
    int y1 = (int)std::floor(pos.y + HEIGHT - e);

    if (vel.z > 0.0f)
    {
        float pz1 = pos.z + HALF_W;
        int   bz = (int)std::floor(pz1);
        float push = 0.0f;
        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
            {
                if (!world.IsSolid(x, y, bz)) continue;
                float pen = pz1 - (float)bz;
                if (pen > 0.0f) push = std::min(push, -pen);
            }
        pos.z += push;
        if (push != 0.0f) vel.z = 0.0f;
    }
    else if (vel.z < 0.0f)
    {
        float pz0 = pos.z - HALF_W;
        int   bz = (int)std::floor(pz0);
        float push = 0.0f;
        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
            {
                if (!world.IsSolid(x, y, bz)) continue;
                float pen = (float)(bz + 1) - pz0;
                if (pen > 0.0f) push = std::max(push, pen);
            }
        pos.z += push;
        if (push != 0.0f) vel.z = 0.0f;
    }
}

void Player::ResolveY(const World& world)
{
    const float e = 0.001f;
    int x0 = (int)std::floor(pos.x - HALF_W + e);
    int x1 = (int)std::floor(pos.x + HALF_W - e);
    int z0 = (int)std::floor(pos.z - HALF_W + e);
    int z1 = (int)std::floor(pos.z + HALF_W - e);

    if (vel.y < 0.0f)
    {
        int   by = (int)std::floor(pos.y);
        float push = 0.0f;
        for (int x = x0; x <= x1; x++)
            for (int z = z0; z <= z1; z++)
            {
                if (!world.IsSolid(x, by, z)) continue;
                float pen = (float)(by + 1) - pos.y;
                if (pen > 0.0f) push = std::max(push, pen);
            }
        pos.y += push;
        if (push != 0.0f) { onGround = true; vel.y = 0.0f; }
    }
    else if (vel.y > 0.0f)
    {
        float py1 = pos.y + HEIGHT;
        int   by = (int)std::floor(py1);
        float push = 0.0f;
        for (int x = x0; x <= x1; x++)
            for (int z = z0; z <= z1; z++)
            {
                if (!world.IsSolid(x, by, z)) continue;
                float pen = py1 - (float)by;
                if (pen > 0.0f) push = std::min(push, -pen);
            }
        pos.y += push;
        if (push != 0.0f) vel.y = 0.0f;
    }
}

// ── main update ───────────────────────────────────────────────────────
void Player::Update(float dt, const glm::vec3& camFront, bool fwd, bool back, bool left, bool right, bool jump, bool crouch, bool booster, bool shouldUpdateControls, const World& world)
{
    // XZ movement — flatten camFront onto horizontal plane
    glm::vec3 flatFront = glm::normalize(glm::vec3(camFront.x, 0.0f, camFront.z));
    glm::vec3 flatRight = glm::normalize(glm::cross(flatFront, glm::vec3(0.0f, 1.0f, 0.0f)));

    glm::vec3 move(0.0f);

    if (shouldUpdateControls)
    {
        if (fwd)   move += flatFront;
        if (back)  move -= flatFront;
        if (right) move += flatRight;
        if (left)  move -= flatRight;
    }

    if (glm::length(move) > 0.001f)
        move = glm::normalize(move);

    vel.x = (booster ? 2.0f : 1.0f) * move.x * SPEED;
    vel.z = (booster ? 2.0f : 1.0f) * move.z * SPEED;

    // Gravity
    if (!canFly)
    {
        vel.y += GRAVITY * dt;
        vel.y = std::max(vel.y, MAX_FALL);
    }
    else
    {
        if (jump)
            vel.y = (booster ? 2.0f : 1.0f) * SPEED;
        else if (crouch)
            vel.y = -(booster ? 2.0f : 1.0f) * SPEED;
        else
            vel.y = 0.0f;
    }

    // Jump — single frame trigger, only when grounded
    if (jump && onGround && !canFly)
    {
        vel.y = JUMP_VEL;
        onGround = false;
    }

    // Move + resolve each axis independently
    pos.x += vel.x * dt;  ResolveX(world);
    onGround = false;        // reset before Y resolve
    pos.y += vel.y * dt;  ResolveY(world);   // sets onGround=true if floor hit
    pos.z += vel.z * dt;  ResolveZ(world);
}