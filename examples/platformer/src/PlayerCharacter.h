#pragma once
#include "graphics/Renderer.h"
#include "physics/KinematicActor.h"

namespace platformerdemo {

/**
 * @class PlayerCharacter
 * @brief `KinematicActor` platformer character: gravity, run, jump, floor snap.
 *
 * Godot equivalent: a `CharacterBody2D` moved with `move_and_slide()`.
 *
 * Nothing in the engine applies gravity to a kinematic body — the script owns
 * the velocity, and the engine only resolves it against the level. That is why
 * the vertical speed is integrated here and then handed to `moveAndSlide()`.
 */
class PlayerCharacter : public pixelroot32::physics::KinematicActor {
public:
    /**
     * @param position Spawn position (top-left).
     * @param w Width in pixels.
     * @param h Height in pixels.
     */
    PlayerCharacter(pixelroot32::math::Vector2 position, int w, int h);

    /**
     * @brief Sets this frame's input.
     * @param dir Horizontal direction, -1, 0 or 1.
     * @param jumpPressed true on the frame the jump button went down.
     */
    void setInput(float dir, bool jumpPressed);

    void update(unsigned long deltaTime) override;

    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    float moveDir = 0.0f;
    bool wantsJump = false;
};

}
