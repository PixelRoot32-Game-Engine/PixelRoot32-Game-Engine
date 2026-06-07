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

void KinematicActor::moveAndSlide(pixelroot32::math::Vector2 velocity, pixelroot32::math::Vector2 upDirection, pixelroot32::core::PhysicsActor** outFloorBody, pixelroot32::math::Scalar dt) {
    namespace math = pixelroot32::math;
    using math::Vector2;
    using math::Scalar;
    using math::toScalar;

    // Reset collision flags
    onFloor = false;
    onCeiling = false;
    onWall = false;

    // --- Pre-slide depenetration: resolve overlaps against solid static/kinematic bodies first ---
    static pixelroot32::core::Actor* startCollisions[16];
    int startCollisionCount = 0;
    if (collisionSystem && collisionSystem->checkCollision(this, startCollisions, startCollisionCount, 16)) {
        for (int i = 0; i < startCollisionCount; ++i) {
            auto* other = startCollisions[i];
            if (other->isPhysicsBody()) {
                auto* physOther = static_cast<pixelroot32::core::PhysicsActor*>(other);
                if (physOther->getBodyType() == pixelroot32::core::PhysicsBodyType::RIGID || physOther->isSensor()) {
                    continue;
                }
                
                // One-way platform filter: only depenetrate if we are coming from above
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
                if (myBox.intersects(otherBox)) {
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
        }
    }

    Vector2 currentMotion = velocity;

    // --- Pre-slide: Inherit floor velocity from persistent state ---
    // Apply only if we had a floor body recently (within tolerance window)
    // Full velocity is inherited; the slide loop resolves any collision issues
    if (floorBody && floorLostCounter < MAX_FLOOR_LOST_FRAMES) {
        currentMotion += floorVelocity * dt;
    }

    // Local floor body tracker for current frame (member floorBody is for persistence)
    pixelroot32::core::PhysicsActor* localFloorBody = nullptr;

    // Slide along surfaces
    slide(currentMotion, upDirection, localFloorBody);

    // --- Update floor persistence state ---
    updateFloorState(onFloor, localFloorBody);

    // --- Post-slide depenetration: push out from overlapping KINEMATIC bodies ---
    if (floorBody && floorBody->getBodyType() == pixelroot32::core::PhysicsBodyType::KINEMATIC) {
        pixelroot32::core::Rect myBox = getHitBox();
        pixelroot32::core::Rect floorBox = floorBody->getHitBox();
        if (myBox.intersects(floorBox)) {
            Scalar overlapX = math::min(myBox.position.x + toScalar(myBox.width),
                                        floorBox.position.x + toScalar(floorBox.width)) -
                              math::max(myBox.position.x, floorBox.position.x);
            Scalar overlapY = math::min(myBox.position.y + toScalar(myBox.height),
                                        floorBox.position.y + toScalar(floorBox.height)) -
                              math::max(myBox.position.y, floorBox.position.y);
            Scalar depenClamp = toScalar(2.0f);
            // Depenetrate along the axis with smaller overlap
            if (overlapX < overlapY && overlapX > CollisionSystem::SLOP) {
                Scalar correction = math::min(overlapX, depenClamp);
                if (position.x < floorBody->position.x) position.x -= correction;
                else position.x += correction;
            } else if (overlapY > CollisionSystem::SLOP) {
                Scalar correction = math::min(overlapY, depenClamp);
                if (position.y < floorBody->position.y) position.y -= correction;
                else position.y += correction;
            }
        }
    }

    // Write the floor body pointer if caller requested it
    if (outFloorBody) {
        *outFloorBody = floorBody;
    }
}

pixelroot32::math::Vector2 KinematicActor::moveAndSlideWithSnap(pixelroot32::math::Vector2 velocity, pixelroot32::math::Vector2 snap, pixelroot32::math::Vector2 upDirection, pixelroot32::math::Scalar dt) {
    namespace math = pixelroot32::math;
    using math::Vector2;
    using math::Scalar;
    using math::toScalar;

    // Reset collision flags
    onFloor = false;
    onCeiling = false;
    onWall = false;

    // --- Pre-slide depenetration: resolve overlaps against solid static/kinematic bodies first ---
    static pixelroot32::core::Actor* startCollisions[16];
    int startCollisionCount = 0;
    if (collisionSystem && collisionSystem->checkCollision(this, startCollisions, startCollisionCount, 16)) {
        for (int i = 0; i < startCollisionCount; ++i) {
            auto* other = startCollisions[i];
            if (other->isPhysicsBody()) {
                auto* physOther = static_cast<pixelroot32::core::PhysicsActor*>(other);
                if (physOther->getBodyType() == pixelroot32::core::PhysicsBodyType::RIGID || physOther->isSensor()) {
                    continue;
                }

                // One-way platform filter: only depenetrate if we are coming from above
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
                if (myBox.intersects(otherBox)) {
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
        }
    }

    Vector2 startPos = position;
    Vector2 currentMotion = velocity;
    Scalar floorThreshold = toScalar(0.70710678f);

    pixelroot32::core::PhysicsActor* localFloorBody = nullptr;

    // Slide along surfaces
    slide(currentMotion, upDirection, localFloorBody);

    // --- Snap post-step: move body toward floor via moveAndCollide ---
    Scalar snapMag = snap.length();
    if (snapMag >= MIN_SNAP) {
        Scalar snapDot = snap.dot(upDirection);
        if (snapDot > toScalar(0)) {
            Vector2 preSnapPos = position;
            Vector2 snapMotion = -upDirection * snapMag;
            KinematicCollision snapCol;
            bool hit = moveAndCollide(snapMotion, &snapCol);
            if (hit) {
                Scalar dot = snapCol.normal.dot(upDirection);
                if (dot > floorThreshold) {
                    onFloor = true;
                    if (snapCol.collider && snapCol.collider->isPhysicsBody()) {
                        auto* phys = static_cast<pixelroot32::core::PhysicsActor*>(snapCol.collider);
                        if (phys->getBodyType() == pixelroot32::core::PhysicsBodyType::KINEMATIC) {
                            localFloorBody = phys;
                            lastFloorNormal = snapCol.normal;
                        }
                    }
                } else {
                    // Hit something but not a floor — restore pre-snap position
                    position = preSnapPos;
                }
            } else {
                // No hit — snap misses, restore pre-snap position
                position = preSnapPos;
            }
        }
    }

    // --- Update floor persistence state ---
    updateFloorState(onFloor, localFloorBody);

    // --- Post-slide depenetration: push out from overlapping KINEMATIC bodies ---
    if (floorBody && floorBody->getBodyType() == pixelroot32::core::PhysicsBodyType::KINEMATIC) {
        pixelroot32::core::Rect myBox = getHitBox();
        pixelroot32::core::Rect floorBox = floorBody->getHitBox();
        if (myBox.intersects(floorBox)) {
            Scalar overlapX = math::min(myBox.position.x + toScalar(myBox.width),
                                        floorBox.position.x + toScalar(floorBox.width)) -
                              math::max(myBox.position.x, floorBox.position.x);
            Scalar overlapY = math::min(myBox.position.y + toScalar(myBox.height),
                                        floorBox.position.y + toScalar(floorBox.height)) -
                              math::max(myBox.position.y, floorBox.position.y);
            Scalar depenClamp = toScalar(2.0f);
            // Depenetrate along the axis with smaller overlap
            if (overlapX < overlapY && overlapX > CollisionSystem::SLOP) {
                Scalar correction = math::min(overlapX, depenClamp);
                if (position.x < floorBody->position.x) position.x -= correction;
                else position.x += correction;
            } else if (overlapY > CollisionSystem::SLOP) {
                Scalar correction = math::min(overlapY, depenClamp);
                if (position.y < floorBody->position.y) position.y -= correction;
                else position.y += correction;
            }
        }
    }

    // Return actual velocity from displacement
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
                onFloor = true;
                // Track the floor body if it is KINEMATIC (for velocity inheritance)
                if (col.collider && col.collider->isPhysicsBody()) {
                    auto* phys = static_cast<pixelroot32::core::PhysicsActor*>(col.collider);
                    if (phys->getBodyType() == pixelroot32::core::PhysicsBodyType::KINEMATIC) {
                        localFloorBody = phys;
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

void KinematicActor::updateFloorState(bool onFloorThisFrame, pixelroot32::core::PhysicsActor* floorBodyResult) {
    if (onFloorThisFrame) {
        floorLostCounter = 0;
        if (floorBodyResult) {
            floorBody = floorBodyResult;
            floorVelocity = floorBodyResult->getVelocity();
        }
        wasOnFloor = true;
    } else {
        floorLostCounter++;
        if (floorLostCounter >= MAX_FLOOR_LOST_FRAMES) {
            wasOnFloor = false;
            floorBody = nullptr;
            floorVelocity = pixelroot32::math::Vector2::ZERO();
        }
    }
}

void KinematicActor::draw(pixelroot32::graphics::Renderer& renderer) {
    (void)renderer;
}

} // namespace pixelroot32::physics
