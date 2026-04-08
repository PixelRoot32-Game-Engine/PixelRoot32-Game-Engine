#include "ProjectileActor.h"
#include "GameConstants.h"
#include <physics/CollisionSystem.h>

namespace spaceinvaders {

namespace core = pixelroot32::core;
namespace gfx = pixelroot32::graphics;
namespace math = pixelroot32::math;
namespace physics = pixelroot32::physics;

ProjectileActor::ProjectileActor(math::Vector2 position, ProjectileType type)
    : RigidActor(position, PROJECTILE_WIDTH, PROJECTILE_HEIGHT), type(type), active(true), previousPosition(position) {
    
    setRestitution(math::toScalar(0.0f));
    setFriction(math::toScalar(0.0f));
    setGravityScale(math::toScalar(0.0f));
    setShape(core::CollisionShape::CIRCLE);
    setRadius(math::toScalar(PROJECTILE_WIDTH * 0.5f));
    setBounce(false);

    math::Scalar speed = (type == ProjectileType::PLAYER_BULLET) ? math::toScalar(-PROJECTILE_SPEED) : math::toScalar(PROJECTILE_SPEED);
    setVelocity(math::toScalar(0), speed);
}

void ProjectileActor::reset(math::Vector2 newPosition, ProjectileType newType) {
    position = newPosition;
    previousPosition = newPosition;
    type = newType;
    active = true;
    math::Scalar speed = (type == ProjectileType::PLAYER_BULLET) ? math::toScalar(-PROJECTILE_SPEED) : math::toScalar(PROJECTILE_SPEED);
    setVelocity(math::toScalar(0), speed);
}

void ProjectileActor::update(unsigned long deltaTime) {
    if (!active) return;

    previousPosition = position;
    physics::RigidActor::update(deltaTime);

    if (position.y < math::toScalar(-PROJECTILE_HEIGHT) || position.y > math::toScalar(DISPLAY_HEIGHT)) {
        active = false;
    }
}

void ProjectileActor::draw(gfx::Renderer& renderer) {
    if (!active) return;
    
    gfx::Color c = (type == ProjectileType::PLAYER_BULLET) ? gfx::Color::White : gfx::Color::Orange;
    
    renderer.drawFilledRectangle(static_cast<int>(position.x), static_cast<int>(position.y), 
                                 static_cast<int>(width), static_cast<int>(height), c);
}

void ProjectileActor::onCollision(core::Actor* other) {
    (void)other;
    active = false;
}

}
