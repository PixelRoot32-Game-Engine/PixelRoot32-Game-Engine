/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * DirtyRectTracker: Bitmap-based dirty region tracking for partial screen updates.
 * Uses 8x8 block granularity (40x30 grid for 320x240 resolution) to minimize memory.
 *
 * Thread Safety:
 * =============
 * This class is NOT thread-safe by default. It is designed for single-threaded game loops
 * where all rendering and tracking occurs on the same execution context.
 *
 * Architecture Assumptions:
 * - The game loop runs on a single core (typical for ESP32 game applications)
 * - Rendering callbacks (drawTile, etc.) are invoked sequentially, not in parallel
 * - No other tasks access the DirtyRectTracker concurrently during a frame
 *
 * ESP32 Dual-Core Considerations:
 * - ESP32 has two cores: Core 0 (WiFi/BT stack) and Core 1 (application/game loop)
 * - Game logic and rendering typically run on Core 1 exclusively
 * - If WiFi/BT callbacks need to trigger rendering, use queue-based communication
 *   to ensure all DirtyRectTracker access happens on Core 1
 *
 * Future Parallel Rendering:
 * - If parallel rendering is needed (e.g., split screen), use portENTER_CRITICAL()
 *   around bitmap operations to make them atomic
 * - Alternatively, use a mutex wrapper for all public methods
 * - The bitmap operations are fast enough that critical section overhead is minimal
 *
 * Example for future atomic operations:
 *   portENTER_CRITICAL();
 *   setBit(blockX, blockY);
 *   portEXIT_CRITICAL();
 */

#pragma once

#include <cstdint>
#include <vector>
#include <cstring>

namespace pixelroot32::graphics {

/**
 * @struct DirtyRect
 * @brief Represents a dirty region that needs to be redrawn.
 */
struct DirtyRect {
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;

    DirtyRect() : x(0), y(0), width(0), height(0) {}
    DirtyRect(int16_t x_, int16_t y_, uint16_t w_, uint16_t h_)
        : x(x_), y(y_), width(w_), height(h_) {}

    /**
     * @brief Calculate the number of pixels in this region.
     */
    uint32_t area() const { return static_cast<uint32_t>(width) * height; }
};

/**
 * @class DirtyRectTracker
 * @brief Tracks which regions of the screen have been modified since last frame.
 *
 * Uses a bitmap-based approach for O(1) marking performance, then converts
 * to merged regions for efficient partial updates.
 *
 * Grid: 40x30 blocks at 8x8 pixels each = 320x240 total resolution
 */
class DirtyRectTracker {
public:
    static constexpr int BLOCK_SIZE = 8;

    // Default dimensions (backward compatible with 320x240)
    static constexpr int DEFAULT_SPRITE_WIDTH = 320;
    static constexpr int DEFAULT_SPRITE_HEIGHT = 240;

private:
    // Resolution-dependent dimensions (configured via constructor or configure())
    int spriteWidth_;
    int spriteHeight_;
    int gridWidth_;
    int gridHeight_;
    int bitmapSize_;

    // Bitmap: 1 bit per 8x8 block (heap-allocated for flexible sizing)
    uint8_t* dirtyBitmap_;

    // Merged rectangles (computed on demand after combineRegions())
    std::vector<DirtyRect> mergedRegions_;

    // State tracking
    bool hasDirty_ = false;
    bool combineEnabled_ = true;

    // Processed blocks tracking (heap-allocated for stack optimization)
    bool* processed_;

    // Previous frame's dirty bitmap for 2-frame carry-over (ghost pixel prevention)
    uint8_t* prevDirtyBitmap_;

public:
    DirtyRectTracker();
    ~DirtyRectTracker();

    // Non-copyable (manages heap memory)
    DirtyRectTracker(const DirtyRectTracker&) = delete;
    DirtyRectTracker& operator=(const DirtyRectTracker&) = delete;

    /**
     * @brief Reconfigure tracker for a different sprite resolution.
     *
     * Reallocates internal structures to match the new dimensions.
     * Clears all existing dirty state. Safe to call multiple times.
     *
     * @param spriteWidth Sprite buffer width in pixels
     * @param spriteHeight Sprite buffer height in pixels
     */
    void configure(int spriteWidth, int spriteHeight);

    // Dimension accessors
    int getGridWidth() const { return gridWidth_; }
    int getGridHeight() const { return gridHeight_; }
    int getSpriteWidth() const { return spriteWidth_; }
    int getSpriteHeight() const { return spriteHeight_; }

    /**
     * @brief Mark a region as dirty (O(1) operation).
     * @param x X coordinate in sprite pixels
     * @param y Y coordinate in sprite pixels
     * @param w Width in pixels
     * @param h Height in pixels
     */
    void markDirty(int x, int y, int w, int h);

    /**
     * @brief Combine adjacent/overlapping dirty blocks into merged regions.
     *
     * Must be called after all markDirty() calls and before getRegions().
     */
    void combineRegions();

    /**
     * @brief Check if any regions are dirty.
     * @return true if there are dirty regions
     */
    bool hasDirtyRegions() const { return hasDirty_; }

    /**
     * @brief Count total dirty pixels without running combineRegions().
     *
     * Fast O(bitmapSize) operation: counts set bits in the dirty bitmap
     * and multiplies by BLOCK_SIZE^2. Used for early threshold detection
     * to avoid the more expensive combineRegions() when the dirty ratio
     * already exceeds the fallback threshold.
     *
     * @return Approximate pixel count (rounded up to 8x8 block boundaries)
     */
    int countDirtyPixels() const;

    /**
     * @brief Get the merged dirty regions.
     * @return Vector of DirtyRect regions (valid after combineRegions())
     */
    const std::vector<DirtyRect>& getRegions() const { return mergedRegions_; }

    /**
     * @brief Clear all dirty tracking state for next frame.
     */
    void clear();

    /**
     * @brief Merge previous frame's dirty regions into current frame.
     *
     * Must be called ONCE per frame, BEFORE countDirtyPixels()/combineRegions().
     * This implements 2-frame dirty carry-over: any region that was dirty in
     * the previous frame is also dirty this frame, ensuring "now-black" areas
     * (moved entities, dead particles, scrolled tiles) get sent to the display.
     *
     * The carry-over naturally decays: regions persist for exactly 2 frames.
     */
    void mergeWithPreviousFrame();

    /**
     * @brief Enable or disable region combining for performance tuning.
     * @param enabled true to enable combining, false for per-block regions
     */
    void setCombineEnabled(bool enabled) { combineEnabled_ = enabled; }

    /**
     * @brief Check if combining is enabled.
     * @return true if combining is enabled
     */
    bool isCombineEnabled() const { return combineEnabled_; }

private:
    /**
     * @brief Internal helper to set a bit in the bitmap.
     */
    inline void setBit(int blockX, int blockY);

    /**
     * @brief Internal helper to check if a bit is set.
     */
    inline bool getBit(int blockX, int blockY) const;

    /**
     * @brief Compute merged regions from bitmap.
     */
    void computeMergedRegions();
};

} // namespace pixelroot32::graphics