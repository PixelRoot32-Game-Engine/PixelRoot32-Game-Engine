/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "physics/KinematicActor.h"

namespace pixelroot32::physics {

KinematicActor::KinematicActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, pixelroot32::math::Scalar w, pixelroot32::math::Scalar h)
    : pixelroot32::core::PhysicsActor(x, y, w, h) {
    setBodyType(pixelroot32::core::PhysicsBodyType::KINEMATIC);
}

bool KinematicActor::moveAndCollide(pixelroot32::math::Vector2 motion, KinematicCollision* outCollision) {
    if (!collisionSystem) {
        position += motion;
        return false;
    }

    int steps = 4;
    pixelroot32::math::Vector2 step = motion * pixelroot32::math::toScalar(1.0f / (float)steps);
    bool collided = false;

    for (int i = 0; i < steps; ++i) {
        pixelroot32::math::Vector2 prevPos = position;
        pixelroot32::core::Rect prevBox = getHitBox();
        position += step;
        
        // Zero-allocation collision check using static array
        static pixelroot32::core::Actor* collisions[16];
        int collisionCount = 0;
        
        if (collisionSystem->checkCollision(this, collisions, collisionCount, 16)) {
            bool shouldBackUp = false;

            for (int ci = 0; ci < collisionCount; ++ci) {
                auto* other = collisions[ci];
                // Determine if this is a collision we must respect
                bool rigid = false;
                if (other->isPhysicsBody()) {
                    if (static_cast<pixelroot32::core::PhysicsActor*>(other)->getBodyType() == pixelroot32::core::PhysicsBodyType::RIGID) {
                        rigid = true;
                    }
                }

                if (rigid) continue; // Allow overlap with rigid bodies so solver can push them

                // It's a static/kinematic collider. Check if moving closer.
                pixelroot32::core::Rect otherBox = other->getHitBox();
                
                if (!prevBox.intersects(otherBox)) {
                    shouldBackUp = true;
                    break;
                } else {
                    using namespace pixelroot32::math;
                    Scalar hw = toScalar((width + otherBox.width) / 2.0f);
                    Scalar hh = toScalar((height + otherBox.height) / 2.0f);
                    
                    Scalar prevDistX = (prevPos.x + toScalar(width/2.0f)) - (otherBox.position.x + toScalar(otherBox.width/2.0f));
                    Scalar prevDistY = (prevPos.y + toScalar(height/2.0f)) - (otherBox.position.y + toScalar(otherBox.height/2.0f));
                    Scalar prevAbsX = (prevDistX < toScalar(0) ? -prevDistX : prevDistX);
                    Scalar prevAbsY = (prevDistY < toScalar(0) ? -prevDistY : prevDistY);
                    
                    Scalar currDistX = (position.x + toScalar(width/2.0f)) - (otherBox.position.x + toScalar(otherBox.width/2.0f));
                    Scalar currDistY = (position.y + toScalar(height/2.0f)) - (otherBox.position.y + toScalar(otherBox.height/2.0f));
                    Scalar currAbsX = (currDistX < toScalar(0) ? -currDistX : currDistX);
                    Scalar currAbsY = (currDistY < toScalar(0) ? -currDistY : currDistY);

                    Scalar prevOverlapX = hw - prevAbsX;
                    Scalar prevOverlapY = hh - prevAbsY;
                    Scalar currOverlapX = hw - currAbsX;
                    Scalar currOverlapY = hh - currAbsY;

                    if (currOverlapX > prevOverlapX || currOverlapY > prevOverlapY) {
                         shouldBackUp = true;
                         break;
                    }
                }
            }

            if (shouldBackUp) {
                position = prevPos;
                collided = true;
                if (outCollision && collisionCount > 0) {
                    outCollision->collider = collisions[0];
                    if (step.x > 0) outCollision->normal = {-1, 0};
                    else if (step.x < 0) outCollision->normal = {1, 0};
                    else if (step.y > 0) outCollision->normal = {0, -1};
                    else if (step.y < 0) outCollision->normal = {0, 1};
                }
                break;
            }
        }
    }

    return collided;
}

void KinematicActor::moveAndSlide(pixelroot32::math::Vector2 velocity, pixelroot32::math::Vector2 upDirection) {
    (void)upDirection;
    KinematicCollision collision;
    if (moveAndCollide(velocity, &collision)) {
        // Sliding logic could be added here
    }
}

void KinematicActor::draw(pixelroot32::graphics::Renderer& renderer) {
    (void)renderer;
}

} // namespace pixelroot32::physics
