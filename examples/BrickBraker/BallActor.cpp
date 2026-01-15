#include "BallActor.h"
#include <core/Engine.h>

namespace brickbreaker {

namespace pr32 = pixelroot32;

using Color = pr32::graphics::Color;

BallActor::BallActor(float x, float y, float initialSpeed, int radius)
    : PhysicsActor(x, y, radius * 2.0f, radius * 2.0f),
      radius(radius),
      isLaunched(false),
      initialSpeed(initialSpeed)
{
    vx = 0;
    vy = 0;

    setCollisionLayer(Layers::BALL);
    setCollisionMask(Layers::PADDLE | Layers::BRICK);
}

void BallActor::attachTo(pixelroot32::core::Actor* paddle) {
    this->paddleReference = paddle;
    this->isLaunched = false;
    this->vx = 0;
    this->vy = 0;
}

void BallActor::launch(float velocityX, float velocityY) {
    this->vx = velocityX;
    this->vy = velocityY;
    this->isLaunched = true;
    this->paddleReference = nullptr;
}

void BallActor::reset(pixelroot32::core::Actor* paddle) {
    attachTo(paddle);
}

void BallActor::update(unsigned long deltaTime) {
    if (!isLaunched && paddleReference != nullptr) {
        this->x = paddleReference->x + (paddleReference->width / 2);
        this->y = paddleReference->y - (this->radius * 2) - 2;

        PhysicsActor::update(deltaTime);
        return; 
    }

    PhysicsActor::update(deltaTime);
}

void BallActor::draw(pr32::graphics::Renderer&  renderer) {
   renderer.drawFilledCircle((int)x, (int)y, radius, Color::White);
}

void BallActor::onCollision(Actor* other) {
    //if (!isLaunched) return;

    PhysicsActor::onCollision(other);
    
    if (other->isInLayer(Layers::PADDLE)) {
        if (vy > 0) {
            float hitPoint = (this->x - (other->x + other->width / 2.0f)) / (other->width / 2.0f);
            this->vx = hitPoint * 150.0f;
            this->vy *= -1;
            this->y = other->y - (this->radius * 2) - 1;
        }
    }

    if (other->isInLayer(Layers::BRICK)) {
        this->vy *= -1;
    }
}

}
