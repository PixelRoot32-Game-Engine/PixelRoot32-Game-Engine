/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * PartialUpdateController implementation - coordinates dirty region tracking.
 */

#include "graphics/PartialUpdateController.h"

namespace pixelroot32::graphics {

namespace logging = pixelroot32::core::logging;

using logging::log;
using logging::LogLevel;

PartialUpdateController::PartialUpdateController()
    : mode_(Mode::Partial)
    , dirtyPixelCount_(0)
    , lastFrameWidth_(320)
    , lastFrameHeight_(240)
    , minRegionPixels_(MIN_REGION_PIXELS)
    , lastRegionCount_(0)
    , lastTotalSentPixels_(0) {
}

void PartialUpdateController::beginFrame() {
    // Keep previous frame bitmap for delta tracking
    // The tracker retains its state for incremental updates
    // Nothing to do here in v1 - each frame starts fresh
}

void PartialUpdateController::endFrame(int frameWidth, int frameHeight) {
    lastFrameWidth_ = frameWidth;
    lastFrameHeight_ = frameHeight;

    // Combine dirty regions into minimal set
    tracker_.combineRegions();

    // Calculate dirty pixel count
    const auto& regions = tracker_.getRegions();
    dirtyPixelCount_ = 0;
    lastRegionCount_ = static_cast<int>(regions.size());
    lastTotalSentPixels_ = 0;

    for (const auto& r : regions) {
        // Only count regions that meet the minimum threshold
        if (r.area() >= minRegionPixels_) {
            dirtyPixelCount_ += r.area();
            lastTotalSentPixels_ += r.area();
        }
    }

    // Calculate dirty ratio and decide mode
    int totalPixels = frameWidth * frameHeight;
    if (totalPixels > 0) {
        float dirtyRatio = static_cast<float>(dirtyPixelCount_) / totalPixels;

        // Fallback to full if dirty ratio exceeds threshold
        if (dirtyRatio > MAX_DIRTY_RATIO_PERCENT / 100.0f) {
            mode_ = Mode::Full;
        }
    }

    // Benchmark logging (if profiling enabled)
    #ifdef PIXELROOT32_ENABLE_PROFILING
    log(LogLevel::Profiling, "PartialUpdate: regions=%d, pixels=%d, ratio=%.2f%%\n",
        lastRegionCount_, lastTotalSentPixels_,
        (totalPixels > 0 ? (float)dirtyPixelCount_ / totalPixels * 100.0f : 0.0f));
    #endif
}

void PartialUpdateController::markDirty(int x, int y, int w, int h) {
    tracker_.markDirty(x, y, w, h);
}

bool PartialUpdateController::shouldUsePartial() const {
    // Only partial if mode is Partial AND we have dirty regions
    if (mode_ != Mode::Partial) {
        return false;
    }

    if (!tracker_.hasDirtyRegions()) {
        return false;
    }

    // Additional check: regions should have meaningful size
    if (dirtyPixelCount_ < MIN_REGION_PIXELS) {
        return false;
    }

    return true;
}

const std::vector<DirtyRect>& PartialUpdateController::getRegions() const {
    return tracker_.getRegions();
}

void PartialUpdateController::clear() {
    tracker_.clear();
    dirtyPixelCount_ = 0;
    // Reset to partial mode for next frame
    mode_ = Mode::Partial;
}

void PartialUpdateController::setMode(Mode mode) {
    mode_ = mode;
}

PartialUpdateController::Mode PartialUpdateController::getMode() const {
    return mode_;
}

bool PartialUpdateController::isModeFull() const {
    return mode_ == Mode::Full;
}

void PartialUpdateController::setMinRegionPixels(int pixels) {
    // Clamp to valid range: 64-4096 (8x8 to 64x64)
    if (pixels < 64) {
        minRegionPixels_ = 64;
    } else if (pixels > 4096) {
        minRegionPixels_ = 4096;
    } else {
        minRegionPixels_ = pixels;
    }
}

void PartialUpdateController::configure(int spriteWidth, int spriteHeight) {
    tracker_.configure(spriteWidth, spriteHeight);
    lastFrameWidth_ = spriteWidth;
    lastFrameHeight_ = spriteHeight;
}

} // namespace pixelroot32::graphics