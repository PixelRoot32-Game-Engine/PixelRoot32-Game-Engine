/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Flat Solver - RigidActor
 */
#include "physics/RigidActor.h"
#include "physics/CollisionSystem.h"
#include "math/MathUtil.h"

namespace pixelroot32::physics {

RigidActor::RigidActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h)
    : pixelroot32::core::PhysicsActor(x, y, w, h) {
    setBodyType(pixelroot32::core::PhysicsBodyType::RIGID);
}

RigidActor::RigidActor(pixelroot32::math::Vector2 position, int w, int h)
    : pixelroot32::core::PhysicsActor(position, w, h) {
    setBodyType(pixelroot32::core::PhysicsBodyType::RIGID);
}

void RigidActor::applyForce(const pixelroot32::math::Vector2& f) {
    force += f;
}

void RigidActor::applyImpulse(const pixelroot32::math::Vector2& j) {
    if (mass > pixelroot32::math::toScalar(0.0f)) {
        velocity += j * (pixelroot32::math::toScalar(1.0f) / mass);
    }
}

void RigidActor::integrate(pixelroot32::math::Scalar dt) {
    using namespace pixelroot32::math;

    Scalar worldGravityY = toScalar(200.0f); 
    force.y += worldGravityY * gravityScale * mass;

    if (mass > toScalar(0.0f)) {
        Vector2 acceleration = force * (toScalar(1.0f) / mass);
        velocity += acceleration * dt;
    }

    force.x = toScalar(0.0f);
    force.y = toScalar(0.0f);

    velocity *= (toScalar(1.0f) - friction * dt);
}

void RigidActor::update(unsigned long deltaTime) {
    (void)deltaTime;
    integrate(pixelroot32::physics::CollisionSystem::FIXED_DT);
}

void RigidActor::draw(pixelroot32::graphics::Renderer& renderer) {
    (void)renderer;
}

}
