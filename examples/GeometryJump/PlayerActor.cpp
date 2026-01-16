#include "PlayerActor.h"
#include "core/Engine.h"
#include "GameConstants.h"

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace geometryjump {

using Color = pr32::graphics::Color;

PlayerActor::PlayerActor(float x, float y, int w, int h, float speedX)
    : pr32::core::PhysicsActor(x, y, w, h), moveSpeedX(speedX) {
    vx = moveSpeedX;
    vy = 0;
    setRestitution(0.0f);
    setCollisionLayer(Layers::PLAYER);
    setCollisionMask(Layers::GROUND | Layers::OBSTACLE);
}

void PlayerActor::update(unsigned long deltaTime) {
    float dt = deltaTime * 0.001f;
    vy += GRAVITY * dt;
    if (vy > MAX_FALL_SPEED) {
        vy = MAX_FALL_SPEED;
    }
    vx = 0.0f;
    pr32::core::PhysicsActor::update(deltaTime);
}

void PlayerActor::draw(pr32::graphics::Renderer& renderer) {
    renderer.drawFilledRectangle(static_cast<int>(x), static_cast<int>(y), width, height, Color::White);
}

bool PlayerActor::isGrounded() const {
    return grounded;
}

void PlayerActor::jump() {
    vy = -JUMP_FORCE;
    grounded = false;
}

void PlayerActor::reset(float startX, float startY) {
    x = startX;
    y = startY;
    vx = 0.0f;
    vy = 0.0f;
    grounded = false;
    isEnabled = true;
}

void PlayerActor::onWorldCollision() {
    auto info = getWorldCollisionInfo();
    grounded = info.bottom;
}

}

