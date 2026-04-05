/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include <cstdint>
#include "math/Scalar.h"
#include "platforms/EngineConfig.h"

namespace pixelroot32::core { class Actor; class Entity; }

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

    void clearDynamic();
    void clear();
    void markStaticDirty();
    void rebuildStaticIfNeeded(pixelroot32::core::Entity* const* entities, uint16_t entityCount);
    void insertDynamic(pixelroot32::core::Actor* actor);
    void getPotentialColliders(pixelroot32::core::Actor* actor, pixelroot32::core::Actor** outArray, int& count, int maxCount);

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
