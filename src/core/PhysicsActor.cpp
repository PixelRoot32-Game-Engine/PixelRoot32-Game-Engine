/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "core/PhysicsActor.h"
#include "core/Engine.h"

namespace pixelroot32::core {

PhysicsActor::PhysicsActor(float x, float y, float w, float h)
    : Actor(x, y, w, h) {}

void PhysicsActor::update(unsigned long deltaTime) {
    float dt = deltaTime * 0.001f;

    integrate(dt);
    resolveWorldBounds();
}

void PhysicsActor::integrate(float dt) {
    x += vx * dt;
    y += vy * dt;

    // Simple friction
    vx *= (1.0f - friction);
    vy *= (1.0f - friction);
}

void PhysicsActor::resolveWorldBounds() {
    // calculate the final limits
    int left = (limits.left != -1 ? limits.left : 0);
    int top = (limits.top != -1 ? limits.top : 0);
    int right = (limits.right != -1 ? limits.right : worldWidth);
    int bottom = (limits.bottom != -1 ? limits.bottom : worldHeight);

    resetWorldCollisionInfo();

    if (x < left) { 
        x = left; 
        vx = -vx * restitution; 
        
        worldCollisionInfo.left = true;
        onWorldCollision(); 
    }
    if (x + width > right) { 
        x = right - width; 
        vx = -vx * restitution; 

        worldCollisionInfo.right = true;
        onWorldCollision(); 
    }   
    if (y < top) { 
        y = top; vy = -vy * restitution; 

        worldCollisionInfo.top = true;
        onWorldCollision(); 
    }
    if (y + height > bottom) { 
        y = bottom - height; 
        vy = -vy * restitution;

        worldCollisionInfo.bottom = true;
        onWorldCollision(); 
    }
}

void PhysicsActor::onCollision(Actor* other) {
    (void)other;
    
    // Default: simple bounce
    vx = -vx * restitution;
}

void PhysicsActor::onWorldCollision() {
    // Hook opcional
}

void PhysicsActor::setVelocity(float x, float y) {
    vx = x;
    vy = y;
}

void PhysicsActor::setRestitution(float r) {
    restitution = r;
}

void PhysicsActor::setFriction(float f) {
    friction = f;
}

void PhysicsActor::setLimits(LimitRect limits) {
    this->limits = limits;
}   

void PhysicsActor::setWorldSize(int width, int height) {
    worldWidth = width;
    worldHeight = height;
}

}