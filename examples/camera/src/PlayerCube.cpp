#include "PlayerCube.h"
#include "core/Actor.h"
#include "GameConstants.h"
#include "GameLayers.h"

namespace pr32 = pixelroot32;

namespace camerademo {
    
namespace gfx = pixelroot32::graphics;
namespace math = pixelroot32::math;

using gfx::Color;

PlayerCube::PlayerCube(math::Vector2 position, int w, int h)
    : KinematicActor(position, w, h)
    , moveSpeed(PLAYER_MOVE_SPEED)
    , jumpImpulse(PLAYER_JUMP_VELOCITY)
    , moveDir(0.0f)
    , wantsJump(false)
    , velocity(0.0f, 0.0f) {
}

void PlayerCube::setInput(float dir, bool jumpPressed) {
    moveDir = dir;
    if (jumpPressed) {
        wantsJump = true;
    }
}

void PlayerCube::update(unsigned long deltaTime) {
    float dt = deltaTime * 0.001f;

    velocity.y += math::toScalar(PLAYER_GRAVITY * dt);
    velocity.x = math::toScalar(moveDir * moveSpeed);
    if (wantsJump && is_on_floor()) {
        velocity.y = math::toScalar(-jumpImpulse);
    }
    wantsJump = false;

    if (velocity.y < math::toScalar(0)) {
        setCollisionMask(Layers::GROUND);
    } else {
        setCollisionMask(Layers::GROUND | Layers::PLATFORM);
    }

    moveAndSlide(velocity * pixelroot32::math::toScalar(dt), pixelroot32::math::Vector2(0, -1));
    if (is_on_floor()) {
        velocity.y = pixelroot32::math::toScalar(0);
    } else if (is_on_ceiling()) {
        velocity.y = pixelroot32::math::toScalar(0);
    }

    if (worldWidth > 0) {
        if (position.x < math::toScalar(0)) {
            position.x = math::toScalar(0);
            velocity.x = math::toScalar(0);
        } else if (position.x + math::toScalar(width) > math::toScalar(worldWidth)) {
            position.x = math::toScalar(worldWidth - width);
            velocity.x = math::toScalar(0);
        }
    }
    
    if (worldHeight > 0) {
        if (position.y > math::toScalar(worldHeight)) {
             reset(math::Vector2(PLAYER_START_X, PLAYER_START_Y));
        }
    }
}

void PlayerCube::draw(gfx::Renderer& renderer) {
    renderer.drawFilledRectangle(static_cast<int>(position.x),
                                 static_cast<int>(position.y),
                                 width,
                                 height,
                                 Color::Cyan);
}

void PlayerCube::reset(math::Vector2 newPos) {
    position = newPos;
    velocity.x = math::toScalar(0.0f);
    velocity.y = math::toScalar(0.0f);
    moveDir = 0.0f;
    wantsJump = false;
}

}
