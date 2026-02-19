/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "physics/SpatialGrid.h"
#include "core/Actor.h"
#include "math/MathUtil.h"

namespace pixelroot32::physics {

using namespace pixelroot32::core;
using namespace pixelroot32::math;

void SpatialGrid::clear() {
    for (int i = 0; i < kMaxCells; ++i) {
        cellCounts[i] = 0;
    }
}

int SpatialGrid::getCellIndex(Scalar x, Scalar y) const {
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

void SpatialGrid::insert(Actor* actor) {
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
            if (cellCounts[idx] < kMaxEntitiesPerCell) {
                cells[idx][cellCounts[idx]++] = actor;
            }
        }
    }
}

void SpatialGrid::getPotentialColliders(Actor* actor, Actor** outArray, int& count, int maxCount) {
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
    static int s_queryId = 0;
    s_queryId++;
    if (s_queryId < 0) s_queryId = 0;

    for (int r = minRow; r <= maxRow; ++r) {
        for (int c = minCol; c <= maxCol; ++c) {
            int idx = r * cols + c;
            for (int i = 0; i < cellCounts[idx]; ++i) {
                Actor* other = cells[idx][i];
                if (other == actor) continue;

                if (other->queryId != s_queryId && count < maxCount) {
                    other->queryId = s_queryId;
                    outArray[count++] = other;
                }
            }
        }
    }
}

} // namespace pixelroot32::physics
