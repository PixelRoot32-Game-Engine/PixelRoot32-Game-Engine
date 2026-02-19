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
#include "math/MathUtil.h"

namespace pixelroot32::physics {

    using namespace pixelroot32::core;
    using namespace pixelroot32::math;

    void CollisionSystem::addEntity(Entity* e) { entities.push_back(e); }
    void CollisionSystem::removeEntity(Entity* e) { 
        entities.erase(std::remove(entities.begin(), entities.end(), e), entities.end());
    }

    void CollisionSystem::update() {
        // --- Step 1: Populate grid (skip static actors) ---
        grid.clear();
        for (auto e : entities) {
            if (e->type != EntityType::ACTOR) continue;
            Actor* actor = static_cast<Actor*>(e);
            if (actor->isPhysicsBody()) {
                PhysicsActor* pa = static_cast<PhysicsActor*>(actor);
                if (pa->getBodyType() == PhysicsBodyType::STATIC) continue;
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

            int count = 0;
            grid.getPotentialColliders(actorA, potential, count, 64);

            for (int i = 0; i < count; ++i) {
                Actor* actorB = potential[i];
                if (actorA >= actorB) continue;

                if ((actorA->mask & actorB->layer) || (actorB->mask & actorA->layer)) {
                    // Collision check handles AABB vs AABB, Circle vs Circle, or Circle vs AABB
                    bool isColliding = false;
                    PhysicsActor* pA = actorA->isPhysicsBody() ? static_cast<PhysicsActor*>(actorA) : nullptr;
                    PhysicsActor* pB = actorB->isPhysicsBody() ? static_cast<PhysicsActor*>(actorB) : nullptr;

                    if (pA && pB) {
                        CollisionShape shapeA = pA->getShape();
                        CollisionShape shapeB = pB->getShape();

                        if (shapeA == CollisionShape::AABB && shapeB == CollisionShape::AABB) {
                            isColliding = actorA->getHitBox().intersects(actorB->getHitBox());
                        } else if (shapeA == CollisionShape::CIRCLE && shapeB == CollisionShape::CIRCLE) {
                            Circle cA = {pA->position.x + pA->getRadius(), pA->position.y + pA->getRadius(), pA->getRadius()};
                            Circle cB = {pB->position.x + pB->getRadius(), pB->position.y + pB->getRadius(), pB->getRadius()};
                            isColliding = intersects(cA, cB);
                        } else {
                            PhysicsActor* circP = (shapeA == CollisionShape::CIRCLE) ? pA : pB;
                            PhysicsActor* boxP = (shapeA == CollisionShape::CIRCLE) ? pB : pA;
                            Circle c = {circP->position.x + circP->getRadius(), circP->position.y + circP->getRadius(), circP->getRadius()};
                            isColliding = intersects(c, boxP->getHitBox());
                        }
                    } else {
                        // Fallback to AABB for non-physics actors
                        isColliding = actorA->getHitBox().intersects(actorB->getHitBox());
                    }

                    if (isColliding) {
                        actorA->onCollision(actorB);
                        actorB->onCollision(actorA);

                        if (pA && pB) {
                            if (dynamicPairCount < kMaxPairs) {
                                dynamicPairs[dynamicPairCount++] = {actorA, actorB};
                            }
                        }
                    }
                }
            }

            // --- Check against ALL static bodies directly ---
            if (!actorA->isPhysicsBody()) continue;
            PhysicsActor* pA = static_cast<PhysicsActor*>(actorA);
            if (pA->getBodyType() == PhysicsBodyType::STATIC) continue;

            for (auto se : entities) {
                if (se->type != EntityType::ACTOR) continue;
                Actor* sActor = static_cast<Actor*>(se);
                if (sActor == actorA || !sActor->isPhysicsBody()) continue;
                
                PhysicsActor* pStatic = static_cast<PhysicsActor*>(sActor);
                if (pStatic->getBodyType() != PhysicsBodyType::STATIC) continue;

                if ((actorA->mask & sActor->layer) || (sActor->mask & actorA->layer)) {
                    bool isColliding = false;
                    CollisionShape shapeA = pA->getShape();
                    CollisionShape shapeStatic = pStatic->getShape();

                    if (shapeA == CollisionShape::AABB && shapeStatic == CollisionShape::AABB) {
                        isColliding = actorA->getHitBox().intersects(sActor->getHitBox());
                    } else if (shapeA == CollisionShape::CIRCLE && shapeStatic == CollisionShape::CIRCLE) {
                        Circle cA = {pA->position.x + pA->getRadius(), pA->position.y + pA->getRadius(), pA->getRadius()};
                        Circle cS = {pStatic->position.x + pStatic->getRadius(), pStatic->position.y + pStatic->getRadius(), pStatic->getRadius()};
                        isColliding = intersects(cA, cS);
                    } else {
                        PhysicsActor* circP = (shapeA == CollisionShape::CIRCLE) ? pA : pStatic;
                        PhysicsActor* boxP = (shapeA == CollisionShape::CIRCLE) ? pStatic : pA;
                        Circle c = {circP->position.x + circP->getRadius(), circP->position.y + circP->getRadius(), circP->getRadius()};
                        isColliding = intersects(c, boxP->getHitBox());
                    }

                    if (isColliding) {
                        actorA->onCollision(sActor);
                        sActor->onCollision(actorA);

                        if (staticPairCount < kMaxPairs) {
                            staticPairs[staticPairCount++] = {actorA, sActor};
                        }
                    }
                }
            }
        }

        // --- Step 3: Iterative relaxation ---
        for (int iter = 0; iter < kRelaxationIterations; ++iter) {
            for (int i = 0; i < dynamicPairCount; ++i) resolve(dynamicPairs[i].a, dynamicPairs[i].b);
            for (int i = 0; i < staticPairCount; ++i) resolve(staticPairs[i].a, staticPairs[i].b);
        }
    }

    // Helper to apply common response logic (separation + velocity zeroing/bounce)
    void applyResponse(PhysicsActor* pA, PhysicsActor* pB, Vector2 normal, Scalar overlap) {
        PhysicsBodyType typeA = pA->getBodyType();
        PhysicsBodyType typeB = pB->getBodyType();

        // Separate
        if (typeA == PhysicsBodyType::RIGID && typeB == PhysicsBodyType::STATIC) {
            pA->position += normal * overlap;
            // Velocity
            Scalar vn = pA->getVelocity().dot(normal);
            if (vn < toScalar(0)) {
                if (pA->bounce) pA->setVelocity(pA->getVelocity().reflect(normal) * pA->getRestitution());
                else pA->setVelocity(pA->getVelocity().slide(normal));
            }
        } else if (typeB == PhysicsBodyType::RIGID && typeA == PhysicsBodyType::STATIC) {
            pB->position -= normal * overlap;
            Scalar vn = pB->getVelocity().dot(normal);
            if (vn > toScalar(0)) {
                if (pB->bounce) pB->setVelocity(pB->getVelocity().reflect(normal) * pB->getRestitution());
                else pB->setVelocity(pB->getVelocity().slide(normal));
            }
        } else if (typeA == PhysicsBodyType::RIGID && typeB == PhysicsBodyType::RIGID) {
            Scalar half = overlap * toScalar(0.5f);
            pA->position += normal * half;
            pB->position -= normal * half;
            // Anti-vibration: Zero approaching velocity
            Scalar vnA = pA->getVelocity().dot(normal);
            Scalar vnB = pB->getVelocity().dot(normal);
            if (vnA < toScalar(0)) pA->setVelocity(pA->getVelocity().slide(normal));
            if (vnB > toScalar(0)) pB->setVelocity(pB->getVelocity().slide(normal));
        } else if (typeA == PhysicsBodyType::RIGID && typeB == PhysicsBodyType::KINEMATIC) {
            pA->position += normal * overlap;
            Scalar vn = pA->getVelocity().dot(normal);
            if (vn < toScalar(0)) pA->setVelocity(pA->getVelocity().slide(normal));
        } else if (typeB == PhysicsBodyType::RIGID && typeA == PhysicsBodyType::KINEMATIC) {
            pB->position -= normal * overlap;
            Scalar vn = pB->getVelocity().dot(normal);
            if (vn > toScalar(0)) pB->setVelocity(pB->getVelocity().slide(normal));
        }
    }

    void CollisionSystem::resolve(Actor* a, Actor* b) {
        PhysicsActor* pA = static_cast<PhysicsActor*>(a);
        PhysicsActor* pB = static_cast<PhysicsActor*>(b);

        CollisionShape shapeA = pA->getShape();
        CollisionShape shapeB = pB->getShape();

        if (shapeA == CollisionShape::AABB && shapeB == CollisionShape::AABB) {
            // --- AABB vs AABB (existing logic) ---
            Rect rectA = pA->getHitBox();
            Rect rectB = pB->getHitBox();
            if (!rectA.intersects(rectB)) return;

            Scalar centerAX = rectA.position.x + toScalar(rectA.width / 2.0f);
            Scalar centerAY = rectA.position.y + toScalar(rectA.height / 2.0f);
            Scalar centerBX = rectB.position.x + toScalar(rectB.width / 2.0f);
            Scalar centerBY = rectB.position.y + toScalar(rectB.height / 2.0f);
            Scalar dx = centerAX - centerBX;
            Scalar dy = centerAY - centerBY;
            Scalar overlapX = toScalar((rectA.width + rectB.width) / 2.0f) - abs(dx);
            Scalar overlapY = toScalar((rectA.height + rectB.height) / 2.0f) - abs(dy);

            if (overlapX < overlapY) {
                Vector2 normal = (dx > 0) ? Vector2(1, 0) : Vector2(-1, 0);
                applyResponse(pA, pB, normal, overlapX);
            } else {
                Vector2 normal = (dy > 0) ? Vector2(0, 1) : Vector2(0, -1);
                applyResponse(pA, pB, normal, overlapY);
            }
        } else if (shapeA == CollisionShape::CIRCLE && shapeB == CollisionShape::CIRCLE) {
            // --- Circle vs Circle ---
            Vector2 centerA = pA->position + Vector2(pA->getRadius(), pA->getRadius());
            Vector2 centerB = pB->position + Vector2(pB->getRadius(), pB->getRadius());
            Vector2 d = centerA - centerB;
            Scalar distSqr = d.lengthSquared();
            Scalar radiusSum = pA->getRadius() + pB->getRadius();
            
            if (distSqr < radiusSum * radiusSum) {
                Scalar dist = sqrt(distSqr);
                Vector2 normal;
                Scalar overlap;

                if (dist > kEpsilon) {
                    normal = d / dist;
                    overlap = radiusSum - dist;
                } else {
                    // Perfectly overlapping: push apart vertically
                    normal = Vector2(0, -1);
                    overlap = radiusSum;
                }
                applyResponse(pA, pB, normal, overlap);
            }
        } else {
            // --- Circle vs AABB ---
            PhysicsActor* pCirc = (shapeA == CollisionShape::CIRCLE) ? pA : pB;
            PhysicsActor* pBox = (shapeA == CollisionShape::CIRCLE) ? pB : pA;
            Scalar r = pCirc->getRadius();
            Vector2 centerC = pCirc->position + Vector2(r, r);
            Rect boxRec = pBox->getHitBox();
            
            Vector2 closestP = centerC;
            closestP.x = clamp(closestP.x, boxRec.position.x, boxRec.position.x + toScalar(boxRec.width));
            closestP.y = clamp(closestP.y, boxRec.position.y, boxRec.position.y + toScalar(boxRec.height));

            Vector2 v = centerC - closestP;
            Scalar distSqr = v.lengthSquared();
            if (distSqr < r * r) {
                Scalar dist = sqrt(distSqr);
                Vector2 normal;
                Scalar overlap;
                if (dist > kEpsilon) {
                    normal = v / dist;
                    overlap = r - dist;
                } else {
                    // Circle center is inside Box: find closest edge
                    Scalar dLeft = centerC.x - boxRec.position.x;
                    Scalar dRight = (boxRec.position.x + toScalar(boxRec.width)) - centerC.x;
                    Scalar dTop = centerC.y - boxRec.position.y;
                    Scalar dBottom = (boxRec.position.y + toScalar(boxRec.height)) - centerC.y;
                    Scalar minDist = dLeft;
                    normal = Vector2(-1, 0);
                    if (dRight < minDist) { minDist = dRight; normal = Vector2(1, 0); }
                    if (dTop < minDist) { minDist = dTop; normal = Vector2(0, -1); }
                    if (dBottom < minDist) { minDist = dBottom; normal = Vector2(0, 1); }
                    overlap = r + minDist;
                }
                if (pCirc == pA) applyResponse(pA, pB, normal, overlap);
                else applyResponse(pA, pB, normal * toScalar(-1.0f), overlap);
            }
        }
    }

    bool CollisionSystem::checkCollision(Actor* actor, Actor** outArray, int& count, int maxCount) {
        count = 0;
        for (auto e : entities) {
            if (e == actor || e->type != EntityType::ACTOR) continue;
            Actor* other = static_cast<Actor*>(e);

            if ((actor->mask & other->layer) || (other->mask & actor->layer)) {
                bool isColliding = false;
                PhysicsActor* pA = actor->isPhysicsBody() ? static_cast<PhysicsActor*>(actor) : nullptr;
                PhysicsActor* pB = other->isPhysicsBody() ? static_cast<PhysicsActor*>(other) : nullptr;

                if (pA && pB) {
                    CollisionShape shapeA = pA->getShape();
                    CollisionShape shapeB = pB->getShape();
                    if (shapeA == CollisionShape::AABB && shapeB == CollisionShape::AABB) {
                        isColliding = actor->getHitBox().intersects(other->getHitBox());
                    } else if (shapeA == CollisionShape::CIRCLE && shapeB == CollisionShape::CIRCLE) {
                        Circle cA = {pA->position.x + pA->getRadius(), pA->position.y + pA->getRadius(), pA->getRadius()};
                        Circle cB = {pB->position.x + pB->getRadius(), pB->position.y + pB->getRadius(), pB->getRadius()};
                        isColliding = intersects(cA, cB);
                    } else {
                        PhysicsActor* circP = (shapeA == CollisionShape::CIRCLE) ? pA : pB;
                        PhysicsActor* boxP = (shapeA == CollisionShape::CIRCLE) ? pB : pA;
                        Circle c = {circP->position.x + circP->getRadius(), circP->position.y + circP->getRadius(), circP->getRadius()};
                        isColliding = intersects(c, boxP->getHitBox());
                    }
                } else {
                    isColliding = actor->getHitBox().intersects(other->getHitBox());
                }

                if (isColliding && count < maxCount) outArray[count++] = other;
            }
        }
        return count > 0;
    }
}
