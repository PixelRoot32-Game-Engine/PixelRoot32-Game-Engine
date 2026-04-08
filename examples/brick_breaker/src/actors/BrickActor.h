#pragma once

#include "../GameLayers.h"

#include <physics/StaticActor.h>

namespace brickbreaker {

/**
 * @class BrickActor
 * @brief Destructible obstacle with hit-point-based durability
 * 
 * Architecture Pattern:
 * Demonstrates StaticActor for immovable, destructible obstacles.
 * StaticActors have zero physics overhead since they never move,
 * making them ideal for stationary level geometry.
 * 
 * Health System:
 * Bricks have multiple hit points (hp), requiring multiple hits
 * to destroy. This creates gameplay depth and strategic choices.
 * 
 * Color Coding by Health:
 * - 4 HP: Dark Gray (toughest)
 * - 3 HP: Red
 * - 2 HP: Orange
 * - 1 HP: Yellow (weakest)
 * 
 * Destruction Sequence:
 * 1. Ball collides with brick
 * 2. Ball::onCollision calls brick->hit()
 * 3. Health decrements
 * 4. If health reaches 0: active = false, collision disabled
 * 5. Score awarded based on damage dealt
 * 
 * Collision Optimization:
 * When destroyed (hp <= 0), collision layers are cleared to 0,
 * preventing unnecessary collision checks against invisible bricks.
 */
class BrickActor : public pixelroot32::physics::StaticActor {
public:
    int hp;       ///< Current hit points
    bool active;  ///< Visibility and collision state

    /**
     * @brief Constructs brick at position.
     * @param position World position (top-left).
     * @param hp Initial hit points (1-4).
     */
    BrickActor(pixelroot32::math::Vector2 position, int hp);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    
    /**
     * @brief Applies damage to brick
     * 
     * Reduces HP by 1. If HP reaches 0, deactivates brick
     * and disables collision detection.
     */
    void hit(); 
    
    /**
     * @brief Returns color based on current HP
     */
    pixelroot32::graphics::Color getColor();

    void onCollision(pixelroot32::core::Actor* other) override;
};

}
