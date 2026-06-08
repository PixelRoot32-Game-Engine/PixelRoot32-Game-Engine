/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file CameraEffects.h
 * @brief Camera effects system — shake, punch, and offset effects.
 *
 * Provides a standalone CameraEffectsSystem with 4 fixed effect slots
 * using std::array (zero heap). Effects are updated via update(dt) and
 * the summed offset is retrieved via getOffset(). Shake uses Xorshift32
 * random (pure integer math) avoiding float operations on non-FPU targets.
 *
 * Feature-gated by PIXELROOT32_ENABLE_CAMERA_EFFECTS.
 * When disabled, the header defines a stub that always returns zero offset.
 */

#pragma once

#include "platforms/EngineConfig.h"
#include "math/Scalar.h"
#include "math/Vector2.h"
#include "math/MathUtil.h"
#include <array>
#include <cstdint>

namespace pixelroot32::graphics {

#if PIXELROOT32_ENABLE_CAMERA_EFFECTS

/**
 * @struct EffectSlot
 * @brief Per-slot state for a single camera effect (20 bytes).
 */
struct EffectSlot {
    /** @brief Effect type determines behaviour. */
    enum class Type : uint8_t { Inactive = 0, Shake, Punch, Offset };

    Type type = Type::Inactive;
    math::Scalar amplitude{0};
    unsigned long duration = 0;  ///< Total duration in ms.
    unsigned long elapsed = 0;   ///< Elapsed time in ms.
    math::Vector2 direction;     ///< Normalized direction (Punch/Offset only).
};

/**
 * @class CameraEffectsSystem
 * @brief Manages up to 4 simultaneous camera effects with round-robin insertion.
 *
 * Effects are updated with a delta time and the resulting summed offset
 * can be fetched for application to a Camera2D. Provides a fast no-op path
 * via hasActiveEffects().
 */
class CameraEffectsSystem {
public:
    static constexpr uint8_t kMaxSlots = 4;

    /** @brief Default constructor — all slots start inactive. */
    CameraEffectsSystem() = default;

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    /**
     * @brief Advance all active effect timers.
     * @param deltaTimeMs Time elapsed since last frame in ms.
     */
    void update(unsigned long deltaTimeMs);

    /**
     * @brief Get the summed offset from all active effects.
     * @return Summed math::Vector2 offset. ZERO if no effects are active.
     *
     * Early-outs when hasActiveEffects() is false — zero loop iteration.
     */
    math::Vector2 getOffset() const;

    /**
     * @brief Fast check for any active effects.
     * @return true if at least one slot is active.
     */
    bool hasActiveEffects() const;

    /**
     * @brief Trigger a shake effect (random oscillation).
     * @param amp Maximum amplitude of the shake.
     * @param dur Duration in ms.
     */
    void triggerShake(math::Scalar amp, unsigned long dur);

    /**
     * @brief Trigger a punch effect (directional impulse with decay).
     * @param amp Initial amplitude.
     * @param dur Duration in ms.
     * @param dir Normalized direction of the punch.
     */
    void triggerPunch(math::Scalar amp, unsigned long dur, const math::Vector2& dir);

    /**
     * @brief Trigger a constant offset effect.
     * @param amp Amplitude/displacement amount.
     * @param dur Duration in ms.
     *
     * The offset direction defaults to RIGHT (1, 0).
     * Displacement stays constant for the full duration, then snaps to zero.
     */
    void triggerOffset(math::Scalar amp, unsigned long dur);

    /**
     * @brief Cancel all active effects immediately.
     */
    void cancelAll();

private:
    std::array<EffectSlot, kMaxSlots> slots_{};
    uint8_t nextSlot_ = 0;  ///< Round-robin insertion index.

    /**
     * @brief Compute a single frame's shake offset using Xorshift32.
     * @param s The shake effect slot.
     * @return Random offset vector bounded by slot amplitude.
     */
    math::Vector2 computeShakeOffset(const EffectSlot& s) const;

    /**
     * @brief Compute offset with linear decay (Punch) or constant (Offset).
     * @param s The effect slot (Punch or Offset type).
     * @return Decayed or constant offset vector.
     */
    math::Vector2 computeDecayOffset(const EffectSlot& s) const;

    /** @brief Find the next available slot index (round-robin). */
    uint8_t allocSlot();
};

#else
// =============================================================================
// Stub — CameraEffects disabled
// =============================================================================

/** @brief Stub EffectSlot (empty when feature disabled). */
struct EffectSlot {};

/**
 * @brief Stub CameraEffectsSystem — always returns zero offset.
 */
class CameraEffectsSystem {
public:
    void update(unsigned long /*deltaTimeMs*/) {}
    math::Vector2 getOffset() const { return math::Vector2::ZERO(); }
    bool hasActiveEffects() const { return false; }
    void triggerShake(math::Scalar /*amp*/, unsigned long /*dur*/) {}
    void triggerPunch(math::Scalar /*amp*/, unsigned long /*dur*/, const math::Vector2& /*dir*/) {}
    void triggerOffset(math::Scalar /*amp*/, unsigned long /*dur*/) {}
    void cancelAll() {}
};

#endif // PIXELROOT32_ENABLE_CAMERA_EFFECTS

} // namespace pixelroot32::graphics
