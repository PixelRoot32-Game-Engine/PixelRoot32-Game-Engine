/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Flat Solver v3.0 - Minimalist Physics Pipeline
 * Order: Detect → Solve Velocity → Integrate Position → Solve Penetration
 */
#include "physics/CollisionSystem.h"
#include "core/Actor.h"
#include "core/PhysicsActor.h"
#include "math/MathUtil.h"

namespace pixelroot32::physics {

    using namespace pixelroot32::core;
    using namespace pixelroot32::math;

    void CollisionSystem::addEntity(Entity* e) { 
        entities.push_back(e); 
    }
    
    void CollisionSystem::removeEntity(Entity* e) { 
        entities.erase(std::remove(entities.begin(), entities.end(), e), entities.end());
    }

    void CollisionSystem::update() {
        detectCollisions();
        solveVelocity();
        integratePositions();
        solvePenetration();
        triggerCallbacks();
    }

    void CollisionSystem::detectCollisions() {
        contacts.clear();
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
        
        static Actor* potential[64];
        
        for (auto e : entities) {
            if (e->type != EntityType::ACTOR) continue;
            Actor* actorA = static_cast<Actor*>(e);
            
            int count = 0;
            grid.getPotentialColliders(actorA, potential, count, 64);
            
            for (int i = 0; i < count; ++i) {
                Actor* actorB = potential[i];
                if (actorA >= actorB) continue;
                
                if ((actorA->mask & actorB->layer) || (actorB->mask & actorA->layer)) {
                    PhysicsActor* pA = actorA->isPhysicsBody() ? static_cast<PhysicsActor*>(actorA) : nullptr;
                    PhysicsActor* pB = actorB->isPhysicsBody() ? static_cast<PhysicsActor*>(actorB) : nullptr;
                    
                    if (pA && pB) {
                        generateContact(pA, pB);
                    }
                }
            }
            
            if (!actorA->isPhysicsBody()) continue;
            PhysicsActor* pA = static_cast<PhysicsActor*>(actorA);
            if (pA->getBodyType() == PhysicsBodyType::STATIC) continue;
            
            bool useCCD = needsCCD(pA);
            
            for (auto se : entities) {
                if (se->type != EntityType::ACTOR) continue;
                Actor* sActor = static_cast<Actor*>(se);
                if (sActor == actorA || !sActor->isPhysicsBody()) continue;
                
                PhysicsActor* pStatic = static_cast<PhysicsActor*>(sActor);
                if (pStatic->getBodyType() != PhysicsBodyType::STATIC) continue;
                
                if ((actorA->mask & sActor->layer) || (sActor->mask & actorA->layer)) {
                    if (useCCD && pA->getShape() == CollisionShape::CIRCLE && 
                        pStatic->getShape() == CollisionShape::AABB) {
                        Scalar hitTime;
                        Vector2 hitNormal;
                        if (sweptCircleVsAABB(pA, pStatic, hitTime, hitNormal)) {
                            Contact contact;
                            contact.bodyA = pA;
                            contact.bodyB = pStatic;
                            contact.normal = hitNormal;
                            contact.restitution = min(pA->getRestitution(), pStatic->getRestitution());
                            contact.penetration = toScalar(0.01f);
                            contact.contactPoint = pA->position + pA->getVelocity() * FIXED_DT * hitTime;
                            contacts.push_back(contact);
                        }
                    } else {
                        generateContact(pA, pStatic);
                    }
                }
            }
            
            if (pA->getBodyType() == PhysicsBodyType::RIGID) {
                for (auto ke : entities) {
                    if (ke->type != EntityType::ACTOR) continue;
                    Actor* kActor = static_cast<Actor*>(ke);
                    if (kActor == actorA || !kActor->isPhysicsBody()) continue;
                    
                    PhysicsActor* pKinematic = static_cast<PhysicsActor*>(kActor);
                    if (pKinematic->getBodyType() != PhysicsBodyType::KINEMATIC) continue;
                    
                    if ((actorA->mask & kActor->layer) || (kActor->mask & actorA->layer)) {
                        generateContact(pA, pKinematic);
                    }
                }
            }
        }
    }

    void CollisionSystem::generateContact(PhysicsActor* a, PhysicsActor* b) {
        Contact contact;
        contact.bodyA = a;
        contact.bodyB = b;
        contact.restitution = min(a->getRestitution(), b->getRestitution());
        
        CollisionShape shapeA = a->getShape();
        CollisionShape shapeB = b->getShape();
        
        if (shapeA == CollisionShape::CIRCLE && shapeB == CollisionShape::CIRCLE) {
            generateCircleVsCircleContact(contact);
        } else if (shapeA == CollisionShape::AABB && shapeB == CollisionShape::AABB) {
            generateAABBVsAABBContact(contact);
        } else {
            PhysicsActor* circle = (shapeA == CollisionShape::CIRCLE) ? a : b;
            PhysicsActor* box = (shapeA == CollisionShape::CIRCLE) ? b : a;
            generateCircleVsAABBContact(contact, circle, box);
        }
        
        if (contact.penetration > 0) {
            contacts.push_back(contact);
        }
    }
    
    void CollisionSystem::generateCircleVsCircleContact(Contact& contact) {
        PhysicsActor* pA = contact.bodyA;
        PhysicsActor* pB = contact.bodyB;
        
        Vector2 centerA = pA->position + Vector2(pA->getRadius(), pA->getRadius());
        Vector2 centerB = pB->position + Vector2(pB->getRadius(), pB->getRadius());
        Vector2 d = centerA - centerB;
        Scalar distSqr = d.lengthSquared();
        Scalar radiusSum = pA->getRadius() + pB->getRadius();
        
        if (distSqr < radiusSum * radiusSum) {
            Scalar dist = sqrt(distSqr);
            
            if (dist > kEpsilon) {
                contact.normal = d / dist;
                contact.penetration = radiusSum - dist;
            } else {
                contact.normal = Vector2(0, -1);
                contact.penetration = radiusSum;
            }
            
            contact.contactPoint = centerB + contact.normal * pB->getRadius();
        }
    }
    
    void CollisionSystem::generateAABBVsAABBContact(Contact& contact) {
        Rect rectA = contact.bodyA->getHitBox();
        Rect rectB = contact.bodyB->getHitBox();
        
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
            contact.normal = (dx > 0) ? Vector2(1, 0) : Vector2(-1, 0);
            contact.penetration = overlapX;
        } else {
            contact.normal = (dy > 0) ? Vector2(0, 1) : Vector2(0, -1);
            contact.penetration = overlapY;
        }
        
        contact.contactPoint = Vector2(
            (max(rectA.position.x, rectB.position.x) + min(rectA.position.x + toScalar(rectA.width), rectB.position.x + toScalar(rectB.width))) / 2,
            (max(rectA.position.y, rectB.position.y) + min(rectA.position.y + toScalar(rectA.height), rectB.position.y + toScalar(rectB.height))) / 2
        );
    }
    
    void CollisionSystem::generateCircleVsAABBContact(Contact& contact, 
                                                       PhysicsActor* circle,
                                                       PhysicsActor* box) {
        Scalar r = circle->getRadius();
        Vector2 centerC = circle->position + Vector2(r, r);
        Rect boxRec = box->getHitBox();
        
        Vector2 closestP = centerC;
        closestP.x = clamp(closestP.x, boxRec.position.x, boxRec.position.x + toScalar(boxRec.width));
        closestP.y = clamp(closestP.y, boxRec.position.y, boxRec.position.y + toScalar(boxRec.height));
        
        Vector2 v = centerC - closestP;
        Scalar distSqr = v.lengthSquared();
        
        if (distSqr < r * r) {
            Scalar dist = sqrt(distSqr);
            
            if (dist > kEpsilon) {
                contact.normal = v / dist;
                contact.penetration = r - dist;
            } else {
                Scalar dLeft = centerC.x - boxRec.position.x;
                Scalar dRight = (boxRec.position.x + toScalar(boxRec.width)) - centerC.x;
                Scalar dTop = centerC.y - boxRec.position.y;
                Scalar dBottom = (boxRec.position.y + toScalar(boxRec.height)) - centerC.y;
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
        }
    }

    void CollisionSystem::solveVelocity() {
        for (int iter = 0; iter < VELOCITY_ITERATIONS; iter++) {
            for (auto& contact : contacts) {
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
        for (auto& contact : contacts) {
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
        for (const auto& contact : contacts) {
            if (contact.bodyA && contact.bodyB) {
                contact.bodyA->onCollision(static_cast<Actor*>(contact.bodyB));
                contact.bodyB->onCollision(static_cast<Actor*>(contact.bodyA));
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

}
