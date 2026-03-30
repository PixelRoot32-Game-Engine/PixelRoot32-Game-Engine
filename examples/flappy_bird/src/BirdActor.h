#pragma once

#include <core/Actor.h>
#include <physics/RigidActor.h>

#include "FlappyBirdConstants.h"

namespace flappy {

/**
 * @class BirdActor
 * @brief Player-controlled bird with gravity and jump mechanics.
 *
 * Uses RigidActor for physics simulation. Jump applies upward velocity;
 * gravity pulls the bird down each frame.
 */
class BirdActor : public pixelroot32::physics::RigidActor {
public:
    /**
     * @brief Constructs bird at position.
     * @param pos Center position (converted to top-left for AABB).
     */
    BirdActor(pixelroot32::math::Vector2 pos);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    void onCollision(pixelroot32::core::Actor* other) override;

    /** @brief Applies jump force (upward velocity). */
    void jump();

    /** @brief Resets position and state for new game. */
    void reset(pixelroot32::math::Vector2 startPos);

    /** @brief Returns true if bird has collided (game over). */
    bool isDead() const { return dead; }

private:
    bool dead = false;
};

} // namespace flappy
