#include "ProjectileActor.h"
#include "GameConstants.h"
#include "physics/CollisionSystem.h"

namespace spaceinvaders {

using namespace pixelroot32::math;

ProjectileActor::ProjectileActor(pixelroot32::math::Vector2 position, ProjectileType type)
    : RigidActor(position, PROJECTILE_WIDTH, PROJECTILE_HEIGHT), type(type), active(true), previousPosition(position) {
    
    setRestitution(toScalar(0.0f));
    setFriction(toScalar(0.0f));
    setGravityScale(toScalar(0.0f));
    setShape(pixelroot32::core::CollisionShape::CIRCLE);
    setRadius(toScalar(PROJECTILE_WIDTH * 0.5f));
    setBounce(false);

    Scalar speed = (type == ProjectileType::PLAYER_BULLET) ? toScalar(-PROJECTILE_SPEED) : toScalar(PROJECTILE_SPEED);
    setVelocity(toScalar(0), speed);
}

void ProjectileActor::reset(pixelroot32::math::Vector2 newPosition, ProjectileType newType) {
    position = newPosition;
    previousPosition = newPosition;
    type = newType;
    active = true;
    Scalar speed = (type == ProjectileType::PLAYER_BULLET) ? toScalar(-PROJECTILE_SPEED) : toScalar(PROJECTILE_SPEED);
    setVelocity(toScalar(0), speed);
}

void ProjectileActor::update(unsigned long deltaTime) {
    if (!active) return;

    previousPosition = position;
    RigidActor::update(deltaTime);

    if (position.y < toScalar(-PROJECTILE_HEIGHT) || position.y > toScalar(DISPLAY_HEIGHT)) {
        active = false;
    }
}

void ProjectileActor::draw(pixelroot32::graphics::Renderer& renderer) {
    if (!active) return;
    
    using Color = pixelroot32::graphics::Color;
    Color c = (type == ProjectileType::PLAYER_BULLET) ? Color::White : Color::Orange;
    
    renderer.drawFilledRectangle(static_cast<int>(position.x), static_cast<int>(position.y), 
                                 static_cast<int>(width), static_cast<int>(height), c);
}

void ProjectileActor::onCollision(pixelroot32::core::Actor* other) {
    (void)other;
    active = false;
}

}
