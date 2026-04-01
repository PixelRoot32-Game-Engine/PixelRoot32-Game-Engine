/**
 * @file MockActor.h
 * @brief Mock Actor class for testing ActorTouchController
 * 
 * Provides a simple mock implementation of the Actor interface
 * for unit testing without requiring the full physics system.
 */
#pragma once

#include "core/Actor.h"
#include "math/Scalar.h"
#include "math/Vector2.h"

namespace pixelroot32::core {

/**
 * @class MockActor
 * @brief Mock implementation of Actor for testing purposes
 * 
 * Provides a simple actor with configurable position and hitbox
 * that implements the required Actor interface.
 */
class MockActor : public Actor {
public:
    /**
     * @brief Construct a mock actor
     * @param x Initial X position
     * @param y Initial Y position
     * @param w Width
     * @param h Height
     */
    MockActor(float x, float y, int w, int h)
        : core::Actor(math::toScalar(x), 
                      math::toScalar(y), w, h)
        , mockHitbox{pixelroot32::math::Vector2(math::toScalar(x), math::toScalar(y)), w, h} {}
    
    /**
     * @brief Construct with custom hitbox
     * @param x Initial X position
     * @param y Initial Y position
     * @param w Width
     * @param h Height
     * @param hitboxX Hitbox X offset
     * @param hitboxY Hitbox Y offset
     * @param hitboxW Hitbox width
     * @param hitboxH Hitbox height
     */
    MockActor(float x, float y, int w, int h, 
              float hitboxX, float hitboxY, int hitboxW, int hitboxH)
        : core::Actor(math::toScalar(x), 
                      math::toScalar(y), w, h)
        , mockHitbox{pixelroot32::math::Vector2(math::toScalar(hitboxX), math::toScalar(hitboxY)), hitboxW, hitboxH} {}

    // =====================================================================
    // Actor interface implementation
    // =====================================================================
    
    core::Rect getHitBox() override {
        return mockHitbox;
    }
    
    void onCollision(Actor* other) override {
        (void)other;
        // Mock implementation - do nothing
    }
    
    void update(unsigned long deltaTime) override {
        (void)deltaTime;
    }
    
    void draw(graphics::Renderer& renderer) override {
        (void)renderer;
        // Mock implementation - do nothing
    }
    
    // =====================================================================
    // Test utilities
    // =====================================================================
    
    /**
     * @brief Set the hitbox for testing
     */
    void setHitBox(float x, float y, int w, int h) {
        mockHitbox = core::Rect{
            math::Vector2(math::toScalar(x), 
                          math::toScalar(y)),
            w, h
        };
    }
    
    /**
     * @brief Reset actor position (for testing)
     */
    void resetPosition(float x, float y) {
        position.x = math::toScalar(x);
        position.y = math::toScalar(y);
    }
    
private:
    /// Custom hitbox for testing
    core::Rect mockHitbox;
};

} // namespace pixelroot32::core