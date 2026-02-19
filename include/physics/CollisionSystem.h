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

/**
 * @class CollisionSystem
 * @brief Flat Solver collision system optimized for ESP32.
 *
 * Architecture:
 *   1. Single detection pass (broadphase + narrowphase)
 *   2. Iterative relaxation (position-only corrections)
 *   3. Static bodies resolved separately as final arbiter
 *   4. Zero heap allocations in hot path
 */
class CollisionSystem {
public:
    void addEntity(pixelroot32::core::Entity* e);
    void removeEntity(pixelroot32::core::Entity* e);

    /**
     * @brief Runs the full collision pipeline: detect, relax, resolve statics.
     */
    void update();

    size_t getEntityCount() const { return entities.size(); }
    void clear() { entities.clear(); }

    /**
     * @brief Checks for collisions for a specific actor (used by KinematicActor).
     * @param actor The actor to check.
     * @param outArray Fixed-size array to store results.
     * @param count Output: number of collisions found.
     * @param maxCount Maximum capacity of outArray.
     * @return true if at least one collision was found.
     */
    bool checkCollision(pixelroot32::core::Actor* actor, pixelroot32::core::Actor** outArray, int& count, int maxCount);

private:
    static constexpr int kMaxPairs = 128;
    static constexpr int kRelaxationIterations = 3;

    struct CollisionPair {
        pixelroot32::core::Actor* a;
        pixelroot32::core::Actor* b;
    };

    void resolve(pixelroot32::core::Actor* a, pixelroot32::core::Actor* b);

    std::vector<pixelroot32::core::Entity*> entities;
    SpatialGrid grid;
};

}
