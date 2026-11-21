#include "PlayerCharacter.h"

#include "GameConstants.h"

namespace platformerdemo {

namespace math = pixelroot32::math;
namespace physics = pixelroot32::physics;

using math::Scalar;
using math::Vector2;
using math::toScalar;

PlayerCharacter::PlayerCharacter(Vector2 position, int w, int h)
    : KinematicActor(position, w, h) {
    setShape(pixelroot32::core::CollisionShape::AABB);
}

void PlayerCharacter::setInput(float dir, bool jumpPressed) {
    moveDir = dir;
    if (jumpPressed) {
        wantsJump = true;
    }
}

void PlayerCharacter::update(unsigned long deltaTime) {
    const float dtSeconds = static_cast<float>(deltaTime) * 0.001f;
    const Scalar dt = toScalar(dtSeconds);

    // The jump is read before it is consumed: floor snap has to be disabled on
    // the same frame the body leaves the ground, or it is pulled back down.
    const bool jumpThisFrame = wantsJump;

    velocity.y += toScalar(PLAYER_GRAVITY * dtSeconds);
    velocity.x = toScalar(moveDir * PLAYER_MOVE_SPEED);

    if (jumpThisFrame && is_on_floor()) {
        velocity.y = toScalar(-PLAYER_JUMP_VELOCITY);
    }
    wantsJump = false;

    // SnapPolicy::Step keeps the body glued to the floor while walking, and
    // only while it was already on the floor, so a jump is not undone. The snap
    // vector must be zero on the jump frame itself.
    const Vector2 snap = jumpThisFrame
        ? Vector2()
        : Vector2(toScalar(0.0f), KinematicActor::MIN_SNAP);

    velocity = moveAndSlide(velocity, dt, Vector2(0, -1), physics::SnapPolicy::Step, snap);
}

void PlayerCharacter::draw(pixelroot32::graphics::Renderer& renderer) {
    const int x = static_cast<int>(position.x);
    const int y = static_cast<int>(position.y);
    renderer.drawFilledRectangle(x, y, width, height, pixelroot32::graphics::Color::Cyan);
    renderer.drawRectangle(x, y, width, height, pixelroot32::graphics::Color::White);
}

}
