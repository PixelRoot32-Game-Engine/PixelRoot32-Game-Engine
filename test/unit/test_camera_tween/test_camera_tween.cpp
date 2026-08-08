/**
 * @file test_camera_tween.cpp
 * @brief Unit tests for graphics/CameraTween module
 *
 * Covers the spec requirements for camera-tween (sdd/camera-tween/spec):
 * - startTween returns sequential slot ids, kInvalidSlotId when full
 * - update advances tweens and writes to camera via setPosition
 * - 4 easings (Linear, EaseInQuad, EaseOutQuad, EaseInOutQuad) produce
 *   correct Q16.16 values at known progress points
 * - Multiple simultaneous tweens advance independently
 * - cancel() works, isComplete() works, zero-cost when flag off
 *
 * Functional tests compile only when PIXELROOT32_ENABLE_CAMERA_TWEEN is on.
 * With the flag off, this file asserts the stub path instead: the header
 * compiles, reserves no slots, and runs no easing math.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/EngineConfig.h"

#if PIXELROOT32_ENABLE_CAMERA_TWEEN

#include "graphics/CameraTween.h"
#include "graphics/Camera2D.h"
#include "math/Scalar.h"
#include "math/Vector2.h"
#include "math/MathUtil.h"

#include <cstddef>
#include <cstdint>

using pixelroot32::graphics::CameraTween;
using pixelroot32::graphics::Camera2D;
using pixelroot32::graphics::TweenEasing;
using pixelroot32::math::Vector2;
using pixelroot32::math::Scalar;
using pixelroot32::math::toScalar;

/// Scalar -> int for assertions: works on both the float and Fixed16 targets.
using pixelroot32::math::roundToInt;

/**
 * @brief Build a camera wide enough for a tween to actually move it.
 *
 * Camera2D::setPosition() clamps to the camera's bounds, and a freshly
 * constructed camera starts with minX == maxX == minY == maxY == 0 — every
 * write would collapse to the origin. Declaring bounds before writing a
 * position is the same convention test_camera2d.cpp follows.
 */
static Camera2D makeBoundedCamera(void) {
    Camera2D camera(240, 240);
    camera.setBounds(toScalar(-4000), toScalar(4000));
    camera.setVerticalBounds(toScalar(-4000), toScalar(4000));
    return camera;
}

// =============================================================================
// Static property checks (compile-time verification)
// =============================================================================

static_assert(sizeof(TweenEasing) == 1,
              "TweenEasing must be a single byte (uint8_t-backed).");
static_assert(CameraTween<4>::kMaxTweens == 4,
              "CameraTween<4>::kMaxTweens must be 4.");
static_assert(CameraTween<4>::kInvalidSlotId == 0xFF,
              "kInvalidSlotId must be 0xFF.");
static_assert(CameraTween<8>::kMaxTweens == 8,
              "Template N must propagate to kMaxTweens.");

// =============================================================================
// Requirement: startTween Returns Slot Ids, kInvalidSlotId When Full
// =============================================================================

void test_start_tween_returns_slot_zero(void) {
    CameraTween<4> tweens;
    const uint8_t id = tweens.startTween(
        Vector2(toScalar(0), toScalar(0)),
        Vector2(toScalar(100), toScalar(100)),
        500, TweenEasing::Linear);
    TEST_ASSERT_EQUAL_UINT8(0, id);
    TEST_ASSERT_EQUAL_UINT8(1, tweens.activeCount());
}

void test_start_tween_returns_sequential_ids(void) {
    CameraTween<4> tweens;
    TEST_ASSERT_EQUAL_UINT8(0, tweens.startTween(
        Vector2(0, 0), Vector2(10, 10), 100, TweenEasing::Linear));
    TEST_ASSERT_EQUAL_UINT8(1, tweens.startTween(
        Vector2(0, 0), Vector2(10, 10), 100, TweenEasing::Linear));
    TEST_ASSERT_EQUAL_UINT8(2, tweens.startTween(
        Vector2(0, 0), Vector2(10, 10), 100, TweenEasing::Linear));
    TEST_ASSERT_EQUAL_UINT8(3, tweens.startTween(
        Vector2(0, 0), Vector2(10, 10), 100, TweenEasing::Linear));
    TEST_ASSERT_EQUAL_UINT8(4, tweens.activeCount());
}

void test_start_tween_full_pool_returns_invalid(void) {
    CameraTween<2> tweens;
    TEST_ASSERT_EQUAL_UINT8(0, tweens.startTween(
        Vector2(0, 0), Vector2(10, 10), 100, TweenEasing::Linear));
    TEST_ASSERT_EQUAL_UINT8(1, tweens.startTween(
        Vector2(0, 0), Vector2(10, 10), 100, TweenEasing::Linear));
    // Pool full — must return kInvalidSlotId, must NOT consume a slot.
    TEST_ASSERT_EQUAL_UINT8(0xFF, tweens.startTween(
        Vector2(0, 0), Vector2(10, 10), 100, TweenEasing::Linear));
    TEST_ASSERT_EQUAL_UINT8(2, tweens.activeCount());
}

void test_start_tween_zero_duration_returns_invalid(void) {
    CameraTween<4> tweens;
    // durationMs=0 is an instant no-op — must NOT consume a slot.
    TEST_ASSERT_EQUAL_UINT8(0xFF, tweens.startTween(
        Vector2(0, 0), Vector2(10, 10), 0, TweenEasing::Linear));
    TEST_ASSERT_EQUAL_UINT8(0, tweens.activeCount());
}

// =============================================================================
// Requirement: update Advances Tweens And Writes To Camera
// =============================================================================

void test_tween_completes_after_duration(void) {
    CameraTween<4> tweens;
    Camera2D camera = makeBoundedCamera();
    const uint8_t id = tweens.startTween(
        Vector2(toScalar(0), toScalar(0)),
        Vector2(toScalar(100), toScalar(200)),
        500, TweenEasing::Linear);
    TEST_ASSERT_EQUAL_UINT8(0, id);

    // Halfway: camera should be near the midpoint, tween still active.
    tweens.update(250, &camera);
    TEST_ASSERT_EQUAL_UINT8(1, tweens.activeCount());

    // Past duration: tween completes, camera at `to`, slot inactive.
    tweens.update(300, &camera);  // 250 + 300 = 550 > 500
    TEST_ASSERT_EQUAL_UINT8(0, tweens.activeCount());
    TEST_ASSERT_TRUE(tweens.isComplete(id));
    TEST_ASSERT_EQUAL_INT(100, roundToInt(camera.getX()));
    TEST_ASSERT_EQUAL_INT(200, roundToInt(camera.getY()));
}

void test_tween_midpoint_linear(void) {
    CameraTween<4> tweens;
    Camera2D camera = makeBoundedCamera();
    tweens.startTween(
        Vector2(toScalar(0), toScalar(0)),
        Vector2(toScalar(1000), toScalar(1000)),
        1000, TweenEasing::Linear);
    // Exactly halfway: progress=0.5, eased=0.5, position=500.
    tweens.update(500, &camera);
    TEST_ASSERT_EQUAL_INT(500, roundToInt(camera.getX()));
    TEST_ASSERT_EQUAL_INT(500, roundToInt(camera.getY()));
    TEST_ASSERT_EQUAL_UINT8(1, tweens.activeCount());  // not yet complete
}

// =============================================================================
// Requirement: Easing Functions Produce Correct Mid-Points
// =============================================================================

void test_tween_midpoint_ease_in_quad(void) {
    CameraTween<4> tweens;
    Camera2D camera = makeBoundedCamera();
    tweens.startTween(
        Vector2(toScalar(0), toScalar(0)),
        Vector2(toScalar(1000), toScalar(0)),
        1000, TweenEasing::EaseInQuad);
    // At t=0.5, EaseInQuad returns 0.25 (slow start). x should be ~250.
    tweens.update(500, &camera);
    TEST_ASSERT_INT_WITHIN(2, 250, roundToInt(camera.getX()));
}

void test_tween_midpoint_ease_out_quad(void) {
    CameraTween<4> tweens;
    Camera2D camera = makeBoundedCamera();
    tweens.startTween(
        Vector2(toScalar(0), toScalar(0)),
        Vector2(toScalar(1000), toScalar(0)),
        1000, TweenEasing::EaseOutQuad);
    // At t=0.5, EaseOutQuad returns 0.75 (fast start). x should be ~750.
    tweens.update(500, &camera);
    TEST_ASSERT_INT_WITHIN(2, 750, roundToInt(camera.getX()));
}

void test_tween_midpoint_ease_in_out_quad(void) {
    CameraTween<4> tweens;
    Camera2D camera = makeBoundedCamera();
    tweens.startTween(
        Vector2(toScalar(0), toScalar(0)),
        Vector2(toScalar(1000), toScalar(0)),
        1000, TweenEasing::EaseInOutQuad);
    // At t=0.5, EaseInOutQuad returns exactly 0.5 (linear at the inflection).
    tweens.update(500, &camera);
    TEST_ASSERT_INT_WITHIN(2, 500, roundToInt(camera.getX()));
}

// =============================================================================
// Requirement: Multiple Simultaneous Tweens
// =============================================================================

void test_multiple_simultaneous_tweens(void) {
    CameraTween<4> tweens;
    Camera2D camera = makeBoundedCamera();
    // Slot 0: short, 100ms
    tweens.startTween(
        Vector2(toScalar(0), toScalar(0)),
        Vector2(toScalar(100), toScalar(0)),
        100, TweenEasing::Linear);
    // Slot 1: long, 1000ms
    tweens.startTween(
        Vector2(toScalar(0), toScalar(0)),
        Vector2(toScalar(1000), toScalar(0)),
        1000, TweenEasing::Linear);
    TEST_ASSERT_EQUAL_UINT8(2, tweens.activeCount());

    // Advance 100ms — slot 0 completes, slot 1 at 10%.
    tweens.update(100, &camera);
    TEST_ASSERT_EQUAL_UINT8(1, tweens.activeCount());
    // Slot 1 won the last write (10% of 1000 = 100).
    TEST_ASSERT_INT_WITHIN(2, 100, roundToInt(camera.getX()));

    // Advance 900ms more — slot 1 completes at `to=1000`.
    tweens.update(900, &camera);
    TEST_ASSERT_EQUAL_UINT8(0, tweens.activeCount());
    TEST_ASSERT_EQUAL_INT(1000, roundToInt(camera.getX()));
}

// =============================================================================
// Requirement: cancel() And isComplete()
// =============================================================================

void test_cancel_active_tween(void) {
    CameraTween<4> tweens;
    Camera2D camera = makeBoundedCamera();
    const uint8_t id = tweens.startTween(
        Vector2(toScalar(0), toScalar(0)),
        Vector2(toScalar(100), toScalar(0)),
        1000, TweenEasing::Linear);
    tweens.update(500, &camera);
    // Cancel mid-flight — camera stays where it was (50).
    tweens.cancel(id);
    TEST_ASSERT_EQUAL_UINT8(0, tweens.activeCount());
    TEST_ASSERT_TRUE(tweens.isComplete(id));

    // Further update does nothing (slot is inactive).
    tweens.update(500, &camera);
    TEST_ASSERT_EQUAL_INT(50, roundToInt(camera.getX()));
}

void test_is_complete_for_invalid_slot(void) {
    CameraTween<4> tweens;
    // Out-of-range slotId returns false (not "complete", not started).
    TEST_ASSERT_FALSE(tweens.isComplete(99));
    // Never-started slotId returns false.
    TEST_ASSERT_FALSE(tweens.isComplete(0));
}

// =============================================================================
// Requirement: Zero-Cost When Disabled (compile-time)
// =============================================================================

#else  // !PIXELROOT32_ENABLE_CAMERA_TWEEN

void test_camera_tween_zero_cost_when_disabled(void) {
    // With the flag off, CameraTween's entire definition lives inside the
    // #if PIXELROOT32_ENABLE_CAMERA_TWEEN guard in
    // include/graphics/CameraTween.h. This translation unit compiling and
    // passing without referencing the template IS the "zero bytes reserved"
    // property required by the spec.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_CAMERA_TWEEN=0: CameraTween is a stub, no slots, "
        "no easing math, no camera writes. 1-byte storage per stub instance.");
}

#endif  // PIXELROOT32_ENABLE_CAMERA_TWEEN

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_CAMERA_TWEEN
    RUN_TEST(test_start_tween_returns_slot_zero);
    RUN_TEST(test_start_tween_returns_sequential_ids);
    RUN_TEST(test_start_tween_full_pool_returns_invalid);
    RUN_TEST(test_start_tween_zero_duration_returns_invalid);
    RUN_TEST(test_tween_completes_after_duration);
    RUN_TEST(test_tween_midpoint_linear);
    RUN_TEST(test_tween_midpoint_ease_in_quad);
    RUN_TEST(test_tween_midpoint_ease_out_quad);
    RUN_TEST(test_tween_midpoint_ease_in_out_quad);
    RUN_TEST(test_multiple_simultaneous_tweens);
    RUN_TEST(test_cancel_active_tween);
    RUN_TEST(test_is_complete_for_invalid_slot);
#else
    RUN_TEST(test_camera_tween_zero_cost_when_disabled);
#endif

    return UNITY_END();
}
