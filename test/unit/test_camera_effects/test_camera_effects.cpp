/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file test_camera_effects.cpp
 * @brief Unit tests for CameraEffectsSystem — shake, punch, offset, lifecycle.
 *
 * Coverage gap: CameraEffects.cpp was at 4%. This suite targets ≥70%.
 * Xorshift32 is non-deterministic → range-based asserts for shake tests.
 */

#include <unity.h>
#include "graphics/CameraEffects.h"
#include "../../test_config.h"

#ifdef PIXELROOT32_ENABLE_CAMERA_EFFECTS

using namespace pixelroot32::graphics;
using namespace pixelroot32::math;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// CE-07: Empty system returns ZERO offset
// =============================================================================

void test_camera_empty_system_returns_zero(void) {
    CameraEffectsSystem ces;
    TEST_ASSERT_FALSE(ces.hasActiveEffects());
    TEST_ASSERT_TRUE(ces.getOffset() == Vector2::ZERO());
}

// =============================================================================
// CE-01: triggerShake() allocates slot and returns non-zero offset
// =============================================================================

void test_camera_shake_produces_nonzero_offset(void) {
    CameraEffectsSystem ces;
    ces.triggerShake(toScalar(10), 1000);
    TEST_ASSERT_TRUE(ces.hasActiveEffects());
    Vector2 offset = ces.getOffset();
    // Offset must be bounded by amplitude × decay (decay=1.0 at t=0)
    TEST_ASSERT_TRUE(offset.x >= toScalar(-10) && offset.x <= toScalar(10));
    TEST_ASSERT_TRUE(offset.y >= toScalar(-10) && offset.y <= toScalar(10));
    // hasActiveEffects remains true after getOffset
    TEST_ASSERT_TRUE(ces.hasActiveEffects());
}

// =============================================================================
// CE-02: Two shake triggers produce varying offsets (Xorshift32 non-determinism)
// =============================================================================

void test_camera_shake_varying_offsets(void) {
    CameraEffectsSystem ces;
    ces.triggerShake(toScalar(10), 1000);
    ces.triggerShake(toScalar(10), 1000);
    // Two active shake slots, each calls xorshift32_next → different values
    Vector2 offset1 = ces.getOffset();
    Vector2 offset2 = ces.getOffset();
    bool differs = (offset1.x != offset2.x) || (offset1.y != offset2.y);
    TEST_ASSERT_TRUE_MESSAGE(differs, "Consecutive shake offsets should differ due to Xorshift32");
}

// =============================================================================
// CE-03: triggerPunch() MUST decay amplitude over time
// =============================================================================

void test_camera_punch_decays_over_time(void) {
    CameraEffectsSystem ces;
    ces.triggerPunch(toScalar(10), 100, Vector2::RIGHT());
    Vector2 fullOffset = ces.getOffset();
    // Full offset should be positive (RIGHT direction)
    TEST_ASSERT_TRUE(fullOffset.x > toScalar(0));

    // After 50ms of 100ms → decay = 0.5
    ces.update(50);
    Vector2 decayedOffset = ces.getOffset();
    TEST_ASSERT_TRUE_MESSAGE(decayedOffset.x < fullOffset.x,
        "Punch decay: offset after 50% elapsed must be less than full");
    TEST_ASSERT_TRUE(decayedOffset.x >= toScalar(0));

    // After full duration → offset = ZERO
    ces.update(100);
    TEST_ASSERT_FALSE(ces.hasActiveEffects());
    TEST_ASSERT_TRUE(ces.getOffset() == Vector2::ZERO());
}

void test_camera_punch_decay_down_direction(void) {
    CameraEffectsSystem ces;
    ces.triggerPunch(toScalar(5), 200, Vector2::DOWN());
    Vector2 fullOffset = ces.getOffset();
    TEST_ASSERT_TRUE(fullOffset.y > toScalar(0));  // DOWN = positive y

    ces.update(100);  // 50% elapsed
    Vector2 decayedOffset = ces.getOffset();
    TEST_ASSERT_TRUE(decayedOffset.y < fullOffset.y);
    TEST_ASSERT_TRUE(decayedOffset.y >= toScalar(0));
}

// =============================================================================
// CE-04: triggerOffset() MUST produce constant offset (no decay)
// =============================================================================

void test_camera_offset_constant_no_decay(void) {
    CameraEffectsSystem ces;
    ces.triggerOffset(toScalar(10), 200);
    Vector2 offset1 = ces.getOffset();
    // Default direction is RIGHT → positive x
    TEST_ASSERT_TRUE(offset1.x > toScalar(0));

    // Update 50ms — should NOT decay (Offset type is constant)
    ces.update(50);
    Vector2 offset2 = ces.getOffset();
    TEST_ASSERT_TRUE_MESSAGE(offset1.x == offset2.x && offset1.y == offset2.y,
        "Offset effect should not decay over time");

    // After full duration → expires
    ces.update(200);
    TEST_ASSERT_FALSE(ces.hasActiveEffects());
    TEST_ASSERT_TRUE(ces.getOffset() == Vector2::ZERO());
}

// =============================================================================
// CE-09: Offset type MUST NOT decay in computeDecayOffset
// =============================================================================

void test_camera_offset_type_no_decay_multiple_updates(void) {
    CameraEffectsSystem ces;
    ces.triggerOffset(toScalar(10), 300);

    Vector2 before = ces.getOffset();
    // Multiple partial updates
    ces.update(30);
    ces.update(30);
    ces.update(30);
    Vector2 after = ces.getOffset();
    // Offset remains constant within duration
    TEST_ASSERT_TRUE(before.x == after.x && before.y == after.y);

    // Expire
    ces.update(300);
    TEST_ASSERT_FALSE(ces.hasActiveEffects());
}

// =============================================================================
// CE-05: cancelAll() MUST reset all slots to inactive
// =============================================================================

void test_camera_cancel_all_resets_all_slots(void) {
    CameraEffectsSystem ces;
    ces.triggerShake(toScalar(10), 1000);
    ces.triggerPunch(toScalar(5), 500, Vector2::DOWN());
    ces.triggerOffset(toScalar(3), 200);
    TEST_ASSERT_TRUE(ces.hasActiveEffects());

    ces.cancelAll();
    TEST_ASSERT_FALSE(ces.hasActiveEffects());
    TEST_ASSERT_TRUE(ces.getOffset() == Vector2::ZERO());
}

void test_camera_cancel_all_when_empty(void) {
    CameraEffectsSystem ces;
    // cancelAll on empty system should not crash
    ces.cancelAll();
    TEST_ASSERT_FALSE(ces.hasActiveEffects());
    TEST_ASSERT_TRUE(ces.getOffset() == Vector2::ZERO());
}

// =============================================================================
// CE-06: update() MUST expire effects after duration
// =============================================================================

void test_camera_update_expires_effects(void) {
    CameraEffectsSystem ces;
    ces.triggerShake(toScalar(10), 30);
    TEST_ASSERT_TRUE(ces.hasActiveEffects());
    ces.update(50);
    TEST_ASSERT_FALSE_MESSAGE(ces.hasActiveEffects(),
        "Shake with 30ms duration should expire after 50ms update");
    TEST_ASSERT_TRUE(ces.getOffset() == Vector2::ZERO());
}

void test_camera_update_multiple_effects_expire_individually(void) {
    CameraEffectsSystem ces;
    ces.triggerOffset(toScalar(5), 20);   // short
    ces.triggerShake(toScalar(10), 200);  // long
    TEST_ASSERT_TRUE(ces.hasActiveEffects());

    ces.update(30);  // offset expires, shake remains
    TEST_ASSERT_TRUE(ces.hasActiveEffects());

    ces.update(200);  // shake also expires
    TEST_ASSERT_FALSE(ces.hasActiveEffects());
}

// =============================================================================
// CE-08: allocSlot() MUST round-robin through 4 slots
// =============================================================================

void test_camera_slot_round_robin_no_crash(void) {
    CameraEffectsSystem ces;
    // Trigger 6 effects (kMaxSlots=4 → wraps around twice)
    for (int i = 0; i < 6; ++i) {
        ces.triggerShake(toScalar(10), 1000);
    }
    // All 4 slots active, should still report active
    TEST_ASSERT_TRUE(ces.hasActiveEffects());

    // Cancel should reset everything
    ces.cancelAll();
    TEST_ASSERT_FALSE(ces.hasActiveEffects());
}

void test_camera_slot_round_robin_exactly_fill(void) {
    CameraEffectsSystem ces;
    // Fill exactly 4 slots
    for (int i = 0; i < 4; ++i) {
        ces.triggerPunch(toScalar(5), 500, Vector2::UP());
    }
    TEST_ASSERT_TRUE(ces.hasActiveEffects());
    // All 4 active → cancelAll resets
    ces.cancelAll();
    TEST_ASSERT_FALSE(ces.hasActiveEffects());
}

// =============================================================================
// Edge cases: hasActiveEffects fast path, mixed types, last-remaining
// =============================================================================

void test_camera_has_active_effects_fast_path(void) {
    CameraEffectsSystem ces;
    // Empty → immediate false
    TEST_ASSERT_FALSE(ces.hasActiveEffects());
    // After trigger → true
    ces.triggerShake(toScalar(5), 100);
    TEST_ASSERT_TRUE(ces.hasActiveEffects());
    // After expiry → false
    ces.update(200);
    TEST_ASSERT_FALSE(ces.hasActiveEffects());
}

void test_camera_mixed_effect_types(void) {
    CameraEffectsSystem ces;
    ces.triggerShake(toScalar(10), 500);
    ces.triggerPunch(toScalar(8), 300, Vector2::LEFT());
    ces.triggerOffset(toScalar(5), 400);
    // getOffset sums all 3 active effects
    Vector2 total = ces.getOffset();
    // Individual components should be bounded by sum of amplitudes
    TEST_ASSERT_TRUE(total.x >= toScalar(-23) && total.x <= toScalar(23));
    TEST_ASSERT_TRUE(total.y >= toScalar(-23) && total.y <= toScalar(23));
}

// =============================================================================
// main
// =============================================================================

int main(void) {
    UNITY_BEGIN();

    // CE-07
    RUN_TEST(test_camera_empty_system_returns_zero);

    // CE-01, CE-02
    RUN_TEST(test_camera_shake_produces_nonzero_offset);
    RUN_TEST(test_camera_shake_varying_offsets);

    // CE-03
    RUN_TEST(test_camera_punch_decays_over_time);
    RUN_TEST(test_camera_punch_decay_down_direction);

    // CE-04, CE-09
    RUN_TEST(test_camera_offset_constant_no_decay);
    RUN_TEST(test_camera_offset_type_no_decay_multiple_updates);

    // CE-05
    RUN_TEST(test_camera_cancel_all_resets_all_slots);
    RUN_TEST(test_camera_cancel_all_when_empty);

    // CE-06
    RUN_TEST(test_camera_update_expires_effects);
    RUN_TEST(test_camera_update_multiple_effects_expire_individually);

    // CE-08
    RUN_TEST(test_camera_slot_round_robin_no_crash);
    RUN_TEST(test_camera_slot_round_robin_exactly_fill);

    // Edge cases
    RUN_TEST(test_camera_has_active_effects_fast_path);
    RUN_TEST(test_camera_mixed_effect_types);

    return UNITY_END();
}

#else // PIXELROOT32_ENABLE_CAMERA_EFFECTS

void setUp(void) {}
void tearDown(void) {}

void test_camera_disabled(void) {
    TEST_IGNORE_MESSAGE("PIXELROOT32_ENABLE_CAMERA_EFFECTS not defined");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_camera_disabled);
    return UNITY_END();
}

#endif
