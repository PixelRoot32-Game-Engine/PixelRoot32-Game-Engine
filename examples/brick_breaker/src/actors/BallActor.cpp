#include "BallActor.h"
#include "BrickBreakerScene.h"
#include "BrickActor.h"
#include "GameConstants.h"

#include <core/Engine.h>
#include <physics/CollisionSystem.h>

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace brickbreaker {

namespace core = pr32::core;
namespace gfx = pr32::graphics;
namespace math = pr32::math;

BallActor::BallActor(math::Vector2 position, math::Scalar initialSpeed, int radius)
    : RigidActor(math::Vector2(position.x - radius, position.y - radius), radius * 2, radius * 2),
      radius(radius),
      isLaunched(false),
      initialSpeed(initialSpeed)
{
    velocity.x = math::Scalar(0);
    velocity.y = math::Scalar(0);

    setRestitution(math::Scalar(1.0f));
    setFriction(math::Scalar(0.0f));
    setGravityScale(math::Scalar(0.0f));
    setShape(core::CollisionShape::CIRCLE);
    setRadius(math::Scalar(radius));
    setBounce(true);

    setCollisionLayer(Layers::BALL);
    setCollisionMask(Layers::PADDLE | Layers::BRICK | Layers::WALL);
}

void BallActor::attachTo(core::Actor* paddle) {
    this->paddleReference = paddle;
    this->isLaunched = false;
    this->velocity.x = math::Scalar(0);
    this->velocity.y = math::Scalar(0);
}

void BallActor::launch(math::Vector2 velocity) {
    this->velocity = velocity;
    this->isLaunched = true;
    this->paddleReference = nullptr;
}

void BallActor::reset(core::Actor* paddle) {
    attachTo(paddle);
}

void BallActor::update(unsigned long deltaTime) {
    if (!isLaunched && paddleReference != nullptr) {
        this->position.x = paddleReference->position.x + (paddleReference->width / 2) - radius;
        this->position.y = paddleReference->position.y - (radius * 2) - 2;
        return;
    }

    RigidActor::update(deltaTime);

    if (isLaunched && abs(velocity.y) < math::Scalar(10)) {
        velocity.y = (velocity.y >= math::Scalar(0)) ? math::Scalar(10) : math::Scalar(-10);
    }
}

void BallActor::draw(gfx::Renderer& renderer) {
    renderer.drawFilledCircle((int)position.x + radius, (int)position.y + radius, radius, gfx::Color::White);
}

void BallActor::onCollision(core::Actor* other) {
    if (other->isInLayer(Layers::WALL)) {
        engine.getAudioEngine().playEvent(sfx::WALL_HIT);
        return;
    }

    if (other->isInLayer(Layers::PADDLE)) {
        math::Scalar hitPoint = (this->position.x + math::toScalar(radius) - 
                            (other->position.x + math::toScalar(other->width / 2))) / 
                            math::toScalar(other->width / 2);

        if (hitPoint > math::toScalar(1)) hitPoint = math::toScalar(1);
        if (hitPoint < math::toScalar(-1)) hitPoint = math::toScalar(-1);

        this->velocity.x = math::toScalar(hitPoint * 150);
        this->velocity.y = -math::abs(this->velocity.y);

        if (math::abs(this->velocity.y) < math::toScalar(50)) {
            this->velocity.y = math::toScalar(-50);
        }

        engine.getAudioEngine().playEvent(sfx::PADDLE_HIT);
    }

    if (other->isInLayer(Layers::BRICK)) {
        BrickActor* brick = static_cast<BrickActor*>(other);
        if (brick && brick->active) {
            int previousHp = brick->hp;
            brick->hit();

            engine.getAudioEngine().playEvent(sfx::BRICK_CRACK);

            BrickBreakerScene* scene = static_cast<BrickBreakerScene*>(
                engine.getCurrentScene().value_or(nullptr));
            if (scene) {
                scene->getParticleEmiter()->burst(
                    math::Vector2(brick->position.x + (brick->width / 2), 
                                       brick->position.y + (brick->height / 2)), 
                    8);

                if (brick->hp <= 0 && previousHp > 0) {
                    scene->addScore(50);
                } else if (brick->hp < previousHp) {
                    scene->addScore(10);
                }
            }
        }
    }
}

void BallActor::onWorldCollision() {
    if (worldCollisionInfo.left || worldCollisionInfo.right || worldCollisionInfo.top) {
        engine.getAudioEngine().playEvent(sfx::WALL_HIT);
    }
}

}
