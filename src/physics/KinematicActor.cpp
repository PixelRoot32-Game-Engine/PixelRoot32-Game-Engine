/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "physics/KinematicActor.h"

namespace pixelroot32::physics {

KinematicActor::KinematicActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h)
    : pixelroot32::core::PhysicsActor(x, y, w, h) {
    setBodyType(pixelroot32::core::PhysicsBodyType::KINEMATIC);
}

KinematicActor::KinematicActor(pixelroot32::math::Vector2 position, int w, int h)
    : pixelroot32::core::PhysicsActor(position, w, h) {
    setBodyType(pixelroot32::core::PhysicsBodyType::KINEMATIC);
}

bool KinematicActor::moveAndCollide(pixelroot32::math::Vector2 motion, KinematicCollision* outCollision, bool testOnly, pixelroot32::math::Scalar safeMargin, bool recoveryAsCollision) {
    (void)recoveryAsCollision; // Not fully implemented
    (void)safeMargin;          // Handled via binary search precision

    if (!collisionSystem || motion.is_zero_approx()) {
        if (!testOnly) position += motion;
        return false;
    }

    using namespace pixelroot32::math;
    
    Vector2 startPos = position;
    Vector2 targetPos = startPos + motion;
    
    // Use a static array for collision query to avoid allocation
    static pixelroot32::core::Actor* collisions[16];
    int collisionCount = 0;
    
    // Helper to check collision at specific position with filtering
    auto checkCollisionRefined = [&](Vector2 pos) -> bool {
        position = pos;
        if (!collisionSystem->checkCollision(this, collisions, collisionCount, 16)) return false;
        
        for (int i = 0; i < collisionCount; ++i) {
            auto* other = collisions[i];
            // Ignore rigid bodies for kinematic movement (they get pushed)
            if (other->isPhysicsBody()) {
                 auto* physOther = static_cast<pixelroot32::core::PhysicsActor*>(other);
                 if (physOther->getBodyType() == pixelroot32::core::PhysicsBodyType::RIGID) {
                     continue; 
                 }
            }
            return true; // Found a valid blocker
        }
        return false;
    };

    // First check at target
    if (!checkCollisionRefined(targetPos)) {
        if (testOnly) position = startPos;
        else position = targetPos;
        return false; 
    }

    // Collision detected. Perform binary search to find safe position.
    Vector2 low = startPos;
    Vector2 high = targetPos;
    Vector2 safePos = startPos;
    
    // 8 iterations gives adequate precision
    for (int i = 0; i < 8; ++i) {
        Vector2 mid = (low + high) * toScalar(0.5f);
        if (checkCollisionRefined(mid)) {
            high = mid; // Collision, move back
        } else {
            safePos = mid; // Safe, try moving further
            low = mid;
        }
    }
    
    // Determine normal
    position = high; // Move to colliding position
    checkCollisionRefined(high); // Refresh collisions
    
    pixelroot32::core::Actor* hitActor = nullptr;
    for (int i = 0; i < collisionCount; ++i) {
         auto* other = collisions[i];
         if (other->isPhysicsBody() && static_cast<pixelroot32::core::PhysicsActor*>(other)->getBodyType() == pixelroot32::core::PhysicsBodyType::RIGID) continue;
         hitActor = other;
         break;
    }
    
    Vector2 normal = Vector2(0, 0);
    if (hitActor) {
         pixelroot32::core::Rect otherBox = hitActor->getHitBox();
         pixelroot32::core::Rect myBox = getHitBox(); // at 'high' position
         
         Scalar hw = toScalar((myBox.width + otherBox.width) / 2.0f);
         Scalar hh = toScalar((myBox.height + otherBox.height) / 2.0f);
         Scalar distX = (myBox.position.x + toScalar(myBox.width/2.0f)) - (otherBox.position.x + toScalar(otherBox.width/2.0f));
         Scalar distY = (myBox.position.y + toScalar(myBox.height/2.0f)) - (otherBox.position.y + toScalar(otherBox.height/2.0f));
         Scalar absX = (distX < toScalar(0) ? -distX : distX);
         Scalar absY = (distY < toScalar(0) ? -distY : distY);
         
         Scalar overlapX = hw - absX;
         Scalar overlapY = hh - absY;
         
         if (overlapX < overlapY) {
             normal = (distX < toScalar(0)) ? Vector2(-1, 0) : Vector2(1, 0);
         } else {
             normal = (distY < toScalar(0)) ? Vector2(0, -1) : Vector2(0, 1);
         }
    } else {
        normal = -motion.normalized();
    }
    
    if (outCollision) {
        outCollision->collider = hitActor;
        outCollision->normal = normal;
        outCollision->position = safePos;
        outCollision->travel = (safePos - startPos).length();
        outCollision->remainder = (motion.length() - outCollision->travel);
        if (outCollision->remainder < toScalar(0)) outCollision->remainder = toScalar(0);
    }
    
    position = testOnly ? startPos : safePos;
    return true;
}

void KinematicActor::moveAndSlide(pixelroot32::math::Vector2 velocity, pixelroot32::math::Vector2 upDirection) {
    (void)upDirection;
    using namespace pixelroot32::math;
    
    Vector2 currentMotion = velocity;
    int maxSlides = 4;
    
    for(int i=0; i<maxSlides; ++i) {
        KinematicCollision col;
        if (moveAndCollide(currentMotion, &col)) {
            Vector2 remainderVector = currentMotion.normalized() * col.remainder;
            currentMotion = remainderVector.slide(col.normal);
            if (currentMotion.is_zero_approx()) break;
        } else {
            break;
        }
    }
}

void KinematicActor::draw(pixelroot32::graphics::Renderer& renderer) {
    (void)renderer;
}

} // namespace pixelroot32::physics
