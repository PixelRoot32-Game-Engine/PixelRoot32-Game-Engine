/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "physics/KinematicActor.h"
#include <cassert>

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
    assert(collisionSystem != nullptr && "KinematicActor: collision system is null. Did you add the actor to a scene?");
    (void)recoveryAsCollision; // Not fully implemented

    if (!collisionSystem || motion.is_zero_approx()) {
        if (!testOnly) position += motion;
        return false;
    }

    namespace math = pixelroot32::math;
    using math::Vector2;
    using math::Scalar;
    using math::toScalar;

    // --- Safe margin: Temporarily expand hitbox by safeMargin for collision detection ---
    int originalWidth = width;
    int originalHeight = height;
    bool hasSafeMargin = safeMargin > toScalar(0.0001f);
    if (hasSafeMargin) {
        int margin = static_cast<int>(safeMargin * 2 + toScalar(0.5f));
        if (margin > 0) {
            width += margin;
            height += margin;
        }
    }
    
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
                 if (physOther->isSensor())
                     continue;  // Sensors do not block kinematic movement; overlap will trigger onCollision later.
                 
                 // Validate one-way platforms
                 if (physOther->isOneWay()) {
                     // Calculate collision normal
                     pixelroot32::core::Rect myBox = getHitBox();
                     pixelroot32::core::Rect otherBox = physOther->getHitBox();
                     
                     Scalar hw = toScalar((myBox.width + otherBox.width) / 2.0f);
                     Scalar hh = toScalar((myBox.height + otherBox.height) / 2.0f);
                     Scalar distX = (myBox.position.x + toScalar(myBox.width/2.0f)) - 
                                   (otherBox.position.x + toScalar(otherBox.width/2.0f));
                     Scalar distY = (myBox.position.y + toScalar(myBox.height/2.0f)) - 
                                   (otherBox.position.y + toScalar(otherBox.height/2.0f));
                     Scalar absX = (distX < toScalar(0) ? -distX : distX);
                     Scalar absY = (distY < toScalar(0) ? -distY : distY);
                     
                     Scalar overlapX = hw - absX;
                     Scalar overlapY = hh - absY;
                     
                     Vector2 normal;
                     if (overlapX < overlapY) {
                         normal = (distX < toScalar(0)) ? Vector2(-1, 0) : Vector2(1, 0);
                     } else {
                         normal = (distY < toScalar(0)) ? Vector2(0, -1) : Vector2(0, 1);
                     }
                     
                     if (!collisionSystem->validateOneWayPlatform(this, physOther, normal)) {
                         continue;  // Ignore this one-way platform
                     }
                 }
            }
            return true; // Found a valid blocker
        }
        return false;
    };

    // First check at target
    if (!checkCollisionRefined(targetPos)) {
        if (hasSafeMargin) {
            width = originalWidth;
            height = originalHeight;
        }
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
         if (other->isPhysicsBody()) {
             auto* physOther = static_cast<pixelroot32::core::PhysicsActor*>(other);
             if (physOther->getBodyType() == pixelroot32::core::PhysicsBodyType::RIGID) continue;
             if (physOther->isSensor()) continue;
         }
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
    
    // Restore original dimensions after safe margin expansion
    if (hasSafeMargin) {
        width = originalWidth;
        height = originalHeight;
    }

    position = testOnly ? startPos : safePos;
    return true;
}

void KinematicActor::resolvePreSlideDepenetration() {
    namespace math = pixelroot32::math;
    using math::Vector2;
    using math::Scalar;
    using math::toScalar;

    static pixelroot32::core::Actor* startCollisions[16];
    int startCollisionCount = 0;
    if (!collisionSystem || !collisionSystem->checkCollision(this, startCollisions, startCollisionCount, 16)) {
        return;
    }

    for (int i = 0; i < startCollisionCount; ++i) {
        auto* other = startCollisions[i];
        if (!other->isPhysicsBody()) continue;

        auto* physOther = static_cast<pixelroot32::core::PhysicsActor*>(other);
        if (physOther->getBodyType() == pixelroot32::core::PhysicsBodyType::RIGID || physOther->isSensor()) {
            continue;
        }

        if (physOther->isOneWay()) {
            pixelroot32::core::Rect otherBox = physOther->getHitBox();
            Scalar previousBottom = getPreviousPosition().y + getHitboxOffset().y + toScalar(height);
            Scalar platformTop = otherBox.position.y;
            if (previousBottom > platformTop + CollisionSystem::SLOP) {
                continue;
            }
        }

        pixelroot32::core::Rect myBox = getHitBox();
        pixelroot32::core::Rect otherBox = physOther->getHitBox();
        if (!myBox.intersects(otherBox)) continue;

        Scalar overlapX = math::min(myBox.position.x + toScalar(myBox.width),
                                    otherBox.position.x + toScalar(otherBox.width)) -
                          math::max(myBox.position.x, otherBox.position.x);
        Scalar overlapY = math::min(myBox.position.y + toScalar(myBox.height),
                                    otherBox.position.y + toScalar(otherBox.height)) -
                          math::max(myBox.position.y, otherBox.position.y);

        Scalar depenClamp = toScalar(4.0f);

        if (overlapX < overlapY && overlapX > CollisionSystem::SLOP) {
            Scalar correction = math::min(overlapX, depenClamp);
            if (position.x + getHitboxOffset().x + toScalar(myBox.width)/2.0f < otherBox.position.x + toScalar(otherBox.width)/2.0f) {
                position.x -= correction;
            } else {
                position.x += correction;
            }
        } else if (overlapY > CollisionSystem::SLOP) {
            Scalar correction = math::min(overlapY, depenClamp);
            if (position.y + getHitboxOffset().y + toScalar(myBox.height)/2.0f < otherBox.position.y + toScalar(otherBox.height)/2.0f) {
                position.y -= correction;
            } else {
                position.y += correction;
            }
        }
    }
}

void KinematicActor::resolvePostSlideKinematicDepenetration(pixelroot32::core::PhysicsActor* targetBody) {
    namespace math = pixelroot32::math;
    using math::Scalar;
    using math::toScalar;

    if (!targetBody || targetBody->getBodyType() != pixelroot32::core::PhysicsBodyType::KINEMATIC) {
        return;
    }

    pixelroot32::core::Rect myBox = getHitBox();
    pixelroot32::core::Rect floorBox = targetBody->getHitBox();
    if (!myBox.intersects(floorBox)) return;

    Scalar overlapX = math::min(myBox.position.x + toScalar(myBox.width),
                                floorBox.position.x + toScalar(floorBox.width)) -
                      math::max(myBox.position.x, floorBox.position.x);
    Scalar overlapY = math::min(myBox.position.y + toScalar(myBox.height),
                                floorBox.position.y + toScalar(floorBox.height)) -
                      math::max(myBox.position.y, floorBox.position.y);
    Scalar depenClamp = toScalar(2.0f);

    if (overlapX < overlapY && overlapX > CollisionSystem::SLOP) {
        Scalar correction = math::min(overlapX, depenClamp);
        if (position.x < targetBody->position.x) position.x -= correction;
        else position.x += correction;
    } else if (overlapY > CollisionSystem::SLOP) {
        Scalar correction = math::min(overlapY, depenClamp);
        if (position.y < targetBody->position.y) position.y -= correction;
        else position.y += correction;
    }
}

bool KinematicActor::hasTopSurfaceSupport(pixelroot32::core::PhysicsActor* floor) {
    if (!floor) return false;
    if (!strictTopSurfaceFloor) return true;

    namespace math = pixelroot32::math;
    using math::Scalar;
    using math::toScalar;

    pixelroot32::core::Rect myBox = getHitBox();
    pixelroot32::core::Rect floorBox = floor->getHitBox();

    Scalar playerCenterX = myBox.position.x + toScalar(myBox.width) / 2;
    Scalar floorLeft = floorBox.position.x;
    Scalar floorRight = floorBox.position.x + toScalar(floorBox.width);
    if (playerCenterX < floorLeft || playerCenterX > floorRight) return false;

    Scalar myBottom = myBox.position.y + toScalar(myBox.height);
    Scalar floorTop = floorBox.position.y;
    Scalar myTop = myBox.position.y;
    if (myTop >= floorTop + CollisionSystem::SLOP) return false;

    Scalar bottomDelta = myBottom - floorTop;
    Scalar alignTolerance = MIN_SNAP + CollisionSystem::SLOP;
    if (bottomDelta < -alignTolerance || bottomDelta > alignTolerance) return false;
    return true;
}

void KinematicActor::applySnapStep(pixelroot32::math::Vector2 snapVector, pixelroot32::math::Vector2 upDirection,
                                   pixelroot32::core::PhysicsActor*& localFloorBody) {
    namespace math = pixelroot32::math;
    using math::Vector2;
    using math::Scalar;
    using math::toScalar;

    if (!wasSnapFloor) return;

    Scalar snapMag = snapVector.length();
    if (snapMag < MIN_SNAP) return;

    if (snapVector.dot(upDirection) >= toScalar(0)) return;

    Vector2 preSnapPos = position;
    Vector2 snapMotion = -upDirection * snapMag;
    KinematicCollision snapCol;
    bool hit = moveAndCollide(snapMotion, &snapCol);
    Scalar floorThreshold = toScalar(0.70710678f);

    if (hit) {
        Scalar dot = snapCol.normal.dot(upDirection);
        if (dot > floorThreshold) {
            pixelroot32::core::PhysicsActor* floorPhys = nullptr;
            if (snapCol.collider && snapCol.collider->isPhysicsBody()) {
                floorPhys = static_cast<pixelroot32::core::PhysicsActor*>(snapCol.collider);
            }
            bool allowFloor = true;
            if (floorPhys &&
                floorPhys->getBodyType() == pixelroot32::core::PhysicsBodyType::KINEMATIC &&
                strictTopSurfaceFloor) {
                allowFloor = hasTopSurfaceSupport(floorPhys);
            }
            if (allowFloor) {
                onFloor = true;
                if (floorPhys &&
                    floorPhys->getBodyType() == pixelroot32::core::PhysicsBodyType::KINEMATIC) {
                    localFloorBody = floorPhys;
                    lastFloorNormal = snapCol.normal;
                }
            }
        } else {
            position = preSnapPos;
        }
    } else {
        position = preSnapPos;
    }
}

pixelroot32::math::Vector2 KinematicActor::moveAndSlide(
    pixelroot32::math::Vector2 velocity,
    pixelroot32::math::Scalar dt,
    pixelroot32::math::Vector2 upDirection,
    SnapPolicy snapPolicy,
    pixelroot32::math::Vector2 snapVector,
    pixelroot32::core::PhysicsActor** outFloorBody) {

    namespace math = pixelroot32::math;
    using math::Vector2;
    using math::Scalar;

    onFloor = false;
    onCeiling = false;
    onWall = false;

    resolvePreSlideDepenetration();

    pixelroot32::core::PhysicsActor* prevKinematicFloor = nullptr;
    if (floorBody != nullptr &&
        floorBody->getBodyType() == pixelroot32::core::PhysicsBodyType::KINEMATIC) {
        prevKinematicFloor = floorBody;
    }

    Vector2 startPos = position;
    Vector2 currentMotion = velocity * dt;

    if (wasSnapFloor && floorBody != nullptr &&
        floorBody->getBodyType() == pixelroot32::core::PhysicsBodyType::KINEMATIC &&
        hasTopSurfaceSupport(floorBody)) {
        currentMotion += floorBody->getVelocity() * dt;
    }

    pixelroot32::core::PhysicsActor* localFloorBody = nullptr;
    slide(currentMotion, upDirection, localFloorBody);

    if (snapPolicy == SnapPolicy::Step) {
        applySnapStep(snapVector, upDirection, localFloorBody);
    } else if (snapPolicy == SnapPolicy::Continuous) {
#if PIXELROOT32_DEBUG_MODE
        static bool warned = false;
        if (!warned) {
            warned = true;
            assert(false && "SnapPolicy::Continuous not implemented");
        }
#endif
    }

    if (onFloor && localFloorBody != nullptr &&
        localFloorBody->getBodyType() == pixelroot32::core::PhysicsBodyType::KINEMATIC) {
        floorBody = localFloorBody;
        floorVelocity = localFloorBody->getVelocity();
    } else if (!onFloor) {
        floorBody = nullptr;
        floorVelocity = Vector2::ZERO();
    } else {
        floorBody = nullptr;
        floorVelocity = Vector2::ZERO();
    }

    pixelroot32::core::PhysicsActor* depenetrateTarget = floorBody;
    if (depenetrateTarget == nullptr && prevKinematicFloor != nullptr) {
        pixelroot32::core::Rect myBox = getHitBox();
        if (myBox.intersects(prevKinematicFloor->getHitBox())) {
            depenetrateTarget = prevKinematicFloor;
        }
    }
    resolvePostSlideKinematicDepenetration(depenetrateTarget);

    if (outFloorBody) {
        *outFloorBody = floorBody;
    }

    wasSnapFloor = onFloor;

    if (dt == Scalar(0)) {
        return Vector2::ZERO();
    }
    Vector2 displacement = position - startPos;
    return displacement / dt;
}

void KinematicActor::slide(pixelroot32::math::Vector2& currentMotion, pixelroot32::math::Vector2 upDirection, pixelroot32::core::PhysicsActor*& localFloorBody) {
    namespace math = pixelroot32::math;
    using math::Scalar;
    using math::Vector2;
    using math::toScalar;

    Scalar floorThreshold = toScalar(0.70710678f);

    for (int i = 0; i < maxSlides; ++i) {
        KinematicCollision col;
        if (moveAndCollide(currentMotion, &col)) {
            Scalar dot = col.normal.dot(upDirection);

            if (dot > floorThreshold) {
                pixelroot32::core::PhysicsActor* floorPhys = nullptr;
                if (col.collider && col.collider->isPhysicsBody()) {
                    floorPhys = static_cast<pixelroot32::core::PhysicsActor*>(col.collider);
                }
                bool allowFloor = true;
                if (floorPhys &&
                    floorPhys->getBodyType() == pixelroot32::core::PhysicsBodyType::KINEMATIC &&
                    strictTopSurfaceFloor) {
                    allowFloor = hasTopSurfaceSupport(floorPhys);
                }
                if (allowFloor) {
                    onFloor = true;
                    if (floorPhys &&
                        floorPhys->getBodyType() == pixelroot32::core::PhysicsBodyType::KINEMATIC) {
                        localFloorBody = floorPhys;
                        lastFloorNormal = col.normal;
                    }
                }
            } else if (dot < -floorThreshold) {
                onCeiling = true;
            } else {
                onWall = true;
            }

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
