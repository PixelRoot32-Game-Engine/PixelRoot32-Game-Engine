/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Flat Solver v3.0 - PhysicsActor Base Class
 */
#include "core/PhysicsActor.h"
#include "core/Engine.h"
#include "platforms/EngineConfig.h"
#include "math/MathUtil.h"

namespace pixelroot32::core {

extern unsigned long gProfilerPhysicsIntegrateTime;
extern unsigned long gProfilerPhysicsIntegrateCount;

PhysicsActor::PhysicsActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h)
    : Actor(x, y, w, h) {
    worldWidth = pixelroot32::platforms::config::LogicalWidth;
    worldHeight = pixelroot32::platforms::config::LogicalHeight;
}

PhysicsActor::PhysicsActor(pixelroot32::math::Vector2 position, int w, int h)
    : Actor(position, w, h) {
    worldWidth = pixelroot32::platforms::config::LogicalWidth;
    worldHeight = pixelroot32::platforms::config::LogicalHeight;
}

void PhysicsActor::update(unsigned long deltaTime) {
    if (bodyType == PhysicsBodyType::STATIC) {
        return;
    }
}

void PhysicsActor::integrate(pixelroot32::math::Scalar dt) {
    (void)dt;
}

void PhysicsActor::resolveWorldBounds() {
    using pixelroot32::math::toScalar;
    using pixelroot32::math::Scalar;

    int left = (limits.left != -1 ? limits.left : 0);
    int top = (limits.top != -1 ? limits.top : 0);
    int right = (limits.right != -1 ? limits.right : worldWidth);
    int bottom = (limits.bottom != -1 ? limits.bottom : worldHeight);

    if (right == 0) right = pixelroot32::platforms::config::LogicalWidth;
    if (bottom == 0) bottom = pixelroot32::platforms::config::LogicalHeight;

    resetWorldCollisionInfo();

    Scalar sLeft = toScalar(left);
    Scalar sTop = toScalar(top);
    Scalar sRight = toScalar(right);
    Scalar sBottom = toScalar(bottom);
    Scalar sWidth = toScalar(width);
    Scalar sHeight = toScalar(height);
    
    Scalar effectiveRestitution = bounce ? restitution : toScalar(0.0f);

    if (position.x < sLeft) { 
        position.x = sLeft; 
        velocity.x = -velocity.x * effectiveRestitution; 
        worldCollisionInfo.left = true;
        onWorldCollision(); 
    }
    if (position.x + sWidth > sRight) { 
        position.x = sRight - sWidth; 
        velocity.x = -velocity.x * effectiveRestitution; 
        worldCollisionInfo.right = true;
        onWorldCollision(); 
    }   
    if (position.y < sTop) { 
        position.y = sTop; 
        velocity.y = -velocity.y * effectiveRestitution; 
        worldCollisionInfo.top = true;
        onWorldCollision(); 
    }
    if (position.y + sHeight > sBottom) { 
        position.y = sBottom - sHeight; 
        velocity.y = -velocity.y * effectiveRestitution;
        worldCollisionInfo.bottom = true;
        onWorldCollision(); 
    }
}

void PhysicsActor::onCollision(Actor* other) {
    (void)other;
}

void PhysicsActor::resetWorldCollisionInfo() {
    worldCollisionInfo.left = false;
    worldCollisionInfo.right = false;
    worldCollisionInfo.top = false;
    worldCollisionInfo.bottom = false;
}

void PhysicsActor::onWorldCollision() {
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

pixelroot32::core::Rect PhysicsActor::getHitBox() {
    return {position, width, height};
}
}
