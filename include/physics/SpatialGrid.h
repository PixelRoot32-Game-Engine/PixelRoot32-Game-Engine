/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include <cstdint>
#include "math/Scalar.h"
#include "platforms/EngineConfig.h"

namespace pixelroot32::core { class Actor; }

namespace pixelroot32::physics {

/**
 * @class SpatialGrid
 * @brief Optimized spatial partitioning system for 2D actors.
 * 
 * Uses a uniform grid to group nearby actors, reducing the number of collision checks.
 * Optimized for ESP32 memory constraints by using fixed allocations or linked headers.
 */
class SpatialGrid {
public:
    static constexpr int kCellSize = pixelroot32::platforms::config::SpatialGridCellSize; ///< Cell size in pixels.
    static constexpr int kMaxCells = (pixelroot32::platforms::config::LogicalWidth / kCellSize + 1) * 
                                     (pixelroot32::platforms::config::LogicalHeight / kCellSize + 1);

    /**
     * @brief Initializes the grid.
     */
    void clear();

    /**
     * @brief Inserts an actor into the grid based on its hitbox.
     * @param actor The actor to insert.
     */
    void insert(pixelroot32::core::Actor* actor);

    /**
     * @brief Retrieves all unique actors in the cells overlapped by the given actor.
     * @param actor The query actor.
     * @param outPotentialColliders Vector to store results.
     */
    void getPotentialColliders(pixelroot32::core::Actor* actor, pixelroot32::core::Actor** outArray, int& count, int maxCount);

private:
    // Linked-list approach to avoid vectors per cell
    // Every cell points to the first actor in it.
    // Each actor will have a temporary 'nextInCell' pointer (needs to be managed carefully).
    // Note: Since an actor can be in multiple cells, this needs a different approach if we want list-based.
    
    // Simpler approach for ESP32: fixed max entities per cell.
    static constexpr int kMaxEntitiesPerCell = pixelroot32::platforms::config::SpatialGridMaxEntitiesPerCell; 
    static pixelroot32::core::Actor* cells[kMaxCells][kMaxEntitiesPerCell];
    static int cellCounts[kMaxCells];

    int getCellIndex(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y) const;
};

} // namespace pixelroot32::physics
