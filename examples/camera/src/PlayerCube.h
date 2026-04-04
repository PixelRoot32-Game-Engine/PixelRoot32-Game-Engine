#pragma once
#include "physics/KinematicActor.h"
#include "graphics/Renderer.h"

namespace camerademo {

/**
 * @class PlayerCube
 * @brief Platformer player with gravity, jump, and one-way platforms.
 */
class PlayerCube : public pixelroot32::physics::KinematicActor {
public:
    /**
     * @brief Constructs player at position.
     * @param position World position (top-left).
     * @param w Width in pixels.
     * @param h Height in pixels.
     */
    PlayerCube(pixelroot32::math::Vector2 position, int w, int h);

    /**
     * @brief Sets input state for this frame.
     * @param dir Horizontal direction (-1.0 to 1.0).
     * @param jumpPressed True if jump was pressed this frame.
     */
    void setInput(float dir, bool jumpPressed);

    void update(unsigned long deltaTime) override;

    void draw(pixelroot32::graphics::Renderer& renderer) override;

    void reset(pixelroot32::math::Vector2 newPos);

private:
    float moveSpeed;
    float jumpImpulse;
    float moveDir;
    bool wantsJump;
    pixelroot32::math::Vector2 velocity;
};

}
