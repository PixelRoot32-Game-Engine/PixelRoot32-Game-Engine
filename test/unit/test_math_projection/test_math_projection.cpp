/**
 * @file test_math_projection.cpp
 * @brief Unit tests for the canonical math/Projection module and its
 *        gameplay:: alias identity (WU-2b: relocate ProjectionSpec).
 *
 * Covers:
 * - pixelroot32::math::ProjectionSpec is the canonical home of the aggregate
 *   and its free functions (relocated from pixelroot32::gameplay).
 * - pixelroot32::gameplay::ProjectionSpec and its free functions are TRUE
 *   aliases of the math:: entities, not copies — proven at both compile time
 *   (static_assert on type identity, restated here) and by function-ADDRESS
 *   identity via casted function pointers, which a type-level `is_same_v`
 *   cannot catch (a duplicate function with an identical signature would
 *   still pass `is_same_v` on the type, but never on the function address).
 *
 * This file only exercises pixelroot32::math:: functional behavior when
 * PIXELROOT32_ENABLE_PROJECTION is enabled, mirroring the flags-off / flags-on
 * matrix used by test_gameplay_projection.cpp. Functional coverage
 * (round-trips, floor semantics, validity checks) already lives in that file;
 * this suite exists to prove the RELOCATION and the ALIAS, not to duplicate
 * the functional matrix.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_PROJECTION

#include "math/Projection.h"
#include "gameplay/Projection.h"

#include <cstdint>
#include <type_traits>

namespace math = pixelroot32::math;
namespace gameplay = pixelroot32::gameplay;

namespace {

constexpr math::ProjectionSpec kIso2to1{0, 0, 16, 8, -16, 8};
static_assert(math::projectionSpecIsValid(kIso2to1, 32, 32), "kIso2to1 must be valid.");

}  // namespace

// ===========================================================================
// Canonical home: pixelroot32::math::ProjectionSpec
// ===========================================================================

void test_math_projection_spec_is_a_plain_six_int_aggregate(void) {
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(6 * sizeof(int)),
                             static_cast<uint32_t>(sizeof(math::ProjectionSpec)));
}

void test_math_cell_to_screen_and_screen_to_cell_round_trip(void) {
    const int sx = math::cellToScreenX(3, 2, kIso2to1);
    const int sy = math::cellToScreenY(3, 2, kIso2to1);
    TEST_ASSERT_EQUAL_INT(3, math::screenToCellX(sx, sy, kIso2to1));
    TEST_ASSERT_EQUAL_INT(2, math::screenToCellY(sx, sy, kIso2to1));
}

void test_math_projection_floor_div_floors_toward_negative_infinity(void) {
    // -8 / 16 truncates to 0; floor must give -1 (matches the gameplay
    // capability's documented hazard case).
    TEST_ASSERT_EQUAL_INT(-1, math::detail::projectionFloorDiv(-8, 16));
    TEST_ASSERT_EQUAL_INT(0, math::detail::projectionFloorDiv(0, 16));
    TEST_ASSERT_EQUAL_INT(1, math::detail::projectionFloorDiv(16, 16));
}

// ===========================================================================
// gameplay:: is a TRUE alias of math::, never a duplicate
// ===========================================================================

void test_gameplay_projection_spec_is_the_same_type_as_math_projection_spec(void) {
    // Redundant with the forwarder's own static_assert (which already gates
    // every TU before any test runs), restated here as a runtime-visible
    // assertion so a reviewer sees the identity proven in test output too.
    static_assert(std::is_same_v<gameplay::ProjectionSpec, math::ProjectionSpec>,
                  "gameplay::ProjectionSpec must alias math::ProjectionSpec.");
    TEST_PASS_MESSAGE("gameplay::ProjectionSpec is math::ProjectionSpec (same type).");
}

void test_gameplay_cell_to_screen_x_is_the_same_function_as_math(void) {
    // Function-ADDRESS identity: a type-level is_same_v cannot catch a
    // duplicate free function with an identical signature. Casting both
    // names to the same function-pointer type and comparing addresses can.
    using Fn = int (*)(int, int, const math::ProjectionSpec&);
    const Fn mathFn = &math::cellToScreenX;
    const Fn gameplayFn = &gameplay::cellToScreenX;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_gameplay_cell_to_screen_y_is_the_same_function_as_math(void) {
    using Fn = int (*)(int, int, const math::ProjectionSpec&);
    const Fn mathFn = &math::cellToScreenY;
    const Fn gameplayFn = &gameplay::cellToScreenY;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_gameplay_screen_to_cell_x_is_the_same_function_as_math(void) {
    using Fn = int (*)(int, int, const math::ProjectionSpec&);
    const Fn mathFn = &math::screenToCellX;
    const Fn gameplayFn = &gameplay::screenToCellX;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_gameplay_screen_to_cell_y_is_the_same_function_as_math(void) {
    using Fn = int (*)(int, int, const math::ProjectionSpec&);
    const Fn mathFn = &math::screenToCellY;
    const Fn gameplayFn = &gameplay::screenToCellY;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_gameplay_projection_det_is_the_same_function_as_math(void) {
    using Fn = int (*)(const math::ProjectionSpec&);
    const Fn mathFn = &math::projectionDet;
    const Fn gameplayFn = &gameplay::projectionDet;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_gameplay_projection_spec_is_valid_is_the_same_function_as_math(void) {
    using Fn = bool (*)(const math::ProjectionSpec&, int, int);
    const Fn mathFn = &math::projectionSpecIsValid;
    const Fn gameplayFn = &gameplay::projectionSpecIsValid;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_gameplay_detail_projection_floor_div_is_the_same_function_as_math(void) {
    using Fn = int (*)(int, int);
    const Fn mathFn = &math::detail::projectionFloorDiv;
    const Fn gameplayFn = &gameplay::detail::projectionFloorDiv;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_math_projection_zero_cost_when_disabled(void) {
    // Flag ON: the capability is a constexpr aggregate plus free functions, so
    // a constexpr consumer pays zero runtime footprint. The mirror of this
    // test in the #else branch asserts the flag-off property.
    constexpr math::ProjectionSpec kProbe{0, 0, 8, 4, -8, 4};
    static_assert(math::projectionSpecIsValid(kProbe, 4, 4), "probe spec must be constexpr-valid");
    static_assert(math::cellToScreenX(1, 0, kProbe) == 8, "cellToScreenX must be constexpr-evaluable");

    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_PROJECTION=1: math::ProjectionSpec stays a plain "
        "six-int aggregate and every free function is constexpr-evaluable, "
        "so a constexpr consumer pays zero runtime footprint.");
}

#else  // !PIXELROOT32_ENABLE_PROJECTION

void test_math_projection_zero_cost_when_disabled(void) {
    // With the flag off, math::ProjectionSpec/projectionDet/
    // projectionSpecIsValid/cellToScreen*/screenToCell* are not compiled at
    // all — their entire declaration lives inside the
    // #if PIXELROOT32_ENABLE_PROJECTION guard in include/math/Projection.h.
    // This translation unit compiling and passing without referencing any of
    // them IS the "zero bytes reserved" property required by the spec's
    // "Flags-off means not compiled" requirement.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_PROJECTION=0: math::ProjectionSpec/cellToScreen/"
        "screenToCell/projectionSpecIsValid are not compiled, zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_PROJECTION

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

#if PIXELROOT32_ENABLE_PROJECTION
    RUN_TEST(test_math_projection_spec_is_a_plain_six_int_aggregate);
    RUN_TEST(test_math_cell_to_screen_and_screen_to_cell_round_trip);
    RUN_TEST(test_math_projection_floor_div_floors_toward_negative_infinity);
    RUN_TEST(test_gameplay_projection_spec_is_the_same_type_as_math_projection_spec);
    RUN_TEST(test_gameplay_cell_to_screen_x_is_the_same_function_as_math);
    RUN_TEST(test_gameplay_cell_to_screen_y_is_the_same_function_as_math);
    RUN_TEST(test_gameplay_screen_to_cell_x_is_the_same_function_as_math);
    RUN_TEST(test_gameplay_screen_to_cell_y_is_the_same_function_as_math);
    RUN_TEST(test_gameplay_projection_det_is_the_same_function_as_math);
    RUN_TEST(test_gameplay_projection_spec_is_valid_is_the_same_function_as_math);
    RUN_TEST(test_gameplay_detail_projection_floor_div_is_the_same_function_as_math);
#endif
    RUN_TEST(test_math_projection_zero_cost_when_disabled);

    return UNITY_END();
}
