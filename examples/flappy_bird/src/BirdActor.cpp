#include "BirdActor.h"
#include "GameLayers.h"

namespace pr32 = pixelroot32;

namespace flappy {

namespace math = pr32::math;
namespace physics = pr32::physics;
namespace core = pr32::core;
namespace graphics = pr32::graphics;

using math::Vector2;
using math::toScalar;
using physics::RigidActor;
using physics::CollisionShape;

BirdActor::BirdActor(Vector2 pos) 
    : RigidActor(Vector2(pos.x - BIRD_RADIUS, pos.y - BIRD_RADIUS), BIRD_RADIUS * 2, BIRD_RADIUS * 2) {
    
    setShape(CollisionShape::CIRCLE);
    setRadius(toScalar(BIRD_RADIUS));

    setRestitution(toScalar(0.0f));
    setFriction(toScalar(0.0f));
    setGravityScale(toScalar(1.5f));

    setCollisionLayer(Layers::BIRD);
    setCollisionMask(Layers::PIPE | Layers::BOUNDS);
}

void BirdActor::update(unsigned long deltaTime) {
    if (dead) return;
    RigidActor::update(deltaTime);
}

void BirdActor::draw(Renderer& renderer) {
    renderer.drawCircle(static_cast<int>(position.x) + BIRD_RADIUS, 
                        static_cast<int>(position.y) + BIRD_RADIUS, 
                        BIRD_RADIUS, Color::White);
}

void BirdActor::onCollision(Actor* other) {
    if (!dead) {
        dead = true;
    }
}

void BirdActor::jump() {
    if (!dead) {
        velocity.y = toScalar(JUMP_FORCE);
    }
}

void BirdActor::reset(Vector2 startPos) {
    position = Vector2(startPos.x - BIRD_RADIUS, startPos.y - BIRD_RADIUS);
    velocity = Vector2(toScalar(0), toScalar(0));
    dead = false;
}

} // namespace flappy
