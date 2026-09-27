/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "physics/SpatialGrid.h"
#include "core/Actor.h"
#include "core/Entity.h"
#include "core/PhysicsActor.h"
#include "core/Log.h"
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

#ifdef PIXELROOT32_DEBUG_MODE
    namespace logging = pixelroot32::core::logging;

    unsigned SpatialGrid::droppedStaticInserts_ = 0;
    unsigned SpatialGrid::droppedDynamicInserts_ = 0;

    // First-hit capacity reports (issue #243). Each limit logs once per
    // process; the static counters keep the cumulative totals for tests.
    // All of this compiles out in release.
    void reportStaticCellLimitOnce() {
        static bool reported = false;
        if (!reported) {
            reported = true;
            logging::log(logging::LogLevel::Warning,
                "SpatialGrid: static per-cell limit reached "
                "(SPATIAL_GRID_MAX_STATIC_PER_CELL=%d); body not registered in that cell. "
                "Raise SPATIAL_GRID_MAX_STATIC_PER_CELL.",
                pixelroot32::platforms::config::SpatialGridMaxStaticPerCell);
        }
    }

    void reportDynamicCellLimitOnce() {
        static bool reported = false;
        if (!reported) {
            reported = true;
            logging::log(logging::LogLevel::Warning,
                "SpatialGrid: dynamic per-cell limit reached "
                "(SPATIAL_GRID_MAX_DYNAMIC_PER_CELL=%d); body not registered in that cell. "
                "Raise SPATIAL_GRID_MAX_DYNAMIC_PER_CELL.",
                pixelroot32::platforms::config::SpatialGridMaxDynamicPerCell);
        }
    }
#endif

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
            if (minCol >= cols) minCol = cols - 1;
            if (maxCol < 0) maxCol = 0;
            if (maxCol >= cols) maxCol = cols - 1;
            if (minRow < 0) minRow = 0;
            if (minRow >= rows) minRow = rows - 1;
            if (maxRow < 0) maxRow = 0;
            if (maxRow >= rows) maxRow = rows - 1;

            for (int r = minRow; r <= maxRow; ++r) {
                for (int c = minCol; c <= maxCol; ++c) {
                    int idx = r * cols + c;
                    if (staticCellCounts[idx] < kMaxStaticPerCell) {
                        staticCells[idx][staticCellCounts[idx]++] = actor;
                    }
#ifdef PIXELROOT32_DEBUG_MODE
                    else {
                        ++droppedStaticInserts_;
                        reportStaticCellLimitOnce();
                    }
#endif
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
        if (minCol >= cols) minCol = cols - 1;
        if (maxCol < 0) maxCol = 0;
        if (maxCol >= cols) maxCol = cols - 1;
        if (minRow < 0) minRow = 0;
        if (minRow >= rows) minRow = rows - 1;
        if (maxRow < 0) maxRow = 0;
        if (maxRow >= rows) maxRow = rows - 1;

        for (int r = minRow; r <= maxRow; ++r) {
            for (int c = minCol; c <= maxCol; ++c) {
                int idx = r * cols + c;
                if (dynamicCellCounts[idx] < kMaxDynamicPerCell) {
                    dynamicCells[idx][dynamicCellCounts[idx]++] = actor;
                }
#ifdef PIXELROOT32_DEBUG_MODE
                else {
                    ++droppedDynamicInserts_;
                    reportDynamicCellLimitOnce();
                }
#endif
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
        if (minCol >= cols) minCol = cols - 1;
        if (maxCol < 0) maxCol = 0;
        if (maxCol >= cols) maxCol = cols - 1;
        if (minRow < 0) minRow = 0;
        if (minRow >= rows) minRow = rows - 1;
        if (maxRow < 0) maxRow = 0;
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

#if PIXELROOT32_ENABLE_SPATIAL_QUERY
    int IRAM_ATTR SpatialGrid::queryRadius(Vector2 center, Scalar radius, Actor** outArray, int maxCount) {
        int minCol = static_cast<int>(center.x - radius) / kCellSize;
        int minRow = static_cast<int>(center.y - radius) / kCellSize;
        int maxCol = static_cast<int>(center.x + radius) / kCellSize;
        int maxRow = static_cast<int>(center.y + radius) / kCellSize;
        int cols = pixelroot32::platforms::config::LogicalWidth / kCellSize + 1;
        int rows = pixelroot32::platforms::config::LogicalHeight / kCellSize + 1;
        if (minCol < 0) minCol = 0;
        if (minCol >= cols) minCol = cols - 1;
        if (maxCol < 0) maxCol = 0;
        if (maxCol >= cols) maxCol = cols - 1;
        if (minRow < 0) minRow = 0;
        if (minRow >= rows) minRow = rows - 1;
        if (maxRow < 0) maxRow = 0;
        if (maxRow >= rows) maxRow = rows - 1;

        int count = 0;
        queryId++;
        if (queryId < 0) queryId = 0;

        for (int r = minRow; r <= maxRow; ++r) {
            for (int c = minCol; c <= maxCol; ++c) {
                int idx = r * cols + c;
                for (int i = 0; i < staticCellCounts[idx] && count < maxCount; ++i) {
                    Actor* other = staticCells[idx][i];
                    if (other->queryId != queryId) {
                        other->queryId = queryId;
                        outArray[count++] = other;
                    }
                }
                for (int i = 0; i < dynamicCellCounts[idx] && count < maxCount; ++i) {
                    Actor* other = dynamicCells[idx][i];
                    if (other->queryId != queryId) {
                        other->queryId = queryId;
                        outArray[count++] = other;
                    }
                }
            }
        }
        return count;
    }

    int IRAM_ATTR SpatialGrid::queryBox(const Rect& box, Actor** outArray, int maxCount) {
        int minCol = static_cast<int>(box.position.x) / kCellSize;
        int minRow = static_cast<int>(box.position.y) / kCellSize;
        int maxCol = static_cast<int>(box.position.x + toScalar(box.width)) / kCellSize;
        int maxRow = static_cast<int>(box.position.y + toScalar(box.height)) / kCellSize;
        int cols = pixelroot32::platforms::config::LogicalWidth / kCellSize + 1;
        int rows = pixelroot32::platforms::config::LogicalHeight / kCellSize + 1;
        if (minCol < 0) minCol = 0;
        if (minCol >= cols) minCol = cols - 1;
        if (maxCol < 0) maxCol = 0;
        if (maxCol >= cols) maxCol = cols - 1;
        if (minRow < 0) minRow = 0;
        if (minRow >= rows) minRow = rows - 1;
        if (maxRow < 0) maxRow = 0;
        if (maxRow >= rows) maxRow = rows - 1;

        int count = 0;
        queryId++;
        if (queryId < 0) queryId = 0;

        for (int r = minRow; r <= maxRow; ++r) {
            for (int c = minCol; c <= maxCol; ++c) {
                int idx = r * cols + c;
                for (int i = 0; i < staticCellCounts[idx] && count < maxCount; ++i) {
                    Actor* other = staticCells[idx][i];
                    if (other->queryId != queryId) {
                        other->queryId = queryId;
                        outArray[count++] = other;
                    }
                }
                for (int i = 0; i < dynamicCellCounts[idx] && count < maxCount; ++i) {
                    Actor* other = dynamicCells[idx][i];
                    if (other->queryId != queryId) {
                        other->queryId = queryId;
                        outArray[count++] = other;
                    }
                }
            }
        }
        return count;
    }
#endif

} // namespace pixelroot32::physics
