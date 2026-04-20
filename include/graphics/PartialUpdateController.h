/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * PartialUpdateController: Coordinates dirty region tracking and partial screen updates.
 * Manages mode selection between full frame and partial updates based on dirty ratio.
 */

#pragma once

#include "graphics/DirtyRectTracker.h"
#include "platforms/EngineConfig.h"

namespace pixelroot32::graphics {

/**
 * @class PartialUpdateController
 * @brief Coordinates dirty rect tracking and decides update strategy.
 *
 * Tracks the dirty ratio and automatically falls back to full frame updates
 * when partial updates would not provide benefit (>70% dirty threshold).
 */
class PartialUpdateController {
public:
    /**
     * @enum Mode
     * @brief Update mode selection
     */
    enum class Mode {
        Full,       ///< Send complete frame (original behavior)
        Partial     ///< Send only dirty regions (optimized)
    };

    // Thresholds
    static constexpr int MIN_REGION_PIXELS = 16 * 16;  // 256 pixels minimum

private:
    Mode mode_ = Mode::Partial;
    DirtyRectTracker tracker_;

    // Statistics
    int dirtyPixelCount_ = 0;
    int lastFrameWidth_ = 320;
    int lastFrameHeight_ = 240;

    // Benchmark configuration
    int minRegionPixels_ = MIN_REGION_PIXELS;  // Configurable threshold (64-4096)

    // Benchmark data (available after endFrame())
    int lastRegionCount_ = 0;
    int lastTotalSentPixels_ = 0;

public:
    PartialUpdateController();

    /**
     * @brief Called at start of frame to prepare tracking.
     */
    void beginFrame();

    /**
     * @brief Called after all game drawing completes, before sendBuffer.
     * @param frameWidth Current frame width
     * @param frameHeight Current frame height
     */
    void endFrame(int frameWidth, int frameHeight);

    /**
     * @brief Mark region as dirty (delegates to tracker).
     * @param x X coordinate
     * @param y Y coordinate
     * @param w Width
     * @param h Height
     */
    void markDirty(int x, int y, int w, int h);

    /**
     * @brief Determine if partial update should be used.
     * @return true if partial mode is beneficial
     */
    bool shouldUsePartial() const;

    /**
     * @brief Get merged dirty regions.
     * @return Reference to vector of dirty regions
     */
    const std::vector<DirtyRect>& getRegions() const;

    /**
     * @brief Clear for next frame.
     */
    void clear();

    /**
     * @brief Configure tracker for actual sprite resolution.
     *
     * Must be called during init when the logical display dimensions are known.
     * Without this call, defaults to 320x240 (backward compatible).
     *
     * @param spriteWidth Sprite buffer width in pixels
     * @param spriteHeight Sprite buffer height in pixels
     */
    void configure(int spriteWidth, int spriteHeight);

    /**
     * @brief Set the update mode.
     * @param mode New mode (Full or Partial)
     */
    void setMode(Mode mode);

    /**
     * @brief Get current mode.
     * @return Current mode
     */
    Mode getMode() const;

    /**
     * @brief Check if in full frame mode.
     * @return true if mode is Full
     */
    bool isModeFull() const;

    /**
     * @brief Enable or disable partial updates.
     * @param enabled true to enable partial updates
     */
    void setPartialUpdateEnabled(bool enabled) {
        setMode(enabled ? Mode::Partial : Mode::Full);
    }

    /**
     * @brief Check if partial updates are enabled.
     * @return true if partial mode is enabled
     */
    bool isPartialUpdateEnabled() const {
        return mode_ == Mode::Partial;
    }

    /**
     * @brief Check if any dirty regions exist.
     * @return true if tracker has dirty regions
     */
    bool hasDirtyRegions() const {
        return tracker_.hasDirtyRegions();
    }

    /**
     * @brief Get the number of dirty pixels in last frame.
     * @return Dirty pixel count
     */
    int getDirtyPixelCount() const { return dirtyPixelCount_; }

    /**
     * @brief Get last frame dimensions.
     * @return Frame width
     */
    int getLastFrameWidth() const { return lastFrameWidth_; }

    /**
     * @brief Get last frame dimensions.
     * @return Frame height
     */
    int getLastFrameHeight() const { return lastFrameHeight_; }

    /**
     * @brief Enable or disable region combining.
     * @param enabled true to enable combining
     */
    void setCombineEnabled(bool enabled) {
        tracker_.setCombineEnabled(enabled);
    }

    // ============================================================================
    // Benchmark API
    // ============================================================================

    /**
     * @brief Set minimum region pixel threshold for partial updates.
     *
     * Only regions with at least this many pixels will be sent as partial updates.
     * Lower values = more regions = more granular but potentially slower.
     * Higher values = fewer regions = less granular but faster.
     *
     * Valid range: 64 to 4096 pixels (16x16 to 64x64)
     * Default: 256 (16x16)
     *
     * @param pixels Minimum pixel count for a valid region
     */
    void setMinRegionPixels(int pixels);

    /**
     * @brief Get current minimum region pixel threshold.
     * @return Current threshold value
     */
    int getMinRegionPixels() const { return minRegionPixels_; }

    /**
     * @brief Get number of regions sent in last frame.
     * @return Region count
     */
    int getLastRegionCount() const { return lastRegionCount_; }

    /**
     * @brief Get total pixels sent in last frame.
     * @return Total sent pixels
     */
    int getLastTotalSentPixels() const { return lastTotalSentPixels_; }
};

} // namespace pixelroot32::graphics