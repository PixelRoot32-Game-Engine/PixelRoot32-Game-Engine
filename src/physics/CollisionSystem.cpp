/*
 * Original work:
 * Copyright (c) nbourre
 * Licensed under the MIT License
 *
 * Modifications:
 * Copyright (c) 2026 PixelRoot32
 *
 * This file remains licensed under the MIT License.
 */
#include "physics/CollisionSystem.h"
#include "core/Actor.h"
#include "core/PhysicsActor.h"

namespace pixelroot32::physics {

    using namespace pixelroot32::core;

    void CollisionSystem::addEntity(Entity* e) { entities.push_back(e); }
    void CollisionSystem::removeEntity(Entity* e) { 
        entities.erase(std::remove(entities.begin(), entities.end(), e), entities.end());
    }

    void CollisionSystem::update() {
        // ================================================================
        // FLAT SOLVER — ESP32 Optimized
        // ================================================================
        // 1. Populate broadphase grid (dynamic bodies only)
        // 2. Single detection pass → collect pairs
        // 3. Iterative relaxation (position correction only)
        // 4. Velocity response after all positions are settled
        // ================================================================

        // --- Step 1: Populate grid (skip static actors) ---
        grid.clear();
        for (auto e : entities) {
            if (e->type != EntityType::ACTOR) continue;
            Actor* actor = static_cast<Actor*>(e);
            if (actor->isPhysicsBody()) {
                PhysicsActor* pa = static_cast<PhysicsActor*>(actor);
                if (pa->getBodyType() == PhysicsBodyType::STATIC) continue; // Statics skip grid
            }
            grid.insert(actor);
        }

        // --- Step 2: Single detection pass ---
        static Actor* potential[64];
        static CollisionPair dynamicPairs[kMaxPairs];
        int dynamicPairCount = 0;
        static CollisionPair staticPairs[kMaxPairs];
        int staticPairCount = 0;

        for (auto e : entities) {
            if (e->type != EntityType::ACTOR) continue;
            Actor* actorA = static_cast<Actor*>(e);

            // --- Check against grid (dynamic vs dynamic) ---
            int count = 0;
            grid.getPotentialColliders(actorA, potential, count, 64);

            for (int i = 0; i < count; ++i) {
                Actor* actorB = potential[i];
                if (actorA >= actorB) continue; // Avoid duplicate pairs

                if ((actorA->mask & actorB->layer) || (actorB->mask & actorA->layer)) {
                    if (actorA->getHitBox().intersects(actorB->getHitBox())) {
                        // Notification callbacks (no physics changes here)
                        actorA->onCollision(actorB);
                        actorB->onCollision(actorA);

                        if (actorA->isPhysicsBody() && actorB->isPhysicsBody()) {
                            if (dynamicPairCount < kMaxPairs) {
                                dynamicPairs[dynamicPairCount++] = {actorA, actorB};
                            }
                        }
                    }
                }
            }

            // --- Check against ALL static bodies directly (bypass grid) ---
            if (!actorA->isPhysicsBody()) continue;
            PhysicsActor* paA = static_cast<PhysicsActor*>(actorA);
            if (paA->getBodyType() == PhysicsBodyType::STATIC) continue; // Static vs static: skip

            for (auto se : entities) {
                if (se->type != EntityType::ACTOR) continue;
                Actor* staticActor = static_cast<Actor*>(se);
                if (staticActor == actorA) continue;
                if (!staticActor->isPhysicsBody()) continue;
                
                PhysicsActor* paStatic = static_cast<PhysicsActor*>(staticActor);
                if (paStatic->getBodyType() != PhysicsBodyType::STATIC) continue;

                if ((actorA->mask & staticActor->layer) || (staticActor->mask & actorA->layer)) {
                    if (actorA->getHitBox().intersects(staticActor->getHitBox())) {
                        actorA->onCollision(staticActor);
                        staticActor->onCollision(actorA);

                        if (staticPairCount < kMaxPairs) {
                            staticPairs[staticPairCount++] = {actorA, staticActor};
                        }
                    }
                }
            }
        }

        // --- Step 3: Iterative relaxation ---
        // Multiple passes of position-only correction for stability.
        // Each pass re-checks the SAME pairs (positions shift each iteration).
        for (int iter = 0; iter < kRelaxationIterations; ++iter) {
            // Pass 1: Dynamic vs Dynamic
            for (int i = 0; i < dynamicPairCount; ++i) {
                resolve(dynamicPairs[i].a, dynamicPairs[i].b);
            }

            // Pass 2: Dynamic vs Static (final arbiter)
            for (int i = 0; i < staticPairCount; ++i) {
                resolve(staticPairs[i].a, staticPairs[i].b);
            }
        }
    }

    void CollisionSystem::resolve(Actor* a, Actor* b) {
        PhysicsActor* pA = static_cast<PhysicsActor*>(a);
        PhysicsActor* pB = static_cast<PhysicsActor*>(b);

        PhysicsBodyType typeA = pA->getBodyType();
        PhysicsBodyType typeB = pB->getBodyType();

        // Skip if both are static (shouldn't happen, but safety)
        if (typeA == PhysicsBodyType::STATIC && typeB == PhysicsBodyType::STATIC) return;

        // Skip if neither is rigid (kinematic vs kinematic handled by moveAndCollide)
        bool isAnyRigid = (typeA == PhysicsBodyType::RIGID || typeB == PhysicsBodyType::RIGID);
        if (!isAnyRigid) return;

        // Re-check overlap (positions may have changed during relaxation)
        Rect rectA = pA->getHitBox();
        Rect rectB = pB->getHitBox();
        if (!rectA.intersects(rectB)) return;

        using pixelroot32::math::Scalar;
        using pixelroot32::math::toScalar;

        // Calculate overlap
        Scalar centerAX = rectA.position.x + toScalar(rectA.width / 2.0f);
        Scalar centerAY = rectA.position.y + toScalar(rectA.height / 2.0f);
        Scalar centerBX = rectB.position.x + toScalar(rectB.width / 2.0f);
        Scalar centerBY = rectB.position.y + toScalar(rectB.height / 2.0f);

        Scalar dx = centerAX - centerBX;
        Scalar dy = centerAY - centerBY;

        Scalar halfWidthSum = toScalar((rectA.width + rectB.width) / 2.0f);
        Scalar halfHeightSum = toScalar((rectA.height + rectB.height) / 2.0f);

        Scalar overlapX = halfWidthSum - (dx < toScalar(0) ? -dx : dx);
        Scalar overlapY = halfHeightSum - (dy < toScalar(0) ? -dy : dy);

        // Determine minimum separation axis
        if (overlapX < overlapY) {
            // --- Resolve on X axis ---
            Scalar normalX = (dx > toScalar(0)) ? toScalar(1.0f) : toScalar(-1.0f);

            if (typeA == PhysicsBodyType::RIGID && typeB == PhysicsBodyType::STATIC) {
                // Push A out entirely
                pA->position.x += normalX * overlapX;
                // Velocity response
                if (pA->bounce) {
                    pA->setVelocity(-pA->getVelocityX() * pA->getRestitution(), pA->getVelocityY());
                } else {
                    pA->setVelocity(toScalar(0), pA->getVelocityY());
                }
            } else if (typeB == PhysicsBodyType::RIGID && typeA == PhysicsBodyType::STATIC) {
                pB->position.x -= normalX * overlapX;
                if (pB->bounce) {
                    pB->setVelocity(-pB->getVelocityX() * pB->getRestitution(), pB->getVelocityY());
                } else {
                    pB->setVelocity(toScalar(0), pB->getVelocityY());
                }
            } else if (typeA == PhysicsBodyType::RIGID && typeB == PhysicsBodyType::KINEMATIC) {
                pA->position.x += normalX * overlapX;
                pA->setVelocity(toScalar(0), pA->getVelocityY());
            } else if (typeB == PhysicsBodyType::RIGID && typeA == PhysicsBodyType::KINEMATIC) {
                pB->position.x -= normalX * overlapX;
                pB->setVelocity(toScalar(0), pB->getVelocityY());
            } else {
                // Both rigid: share the correction 50/50
                Scalar half = overlapX * toScalar(0.5f);
                pA->position.x += normalX * half;
                pB->position.x -= normalX * half;
            }
        } else {
            // --- Resolve on Y axis ---
            Scalar normalY = (dy > toScalar(0)) ? toScalar(1.0f) : toScalar(-1.0f);

            if (typeA == PhysicsBodyType::RIGID && typeB == PhysicsBodyType::STATIC) {
                pA->position.y += normalY * overlapY;
                if (pA->bounce) {
                    pA->setVelocity(pA->getVelocityX(), -pA->getVelocityY() * pA->getRestitution());
                } else {
                    pA->setVelocity(pA->getVelocityX(), toScalar(0));
                }
            } else if (typeB == PhysicsBodyType::RIGID && typeA == PhysicsBodyType::STATIC) {
                pB->position.y -= normalY * overlapY;
                if (pB->bounce) {
                    pB->setVelocity(pB->getVelocityX(), -pB->getVelocityY() * pB->getRestitution());
                } else {
                    pB->setVelocity(pB->getVelocityX(), toScalar(0));
                }
            } else if (typeA == PhysicsBodyType::RIGID && typeB == PhysicsBodyType::KINEMATIC) {
                pA->position.y += normalY * overlapY;
                pA->setVelocity(pA->getVelocityX(), toScalar(0));
            } else if (typeB == PhysicsBodyType::RIGID && typeA == PhysicsBodyType::KINEMATIC) {
                pB->position.y -= normalY * overlapY;
                pB->setVelocity(pB->getVelocityX(), toScalar(0));
            } else {
                // Both rigid: share 50/50
                Scalar half = overlapY * toScalar(0.5f);
                pA->position.y += normalY * half;
                pB->position.y -= normalY * half;
            }
        }
    }

    bool CollisionSystem::checkCollision(Actor* actor, Actor** outArray, int& count, int maxCount) {
        count = 0;
        for (auto e : entities) {
            if (e == actor) continue;
            if (e->type != EntityType::ACTOR) continue;

            Actor* other = static_cast<Actor*>(e);

            if ((actor->mask & other->layer) || (other->mask & actor->layer)) {
                if (actor->getHitBox().intersects(other->getHitBox())) {
                    if (count < maxCount) {
                        outArray[count++] = other;
                    }
                }
            }
        }
        return count > 0;
    }
}
