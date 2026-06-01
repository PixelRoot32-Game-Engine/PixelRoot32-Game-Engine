/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file CameraEffects.cpp
 * @brief Implementation of CameraEffectsSystem — shake, punch, offset.
 *
 * All math uses type-agnostic Scalar arithmetic: float on FPU targets,
 * Fixed16 on non-FPU targets. Shake offset uses Xorshift32 random bits
 * normalized to [-1,1] as Scalar, then scaled by amplitude and decay.
 * Punch uses linear decay. Offset holds constant until expiry.
 */

#include "graphics/CameraEffects.h"

namespace pixelroot32::graphics {

#if PIXELROOT32_ENABLE_CAMERA_EFFECTS

// =============================================================================
// update() — advance effect timers
// =============================================================================

void CameraEffectsSystem::update(unsigned long deltaTimeMs) {
    for (auto& slot : slots_) {
        if (slot.type == EffectSlot::Type::Inactive) continue;

        slot.elapsed += deltaTimeMs;
        if (slot.elapsed >= slot.duration) {
            // Effect completed — reset to inactive
            slot.type = EffectSlot::Type::Inactive;
            slot.elapsed = 0;
        }
    }
}

// =============================================================================
// getOffset() — summed offset vector
// =============================================================================

math::Vector2 CameraEffectsSystem::getOffset() const {
    // Fast no-op path: zero loop iterations when no effects are active.
    if (!hasActiveEffects()) {
        return math::Vector2::ZERO();
    }

    math::Vector2 total;
    for (const auto& slot : slots_) {
        switch (slot.type) {
            case EffectSlot::Type::Shake:
                total += computeShakeOffset(slot);
                break;
            case EffectSlot::Type::Punch:
                total += computeDecayOffset(slot);
                break;
            case EffectSlot::Type::Offset:
                total += computeDecayOffset(slot);
                break;
            default:
                break;
        }
    }
    return total;
}

// =============================================================================
// hasActiveEffects()
// =============================================================================

bool CameraEffectsSystem::hasActiveEffects() const {
    for (const auto& slot : slots_) {
        if (slot.type != EffectSlot::Type::Inactive) {
            return true;
        }
    }
    return false;
}

// =============================================================================
// triggerShake()
// =============================================================================

void CameraEffectsSystem::triggerShake(math::Scalar amp, unsigned long dur) {
    uint8_t idx = allocSlot();
    auto& slot = slots_[idx];
    slot.type = EffectSlot::Type::Shake;
    slot.amplitude = amp;
    slot.duration = dur;
    slot.elapsed = 0;
    slot.direction = math::Vector2::ZERO();  // unused for shake
}

// =============================================================================
// triggerPunch()
// =============================================================================

void CameraEffectsSystem::triggerPunch(
    math::Scalar amp, unsigned long dur, const math::Vector2& dir)
{
    uint8_t idx = allocSlot();
    auto& slot = slots_[idx];
    slot.type = EffectSlot::Type::Punch;
    slot.amplitude = amp;
    slot.duration = dur;
    slot.elapsed = 0;
    slot.direction = dir;
}

// =============================================================================
// triggerOffset()
// =============================================================================

void CameraEffectsSystem::triggerOffset(math::Scalar amp, unsigned long dur) {
    uint8_t idx = allocSlot();
    auto& slot = slots_[idx];
    slot.type = EffectSlot::Type::Offset;
    slot.amplitude = amp;
    slot.duration = dur;
    slot.elapsed = 0;
    slot.direction = math::Vector2::RIGHT();  // default direction (1, 0)
}

// =============================================================================
// cancelAll()
// =============================================================================

void CameraEffectsSystem::cancelAll() {
    for (auto& slot : slots_) {
        slot.type = EffectSlot::Type::Inactive;
        slot.amplitude = math::toScalar(0);
        slot.duration = 0;
        slot.elapsed = 0;
        slot.direction = math::Vector2::ZERO();
    }
    nextSlot_ = 0;
}

// =============================================================================
// computeShakeOffset() — Xorshift32 random, type-agnostic Scalar math
// =============================================================================
//
// Uses Xorshift32 to generate random bits, splits into two signed 16-bit
// values, normalizes to [-1, 1] as Scalar, then scales by amplitude and
// linear decay. No type-conditional branches — works for both float and
// Fixed16 via toScalar() and Scalar arithmetic.

math::Vector2 CameraEffectsSystem::computeShakeOffset(const EffectSlot& s) const {
    uint32_t r = math::detail::xorshift32_next();

    // Split 32 bits into two signed 16-bit ranges: [-32768, 32767]
    int32_t rx = static_cast<int32_t>(r & 0xFFFF) - 32768;
    int32_t ry = static_cast<int32_t>((r >> 16) & 0xFFFF) - 32768;

    // Normalize to [-1, 1] as Scalar (float or Fixed16)
    math::Scalar normX = math::toScalar(rx) / math::toScalar(32768);
    math::Scalar normY = math::toScalar(ry) / math::toScalar(32768);

    // Linear decay factor: 1.0 at t=0, ~0.0 at t=duration
    unsigned long remaining = s.duration - s.elapsed;
    math::Scalar decay = math::toScalar(static_cast<unsigned long>(remaining))
                       / math::toScalar(s.duration);

    // Result = (norm * amplitude * decay) for each axis
    return math::Vector2(
        normX * s.amplitude * decay,
        normY * s.amplitude * decay
    );
}

// =============================================================================
// computeDecayOffset() — linear decay for Punch, constant for Offset
// =============================================================================

math::Vector2 CameraEffectsSystem::computeDecayOffset(const EffectSlot& s) const {
    if (s.type == EffectSlot::Type::Punch) {
        // Linear decay: factor goes from 1.0 → 0.0 over duration
        unsigned long remaining = s.duration - s.elapsed;
        math::Scalar decay = math::toScalar(static_cast<unsigned long>(remaining))
                           / math::toScalar(s.duration);
        return math::Vector2(
            s.direction.x * s.amplitude * decay,
            s.direction.y * s.amplitude * decay
        );
    } else {
        // Offset type: constant displacement, no decay
        return math::Vector2(
            s.direction.x * s.amplitude,
            s.direction.y * s.amplitude
        );
    }
}

// =============================================================================
// allocSlot() — round-robin slot allocation
// =============================================================================

uint8_t CameraEffectsSystem::allocSlot() {
    uint8_t idx = nextSlot_;
    nextSlot_ = (nextSlot_ + 1) % kMaxSlots;
    return idx;
}

#endif // PIXELROOT32_ENABLE_CAMERA_EFFECTS

} // namespace pixelroot32::graphics
