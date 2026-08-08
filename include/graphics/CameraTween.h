/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file CameraTween.h
 * @brief Camera interpolation helper — fixed-capacity tween pool with 4 easing
 *        curves, all using Q16.16 integer math (no float in the hot path).
 *
 * Example (Scene composition pattern):
 * @code
 * class MyScene : public Scene {
 *     Camera2D camera{240, 240};
 *     pixelroot32::graphics::CameraTween<4> tweens_;
 * public:
 *     void update(unsigned long dt) override {
 *         tweens_.update(static_cast<uint16_t>(dt), &camera);
 *     }
 *     void onEnterRoom() {
 *         tweens_.startTween(camera.getPosition(), targetPos, 500,
 *                            TweenEasing::EaseInOutQuad);
 *     }
 * };
 * @endcode
 *
 * Feature-gated by PIXELROOT32_ENABLE_CAMERA_TWEEN (default 0).
 * When disabled, the header defines a stub with no-op methods, matching the
 * TransitionEffect.h / StateMachine.h opt-in pattern.
 */

#pragma once

#include "platforms/EngineConfig.h"
#include "math/Vector2.h"
#include "math/Scalar.h"
#include <cstdint>

namespace pixelroot32::graphics {

class Camera2D;

/**
 * @enum TweenEasing
 * @brief Easing curves for camera tween interpolation.
 *
 * All four curves map a normalised progress [0, 1] to an eased value [0, 1].
 * The easing math is computed in Q16.16 fixed-point (raw values 0..65536) so
 * that the hot path contains zero floating-point operations on the Fixed16
 * (ESP32-C3) scalar path.
 */
enum class TweenEasing : uint8_t {
    Linear = 0,        ///< Constant-rate interpolation: result = t.
    EaseInQuad,        ///< Quadratic ease-in: slow start, fast end. result = t².
    EaseOutQuad,       ///< Quadratic ease-out: fast start, slow end. result = 1 − (1−t)².
    EaseInOutQuad      ///< Quadratic ease-in-out: slow both ends. Piecewise t² / 1−2(1−t)².
};

#if PIXELROOT32_ENABLE_CAMERA_TWEEN

#include "graphics/Camera2D.h"

namespace detail {

/** @brief Convert a Q16.16 raw progress value (0..65536) to Scalar (0.0..1.0). */
inline pixelroot32::math::Scalar scalarFromQ16(uint32_t rawProgress) {
    if constexpr (std::is_same_v<pixelroot32::math::Scalar, float>) {
        return static_cast<float>(rawProgress) / 65536.0f;
    } else {
        return pixelroot32::math::Fixed16::fromRaw(static_cast<int32_t>(rawProgress));
    }
}

} // namespace detail

/**
 * @class CameraTween
 * @brief Fixed-capacity camera tween pool with enum-based easing.
 *
 * Owns a fixed array of N tween slots, each tracking a `from` → `to`
 * Vector2 interpolation with an independent duration and easing curve.
 * Every active tween writes its current interpolated position to the
 * camera via `Camera2D::setPosition()` on each `update()` call. When
 * multiple tweens are active, last-write-wins on the camera position.
 *
 * Zero heap allocation — the slot array and all easing math live on the
 * stack or in the object's fixed-size storage. Copy and move are deleted
 * because the pool is its identity.
 *
 * @tparam N Maximum number of simultaneously active tweens (default 4).
 *           When N=0, startTween() always returns kInvalidSlotId and
 *           activeCount() always returns 0.
 */
template <uint8_t N = 4>
class CameraTween {
public:
    /** @brief Maximum number of simultaneous tweens. */
    static constexpr uint8_t kMaxTweens = N;

    /**
     * @brief Sentinel returned by startTween() when the pool is full,
     *        durationMs is 0, or the N=0 degenerate case is used.
     */
    static constexpr uint8_t kInvalidSlotId = 0xFF;

    /** @brief Default constructor — all slots inactive. */
    CameraTween() : activeCount_(0) {
        for (uint8_t i = 0; i < slotCount(); ++i) {
            slots_[i].active = false;
        }
    }

    /** @brief Pool identity — copying would alias state. */
    CameraTween(const CameraTween&) = delete;
    /** @brief Pool identity — copy-assigning would alias state. */
    CameraTween& operator=(const CameraTween&) = delete;

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    /**
     * @brief Start a new camera tween.
     * @param from      Starting position (world coordinates).
     * @param to        Target position (world coordinates).
     * @param durationMs Total duration in milliseconds, 1..65535.
     *                   0 is treated as an instant no-op (returns kInvalidSlotId
     *                   without consuming a slot).
     * @param easing    Easing curve for the interpolation.
     * @return Slot id 0..N-1 on success, or kInvalidSlotId (0xFF) if the pool
     *         is full, N==0, or durationMs==0.
     */
    uint8_t startTween(pixelroot32::math::Vector2 from,
                        pixelroot32::math::Vector2 to,
                        uint16_t durationMs,
                        TweenEasing easing) {
        if (durationMs == 0) return kInvalidSlotId;

        const uint8_t count = slotCount();
        for (uint8_t i = 0; i < count; ++i) {
            if (!slots_[i].active) {
                slots_[i].from = from;
                slots_[i].to = to;
                slots_[i].elapsedMs = 0;
                slots_[i].durationMs = durationMs;
                slots_[i].easing = static_cast<uint8_t>(easing);
                slots_[i].active = true;
                ++activeCount_;
                return i;
            }
        }
        return kInvalidSlotId; // pool full or N==0
    }

    /**
     * @brief Advance all active tweens and write interpolated positions
     *        to the camera.
     *
     * Each active slot advances by `deltaTimeMs`. When a tween's elapsed time
     * reaches or exceeds its duration, the slot calls `camera->setPosition(to)`
     * exactly once and marks itself inactive. Partially advanced tweens write
     * their eased position every frame.
     *
     * @param deltaTimeMs Milliseconds elapsed since the last frame.
     * @param camera      Target camera (must not be nullptr when tweens are
     *                    active; safe nullptr when activeCount()==0).
     */
    void update(uint16_t deltaTimeMs, Camera2D* camera) {
        if (activeCount_ == 0 || camera == nullptr) return;

        const uint8_t count = slotCount();
        for (uint8_t i = 0; i < count; ++i) {
            if (!slots_[i].active) continue;

            slots_[i].elapsedMs += deltaTimeMs;

            if (slots_[i].elapsedMs >= slots_[i].durationMs) {
                // Tween complete — write final position exactly once.
                camera->setPosition(slots_[i].to);
                slots_[i].active = false;
                --activeCount_;
            } else {
                // Compute eased progress in Q16.16, then mix with lerp.
                // elapsedMs <= 65535 and durationMs <= 65535,
                // so (elapsedMs << 16) fits in uint32_t.
                const uint32_t progressRaw =
                    (static_cast<uint32_t>(slots_[i].elapsedMs) << 16)
                    / slots_[i].durationMs;

                const uint32_t easedRaw = applyEasing(
                    static_cast<TweenEasing>(slots_[i].easing), progressRaw);

                const pixelroot32::math::Scalar t =
                    detail::scalarFromQ16(easedRaw);

                const pixelroot32::math::Vector2 pos =
                    slots_[i].from.lerp(slots_[i].to, t);

                camera->setPosition(pos);
            }
        }
    }

    /**
     * @brief Check whether a given tween slot has completed.
     * @param slotId The slot id returned by startTween().
     * @return true if the slot exists, was previously started, and has
     *         finished (elapsed ≥ duration); false otherwise, including
     *         for invalid/out-of-range slotId.
     *
     * A cancelled slot is also "complete" — cancel() marks it inactive.
     */
    bool isComplete(uint8_t slotId) const {
        if (slotId >= slotCount()) return false;
        // A slot is complete when it was started (durationMs > 0) and is now
        // inactive — whether it ran to duration or was cancelled early.
        return !slots_[slotId].active && slots_[slotId].durationMs > 0;
    }

    /**
     * @brief Number of currently active (running) tweens.
     * @return Count in 0..N.
     */
    uint8_t activeCount() const {
        return activeCount_;
    }

    /**
     * @brief Cancel a tween without writing to the camera.
     *
     * Marks the slot inactive immediately. The camera position is NOT
     * changed — it stays wherever the last `update()` call left it.
     * Invalid `slotId` (≥N or never started) is a silent no-op.
     *
     * @param slotId The slot id returned by startTween().
     */
    void cancel(uint8_t slotId) {
        if (slotId >= slotCount()) return;
        if (!slots_[slotId].active) return;
        slots_[slotId].active = false;
        --activeCount_;
    }

private:
    // -----------------------------------------------------------------------
    // Tween slot — 24 bytes per slot on 32-bit, 40 bytes on 64-bit.
    // -----------------------------------------------------------------------
    struct Slot {
        pixelroot32::math::Vector2 from;          ///< Start position.
        pixelroot32::math::Vector2 to;            ///< End position.
        uint16_t elapsedMs;                       ///< Milliseconds elapsed.
        uint16_t durationMs;                      ///< Total duration in ms.
        uint8_t  easing;                          ///< TweenEasing stored as uint8_t.
        bool     active;                          ///< Whether this slot is running.
    };

    static constexpr uint32_t ONE_Q16  = 65536;   ///< 1.0 in Q16.16.
    static constexpr uint32_t HALF_Q16 = 32768;   ///< 0.5 in Q16.16.

    /**
     * @brief Compute eased progress in Q16.16 raw integer arithmetic.
     *
     * All easing curves operate on normalised progress values represented
     * as raw uint32_t in the range [0, ONE_Q16] (= [0.0, 1.0]).
     *
     * | Easing          | Formula (raw Q16.16, ONE = 65536)                  |
     * |-----------------|-----------------------------------------------------|
     * | Linear          | `result = t`                                        |
     * | EaseInQuad      | `result = (t * t) >> 16`                            |
     * | EaseOutQuad     | `result = ONE - ((ONE - t) * (ONE - t) >> 16)`       |
     * | EaseInOutQuad   | t < 0.5 : `result = (2 * t * t) >> 16`              |
     * |                 | t ≥ 0.5 : `result = ONE - ((2 * inv * inv) >> 16)`  |
     *
     * No float operations — pure integer multiplication and shifts.
     *
     * @param easing      One of the four TweenEasing values.
     * @param progressRaw Normalised progress in Q16.16 (0..ONE_Q16).
     * @return Eased progress in Q16.16 (0..ONE_Q16).
     */
    static uint32_t applyEasing(TweenEasing easing, uint32_t progressRaw) {
        switch (easing) {
            case TweenEasing::Linear:
                return progressRaw;

            case TweenEasing::EaseInQuad:
                return (progressRaw * progressRaw) >> 16;

            case TweenEasing::EaseOutQuad: {
                const uint32_t inv = ONE_Q16 - progressRaw;
                return ONE_Q16 - ((inv * inv) >> 16);
            }

            case TweenEasing::EaseInOutQuad: {
                if (progressRaw < HALF_Q16) {
                    // First half: 2*t²
                    return (2 * progressRaw * progressRaw) >> 16;
                } else {
                    // Second half: 1 − 2*(1−t)²
                    const uint32_t inv = ONE_Q16 - progressRaw;
                    return ONE_Q16 - ((2 * inv * inv) >> 16);
                }
            }
        }
        return progressRaw; // unreachable — compiler silence
    }

    /** @brief Effective slot count (N==0 collapses to 1 dummy slot). */
    static constexpr uint8_t slotCount() { return (N > 0) ? N : 1; }

    Slot slots_[slotCount()];
    uint8_t activeCount_;
};

#else // !PIXELROOT32_ENABLE_CAMERA_TWEEN
// =============================================================================
// Stub — CameraTween disabled (default). Every method is a no-op.
// =============================================================================

/**
 * @brief Stub CameraTween — all methods are no-ops when the feature is disabled.
 *
 * The stub template accepts any N so that code written as
 * `CameraTween<4> tweens_;` compiles identically in both flag states.
 * When the feature is off the object occupies 1 byte of storage and every
 * method returns the safe/default value.
 */
template <uint8_t N = 4>
class CameraTween {
public:
    /** @copydoc CameraTween<N>::kMaxTweens */
    static constexpr uint8_t kMaxTweens = N;
    /** @copydoc CameraTween<N>::kInvalidSlotId */
    static constexpr uint8_t kInvalidSlotId = 0xFF;

    uint8_t startTween(pixelroot32::math::Vector2 /*from*/,
                       pixelroot32::math::Vector2 /*to*/,
                       uint16_t /*durationMs*/,
                       TweenEasing /*easing*/) {
        return kInvalidSlotId;
    }

    void update(uint16_t /*deltaTimeMs*/, Camera2D* /*camera*/) {}

    bool isComplete(uint8_t /*slotId*/) const { return true; }

    uint8_t activeCount() const { return 0; }

    void cancel(uint8_t /*slotId*/) {}
};

#endif // PIXELROOT32_ENABLE_CAMERA_TWEEN

} // namespace pixelroot32::graphics
