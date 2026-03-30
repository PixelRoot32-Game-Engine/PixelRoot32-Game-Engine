/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unified test runner for test_platforms - includes all platform tests
 */

#include <unity.h>
#include "../../test_config.h"
#include <platforms/mock/MockTiming.h>

// Include test headers
#include "test_platform_display.h"
#include "test_platform_input.h"
#include "test_platform_timer.h"

void setUp(void) {
    test_setup();
    // Setup for test_platform_timer tests
    pixelroot32::platforms::mock::g_mockTiming = new pixelroot32::platforms::mock::MockTimingProvider();
}

void tearDown(void) {
    delete pixelroot32::platforms::mock::g_mockTiming;
    pixelroot32::platforms::mock::g_mockTiming = nullptr;
    test_teardown();
}

int main() {
    UNITY_BEGIN();

    // Display Config Tests (22 tests)
    RUN_TEST(test_display_config_none_type);
    RUN_TEST(test_display_config_with_scaling);
    RUN_TEST(test_display_config_auto_logical_resolution);
    RUN_TEST(test_display_config_with_offsets);
    RUN_TEST(test_display_config_with_rotation);
    RUN_TEST(test_display_config_st7789_type);
    RUN_TEST(test_display_config_st7735_type);
    RUN_TEST(test_display_config_pin_configuration);
    RUN_TEST(test_display_config_hardware_i2c_flag);
    RUN_TEST(test_display_config_needs_scaling_true);
    RUN_TEST(test_display_config_needs_scaling_false);
    RUN_TEST(test_display_config_scale_x);
    RUN_TEST(test_display_config_scale_y);
    RUN_TEST(test_display_config_no_scaling_factor);
    RUN_TEST(test_display_config_width_alias);
    RUN_TEST(test_display_config_height_alias);
    RUN_TEST(test_display_config_move_constructor);
    RUN_TEST(test_display_config_move_assignment);
    RUN_TEST(test_display_config_copy_constructor);
    RUN_TEST(test_display_config_zero_resolution);
    RUN_TEST(test_display_config_unusual_rotation);

    // Input Config Tests (12 tests)
    RUN_TEST(test_input_config_default_constructor);
    RUN_TEST(test_input_config_single_input);
    RUN_TEST(test_input_config_multiple_inputs);
    RUN_TEST(test_input_config_zero_count);
    RUN_TEST(test_input_config_negative_count);
    RUN_TEST(test_input_config_array_size_matches_count);
    RUN_TEST(test_input_config_array_values_correct);
    #ifdef PLATFORM_NATIVE
    RUN_TEST(test_input_config_native_button_names_type);
    #else
    RUN_TEST(test_input_config_esp32_pins_type);
    #endif
    RUN_TEST(test_input_config_large_count);
    RUN_TEST(test_input_config_duplicate_values);

    // Timer Tests (16 tests)
    RUN_TEST(test_timer_initializes_at_zero);
    RUN_TEST(test_timer_advance_milliseconds);
    RUN_TEST(test_timer_advance_multiple_times);
    RUN_TEST(test_timer_set_time);
    RUN_TEST(test_timer_reset);
    RUN_TEST(test_timer_micros_advance);
    RUN_TEST(test_timer_micros_precision);
    RUN_TEST(test_mockMillis_returns_provider_time);
    RUN_TEST(test_mockMillis_returns_zero_without_provider);
    // Recreate mock timing after the above test deletes it
    delete pixelroot32::platforms::mock::g_mockTiming;
    pixelroot32::platforms::mock::g_mockTiming = new pixelroot32::platforms::mock::MockTimingProvider();
    RUN_TEST(test_timer_delta_calculation);
    RUN_TEST(test_timer_delta_multiple_frames);
    RUN_TEST(test_timer_advance_zero);
    RUN_TEST(test_timer_advance_large_value);
    RUN_TEST(test_timer_millis_monotonic);

    return UNITY_END();
}
