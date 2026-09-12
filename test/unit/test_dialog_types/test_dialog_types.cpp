/**
 * @file test_dialog_types.cpp
 * @brief Unit tests for gameplay/DialogTypes module
 *
 * Covers the spec requirement this slice formally owns:
 * - Flat header placement and flag wiring (dialog-runner capability, requirement 6)
 *
 * Scenario 2 of that requirement is worded for "DialogRunner-gated test files"
 * compiling only their #else TEST_PASS_MESSAGE branch when the flag is off.
 * DialogRunner does not exist yet in this slice (dialog/runner-types) -- only
 * DialogTypes.h ships here -- so this file adapts that wording to
 * "DialogTypes.h-gated test files" and proves the identical property for the
 * header this slice actually introduces. See sdd/dialog-mvp/tasks, Phase 2.
 *
 * Beyond that one formal requirement, this file also carries non-spec-mandated
 * static-layout regression guards backed by design section 6's field-by-field
 * arithmetic: sizeof(DialogChoice), sizeof(DialogLine), sizeof(DialogScript),
 * and std::is_trivially_destructible on DialogLine/DialogChoice. These pin
 * the flash/RAM cost of the script data model so a future field addition is a
 * conscious, reviewed bump rather than silent drift. DialogChoice and
 * DialogLine are documented in design section 4.2 with explicit dual figures
 * for both ESP32 (4-byte pointer) and 64-bit native (8-byte pointer); this
 * file asserts both. DialogScript's byte-exact figure (12 B) is documented in
 * design section 6 only as an ESP32 flash-budget number -- no native figure
 * is specified anywhere in the design -- so that concrete pin is scoped to
 * ESP32 builds only; on native, DialogScript's type and pointer/count fields
 * are still exercised (see DialogTypes.h's own generic
 * `sizeof(DialogChoice) <= 2 * sizeof(void*)` guard for the parallel pattern
 * applied to DialogChoice at the header level).
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_DIALOG is
 * enabled, since DialogTypes.h is entirely guarded behind that flag (see
 * include/gameplay/DialogTypes.h). This file therefore compiles cleanly in
 * BOTH the default (flag off) and opt-in (flag on) configurations, matching
 * the "no behavior change for existing examples" goal and the flags-off /
 * flags-on CI matrix (platformio.ini's native_test vs native_test_gameplay).
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_DIALOG

#include "gameplay/DialogTypes.h"

#include <cstdint>
#include <type_traits>

using namespace pixelroot32::gameplay;

// =============================================================================
// Static layout regression guards (design section 6's field-by-field
// arithmetic). ESP32 assumes a 4-byte pointer; native (this repo's only
// buildable/testable target in this environment) is 64-bit, 8-byte pointer.
// =============================================================================

#ifdef ESP32
static_assert(sizeof(DialogChoice) == 8,
              "DialogChoice must be 8 bytes on ESP32: text(4)+next(2)+tag(2), no padding.");
static_assert(sizeof(DialogLine) == 20,
              "DialogLine must be 20 bytes on ESP32: 4+4+2+2+2+1+1+1+1=18, aligned to 4.");
static_assert(sizeof(DialogScript) == 12,
              "DialogScript must be 12 bytes on ESP32: 4+4+2+2, no padding.");
#else
static_assert(sizeof(DialogChoice) == 16,
              "DialogChoice must be 16 bytes on 64-bit native: text(8)+next(2)+tag(2), padded to align 8.");
static_assert(sizeof(DialogLine) == 32,
              "DialogLine must be 32 bytes on 64-bit native: two 8-byte pointers plus 10 bytes "
              "of trailing fields, padded to align 8.");
// DialogScript has no documented native figure (design section 6 states the
// 12 B figure as an ESP32 flash-budget number only); no native static_assert
// is pinned here for that reason.
#endif

// =============================================================================
// Requirement: Flat header placement and flag wiring (adapted scenario 2)
// =============================================================================

void test_dialog_types_flag_on_compiles_functional_branch(void) {
    // With the flag on, DialogTypes.h's enums/structs are fully declared and
    // usable -- this whole translation unit compiling past the #if guard,
    // plus a real, non-trivial constant check below, IS the proof that the
    // functional branch (not the #else stub) compiled.
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, kNoLine);
}

// =============================================================================
// Sentinels and flag bits
// =============================================================================

void test_dialog_types_no_line_sentinel_is_0xffff(void) {
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, kNoLine);
}

void test_dialog_types_no_choice_sentinel_is_0xff(void) {
    TEST_ASSERT_EQUAL_HEX8(0xFF, kNoChoice);
}

void test_dialog_types_line_flag_allow_cancel_is_0x01(void) {
    TEST_ASSERT_EQUAL_HEX8(0x01, kLineFlagAllowCancel);
}

// =============================================================================
// Trivial destructibility (zero heap, .rodata-resident script data)
// =============================================================================

void test_dialog_types_dialog_line_and_choice_are_trivially_destructible(void) {
    static_assert(std::is_trivially_destructible<DialogLine>::value,
                  "DialogLine must be trivially destructible: script data lives in "
                  "flash and is never destroyed.");
    static_assert(std::is_trivially_destructible<DialogChoice>::value,
                  "DialogChoice must be trivially destructible for the same reason.");
    TEST_ASSERT_TRUE(std::is_trivially_destructible<DialogLine>::value);
    TEST_ASSERT_TRUE(std::is_trivially_destructible<DialogChoice>::value);
}

// =============================================================================
// Static-layout regression guards -- runtime mirrors of the static_asserts
// above, matching the engine's existing sizeof-guard convention (e.g.
// test_gameplay_grid_space.cpp, test_gameplay_state_machine.cpp).
// =============================================================================

void test_dialog_types_dialog_choice_size_guard(void) {
#ifdef ESP32
    TEST_ASSERT_EQUAL_UINT32(8u, static_cast<uint32_t>(sizeof(DialogChoice)));
#else
    TEST_ASSERT_EQUAL_UINT32(16u, static_cast<uint32_t>(sizeof(DialogChoice)));
#endif
}

void test_dialog_types_dialog_line_size_guard(void) {
#ifdef ESP32
    TEST_ASSERT_EQUAL_UINT32(20u, static_cast<uint32_t>(sizeof(DialogLine)));
#else
    TEST_ASSERT_EQUAL_UINT32(32u, static_cast<uint32_t>(sizeof(DialogLine)));
#endif
}

void test_dialog_types_dialog_script_size_guard(void) {
#ifdef ESP32
    TEST_ASSERT_EQUAL_UINT32(12u, static_cast<uint32_t>(sizeof(DialogScript)));
#else
    TEST_PASS_MESSAGE(
        "DialogScript's byte-exact regression pin (12 B) is an ESP32 "
        "flash-budget figure only (design section 6); no 64-bit native "
        "figure is documented, so no numeric assertion is pinned here.");
#endif
}

#else  // !PIXELROOT32_ENABLE_DIALOG

void test_dialog_types_flag_off_stub_compiles(void) {
    // With the flag off, every DialogTypes.h symbol lives entirely inside the
    // #if PIXELROOT32_ENABLE_DIALOG guard in include/gameplay/DialogTypes.h.
    // This translation unit compiling and passing without referencing any of
    // them IS the "zero bytes reserved" property required by the "Flat
    // header placement and flag wiring" requirement's flag-off scenario,
    // adapted here to DialogTypes.h since DialogRunner does not exist yet in
    // this slice.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_DIALOG=0: DialogTypes.h's enums and structs are "
        "not compiled, zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_DIALOG

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

#if PIXELROOT32_ENABLE_DIALOG
    RUN_TEST(test_dialog_types_flag_on_compiles_functional_branch);
    RUN_TEST(test_dialog_types_no_line_sentinel_is_0xffff);
    RUN_TEST(test_dialog_types_no_choice_sentinel_is_0xff);
    RUN_TEST(test_dialog_types_line_flag_allow_cancel_is_0x01);
    RUN_TEST(test_dialog_types_dialog_line_and_choice_are_trivially_destructible);
    RUN_TEST(test_dialog_types_dialog_choice_size_guard);
    RUN_TEST(test_dialog_types_dialog_line_size_guard);
    RUN_TEST(test_dialog_types_dialog_script_size_guard);
#else
    RUN_TEST(test_dialog_types_flag_off_stub_compiles);
#endif

    return UNITY_END();
}
