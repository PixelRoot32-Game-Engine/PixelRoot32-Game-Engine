/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file TransitionEffect.cpp
 * @brief Implementation of TransitionEffect — Fade (palette LUT) and Iris (circle wipe).
 *
 * All buffer operations use direct 8bpp pixel access. Fade pre-computes a
 * 256-byte look-up table from the current progress and direction, then maps
 * every byte in the buffer through it. Iris uses (x-cx)²+(y-cy)² > r² with
 * integer math — no sqrt, no float operations on non-FPU targets.
 *
 * Both effects guard against nullptr buffer via an early return in apply().
 */

#include "graphics/TransitionEffect.h"

namespace pixelroot32::graphics {

#if PIXELROOT32_ENABLE_SCENE_TRANSITIONS

// =============================================================================
// init() — configure effect parameters
// =============================================================================

void TransitionEffect::init(TransitionType type, TransitionDirection direction,
                            unsigned long durationMs) {
    type_ = type;
    direction_ = direction;
    durationMs_ = durationMs;
    elapsedMs_ = 0;
    irisCx_ = -1;   // Reset custom iris center to default.
    irisCy_ = -1;
}

// =============================================================================
// update() — advance elapsed time
// =============================================================================

void TransitionEffect::update(unsigned long deltaTimeMs) {
    if (!isActive()) return;

    elapsedMs_ += deltaTimeMs;
    if (elapsedMs_ > durationMs_) {
        elapsedMs_ = durationMs_;
    }
}

// =============================================================================
// getProgress() — normalised progress [0.0, 1.0]
// =============================================================================

float TransitionEffect::getProgress() const {
    if (durationMs_ == 0) return 1.0f;
    return static_cast<float>(elapsedMs_) / static_cast<float>(durationMs_);
}

// =============================================================================
// apply() — dispatch to the active effect
// =============================================================================

void TransitionEffect::apply(uint8_t* buffer, int width, int height) {
    // Null buffer guard — safe no-op.
    if (buffer == nullptr) return;
    // Not initialised — safe no-op (avoids division by zero in progress calc).
    if (durationMs_ == 0) return;

    switch (type_) {
        case TransitionType::Fade:
            applyFade(buffer, width, height);
            break;

        case TransitionType::Iris:
            applyIris(buffer, width, height);
            break;
    }
}

// =============================================================================
// computeFadeLut() — fill the 256-byte palette LUT
// =============================================================================

void TransitionEffect::computeFadeLut(uint8_t* lut, uint16_t scaledProgress) const {
    if (direction_ == TransitionDirection::Out) {
        // Fade Out:  LUT[i] = i * (256 - p) / 256
        // Progress=0 → LUT[i] = i   (full brightness)
        // Progress=1 → LUT[i] = 0   (black)
        for (int i = 0; i < 256; ++i) {
            lut[i] = static_cast<uint8_t>((i * (256 - scaledProgress)) >> 8);
        }
    } else {
        // Fade In:   LUT[i] = i * p / 256
        // Progress=0 → LUT[i] = 0   (black)
        // Progress=1 → LUT[i] = i   (full brightness)
        for (int i = 0; i < 256; ++i) {
            lut[i] = static_cast<uint8_t>((i * scaledProgress) >> 8);
        }
    }
}

// =============================================================================
// applyFade() — map every pixel through the fade LUT
// =============================================================================

void TransitionEffect::applyFade(uint8_t* buffer, int width, int height) {
    // Compute progress as scaled 0..256 (Q8.8 unsigned).
    unsigned long elapsed = elapsedMs_;
    unsigned long duration = durationMs_;
    uint16_t scaledProgress = 0;
    if (duration > 0) {
        scaledProgress = static_cast<uint16_t>((elapsed * 256) / duration);
        if (scaledProgress > 256) scaledProgress = 256;
    }

    // Pre-compute LUT for this frame's progress.
    uint8_t lut[256];
    computeFadeLut(lut, scaledProgress);

    // Map every pixel in the buffer through the LUT.
    int totalPixels = width * height;
    for (int i = 0; i < totalPixels; ++i) {
        buffer[i] = lut[buffer[i]];
    }
}

// =============================================================================
// applyIris() — circular wipe via distance² test
// =============================================================================

void TransitionEffect::applyIris(uint8_t* buffer, int width, int height) {
    // Compute progress as scaled 0..256 (Q8.8 unsigned).
    unsigned long elapsed = elapsedMs_;
    unsigned long duration = durationMs_;
    uint16_t scaledProgress = 0;
    if (duration > 0) {
        scaledProgress = static_cast<uint16_t>((elapsed * 256) / duration);
        if (scaledProgress > 256) scaledProgress = 256;
    }

    // Determine iris center.
    int cx = (irisCx_ >= 0) ? irisCx_ : (width / 2);
    int cy = (irisCy_ >= 0) ? irisCy_ : (height / 2);

    // Compute maximum radius² (distance from center to farthest corner).
    int dxMax = (cx >= width - 1 - cx) ? cx : (width - 1 - cx);
    int dyMax = (cy >= height - 1 - cy) ? cy : (height - 1 - cy);
    int maxRadius2 = dxMax * dxMax + dyMax * dyMax;

    // Current radius² at this progress.
    int r2;
    if (direction_ == TransitionDirection::Out) {
        // Iris Out:  shrink from full to empty.
        // Progress=0 → r2 = maxRadius2  (full image visible).
        // Progress=1 → r2 = 0           (nothing visible).
        r2 = static_cast<int>((maxRadius2 * (256 - scaledProgress)) >> 8);
    } else {
        // Iris In:   expand from center.
        // Progress=0 → r2 = 0           (only center visible).
        // Progress=1 → r2 = maxRadius2  (full image visible).
        r2 = static_cast<int>((maxRadius2 * scaledProgress) >> 8);
    }

    // When r2 is zero and direction is Out, clear everything (fast path).
    if (r2 == 0 && direction_ == TransitionDirection::Out) {
        int totalPixels = width * height;
        for (int i = 0; i < totalPixels; ++i) {
            buffer[i] = 0;
        }
        return;
    }

    // Clear pixels outside the circle.
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int dx = x - cx;
            int dy = y - cy;
            int dist2 = dx * dx + dy * dy;
            if (dist2 > r2) {
                buffer[y * width + x] = 0;
            }
        }
    }
}

#endif // PIXELROOT32_ENABLE_SCENE_TRANSITIONS

} // namespace pixelroot32::graphics
