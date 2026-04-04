/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "physics/SpatialGrid.h"
#include "core/Actor.h"
#include "core/Entity.h"
#include "core/PhysicsActor.h"
#include "math/MathUtil.h"

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

namespace pixelroot32::physics {

    namespace core = pixelroot32::core;
    namespace math = pixelroot32::math;
    using core::Actor;
    using core::Entity;
    using core::EntityType;
    using core::PhysicsActor;
    using core::PhysicsBodyType;
    using core::Rect;
    using math::Scalar;
    using math::Vector2;
    using math::toScalar;

    Actor* SpatialGrid::staticCells[SpatialGrid::kMaxCells][SpatialGrid::kMaxStaticPerCell];
    int SpatialGrid::staticCellCounts[SpatialGrid::kMaxCells];
    Actor* SpatialGrid::dynamicCells[SpatialGrid::kMaxCells][SpatialGrid::kMaxDynamicPerCell];
    int SpatialGrid::dynamicCellCounts[SpatialGrid::kMaxCells];

    void SpatialGrid::clear() {
        for (int i = 0; i < kMaxCells; ++i) {
            staticCellCounts[i] = 0;
            dynamicCellCounts[i] = 0;
        }
        staticDirty = true;
    }

    void SpatialGrid::clearDynamic() {
        for (int i = 0; i < kMaxCells; ++i) {
            dynamicCellCounts[i] = 0;
        }
    }

    void SpatialGrid::markStaticDirty() {
        staticDirty = true;
    }

    int IRAM_ATTR SpatialGrid::getCellIndex(Scalar x, Scalar y) const {
        int ix = static_cast<int>(x) / kCellSize;
        int iy = static_cast<int>(y) / kCellSize;
        int cols = pixelroot32::platforms::config::LogicalWidth / kCellSize + 1;
        int rows = pixelroot32::platforms::config::LogicalHeight / kCellSize + 1;
        if (ix < 0) ix = 0;
        if (ix >= cols) ix = cols - 1;
        if (iy < 0) iy = 0;
        if (iy >= rows) iy = rows - 1;
        return iy * cols + ix;
    }

    void SpatialGrid::rebuildStaticIfNeeded(Entity* const* entities, uint16_t entityCount) {
        if (!staticDirty) return;
        for (int i = 0; i < kMaxCells; ++i) {
            staticCellCounts[i] = 0;
        }
        for (uint16_t i = 0; i < entityCount; ++i) {
            Entity* e = entities[i];
            if (e->type != EntityType::ACTOR) continue;
            Actor* actor = static_cast<Actor*>(e);
            if (!actor->isPhysicsBody()) continue;
            PhysicsActor* pa = static_cast<PhysicsActor*>(actor);
            if (pa->getBodyType() != PhysicsBodyType::STATIC) continue;

            Rect rect = actor->getHitBox();
            int minCol = static_cast<int>(rect.position.x) / kCellSize;
            int minRow = static_cast<int>(rect.position.y) / kCellSize;
            int maxCol = static_cast<int>(rect.position.x + toScalar(rect.width)) / kCellSize;
            int maxRow = static_cast<int>(rect.position.y + toScalar(rect.height)) / kCellSize;
            int cols = pixelroot32::platforms::config::LogicalWidth / kCellSize + 1;
            int rows = pixelroot32::platforms::config::LogicalHeight / kCellSize + 1;
            if (minCol < 0) minCol = 0;
            if (maxCol >= cols) maxCol = cols - 1;
            if (minRow < 0) minRow = 0;
            if (maxRow >= rows) maxRow = rows - 1;

            for (int r = minRow; r <= maxRow; ++r) {
                for (int c = minCol; c <= maxCol; ++c) {
                    int idx = r * cols + c;
                    if (staticCellCounts[idx] < kMaxStaticPerCell) {
                        staticCells[idx][staticCellCounts[idx]++] = actor;
                    }
                }
            }
        }
        staticDirty = false;
    }

    void IRAM_ATTR SpatialGrid::insertDynamic(Actor* actor) {
        Rect rect = actor->getHitBox();
        int minCol = static_cast<int>(rect.position.x) / kCellSize;
        int minRow = static_cast<int>(rect.position.y) / kCellSize;
        int maxCol = static_cast<int>(rect.position.x + toScalar(rect.width)) / kCellSize;
        int maxRow = static_cast<int>(rect.position.y + toScalar(rect.height)) / kCellSize;
        int cols = pixelroot32::platforms::config::LogicalWidth / kCellSize + 1;
        int rows = pixelroot32::platforms::config::LogicalHeight / kCellSize + 1;
        if (minCol < 0) minCol = 0;
        if (maxCol >= cols) maxCol = cols - 1;
        if (minRow < 0) minRow = 0;
        if (maxRow >= rows) maxRow = rows - 1;

        for (int r = minRow; r <= maxRow; ++r) {
            for (int c = minCol; c <= maxCol; ++c) {
                int idx = r * cols + c;
                if (dynamicCellCounts[idx] < kMaxDynamicPerCell) {
                    dynamicCells[idx][dynamicCellCounts[idx]++] = actor;
                }
            }
        }
    }

    void IRAM_ATTR SpatialGrid::getPotentialColliders(Actor* actor, Actor** outArray, int& count, int maxCount) {
        Rect rect = actor->getHitBox();
        int minCol = static_cast<int>(rect.position.x) / kCellSize;
        int minRow = static_cast<int>(rect.position.y) / kCellSize;
        int maxCol = static_cast<int>(rect.position.x + toScalar(rect.width)) / kCellSize;
        int maxRow = static_cast<int>(rect.position.y + toScalar(rect.height)) / kCellSize;
        int cols = pixelroot32::platforms::config::LogicalWidth / kCellSize + 1;
        int rows = pixelroot32::platforms::config::LogicalHeight / kCellSize + 1;
        if (minCol < 0) minCol = 0;
        if (maxCol >= cols) maxCol = cols - 1;
        if (minRow < 0) minRow = 0;
        if (maxRow >= rows) maxRow = rows - 1;

        count = 0;
        queryId++;
        if (queryId < 0) queryId = 0;

        for (int r = minRow; r <= maxRow; ++r) {
            for (int c = minCol; c <= maxCol; ++c) {
                int idx = r * cols + c;
                for (int i = 0; i < staticCellCounts[idx] && count < maxCount; ++i) {
                    Actor* other = staticCells[idx][i];
                    if (other == actor) continue;
                    if (other->queryId != queryId) {
                        other->queryId = queryId;
                        outArray[count++] = other;
                    }
                }
                for (int i = 0; i < dynamicCellCounts[idx] && count < maxCount; ++i) {
                    Actor* other = dynamicCells[idx][i];
                    if (other == actor) continue;
                    if (other->queryId != queryId) {
                        other->queryId = queryId;
                        outArray[count++] = other;
                    }
                }
            }
        }
    }

} // namespace pixelroot32::physics
