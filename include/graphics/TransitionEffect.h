/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file TransitionEffect.h
 * @brief Scene transition effects — Fade (palette LUT) and Iris (circular wipe).
 *
 * Provides a standalone TransitionEffect class with zero-allocation implementations
 * for direct 8bpp framebuffer manipulation. Fade uses a pre-computed 256-byte LUT
 * to dim or brighten the palette. Iris uses (x-cx)²+(y-cy)² > r² circle test with
 * no sqrt, defaulting to buffer center.
 *
 * Feature-gated by PIXELROOT32_ENABLE_SCENE_TRANSITIONS.
 * When disabled, the header defines a stub that always returns no-ops.
 */

#pragma once

#include "platforms/EngineConfig.h"
#include <cstdint>

namespace pixelroot32::graphics {

/**
 * @enum TransitionType
 * @brief Types of scene transitions.
 */
enum class TransitionType : uint8_t {
    Fade = 0,       ///< Palette LUT dimming/brightening.
    Iris,           ///< Circular wipe from/to center.
    DiagonalWipe    ///< Diagonal sweep wipe from one corner to the opposite.
};

/**
 * @enum WipeDirection
 * @brief Corner-to-corner directions for DiagonalWipe transitions.
 *
 * Values name the source and target corners: NW_SE starts from top-left
 * (north-west) and sweeps toward bottom-right (south-east).
 */
enum class WipeDirection : uint8_t {
    NW_SE = 0,  ///< Top-left to bottom-right.
    NE_SW = 1,  ///< Top-right to bottom-left.
    SE_NW = 2,  ///< Bottom-right to top-left.
    SW_NE = 3   ///< Bottom-left to top-right.
};

/**
 * @enum TransitionDirection
 * @brief Direction of the transition effect.
 *
 * Out: starts fully visible and transitions to black/invisible.
 * In:  starts black/invisible and transitions to fully visible.
 */
enum class TransitionDirection : uint8_t {
    Out = 0,    ///< Exit direction (visible → hidden).
    In          ///< Entry direction (hidden → visible).
};

#if PIXELROOT32_ENABLE_SCENE_TRANSITIONS

/**
 * @class TransitionEffect
 * @brief Manages a single scene transition with zero runtime allocation.
 *
 * Pre-computes a 256-byte LUT for Fade effects per frame (LUT is computed
 * in apply()). Iris uses (x-cx)²+(y-cy)² > r² with no sqrt. All state is
 * fixed-size — no heap allocation in update() or apply().
 *
 * Typical lifecycle:
 *   effect.init(Fade, Out, 500);
 *   while (effect.isActive()) {
 *       effect.update(dt);
 *       effect.apply(buffer, width, height);
 *   }
 */
class TransitionEffect {
public:
    TransitionEffect() = default;

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    /**
     * @brief Initialise the effect with type, direction and duration.
     * @param type Fade or Iris transition.
     * @param direction Out (visible→hidden) or In (hidden→visible).
     * @param durationMs Total duration of the transition in milliseconds.
     */
    void init(TransitionType type, TransitionDirection direction, unsigned long durationMs);

    /**
     * @brief Advance the effect timer.
     * @param deltaTimeMs Time elapsed since last frame in ms.
     *
     * Clamps elapsed to duration — isActive() returns false once elapsed ≥ duration.
     */
    void update(unsigned long deltaTimeMs);

    /**
     * @brief Apply the transition effect to an 8bpp framebuffer.
     * @param buffer Pointer to the 8bpp pixel data (can be nullptr — safe no-op).
     * @param width Buffer width in pixels.
     * @param height Buffer height in pixels.
     *
     * Fade: pre-computes LUT from current progress, then maps each pixel through it.
     * Iris: zeroes pixels whose (x-cx)²+(y-cy)² exceeds current radius² (no sqrt).
     *
     * Safe to call when not active — returns immediately with no side effects.
     */
    void apply(uint8_t* buffer, int width, int height);

    /**
     * @brief Apply the transition effect to an RGB565 framebuffer.
     * @param buffer Pointer to RGB565 pixel data (can be nullptr — safe no-op).
     * @param width Buffer width in pixels.
     * @param height Buffer height in pixels.
     *
     * Used on native/SDL2 path where no 8bpp sprite buffer exists.
     * Iris zeroes pixels outside the circle. Fade darkens/brightens via channel scaling.
     *
     * Safe to call when not active — returns immediately with no side effects.
     */
    void applyRGB565(uint16_t* buffer, int width, int height);

    /**
     * @brief Check whether the transition is still running.
     * @return true while elapsed < duration or while hold frames remain.
     *
     * After elapsed reaches duration, isActive() stays true for
     * holdFrames_ additional update ticks. This provides a safety
     * window for systems that read isActive() before the final
     * apply() call in the same frame.
     */
    bool isActive() const {
        // Uninitialised effect is never active.
        if (durationMs_ == 0) return false;
        // Normal phase: advancing toward duration.
        if (elapsedMs_ < durationMs_) return true;
        // Hold phase: safety window for the final apply() call.
        return holdCounter_ < holdFrames_;
    }

    /**
     * @brief Get normalised progress of the transition.
     * @return 0.0 at start, 1.0 at completion.
     */
    float getProgress() const;

    /**
     * @brief Override the iris center for both Out and In phases (backward compat).
     * @param cx X-coordinate of the iris center.
     * @param cy Y-coordinate of the iris center.
     *
     * Sets both Out and In phases to the same center. Equivalent to calling
     * both setIrisOutCenter() and setIrisInCenter().
     * Only relevant for Iris transitions. Default is buffer center.
     * Call after init() and before apply().
     */
    void setIrisCenter(int cx, int cy) {
        setIrisOutCenter(cx, cy);
        setIrisInCenter(cx, cy);
    }

    /**
     * @brief Override the iris center for the Out (closing) phase.
     * @param cx X-coordinate of the iris center.
     * @param cy Y-coordinate of the iris center.
     *
     * Sets the center used when the iris closes (Out direction).
     * Only relevant for Iris transitions. Default is buffer center.
     * Call after init() and before apply().
     */
    void setIrisOutCenter(int cx, int cy) {
        irisOutCx_ = cx;
        irisOutCy_ = cy;
    }

    /**
     * @brief Override the iris center for the In (opening) phase.
     * @param cx X-coordinate of the iris center.
     * @param cy Y-coordinate of the iris center.
     *
     * Sets the center used when the iris opens (In direction).
     * Only relevant for Iris transitions. Default is buffer center.
     * Call after init() and before apply().
     */
    void setIrisInCenter(int cx, int cy) {
        irisInCx_ = cx;
        irisInCy_ = cy;
    }

    /**
     * @brief Set the wipe direction for DiagonalWipe transitions.
     * @param dir Corner-to-corner direction (default: NE_SW).
     *
     * Only relevant for DiagonalWipe transitions. Call after init().
     */
    void setWipeDirection(WipeDirection dir) { wipeDirection_ = dir; }

    /**
     * @brief Set the number of hold frames after duration expires.
     * @param frames Number of update ticks the effect stays active
     *               after reaching the duration boundary (default: 1).
     *
     * Provides a safety window for systems that read isActive() before
     * the final apply() call. Use 0 to disable hold entirely.
     */
    void setHoldFrames(uint8_t frames) { holdFrames_ = frames; }

private:
    TransitionType type_ = TransitionType::Fade;
    TransitionDirection direction_ = TransitionDirection::Out;
    unsigned long durationMs_ = 0;
    unsigned long elapsedMs_ = 0;
    int irisOutCx_ = -1;  ///< Custom iris center X for Out phase (-1 = use buffer center).
    int irisOutCy_ = -1;  ///< Custom iris center Y for Out phase (-1 = use buffer center).
    int irisInCx_ = -1;   ///< Custom iris center X for In phase (-1 = use buffer center).
    int irisInCy_ = -1;   ///< Custom iris center Y for In phase (-1 = use buffer center).
    WipeDirection wipeDirection_ = WipeDirection::NE_SW;  ///< DiagonalWipe corner direction.
    uint8_t holdFrames_ = 1;   ///< Number of hold ticks after duration expires.
    uint8_t holdCounter_ = 0;  ///< Current hold tick count.

    /**
     * @brief Fill a 256-byte LUT for the current fade direction and progress.
     * @param lut[256] Output LUT array (written for all 256 entries).
     * @param scaledProgress Progress in Q8.8 format (0..256, where 256 = 1.0).
     *
     * Out: lut[i] = i * (256-p) / 256 — dims to black.
     * In:  lut[i] = i * p     / 256 — brightens from black.
     */
    void computeFadeLut(uint8_t* lut, uint16_t scaledProgress) const;

    /**
     * @brief Apply the fade LUT to the entire 8bpp buffer.
     */
    void applyFade(uint8_t* buffer, int width, int height);

    /**
     * @brief Apply the iris wipe to the entire 8bpp buffer.
     */
    void applyIris(uint8_t* buffer, int width, int height);

    /**
     * @brief Apply the iris wipe to an RGB565 buffer.
     */
    void applyIrisRGB565(uint16_t* buffer, int width, int height);

    /**
     * @brief Apply the fade to an RGB565 buffer via channel scaling.
     */
    void applyFadeRGB565(uint16_t* buffer, int width, int height);
};

#else
// =============================================================================
// Stub — Scene transitions disabled
// =============================================================================

/**
 * @brief Stub TransitionEffect — all methods are no-ops when feature is disabled.
 */
class TransitionEffect {
public:
    void init(TransitionType /*type*/, TransitionDirection /*direction*/, unsigned long /*durationMs*/) {}
    void update(unsigned long /*deltaTimeMs*/) {}
    void apply(uint8_t* /*buffer*/, int /*width*/, int /*height*/) {}
    void applyRGB565(uint16_t* /*buffer*/, int /*width*/, int /*height*/) {}
    bool isActive() const { return false; }
    float getProgress() const { return 1.0f; }
    void setIrisCenter(int /*cx*/, int /*cy*/) {}
    void setIrisOutCenter(int /*cx*/, int /*cy*/) {}
    void setIrisInCenter(int /*cx*/, int /*cy*/) {}
    void setWipeDirection(WipeDirection /*dir*/) {}
    void setHoldFrames(uint8_t /*frames*/) {}
};

#endif // PIXELROOT32_ENABLE_SCENE_TRANSITIONS

} // namespace pixelroot32::graphics
