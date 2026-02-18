/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "core/PhysicsActor.h"
#include "core/Engine.h"
#include "platforms/EngineConfig.h"
#include "math/MathUtil.h"

namespace pixelroot32::core {

extern unsigned long gProfilerPhysicsIntegrateTime;
extern unsigned long gProfilerPhysicsIntegrateCount;

PhysicsActor::PhysicsActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, pixelroot32::math::Scalar w, pixelroot32::math::Scalar h)
    : Actor(x, y, static_cast<int>(w), static_cast<int>(h)) {
    worldWidth = pixelroot32::platforms::config::LogicalWidth;
    worldHeight = pixelroot32::platforms::config::LogicalHeight;
}

PhysicsActor::PhysicsActor(pixelroot32::math::Vector2 position, int w, int h)
    : Actor(position, w, h) {
    worldWidth = pixelroot32::platforms::config::LogicalWidth;
    worldHeight = pixelroot32::platforms::config::LogicalHeight;
}

void PhysicsActor::update(unsigned long deltaTime) {
    // Convert ms to seconds
    pixelroot32::math::Scalar dt = pixelroot32::math::toScalar(static_cast<float>(deltaTime) * 0.001f);

    integrate(dt);
    resolveWorldBounds();
}

void PhysicsActor::integrate(pixelroot32::math::Scalar dt) {
    unsigned long t0 = 0;
    if constexpr (pixelroot32::platforms::config::EnableProfiling) {
        t0 = pixelroot32::platforms::config::profilerMicros();
    }
    
    // Update position using velocity
    // Entity now uses Vector2 for position, so we can add directly
    position += velocity * dt;

    // Apply friction
    pixelroot32::math::Scalar one = pixelroot32::math::toScalar(1.0f);
    velocity *= (one - friction);

    if constexpr (pixelroot32::platforms::config::EnableProfiling) {
        gProfilerPhysicsIntegrateTime += pixelroot32::platforms::config::profilerMicros() - t0;
        gProfilerPhysicsIntegrateCount++;
    }
}

void PhysicsActor::resolveWorldBounds() {
    using pixelroot32::math::toScalar;
    using pixelroot32::math::Scalar;

    // calculate the final limits
    int left = (limits.left != -1 ? limits.left : 0);
    int top = (limits.top != -1 ? limits.top : 0);
    int right = (limits.right != -1 ? limits.right : worldWidth);
    int bottom = (limits.bottom != -1 ? limits.bottom : worldHeight);

    // If worldSize is still 0 (e.g. if initialized before LOGICAL_WIDTH was available or 
    // if manual worldWidth/Height weren't set), fallback to pixelroot32::platforms::config::LogicalWidth/Height
    if (right == 0) right = pixelroot32::platforms::config::LogicalWidth;
    if (bottom == 0) bottom = pixelroot32::platforms::config::LogicalHeight;

    resetWorldCollisionInfo();

    Scalar sLeft = toScalar(left);
    Scalar sTop = toScalar(top);
    Scalar sRight = toScalar(right);
    Scalar sBottom = toScalar(bottom);
    Scalar sWidth = toScalar(width);
    Scalar sHeight = toScalar(height);

    if (position.x < sLeft) { 
        position.x = sLeft; 
        velocity.x = -velocity.x * restitution; 
        
        worldCollisionInfo.left = true;
        onWorldCollision(); 
    }
    if (position.x + sWidth > sRight) { 
        position.x = sRight - sWidth; 
        velocity.x = -velocity.x * restitution; 

        worldCollisionInfo.right = true;
        onWorldCollision(); 
    }   
    if (position.y < sTop) { 
        position.y = sTop; 
        velocity.y = -velocity.y * restitution; 
        
        worldCollisionInfo.top = true;
        onWorldCollision(); 
    }
    if (position.y + sHeight > sBottom) { 
        position.y = sBottom - sHeight; 
        velocity.y = -velocity.y * restitution;

        worldCollisionInfo.bottom = true;
        onWorldCollision(); 
    }
}

void PhysicsActor::onCollision(Actor* other) {
    (void)other;
    
    // Default: simple bounce
    velocity.x = -velocity.x * restitution;
    // Note: Usually we would bounce based on collision normal, but for simple AABB overlap default,
    // just reversing X is a placeholder behavior. Real physics would use collision normals.
}

void PhysicsActor::resetWorldCollisionInfo() {
    worldCollisionInfo.left = false;
    worldCollisionInfo.right = false;
    worldCollisionInfo.top = false;
    worldCollisionInfo.bottom = false;
}

void PhysicsActor::onWorldCollision() {
    // Default implementation does nothing
}

void PhysicsActor::setLimits(int left, int top, int right, int bottom) {
    limits = LimitRect(left, top, right, bottom);
}

void PhysicsActor::setWorldBounds(int w, int h) {
    worldWidth = w;
    worldHeight = h;
}

WorldCollisionInfo PhysicsActor::getWorldCollisionInfo() const {
    return worldCollisionInfo;
}

} // namespace pixelroot32::core
