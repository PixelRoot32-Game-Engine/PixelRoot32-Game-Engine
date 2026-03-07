/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Flat Solver - Minimalist Physics Pipeline
 * Order: Integrate Position → Detect → Solve Velocity → Solve Penetration
 * Note: Integration happens first to enable spatial crossing detection for one-way platforms
 */
#include "physics/CollisionSystem.h"
#include "core/Actor.h"
#include "core/PhysicsActor.h"
#include "math/MathUtil.h"
#include <cassert>

namespace pixelroot32::physics {

    using namespace pixelroot32::core;
    using namespace pixelroot32::math;

    namespace {
        struct ScalarRect {
            Scalar x, y, w, h;
            static ScalarRect from(const Rect& r) {
                return { r.position.x, r.position.y, toScalar(r.width), toScalar(r.height) };
            }
        };
    }

    void CollisionSystem::addEntity(Entity* e) {
        assert(e != nullptr && "Cannot add null entity to collision system");
        if (e->type == EntityType::ACTOR) {
            Actor* actor = static_cast<Actor*>(e);
            actor->entityId = nextEntityId++;
            if (nextEntityId == 0) nextEntityId = 1;  // Wrap: 0 is reserved
        }
        entities.push_back(e);
        grid.markStaticDirty();
    }

    void CollisionSystem::removeEntity(Entity* e) {
        assert(e != nullptr && "Cannot remove null entity from collision system");
        entities.erase(std::remove(entities.begin(), entities.end(), e), entities.end());
        grid.markStaticDirty();
    }

    void CollisionSystem::update() {
        // Store previous positions before integration
        for (auto e : entities) {
            if (e->type == EntityType::ACTOR) {
                Actor* actor = static_cast<Actor*>(e);
                if (actor->isPhysicsBody()) {
                    static_cast<PhysicsActor*>(actor)->updatePreviousPosition();
                }
            }
        }
        
        // Integrate positions FIRST to enable spatial crossing detection
        PIXELROOT32_PROFILE_BEGIN(Physics_IntegratePositions);
        integratePositions();
        PIXELROOT32_PROFILE_END(Physics_IntegratePositions);
        
        // Then detect collisions using previous and current positions
        PIXELROOT32_PROFILE_BEGIN(Physics_DetectCollisions);
        detectCollisions();
        PIXELROOT32_PROFILE_END(Physics_DetectCollisions);
        PIXELROOT32_PROFILE_BEGIN(Physics_SolveVelocity);
        solveVelocity();
        PIXELROOT32_PROFILE_END(Physics_SolveVelocity);
        PIXELROOT32_PROFILE_BEGIN(Physics_SolvePenetration);
        solvePenetration();
        PIXELROOT32_PROFILE_END(Physics_SolvePenetration);
        PIXELROOT32_PROFILE_BEGIN(Physics_TriggerCallbacks);
        triggerCallbacks();
        PIXELROOT32_PROFILE_END(Physics_TriggerCallbacks);
    }

    void CollisionSystem::detectCollisions() {
        contactCount = 0;
        grid.rebuildStaticIfNeeded(entities);
        grid.clearDynamic();

        for (auto e : entities) {
            if (e->type != EntityType::ACTOR) continue;
            Actor* actor = static_cast<Actor*>(e);
            if (!actor->isPhysicsBody()) continue;
            PhysicsActor* pa = static_cast<PhysicsActor*>(actor);
            if (pa->getBodyType() != PhysicsBodyType::STATIC)
                grid.insertDynamic(actor);
        }

        static Actor* potential[64];
        
        for (auto e : entities) {
            if (e->type != EntityType::ACTOR) continue;
            Actor* actorA = static_cast<Actor*>(e);
            if (!actorA->isPhysicsBody()) continue;
            PhysicsActor* pA = static_cast<PhysicsActor*>(actorA);
            
            int count = 0;
            grid.getPotentialColliders(actorA, potential, count, 64);
            
            for (int i = 0; i < count; ++i) {
                Actor* actorB = potential[i];
                // Deduplicate by entityId: process each pair once (A with smaller id).
                if (actorA->entityId >= actorB->entityId) continue;
                
                if (!actorB->isPhysicsBody()) continue;
                PhysicsActor* pB = static_cast<PhysicsActor*>(actorB);
                
                // STATIC vs STATIC: never generate contact.
                if (pA->getBodyType() == PhysicsBodyType::STATIC &&
                    pB->getBodyType() == PhysicsBodyType::STATIC) continue;
                // KINEMATIC vs KINEMATIC: each resolves on its own.
                if (pA->getBodyType() == PhysicsBodyType::KINEMATIC &&
                    pB->getBodyType() == PhysicsBodyType::KINEMATIC) continue;
                
                // Layer/mask filter.
                if (!(actorA->mask & actorB->layer) && !(actorB->mask & actorA->layer)) continue;
                
                // CCD path: fast RIGID circle vs STATIC AABB (only for this pair type).
                PhysicsActor* moving = nullptr;
                PhysicsActor* staticBody = nullptr;
                if (pA->getBodyType() != PhysicsBodyType::STATIC && pB->getBodyType() == PhysicsBodyType::STATIC) {
                    moving = pA;
                    staticBody = pB;
                } else if (pB->getBodyType() != PhysicsBodyType::STATIC && pA->getBodyType() == PhysicsBodyType::STATIC) {
                    moving = pB;
                    staticBody = pA;
                }
                if (moving && staticBody && needsCCD(moving) &&
                    moving->getShape() == CollisionShape::CIRCLE &&
                    staticBody->getShape() == CollisionShape::AABB) {
                    Scalar hitTime;
                    Vector2 hitNormal;
                    if (sweptCircleVsAABB(moving, staticBody, hitTime, hitNormal)) {
                        // One-way platform validation
                        if (staticBody->isOneWay()) {
                            if (!validateOneWayPlatform(moving, staticBody, hitNormal)) {
                                continue;  // Skip this collision
                            }
                        }
                        
                        Contact contact;
                        contact.bodyA = moving;
                        contact.bodyB = staticBody;
                        contact.normal = hitNormal;
                        Scalar rA = moving->bounce ? moving->getRestitution() : toScalar(0.0f);
                        Scalar rB = staticBody->bounce ? staticBody->getRestitution() : toScalar(0.0f);
                        contact.restitution = min(rA, rB);
                        contact.penetration = toScalar(0.01f);
                        contact.contactPoint = moving->position + moving->getVelocity() * FIXED_DT * hitTime;
                        contact.isSensorContact = moving->isSensor() || staticBody->isSensor();
                        if (contactCount < kMaxContacts)
                            contacts[contactCount++] = contact;
                    }
                } else {
                    generateContact(pA, pB);
                }
            }
        }
    }

    bool CollisionSystem::generateContact(PhysicsActor* a, PhysicsActor* b) {
        assert(a != nullptr && "generateContact: bodyA is null");
        assert(b != nullptr && "generateContact: bodyB is null");
        assert(a != b && "generateContact: bodyA and bodyB are the same actor");
        
        Contact contact;
        contact.bodyA = a;
        contact.bodyB = b;
        contact.penetration = toScalar(0);
        Scalar rA = a->bounce ? a->getRestitution() : toScalar(0.0f);
        Scalar rB = b->bounce ? b->getRestitution() : toScalar(0.0f);
        contact.restitution = min(rA, rB);
        
        CollisionShape shapeA = a->getShape();
        CollisionShape shapeB = b->getShape();
        
        bool hit = false;
        if (shapeA == CollisionShape::CIRCLE && shapeB == CollisionShape::CIRCLE) {
            hit = generateCircleVsCircleContact(contact);
        } else if (shapeA == CollisionShape::AABB && shapeB == CollisionShape::AABB) {
            hit = generateAABBVsAABBContact(contact);
        } else {
            PhysicsActor* circle = (shapeA == CollisionShape::CIRCLE) ? a : b;
            PhysicsActor* box = (shapeA == CollisionShape::CIRCLE) ? b : a;
            hit = generateCircleVsAABBContact(contact, circle, box);
        }
        
        // One-way platform filter: validate spatial crossing
        if (hit && b->isOneWay()) {
            hit = validateOneWayPlatform(a, b, contact.normal);
        }
        if (hit && a->isOneWay()) {
            hit = validateOneWayPlatform(b, a, -contact.normal);
        }
        
        if (hit) {
            contact.isSensorContact = a->isSensor() || b->isSensor();
            if (contactCount < kMaxContacts)
                contacts[contactCount++] = contact;
        }
        return hit;
    }
    
    bool CollisionSystem::generateCircleVsCircleContact(Contact& contact) {
        PhysicsActor* pA = contact.bodyA;
        PhysicsActor* pB = contact.bodyB;
        
        Vector2 centerA = pA->position + Vector2(pA->getRadius(), pA->getRadius());
        Vector2 centerB = pB->position + Vector2(pB->getRadius(), pB->getRadius());
        Vector2 d = centerA - centerB;
        Scalar distSqr = d.lengthSquared();
        Scalar radiusSum = pA->getRadius() + pB->getRadius();
        
        if (distSqr >= radiusSum * radiusSum) {
            return false;
        }
        
        Scalar dist = sqrt(distSqr);
        if (dist > kEpsilon) {
            contact.normal = d / dist;
            contact.penetration = radiusSum - dist;
        } else {
            contact.normal = Vector2(0, -1);
            contact.penetration = radiusSum;
        }
        contact.contactPoint = centerB + contact.normal * pB->getRadius();
        return true;
    }
    
    bool CollisionSystem::generateAABBVsAABBContact(Contact& contact) {
        Rect rectA = contact.bodyA->getHitBox();
        Rect rectB = contact.bodyB->getHitBox();

        if (!rectA.intersects(rectB)) {
            return false;
        }

        ScalarRect sa = ScalarRect::from(rectA);
        ScalarRect sb = ScalarRect::from(rectB);
        Scalar centerAX = sa.x + sa.w / 2;
        Scalar centerAY = sa.y + sa.h / 2;
        Scalar centerBX = sb.x + sb.w / 2;
        Scalar centerBY = sb.y + sb.h / 2;
        Scalar dx = centerAX - centerBX;
        Scalar dy = centerAY - centerBY;
        Scalar overlapX = (sa.w + sb.w) / 2 - abs(dx);
        Scalar overlapY = (sa.h + sb.h) / 2 - abs(dy);

        if (overlapX < overlapY) {
            contact.normal = (dx > 0) ? Vector2(1, 0) : Vector2(-1, 0);
            contact.penetration = overlapX;
        } else {
            contact.normal = (dy > 0) ? Vector2(0, 1) : Vector2(0, -1);
            contact.penetration = overlapY;
        }

        contact.contactPoint = Vector2(
            (max(sa.x, sb.x) + min(sa.x + sa.w, sb.x + sb.w)) / 2,
            (max(sa.y, sb.y) + min(sa.y + sa.h, sb.y + sb.h)) / 2
        );
        return true;
    }
    
    bool CollisionSystem::generateCircleVsAABBContact(Contact& contact,
                                                       PhysicsActor* circle,
                                                       PhysicsActor* box) {
        Scalar r = circle->getRadius();
        Vector2 centerC = circle->position + Vector2(r, r);
        ScalarRect boxRec = ScalarRect::from(box->getHitBox());

        Vector2 closestP = centerC;
        closestP.x = clamp(closestP.x, boxRec.x, boxRec.x + boxRec.w);
        closestP.y = clamp(closestP.y, boxRec.y, boxRec.y + boxRec.h);

        Vector2 v = centerC - closestP;
        Scalar distSqr = v.lengthSquared();

        if (distSqr >= r * r) {
            return false;
        }

        Scalar dist = sqrt(distSqr);
        if (dist > kEpsilon) {
            contact.normal = v / dist;
            contact.penetration = r - dist;
        } else {
            Scalar dLeft = centerC.x - boxRec.x;
            Scalar dRight = (boxRec.x + boxRec.w) - centerC.x;
            Scalar dTop = centerC.y - boxRec.y;
            Scalar dBottom = (boxRec.y + boxRec.h) - centerC.y;
            Scalar minDist = dLeft;
            contact.normal = Vector2(-1, 0);
            if (dRight < minDist) { minDist = dRight; contact.normal = Vector2(1, 0); }
            if (dTop < minDist) { minDist = dTop; contact.normal = Vector2(0, -1); }
            if (dBottom < minDist) { minDist = dBottom; contact.normal = Vector2(0, 1); }
            contact.penetration = r + minDist;
        }

        contact.contactPoint = closestP;
        if (circle == contact.bodyB) {
            contact.normal = -contact.normal;
        }
        return true;
    }

    void CollisionSystem::solveVelocity() {
        for (int iter = 0; iter < VELOCITY_ITERATIONS; iter++) {
            for (int i = 0; i < contactCount; ++i) {
                Contact& contact = contacts[i];
                if (contact.isSensorContact) continue;
                
                PhysicsActor* bodyA = contact.bodyA;
                PhysicsActor* bodyB = contact.bodyB;
                
                if (bodyA->getBodyType() == PhysicsBodyType::STATIC && 
                    bodyB->getBodyType() == PhysicsBodyType::STATIC) {
                    continue;
                }
                
                Vector2 rv = bodyA->getVelocity() - bodyB->getVelocity();
                Scalar vn = rv.dot(contact.normal);
                
                if (vn > 0) continue;
                
                Scalar invMassA = (bodyA->getBodyType() == PhysicsBodyType::RIGID) ? 
                                 (toScalar(1.0f) / bodyA->getMass()) : toScalar(0.0f);
                Scalar invMassB = (bodyB->getBodyType() == PhysicsBodyType::RIGID) ? 
                                 (toScalar(1.0f) / bodyB->getMass()) : toScalar(0.0f);
                
                Scalar totalInvMass = invMassA + invMassB;
                if (totalInvMass <= kEpsilon) continue;
                
                Scalar e = contact.restitution;
                if (abs(vn) < VELOCITY_THRESHOLD) {
                    e = toScalar(0.0f);
                }
                
                Scalar j = -(toScalar(1.0f) + e) * vn;
                j /= totalInvMass;
                
                Vector2 impulse = contact.normal * j;
                
                if (bodyA->getBodyType() == PhysicsBodyType::RIGID) {
                    bodyA->setVelocity(bodyA->getVelocity() + impulse * invMassA);
                }
                if (bodyB->getBodyType() == PhysicsBodyType::RIGID) {
                    bodyB->setVelocity(bodyB->getVelocity() - impulse * invMassB);
                }
            }
        }
    }

    void CollisionSystem::integratePositions() {
        for (auto e : entities) {
            if (e->type != EntityType::ACTOR) continue;
            Actor* actor = static_cast<Actor*>(e);
            if (!actor->isPhysicsBody()) continue;
            
            PhysicsActor* pa = static_cast<PhysicsActor*>(actor);
            if (pa->getBodyType() != PhysicsBodyType::RIGID) continue;
            
            Vector2 vel = pa->getVelocity();
            if (abs(vel.x) < MIN_VELOCITY) vel.x = toScalar(0.0f);
            if (abs(vel.y) < MIN_VELOCITY) vel.y = toScalar(0.0f);
            pa->setVelocity(vel);
            
            pa->position = pa->position + vel * FIXED_DT;
        }
    }

    void CollisionSystem::solvePenetration() {
        for (int i = 0; i < contactCount; ++i) {
            Contact& contact = contacts[i];
            if (contact.isSensorContact) continue;
            if (contact.penetration <= SLOP) continue;
            
            PhysicsActor* bodyA = contact.bodyA;
            PhysicsActor* bodyB = contact.bodyB;
            
            Scalar invMassA = (bodyA->getBodyType() == PhysicsBodyType::RIGID) ? 
                             (toScalar(1.0f) / bodyA->getMass()) : toScalar(0.0f);
            Scalar invMassB = (bodyB->getBodyType() == PhysicsBodyType::RIGID) ? 
                             (toScalar(1.0f) / bodyB->getMass()) : toScalar(0.0f);
            
            Scalar totalInvMass = invMassA + invMassB;
            if (totalInvMass <= kEpsilon) continue;
            
            Scalar correction = (contact.penetration - SLOP) * BIAS;
            Vector2 correctionVec = contact.normal * (correction / totalInvMass);
            
            if (bodyA->getBodyType() == PhysicsBodyType::RIGID) {
                bodyA->position = bodyA->position + correctionVec * invMassA;
            }
            if (bodyB->getBodyType() == PhysicsBodyType::RIGID) {
                bodyB->position = bodyB->position - correctionVec * invMassB;
            }
        }
    }

    void CollisionSystem::triggerCallbacks() {
        for (int i = 0; i < contactCount; ++i) {
            const Contact& contact = contacts[i];
            if (contact.bodyA && contact.bodyB) {
                contact.bodyA->onCollision(static_cast<Actor*>(contact.bodyB));
                contact.bodyB->onCollision(static_cast<Actor*>(contact.bodyA));
            }
        }
    }

    bool CollisionSystem::checkCollision(Actor* actor, Actor** outArray, int& count, int maxCount) {
        assert(actor != nullptr && "checkCollision: actor is null");
        assert(outArray != nullptr && "checkCollision: outArray is null");
        assert(maxCount > 0 && "checkCollision: maxCount must be > 0");
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

    bool CollisionSystem::needsCCD(PhysicsActor* body) const {
        if (body->getShape() != CollisionShape::CIRCLE) return false;
        
        Scalar speed = body->getVelocity().length();
        Scalar movement = speed * FIXED_DT;
        Scalar threshold = body->getRadius() * CCD_THRESHOLD;
        
        return movement > threshold;
    }
    
    bool CollisionSystem::sweptCircleVsAABB(PhysicsActor* circle,
                                           PhysicsActor* box,
                                           Scalar& outTime,
                                           Vector2& outNormal) {
        Vector2 startPos = circle->position;
        Vector2 endPos = startPos + circle->getVelocity() * FIXED_DT;
        Scalar radius = circle->getRadius();
        Rect boxRect = box->getHitBox();
        
        Vector2 delta = endPos - startPos;
        Scalar distance = delta.length();
        
        int steps = 2;
        if (distance > radius * 2) steps = 4;
        if (distance > radius * 4) steps = 8;
        
        Vector2 prevPos = startPos;
        
        for (int i = 1; i <= steps; i++) {
            Scalar t = static_cast<Scalar>(i) / steps;
            Vector2 samplePos = startPos + delta * t;
            
            Circle tempCircle = {samplePos.x + radius, samplePos.y + radius, radius};
            
            if (intersects(tempCircle, boxRect)) {
                outTime = static_cast<Scalar>(i - 1) / steps;
                
                Vector2 center = Vector2(samplePos.x + radius, samplePos.y + radius);
                Vector2 boxCenter = Vector2(
                    boxRect.position.x + toScalar(boxRect.width) / 2,
                    boxRect.position.y + toScalar(boxRect.height) / 2
                );
                Vector2 toBox = boxCenter - center;
                
                if (abs(toBox.x) > abs(toBox.y)) {
                    outNormal = (toBox.x > 0) ? Vector2(-1, 0) : Vector2(1, 0);
                } else {
                    outNormal = (toBox.y > 0) ? Vector2(0, -1) : Vector2(0, 1);
                }
                
                return true;
            }
            
            prevPos = samplePos;
        }
        
        return false;
    }

    bool CollisionSystem::validateOneWayPlatform(
        PhysicsActor* actor,
        PhysicsActor* platform,
        const Vector2& collisionNormal
    ) {
        // Not a one-way platform, always valid
        if (!platform->isOneWay()) return true;
        
        // One-way platforms only affect vertical collisions
        // Reject horizontal collisions (side collisions) completely
        Scalar absNormalY = (collisionNormal.y < toScalar(0)) ? -collisionNormal.y : collisionNormal.y;
        if (absNormalY < toScalar(0.1f)) {
            // Normal is mostly horizontal, ignore this collision for one-way platforms
            return false;
        }
        
        // One-way platforms only block from above (normal pointing up to push actor up)
        // In this engine, Y increases downward, so normal.y < 0 means pointing up
        if (collisionNormal.y >= toScalar(0)) return false;
        
        // Check if actor crossed platform surface from above
        Rect platformBox = platform->getHitBox();
        Scalar platformTop = platformBox.position.y;
        
        Scalar previousBottom = actor->getPreviousPosition().y + toScalar(actor->height);
        Scalar currentBottom = actor->position.y + toScalar(actor->height);
        
        // Must have been above surface and now at/below surface
        bool crossedFromAbove = (previousBottom <= platformTop) && 
                               (currentBottom >= platformTop);
        
        // Must be moving down or stationary
        bool movingDown = actor->getVelocity().y >= toScalar(0);
        
        return crossedFromAbove && movingDown;
    }

}
