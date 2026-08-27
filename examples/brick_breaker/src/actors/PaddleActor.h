#pragma once

#include "../GameLayers.h"

#include <physics/KinematicActor.h>
#include <input/InputManager.h>
#include <graphics/Color.h>
#include <math/Scalar.h>

namespace brickbreaker {

/**
 * @class PaddleActor
 * @brief Player-controlled paddle with horizontal movement
 * 
 * Architecture Pattern:
 * Demonstrates KinematicActor for player-controlled horizontal movement.
 * Unlike SpaceInvaders which uses grid-based discrete positions, this
 * paddle uses continuous movement for smooth analog-like control.
 * 
 * Movement Model:
 * - Continuous position updates based on input
 * - Frame-rate independent using deltaTime scaling
 * - Boundary clamping to prevent leaving screen
 * 
 * Collision Integration:
 * Uses moveAndSlide() from KinematicActor for collision response.
 * This ensures the paddle stops at walls/obstacles naturally.
 * 
 * Visual Design:
 * Simple rectangular paddle that bounces the ball at angles based
 * on hit position (implemented in BallActor::onCollision).
 */
class PaddleActor : public pixelroot32::physics::KinematicActor {
private:
    pixelroot32::math::Scalar speed = pixelroot32::math::Scalar(200.0f);
    pixelroot32::graphics::Color color = pixelroot32::graphics::Color::White;
    int screenWidth;

public:
    /**
     * @brief Constructs paddle at position.
     * @param position World position (top-left).
     * @param w Width in pixels.
     * @param h Height in pixels.
     * @param sWidth Screen width for boundary clamping.
     */
    PaddleActor(pixelroot32::math::Vector2 position, int w, int h, int sWidth);

    void update(unsigned long dt) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    void onCollision(pixelroot32::core::Actor* other) override;
};

}
