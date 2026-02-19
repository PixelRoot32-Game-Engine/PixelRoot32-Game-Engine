/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Flat Solver v3.0 - RigidActor
 * Velocity integration only - position handled by CollisionSystem
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

    // 1. Accumulate gravity
    force.y += worldGravityY * gravityScale * mass;

    // 2. Linear integration - VELOCITY ONLY
    // Position is integrated by CollisionSystem::integratePositions()
    if (mass > toScalar(0.0f)) {
        Vector2 acceleration = force * (toScalar(1.0f) / mass);
        velocity += acceleration * dt;
    }

    // 3. Reset force for next frame
    force.x = toScalar(0.0f);
    force.y = toScalar(0.0f);

    // 4. Apply friction/damping to velocity
    // Note: This happens BEFORE position integration
    velocity *= (toScalar(1.0f) - friction * dt);
}

void RigidActor::update(unsigned long deltaTime) {
    // Flat Solver v3.0: Fixed timestep for determinism
    // Position integration is handled by CollisionSystem
    (void)deltaTime;
    integrate(pixelroot32::physics::CollisionSystem::FIXED_DT);
}

void RigidActor::draw(pixelroot32::graphics::Renderer& renderer) {
    (void)renderer;
}

} // namespace pixelroot32::physics
