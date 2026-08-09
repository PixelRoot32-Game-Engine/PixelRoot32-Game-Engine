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

namespace {

/**
 * @brief Scale an RGB332 byte toward black, one channel at a time.
 *
 * The 8bpp framebuffer byte is a packed colour (RRRGGGBB), not an intensity —
 * see packRgb565ToTftSprite8() in Renderer.cpp. Multiplying the byte as a
 * scalar carries bits across the channel boundaries: half of pure red (0xE0)
 * lands on 0x70, which reads back as R=3, G=4 — more green than red. That is
 * a hue rotation, not a fade, and it is why a brown cave turned blue mid-fade
 * on hardware while native (RGB565, scaled per channel) looked correct.
 *
 * @param rgb332 Packed source colour.
 * @param factor Q8.8 scale in [0, 256]; 256 is identity, 0 is black.
 */
inline uint8_t scaleRgb332(uint8_t rgb332, uint16_t factor) {
    const uint16_t r = static_cast<uint16_t>((rgb332 >> 5) & 0x07);
    const uint16_t g = static_cast<uint16_t>((rgb332 >> 2) & 0x07);
    const uint16_t b = static_cast<uint16_t>(rgb332 & 0x03);
    return static_cast<uint8_t>(((((r * factor) >> 8) & 0x07) << 5) |
                                ((((g * factor) >> 8) & 0x07) << 2) |
                                (((b * factor) >> 8) & 0x03));
}

/**
 * @brief Scale an RGB565 word toward black, one channel at a time.
 *
 * Same reasoning as scaleRgb332(): a packed colour is not a scalar.
 */
inline uint16_t scaleRgb565(uint16_t rgb565, uint16_t factor) {
    const uint32_t r = static_cast<uint32_t>((rgb565 >> 11) & 0x1F);
    const uint32_t g = static_cast<uint32_t>((rgb565 >> 5) & 0x3F);
    const uint32_t b = static_cast<uint32_t>(rgb565 & 0x1F);
    return static_cast<uint16_t>(((((r * factor) >> 8) & 0x1F) << 11) |
                                 ((((g * factor) >> 8) & 0x3F) << 5) |
                                 (((b * factor) >> 8) & 0x1F));
}

} // namespace

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
    subStepAccumulator_ = 0;
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
        unsigned long advance = deltaTimeMs;

        // Sub-step accumulator: only for DiagonalWipe when enabled
        // (subStepMs_ > 0), to prevent flicker from fractional pixel
        // boundary positions.
        if (type_ == TransitionType::DiagonalWipe && subStepMs_ > 0) {
            subStepAccumulator_ += deltaTimeMs;
            advance = 0;
            while (subStepAccumulator_ >= subStepMs_) {
                subStepAccumulator_ -= subStepMs_;
                advance += subStepMs_;
            }
        }

        elapsedMs_ += advance;
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
    // Out dims to black as progress rises; In brightens from black.
    const uint16_t factor = (direction_ == TransitionDirection::Out)
                                ? static_cast<uint16_t>(256 - scaledProgress)
                                : scaledProgress;

    // Still one 256-entry table built once per frame — the cost is unchanged.
    // Only how an entry is derived changes: per RGB332 channel, not per byte.
    for (int i = 0; i < 256; ++i) {
        lut[i] = scaleRgb332(static_cast<uint8_t>(i), factor);
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
    // Only feather when the front has actually started moving (progress > 0).
    // At progress=0, no pixels should change for Out; all pixels clear for In.
    bool useFeather = (scaledProgress > 0);
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

        // Feather zone: 1-pixel band at the wipe front boundary.
        // diff = lineValue - front (signed distance).
        // diff < 0 → behind front. diff == 0 → at front (feather). diff > 0 → ahead.
        int diff = lineValue - front;

        if (diff < 0) {
            // Behind the advancing front.
            if (direction_ == TransitionDirection::Out) {
                buffer[i] = 0;  // Out: wiped area is hidden.
            }
            // In: behind front means revealed → keep unchanged.
        } else if (diff > 0) {
            // Ahead of the advancing front.
            if (direction_ == TransitionDirection::In) {
                buffer[i] = 0;  // In: ahead is still hidden.
            }
            // Out: ahead of front means not yet wiped → keep unchanged.
        } else {
            // diff == 0: exactly at the wipe front boundary.
            if (useFeather) {
                // 1-pixel feather band: 50% blend toward black (same for Out
                // and In — center of a symmetric blend from 0 to pixel).
                // Per channel: the byte is RGB332, not an intensity.
                buffer[i] = scaleRgb332(buffer[i], 128);
            } else {
                // No feather (progress=0 or progress=1): hard boundary.
                if (direction_ == TransitionDirection::In) {
                    // In: at the front is still hidden.
                    buffer[i] = 0;
                }
                // Out: at the front is not yet cleared → keep unchanged.
            }
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
            applyDiagonalWipeRGB565(buffer, width, height);
            break;
    }
}

// =============================================================================
// applyDiagonalWipeRGB565() — diagonal sweep wipe on RGB565 buffer
// =============================================================================

void TransitionEffect::applyDiagonalWipeRGB565(uint16_t* buffer, int width, int height) {
    unsigned long elapsed = elapsedMs_;
    unsigned long duration = durationMs_;
    uint16_t scaledProgress = 0;
    if (duration > 0) {
        scaledProgress = static_cast<uint16_t>((elapsed * 256) / duration);
        if (scaledProgress > 256) scaledProgress = 256;
    }

    int front = (width + height) * static_cast<int>(scaledProgress) / 256;
    bool useFeather = (scaledProgress > 0);
    int totalPixels = width * height;

    for (int i = 0; i < totalPixels; ++i) {
        int x = i % width;
        int y = i / width;

        int lineValue;
        switch (wipeDirection_) {
            case WipeDirection::NW_SE:
                lineValue = x + y;
                break;
            case WipeDirection::SE_NW:
                lineValue = (width - 1 - x) + (height - 1 - y);
                break;
            case WipeDirection::SW_NE:
                lineValue = x + (height - 1 - y);
                break;
            default: // WipeDirection::NE_SW
                lineValue = (width - 1 - x) + y;
                break;
        }

        int diff = lineValue - front;

        if (diff < 0) {
            if (direction_ == TransitionDirection::Out) {
                buffer[i] = 0;
            }
        } else if (diff > 0) {
            if (direction_ == TransitionDirection::In) {
                buffer[i] = 0;
            }
        } else {
            // diff == 0: at the wipe front.
            if (useFeather) {
                // Per channel: an RGB565 word is not a scalar either. This one
                // was wrong on every platform, not just the 8bpp path.
                buffer[i] = scaleRgb565(buffer[i], 128);
            } else if (direction_ == TransitionDirection::In) {
                buffer[i] = 0;
            }
        }
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

    // Out dims to black as progress rises; In brightens from black. Same rule
    // the 8bpp path uses in computeFadeLut(), only a wider pixel format.
    const uint16_t factor = (direction_ == TransitionDirection::Out)
                                ? static_cast<uint16_t>(256 - scaledProgress)
                                : scaledProgress;

    // No LUT here: 65536 entries would cost more than the 240x240 buffer it
    // would serve. Per-pixel channel maths is the cheaper side of that trade.
    int totalPixels = width * height;
    for (int i = 0; i < totalPixels; ++i) {
        buffer[i] = scaleRgb565(buffer[i], factor);
    }
}

#endif // PIXELROOT32_ENABLE_SCENE_TRANSITIONS

} // namespace pixelroot32::graphics
