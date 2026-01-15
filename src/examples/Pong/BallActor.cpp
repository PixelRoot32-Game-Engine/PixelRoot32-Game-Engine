#include "BallActor.h"
#include "Engine.h"
#include <cstdlib>
#include <cmath>
#include <Config.h>

namespace pong {

namespace pr32 = pixelroot32;

using Color = pr32::graphics::Color;

BallActor::BallActor(float x, float y, float initialSpeed, int radius)
    : PhysicsActor(x, y, radius * 2.0f, radius * 2.0f),
      radius(radius),
      isActive(false),
      respawnTimer(0),
      initialSpeed(initialSpeed)
{
    vx = 0;
    vy = 0;

    setCollisionLayer(Layers::BALL);
    setCollisionMask(Layers::PADDLE);
}

void BallActor::reset() {
    x = worldWidth / 2.0f;
    y = worldHeight / 2.0f;

    vx = 0;
    vy = 0;
    respawnTimer = 500;
    isActive = false;
    isEnabled = true;
}

void BallActor::update(unsigned long deltaTime) {
    if (!isActive) {
        if (respawnTimer > deltaTime) {
            respawnTimer -= deltaTime;
            return;
        } else {
            respawnTimer = 0;
            isActive = true;

            vx = (rand() % 2 == 0 ? 1 : -1) * initialSpeed;
            vy = ((rand() % 100) / 100.0f - 0.5f) * initialSpeed;
        }
    }

    PhysicsActor::update(deltaTime);
}

void BallActor::draw(pr32::graphics::Renderer& renderer) {
    if(isEnabled) 
        renderer.drawFilledCircle((int)x, (int)y, radius, Color::White);
}

pixelroot32::core::Rect BallActor::getHitBox() {
    return { x - radius, y - radius, radius * 2, radius * 2 };
}

void BallActor::onCollision(pr32::core::Actor* other) {
    PhysicsActor::onCollision(other);

    if (vx > 0) {
        x = other->x + other->width + radius;
    } else {
        x = other->x - radius;
    }

    float impactPos = (y - other->y) / other->height - 0.5f;
    vy += impactPos * 50.0f;
}

}