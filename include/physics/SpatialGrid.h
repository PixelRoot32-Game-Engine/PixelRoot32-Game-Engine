/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include <cstdint>
#include "math/Scalar.h"
#include "math/Vector2.h"
#include "platforms/EngineConfig.h"

namespace pixelroot32::core { class Actor; class Entity; struct Rect; }

namespace pixelroot32::physics {

/**
 * @class SpatialGrid
 * @brief Optimized spatial partitioning with separate static/dynamic layers.
 *
 * Static layer: built once per level (or when entities change), not cleared each frame.
 * Dynamic layer: cleared and refilled every frame (RIGID, KINEMATIC).
 * Reduces per-frame cost when many static tiles are present.
 */
class SpatialGrid {
public:
    static constexpr int kCellSize = pixelroot32::platforms::config::SpatialGridCellSize;
    static constexpr int kMaxCells = (pixelroot32::platforms::config::LogicalWidth / kCellSize + 1) *
                                     (pixelroot32::platforms::config::LogicalHeight / kCellSize + 1);
    static constexpr int kMaxStaticPerCell = pixelroot32::platforms::config::SpatialGridMaxStaticPerCell;
    static constexpr int kMaxDynamicPerCell = pixelroot32::platforms::config::SpatialGridMaxDynamicPerCell;

    /**
     * @brief Clears all dynamic entities from the grid.
     */
    void clearDynamic();

    /**
     * @brief Clears all entities (static and dynamic) from the grid.
     */
    void clear();

    /**
     * @brief Marks the static layer as dirty, requiring a rebuild.
     */
    void markStaticDirty();

    /**
     * @brief Rebuilds the static layer if marked dirty.
     * @param entities Pointer to array of entities.
     * @param entityCount Total number of entities.
     */
    void rebuildStaticIfNeeded(pixelroot32::core::Entity* const* entities, uint16_t entityCount);

    /**
     * @brief Inserts a dynamic actor into the grid.
     * @param actor The actor to insert.
     */
    void insertDynamic(pixelroot32::core::Actor* actor);

    /**
     * @brief Gets potential colliders for a given actor from the grid.
     * @param actor The actor to query for.
     * @param outArray Output array to store potential colliders.
     * @param count Reference to store the number of colliders found.
     * @param maxCount Maximum number of colliders to return.
     */
    void getPotentialColliders(pixelroot32::core::Actor* actor, pixelroot32::core::Actor** outArray, int& count, int maxCount);

#if PIXELROOT32_ENABLE_SPATIAL_QUERY
    /**
     * @brief Raw, unfiltered radius query against the grid (static + dynamic cells).
     *
     * Sweeps the same cell range (derived from the circle's bounding box) and
     * reuses the same `queryId` dedup pass as getPotentialColliders(), so an
     * actor spanning several cells is reported once. Unlike
     * getPotentialColliders(), this has no anchor actor to exclude — every
     * actor found in the overlapped cells is a candidate. This is a coarse,
     * cell-level result: it is NOT itself the circle-vs-hitbox test. Callers
     * needing layer filtering and the exact narrow-phase test should use
     * CollisionSystem::queryRadius(), which wraps this primitive.
     *
     * @warning Cross-scene visibility limitation (accepted, not fixed by
     * this capability): `staticCells`/`dynamicCells` are `static` member
     * storage shared across ALL `SpatialGrid` instances (see class doc and
     * design.md Risks). A query issued against one scene's grid MAY return
     * static geometry registered by another simultaneously-alive scene's
     * grid. Pre-existing, documented, unguarded.
     *
     * @param center Query circle center.
     * @param radius Query circle radius. No range validation happens at
     *        this layer; CollisionSystem::queryRadius() enforces
     *        SPATIAL_QUERY_MAX_RADIUS before calling this primitive.
     * @param outArray Output array to store matching actors.
     * @param maxCount Maximum number of actors to write to outArray.
     * @return Number of actors written to outArray.
     */
    int queryRadius(pixelroot32::math::Vector2 center, pixelroot32::math::Scalar radius,
                     pixelroot32::core::Actor** outArray, int maxCount);

    /**
     * @brief Raw, unfiltered box query against the grid (static + dynamic cells).
     *
     * Sweeps the cell range covered by @p box using the same cell-range
     * clamping math and `queryId` dedup pass as getPotentialColliders() and
     * queryRadius(). See queryRadius() for the cross-scene visibility
     * limitation, which applies identically here.
     *
     * @param box Query rectangle.
     * @param outArray Output array to store matching actors.
     * @param maxCount Maximum number of actors to write to outArray.
     * @return Number of actors written to outArray.
     */
    int queryBox(const pixelroot32::core::Rect& box,
                 pixelroot32::core::Actor** outArray, int maxCount);
#endif

private:
    static pixelroot32::core::Actor* staticCells[kMaxCells][kMaxStaticPerCell];
    static int staticCellCounts[kMaxCells];
    static pixelroot32::core::Actor* dynamicCells[kMaxCells][kMaxDynamicPerCell];
    static int dynamicCellCounts[kMaxCells];

    bool staticDirty = true;
    int queryId = 0;

    int getCellIndex(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y) const;
};

} // namespace pixelroot32::physics
