/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * DirtyRectTracker implementation - bitmap-based dirty region tracking.
 */

#include "graphics/DirtyRectTracker.h"

namespace pixelroot32::graphics {

DirtyRectTracker::DirtyRectTracker()
    : spriteWidth_(DEFAULT_SPRITE_WIDTH)
    , spriteHeight_(DEFAULT_SPRITE_HEIGHT)
    , gridWidth_((DEFAULT_SPRITE_WIDTH + BLOCK_SIZE - 1) / BLOCK_SIZE)
    , gridHeight_((DEFAULT_SPRITE_HEIGHT + BLOCK_SIZE - 1) / BLOCK_SIZE)
    , bitmapSize_(0)
    , dirtyBitmap_(nullptr)
    , hasDirty_(false)
    , combineEnabled_(true)
    , processed_(nullptr)
{
    bitmapSize_ = (gridWidth_ * gridHeight_ + 7) / 8;
    dirtyBitmap_ = new uint8_t[bitmapSize_]();
    prevDirtyBitmap_ = new uint8_t[bitmapSize_]();
    processed_ = new bool[gridWidth_ * gridHeight_]();

    // Pre-allocate region storage to avoid heap churn during gameplay.
    // After reserve(), clear() keeps capacity — push_back never re-allocates.
    mergedRegions_.reserve(64);
}

DirtyRectTracker::~DirtyRectTracker() {
    // Clean up heap-allocated arrays
    delete[] processed_;
    processed_ = nullptr;
    delete[] dirtyBitmap_;
    dirtyBitmap_ = nullptr;
    delete[] prevDirtyBitmap_;
    prevDirtyBitmap_ = nullptr;
}

void DirtyRectTracker::configure(int spriteWidth, int spriteHeight) {
    if (spriteWidth <= 0 || spriteHeight <= 0) return;
    if (spriteWidth == spriteWidth_ && spriteHeight == spriteHeight_) return;

    spriteWidth_ = spriteWidth;
    spriteHeight_ = spriteHeight;
    gridWidth_ = (spriteWidth + BLOCK_SIZE - 1) / BLOCK_SIZE;
    gridHeight_ = (spriteHeight + BLOCK_SIZE - 1) / BLOCK_SIZE;
    bitmapSize_ = (gridWidth_ * gridHeight_ + 7) / 8;

    // Reallocate memory for new dimensions
    delete[] dirtyBitmap_;
    delete[] prevDirtyBitmap_;
    delete[] processed_;
    dirtyBitmap_ = new uint8_t[bitmapSize_]();
    prevDirtyBitmap_ = new uint8_t[bitmapSize_]();
    processed_ = new bool[gridWidth_ * gridHeight_]();

    hasDirty_ = false;
    mergedRegions_.clear();
}

inline void DirtyRectTracker::setBit(int blockX, int blockY) {
    if (blockX < 0 || blockX >= gridWidth_ || blockY < 0 || blockY >= gridHeight_) {
        return;
    }
    int index = blockY * gridWidth_ + blockX;
    dirtyBitmap_[index / 8] |= (1 << (index % 8));
}

inline bool DirtyRectTracker::getBit(int blockX, int blockY) const {
    if (blockX < 0 || blockX >= gridWidth_ || blockY < 0 || blockY >= gridHeight_) {
        return false;
    }
    int index = blockY * gridWidth_ + blockX;
    return (dirtyBitmap_[index / 8] & (1 << (index % 8))) != 0;
}

void DirtyRectTracker::markDirty(int x, int y, int w, int h) {
    // Clip to bounds
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (w <= 0 || h <= 0) return;
    if (x >= spriteWidth_ || y >= spriteHeight_) return;

    // Clamp to sprite bounds
    w = std::min(w, spriteWidth_ - x);
    h = std::min(h, spriteHeight_ - y);
    if (w <= 0 || h <= 0) return;

    // Convert to block coordinates
    int startBlockX = x / BLOCK_SIZE;
    int startBlockY = y / BLOCK_SIZE;
    int endBlockX = (x + w + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int endBlockY = (y + h + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // Clamp to grid bounds
    endBlockX = std::min(endBlockX, gridWidth_);
    endBlockY = std::min(endBlockY, gridHeight_);

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
        for (int by = 0; by < gridHeight_; ++by) {
            for (int bx = 0; bx < gridWidth_; ++bx) {
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
    std::memset(processed_, 0, gridWidth_ * gridHeight_ * sizeof(bool));

    // Scan in row-major order
    for (int by = 0; by < gridHeight_; ++by) {
        for (int bx = 0; bx < gridWidth_; ++bx) {
            int idx = by * gridWidth_ + bx;

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
            while (expandX < gridWidth_ && getBit(expandX, by) && !processed_[by * gridWidth_ + expandX]) {
                rw += BLOCK_SIZE;
                processed_[by * gridWidth_ + expandX] = true;
                ++expandX;
            }

            // Now expand down - check each row for full horizontal coverage
            while (by + rh / BLOCK_SIZE < gridHeight_) {
                int nextRow = by + rh / BLOCK_SIZE;
                bool rowComplete = true;
                int colsInRegion = rw / BLOCK_SIZE;

                // Check if all columns in current region width are dirty in next row
                for (int dx = 0; dx < colsInRegion; ++dx) {
                    int cx = bx + dx;
                    if (cx >= gridWidth_ || !getBit(cx, nextRow) || processed_[nextRow * gridWidth_ + cx]) {
                        rowComplete = false;
                        break;
                    }
                }

                if (!rowComplete) break;

                // Extend region downward
                rh += BLOCK_SIZE;
                for (int dx = 0; dx < colsInRegion; ++dx) {
                    processed_[nextRow * gridWidth_ + (bx + dx)] = true;
                }
            }

            mergedRegions_.push_back(DirtyRect(rx, ry, rw, rh));
        }
    }
}

void DirtyRectTracker::mergeWithPreviousFrame() {
    if (!dirtyBitmap_ || !prevDirtyBitmap_) return;

    // 2-Frame Carry-over:
    // A := current frame explicit dirties
    // B := previous frame explicit dirties
    // We want to draw A | B, and save A to be the next frame's B.
    for (int i = 0; i < bitmapSize_; ++i) {
        uint8_t current = dirtyBitmap_[i];
        dirtyBitmap_[i] |= prevDirtyBitmap_[i];
        prevDirtyBitmap_[i] = current;
    }
}

void DirtyRectTracker::clear() {
    // Zero the bitmap. The previous frame's dirties are safely stored in
    // prevDirtyBitmap_ (thanks to mergeWithPreviousFrame swapping them)
    std::memset(dirtyBitmap_, 0, bitmapSize_);
    mergedRegions_.clear();
    hasDirty_ = false;
}

int DirtyRectTracker::countDirtyPixels() const {
    if (!hasDirty_ || !dirtyBitmap_) {
        return 0;
    }

    // Count set bits in the bitmap (each bit = one 8x8 block = 64 pixels)
    // Use byte-level popcount for broad compiler compatibility.
    // __builtin_popcount is available on GCC/Clang (ESP32 toolchain included).
    int blockCount = 0;
    for (int i = 0; i < bitmapSize_; ++i) {
        blockCount += __builtin_popcount(dirtyBitmap_[i]);
    }

    return blockCount * (BLOCK_SIZE * BLOCK_SIZE);
}

} // namespace pixelroot32::graphics