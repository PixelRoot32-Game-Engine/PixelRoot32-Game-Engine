#pragma once

#include "../GameLayers.h"
#include <physics/RigidActor.h>
#include <graphics/Color.h>

namespace brickbreaker {

/**
 * @class BallActor
 * @brief Physics-simulated ball with paddle-sticking mechanic
 * 
 * Architecture Pattern:
 * Demonstrates RigidActor with gameplay-driven velocity modification.
 * The physics system handles position integration and wall bounces,
 * but gameplay code (onCollision) directly modifies velocity for
 * arcade-style paddle physics.
 * 
 * Paddle-Sticking Mechanic:
 * Before launch, the ball "sticks" to the paddle, moving with it.
 * This allows players to aim their serve before committing to launch.
 * Once launched, standard physics simulation takes over.
 * 
 * Collision Response Hierarchy:
 * 1. WALL: Physics handles bounce, callback plays sound
 * 2. PADDLE: Gameplay applies "english" (angle based on hit position)
 * 3. BRICK: Gameplay destroys brick, applies score
 * 
 * This demonstrates hybrid physics/gameplay collision handling where
 * some collisions are fully automatic (walls) while others trigger
 * gameplay logic (paddle/brick).
 */
class BallActor : public pixelroot32::physics::RigidActor {
public:
    int radius;            ///< Ball radius in pixels
    bool isLaunched;       ///< False when sticking to paddle

    /**
     * @brief Constructs ball at position.
     * @param position World position (center).
     * @param initialSpeed Unused; kept for API compatibility.
     * @param radius Ball radius in pixels.
     */
    BallActor(pixelroot32::math::Vector2 position, pixelroot32::math::Scalar initialSpeed, int radius);

    /**
     * @brief Attaches ball to paddle before launch
     * 
     * Ball becomes "stuck" to paddle, moving with it until launch().
     * This gives players aiming control before committing to shot.
     */
    void attachTo(pixelroot32::core::Actor* paddle);
    
    /**
     * @brief Launches ball with specified velocity
     * 
     * Transitions from "stuck" state to physics simulation.
     * Velocity is applied and physics system takes over movement.
     */
    void launch(pixelroot32::math::Vector2 velocity);
    
    /**
     * @brief Resets to paddle-sticking state
     */
    void reset(pixelroot32::core::Actor* paddle);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    
    /**
     * @brief Handles paddle "english" and brick destruction
     * 
     * PADDLE: Applies velocity modification based on hit position
     * BRICK: Destroys brick, adds score, spawns particles
     * WALL: Sound only (physics handles bounce)
     */
    void onCollision(pixelroot32::core::Actor* other) override;
    
    /**
     * @brief Handles wall collision sounds
     */
    void onWorldCollision() override;

private:
    pixelroot32::math::Scalar initialSpeed;    ///< Unused; kept for API compatibility
    pixelroot32::core::Actor* paddleReference = nullptr; ///< Paddle to stick to before launch
};

}
