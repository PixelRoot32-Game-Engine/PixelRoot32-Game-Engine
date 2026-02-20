/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Flat Solver - Minimalist Physics Pipeline
 * Order: Detect → Solve Velocity → Integrate Position → Solve Penetration
 */
#pragma once
#include <vector>
#include <algorithm>
#include "physics/CollisionTypes.h"
#include "physics/SpatialGrid.h"
#include "math/Vector2.h"
#include "core/Entity.h"
#include "platforms/EngineConfig.h"

namespace pixelroot32::core { class Actor; class PhysicsActor; }

namespace pixelroot32::physics {

struct KinematicCollision {
    pixelroot32::core::Actor* collider = nullptr;
    pixelroot32::math::Vector2 normal;
    pixelroot32::math::Vector2 position;
    pixelroot32::math::Scalar travel;
    pixelroot32::math::Scalar remainder;
};

struct Contact {
    pixelroot32::core::PhysicsActor* bodyA = nullptr;
    pixelroot32::core::PhysicsActor* bodyB = nullptr;
    pixelroot32::math::Vector2 normal;
    pixelroot32::math::Vector2 contactPoint;
    pixelroot32::math::Scalar penetration = pixelroot32::math::toScalar(0);
    pixelroot32::math::Scalar restitution = pixelroot32::math::toScalar(0);
};

class CollisionSystem {
public:
    static constexpr pixelroot32::math::Scalar FIXED_DT = pixelroot32::math::toScalar(1.0f / 60.0f);
    static constexpr pixelroot32::math::Scalar SLOP = pixelroot32::math::toScalar(0.02f);
    static constexpr pixelroot32::math::Scalar BIAS = pixelroot32::math::toScalar(0.2f);
    static constexpr pixelroot32::math::Scalar VELOCITY_THRESHOLD = pixelroot32::math::toScalar(0.5f);
    static constexpr pixelroot32::math::Scalar MIN_VELOCITY = pixelroot32::math::toScalar(0.01f);
    static constexpr int VELOCITY_ITERATIONS = 2;
    static constexpr pixelroot32::math::Scalar CCD_THRESHOLD = pixelroot32::math::toScalar(3.0f);
    
    void addEntity(pixelroot32::core::Entity* e);
    void removeEntity(pixelroot32::core::Entity* e);
    void update();
    void detectCollisions();
    void solveVelocity();
    void integratePositions();
    void solvePenetration();
    void triggerCallbacks();

    size_t getEntityCount() const { return entities.size(); }
    void clear() { entities.clear(); contacts.clear(); }

    bool checkCollision(pixelroot32::core::Actor* actor, 
                       pixelroot32::core::Actor** outArray, 
                       int& count, int maxCount);

    bool needsCCD(pixelroot32::core::PhysicsActor* body) const;
    bool sweptCircleVsAABB(pixelroot32::core::PhysicsActor* circle,
                          pixelroot32::core::PhysicsActor* box,
                          pixelroot32::math::Scalar& outTime,
                          pixelroot32::math::Vector2& outNormal);

private:
    static constexpr int kMaxPairs = pixelroot32::platforms::config::PhysicsMaxPairs;
    static constexpr int kRelaxationIterations = pixelroot32::platforms::config::PhysicsRelaxationIterations;
    static constexpr pixelroot32::math::Scalar kEpsilon = pixelroot32::math::toScalar(0.0001f);

    struct CollisionPair {
        pixelroot32::core::Actor* a;
        pixelroot32::core::Actor* b;
    };

    std::vector<pixelroot32::core::Entity*> entities;
    std::vector<Contact> contacts;
    SpatialGrid grid;
    
    void generateContact(pixelroot32::core::PhysicsActor* a, 
                        pixelroot32::core::PhysicsActor* b);
    void generateCircleVsCircleContact(Contact& contact);
    void generateAABBVsAABBContact(Contact& contact);
    void generateCircleVsAABBContact(Contact& contact, 
                                     pixelroot32::core::PhysicsActor* circle,
                                     pixelroot32::core::PhysicsActor* box);
};

}
