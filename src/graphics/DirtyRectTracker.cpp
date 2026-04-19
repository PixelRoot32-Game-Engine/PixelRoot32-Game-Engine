/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * DirtyRectTracker implementation - bitmap-based dirty region tracking.
 */

#include "graphics/DirtyRectTracker.h"

namespace pixelroot32::graphics {

DirtyRectTracker::DirtyRectTracker() {
    // Initialize bitmap to zero
    std::memset(dirtyBitmap_, 0, sizeof(dirtyBitmap_));

    // Allocate processed_ array on heap (GRID_WIDTH * GRID_HEIGHT = 1200 bools)
    processed_ = new bool[GRID_WIDTH * GRID_HEIGHT]();

    hasDirty_ = false;
    combineEnabled_ = true;
}

DirtyRectTracker::~DirtyRectTracker() {
    // Clean up heap-allocated processed_ array
    delete[] processed_;
    processed_ = nullptr;
}

inline void DirtyRectTracker::setBit(int blockX, int blockY) {
    if (blockX < 0 || blockX >= GRID_WIDTH || blockY < 0 || blockY >= GRID_HEIGHT) {
        return;
    }
    int index = blockY * GRID_WIDTH + blockX;
    dirtyBitmap_[index / 8] |= (1 << (index % 8));
}

inline bool DirtyRectTracker::getBit(int blockX, int blockY) const {
    if (blockX < 0 || blockX >= GRID_WIDTH || blockY < 0 || blockY >= GRID_HEIGHT) {
        return false;
    }
    int index = blockY * GRID_WIDTH + blockX;
    return (dirtyBitmap_[index / 8] & (1 << (index % 8))) != 0;
}

void DirtyRectTracker::markDirty(int x, int y, int w, int h) {
    // Clip to bounds
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (w <= 0 || h <= 0) return;
    if (x >= SPRITE_WIDTH || y >= SPRITE_HEIGHT) return;

    // Clamp to sprite bounds
    w = std::min(w, SPRITE_WIDTH - x);
    h = std::min(h, SPRITE_HEIGHT - y);
    if (w <= 0 || h <= 0) return;

    // Convert to block coordinates
    int startBlockX = x / BLOCK_SIZE;
    int startBlockY = y / BLOCK_SIZE;
    int endBlockX = (x + w + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int endBlockY = (y + h + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // Clamp to grid bounds
    endBlockX = std::min(endBlockX, GRID_WIDTH);
    endBlockY = std::min(endBlockY, GRID_HEIGHT);

    // Mark all covered blocks as dirty (O(blocks) operation)
    for (int by = startBlockY; by < endBlockY; ++by) {
        for (int bx = startBlockX; bx < endBlockX; ++bx) {
            setBit(bx, by);
        }
    }
    hasDirty_ = true;
}

void DirtyRectTracker::combineRegions() {
    // Clear previous merged regions
    mergedRegions_.clear();

    if (!hasDirty_) {
        return;
    }

    // Use combining algorithm or per-block mode
    if (combineEnabled_) {
        computeMergedRegions();
    } else {
        // Fast path: just iterate bitmap, each dirty block is a separate region
        for (int by = 0; by < GRID_HEIGHT; ++by) {
            for (int bx = 0; bx < GRID_WIDTH; ++bx) {
                if (getBit(bx, by)) {
                    mergedRegions_.push_back(DirtyRect(
                        static_cast<int16_t>(bx * BLOCK_SIZE),
                        static_cast<int16_t>(by * BLOCK_SIZE),
                        BLOCK_SIZE,
                        BLOCK_SIZE
                    ));
                }
            }
        }
    }
}

void DirtyRectTracker::computeMergedRegions() {
    // Reset processed_ tracking array
    std::memset(processed_, 0, GRID_WIDTH * GRID_HEIGHT * sizeof(bool));

    // Scan in row-major order
    for (int by = 0; by < GRID_HEIGHT; ++by) {
        for (int bx = 0; bx < GRID_WIDTH; ++bx) {
            int idx = by * GRID_WIDTH + bx;

            // Skip if already processed or not dirty
            if (processed_[idx] || !getBit(bx, by)) {
                continue;
            }

            // Found start of a new region - expand horizontally first
            int16_t rx = static_cast<int16_t>(bx * BLOCK_SIZE);
            int16_t ry = static_cast<int16_t>(by * BLOCK_SIZE);
            uint16_t rw = BLOCK_SIZE;
            uint16_t rh = BLOCK_SIZE;

            // Mark initial block as processed BEFORE expanding
            processed_[idx] = true;

            // Expand right while adjacent blocks are dirty
            int expandX = bx + 1;
            while (expandX < GRID_WIDTH && getBit(expandX, by) && !processed_[by * GRID_WIDTH + expandX]) {
                rw += BLOCK_SIZE;
                processed_[by * GRID_WIDTH + expandX] = true;
                ++expandX;
            }

            // Now expand down - check each row for full horizontal coverage
            while (by + rh / BLOCK_SIZE < GRID_HEIGHT) {
                int nextRow = by + rh / BLOCK_SIZE;
                bool rowComplete = true;
                int colsInRegion = rw / BLOCK_SIZE;

                // Check if all columns in current region width are dirty in next row
                for (int dx = 0; dx < colsInRegion; ++dx) {
                    int cx = bx + dx;
                    if (cx >= GRID_WIDTH || !getBit(cx, nextRow) || processed_[nextRow * GRID_WIDTH + cx]) {
                        rowComplete = false;
                        break;
                    }
                }

                if (!rowComplete) break;

                // Extend region downward
                rh += BLOCK_SIZE;
                for (int dx = 0; dx < colsInRegion; ++dx) {
                    processed_[nextRow * GRID_WIDTH + (bx + dx)] = true;
                }
            }

            mergedRegions_.push_back(DirtyRect(rx, ry, rw, rh));
        }
    }
}

void DirtyRectTracker::clear() {
    // Zero the bitmap
    std::memset(dirtyBitmap_, 0, sizeof(dirtyBitmap_));
    mergedRegions_.clear();
    hasDirty_ = false;
}

} // namespace pixelroot32::graphics