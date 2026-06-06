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
// smoothstepQ8() — Q8.8 smoothstep easing
// =============================================================================

uint16_t smoothstepQ8(uint16_t t) {
    // Edge cases: no easing needed at exact endpoints.
    if (t == 0) return 0;
    if (t >= 256) return 256;

    uint32_t t32 = t;
    // t^2 in Q8.8: multiply then shift right by 8 bits.
    uint32_t t2 = (t32 * t32) >> 8;
    // (3 - 2t) in Q8.8: 3.0 = 768 in fixed point.
    int32_t threeMinus2t = 768 - 2 * (int32_t)t;
    // result = t^2 * (3 - 2t) / 65536 (two shifts of 8 bits each).
    uint32_t result = (t2 * (uint32_t)threeMinus2t) >> 8;

    // Safety clamp — should never exceed 256 for valid t in [0, 256].
    if (result > 256) result = 256;
    return (uint16_t)result;
}

// =============================================================================
// init() — configure effect parameters
// =============================================================================

void TransitionEffect::init(TransitionType type, TransitionDirection direction,
                            unsigned long durationMs) {
    type_ = type;
    direction_ = direction;
    durationMs_ = durationMs;
    elapsedMs_ = 0;
    holdCounter_ = 0;
    irisOutCx_ = -1;  // Reset custom iris centers to default.
    irisOutCy_ = -1;
    irisInCx_ = -1;
    irisInCy_ = -1;
}

// =============================================================================
// update() — advance elapsed time
// =============================================================================

void TransitionEffect::update(unsigned long deltaTimeMs) {
    if (!isActive()) return;

    if (elapsedMs_ < durationMs_) {
        // Normal phase: advance the timer.
        elapsedMs_ += deltaTimeMs;
        if (elapsedMs_ > durationMs_) {
            elapsedMs_ = durationMs_;
        }
    } else {
        // Hold phase: consume one hold tick per update() call.
        // isActive() returns holdCounter_ < holdFrames_ during this phase.
        holdCounter_++;
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

        case TransitionType::DiagonalWipe:
            applyDiagonalWipe(buffer, width, height);
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

    // Determine iris center based on direction.
    int cx, cy;
    if (direction_ == TransitionDirection::Out) {
        cx = (irisOutCx_ >= 0) ? irisOutCx_ : (width / 2);
        cy = (irisOutCy_ >= 0) ? irisOutCy_ : (height / 2);
    } else {
        cx = (irisInCx_ >= 0) ? irisInCx_ : (width / 2);
        cy = (irisInCy_ >= 0) ? irisInCy_ : (height / 2);
    }

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

// =============================================================================
// applyDiagonalWipe() — diagonal sweep wipe on 8bpp buffer
// =============================================================================

void TransitionEffect::applyDiagonalWipe(uint8_t* buffer, int width, int height) {
    unsigned long elapsed = elapsedMs_;
    unsigned long duration = durationMs_;
    uint16_t scaledProgress = 0;
    if (duration > 0) {
        scaledProgress = static_cast<uint16_t>((elapsed * 256) / duration);
        if (scaledProgress > 256) scaledProgress = 256;
    }

    // Front position along the diagonal axis, in pixel units.
    int front = (width + height) * static_cast<int>(scaledProgress) / 256;
    int totalPixels = width * height;

    for (int i = 0; i < totalPixels; ++i) {
        int x = i % width;
        int y = i / width;

        // Compute the pixel's line value based on wipe direction.
        // Small values are closest to the starting corner and are cleared first
        // in Out mode. Large values are closest to the ending corner.
        int lineValue;
        switch (wipeDirection_) {
            case WipeDirection::NW_SE:
                // Top-left (0,0) = 0 → Bottom-right (W-1,H-1) = W+H-2
                lineValue = x + y;
                break;
            case WipeDirection::SE_NW:
                // Bottom-right (W-1,H-1) = 0 → Top-left (0,0) = W+H-2
                lineValue = (width - 1 - x) + (height - 1 - y);
                break;
            case WipeDirection::SW_NE:
                // Bottom-left (0,H-1) = 0 → Top-right (W-1,0) = W+H-2
                lineValue = x + (height - 1 - y);
                break;
            default: // WipeDirection::NE_SW
                // Top-right (W-1,0) = 0 → Bottom-left (0,H-1) = W+H-2
                lineValue = (width - 1 - x) + y;
                break;
        }

        if (lineValue < front) {
            buffer[i] = 0;
        }
    }
}

// =============================================================================
// applyRGB565() — dispatch to RGB565-specific effects (native/SDL2 path)
// =============================================================================

void TransitionEffect::applyRGB565(uint16_t* buffer, int width, int height) {
    if (buffer == nullptr) return;
    if (durationMs_ == 0) return;

    switch (type_) {
        case TransitionType::Fade:
            applyFadeRGB565(buffer, width, height);
            break;

        case TransitionType::Iris:
            applyIrisRGB565(buffer, width, height);
            break;

        case TransitionType::DiagonalWipe:
            // TODO: Implement in PR 2
            break;
    }
}

// =============================================================================
// applyIrisRGB565() — circular wipe on RGB565 buffer
// =============================================================================

void TransitionEffect::applyIrisRGB565(uint16_t* buffer, int width, int height) {
    unsigned long elapsed = elapsedMs_;
    unsigned long duration = durationMs_;
    uint16_t scaledProgress = 0;
    if (duration > 0) {
        scaledProgress = static_cast<uint16_t>((elapsed * 256) / duration);
        if (scaledProgress > 256) scaledProgress = 256;
    }

    int cx, cy;
    if (direction_ == TransitionDirection::Out) {
        cx = (irisOutCx_ >= 0) ? irisOutCx_ : (width / 2);
        cy = (irisOutCy_ >= 0) ? irisOutCy_ : (height / 2);
    } else {
        cx = (irisInCx_ >= 0) ? irisInCx_ : (width / 2);
        cy = (irisInCy_ >= 0) ? irisInCy_ : (height / 2);
    }

    int dxMax = (cx >= width - 1 - cx) ? cx : (width - 1 - cx);
    int dyMax = (cy >= height - 1 - cy) ? cy : (height - 1 - cy);
    int maxRadius2 = dxMax * dxMax + dyMax * dyMax;

    int r2;
    if (direction_ == TransitionDirection::Out) {
        r2 = static_cast<int>((maxRadius2 * (256 - scaledProgress)) >> 8);
    } else {
        r2 = static_cast<int>((maxRadius2 * scaledProgress) >> 8);
    }

    if (r2 == 0 && direction_ == TransitionDirection::Out) {
        int totalPixels = width * height;
        for (int i = 0; i < totalPixels; ++i) {
            buffer[i] = 0;
        }
        return;
    }

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

// =============================================================================
// applyFadeRGB565() — fade darkening/brightening on RGB565 buffer
// =============================================================================

void TransitionEffect::applyFadeRGB565(uint16_t* buffer, int width, int height) {
    unsigned long elapsed = elapsedMs_;
    unsigned long duration = durationMs_;
    uint16_t scaledProgress = 0;
    if (duration > 0) {
        scaledProgress = static_cast<uint16_t>((elapsed * 256) / duration);
        if (scaledProgress > 256) scaledProgress = 256;
    }

    int totalPixels = width * height;
    for (int i = 0; i < totalPixels; ++i) {
        uint16_t pixel = buffer[i];
        // Extract RGB565 channels.
        uint8_t r = (pixel >> 11) & 0x1F;
        uint8_t g = (pixel >> 5) & 0x3F;
        uint8_t b = pixel & 0x1F;

        if (direction_ == TransitionDirection::Out) {
            // Darken: scale down by (256 - progress) / 256
            uint16_t factor = 256 - scaledProgress;
            r = static_cast<uint8_t>((r * factor) >> 8);
            g = static_cast<uint8_t>((g * factor) >> 8);
            b = static_cast<uint8_t>((b * factor) >> 8);
        } else {
            // Brighten: scale up by progress / 256
            r = static_cast<uint8_t>((r * scaledProgress) >> 8);
            g = static_cast<uint8_t>((g * scaledProgress) >> 8);
            b = static_cast<uint8_t>((b * scaledProgress) >> 8);
        }

        buffer[i] = static_cast<uint16_t>((r << 11) | (g << 5) | b);
    }
}

#endif // PIXELROOT32_ENABLE_SCENE_TRANSITIONS

} // namespace pixelroot32::graphics
