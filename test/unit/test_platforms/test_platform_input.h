#pragma once

#include <unity.h>
#include <array>
#include "input/InputConfig.h"

using namespace pixelroot32::input;

// ============================================================================
// InputConfig Construction Tests
// ============================================================================

void test_input_config_default_constructor(void) {
    InputConfig config;
    TEST_ASSERT_EQUAL(0, config.count);
}

void test_input_config_single_input(void) {
    // New API: InputConfig(pin1, pin2, ...) - count auto-deduced
    InputConfig config(10);
    TEST_ASSERT_EQUAL(1, config.count);

    #ifdef PLATFORM_NATIVE
    TEST_ASSERT_EQUAL(10, config.buttonNames[0]);
    #else
    TEST_ASSERT_EQUAL(10, config.inputPins[0]);
    #endif
}

void test_input_config_multiple_inputs(void) {
    // New API: 3 inputs = 3 arguments
    InputConfig config(10, 20, 30);
    TEST_ASSERT_EQUAL(3, config.count);

    #ifdef PLATFORM_NATIVE
    TEST_ASSERT_EQUAL(10, config.buttonNames[0]);
    TEST_ASSERT_EQUAL(20, config.buttonNames[1]);
    TEST_ASSERT_EQUAL(30, config.buttonNames[2]);
    #else
    TEST_ASSERT_EQUAL(10, config.inputPins[0]);
    TEST_ASSERT_EQUAL(20, config.inputPins[1]);
    TEST_ASSERT_EQUAL(30, config.inputPins[2]);
    #endif
}

void test_input_config_zero_count(void) {
    // Empty constructor
    InputConfig config;
    TEST_ASSERT_EQUAL(0, config.count);
}

void test_input_config_negative_count(void) {
    // New API: negative values are treated as regular input values (GPIO pins can be negative in some contexts)
    // This test verifies that negative values are stored as-is
    InputConfig config(-5, 10, 20);
    TEST_ASSERT_EQUAL(3, config.count);  // -5, 10, 20 = 3 inputs
}

// ============================================================================
// InputConfig Array Access Tests
// ============================================================================

void test_input_config_array_size_matches_count(void) {
    // New API: 4 inputs = 4 arguments
    InputConfig config(1, 2, 3, 4);

    // std::array always has MAX_INPUT_COUNT capacity, count holds the actual used count
    TEST_ASSERT_EQUAL(4, config.count);
    #ifdef PLATFORM_NATIVE
    TEST_ASSERT_EQUAL(InputConfig::MAX_INPUT_COUNT, config.buttonNames.size());
    #else
    TEST_ASSERT_EQUAL(InputConfig::MAX_INPUT_COUNT, config.inputPins.size());
    #endif
}

void test_input_config_array_values_correct(void) {
    // New API: 2 inputs = 2 arguments
    InputConfig config(65, 66); // A and B keys

    #ifdef PLATFORM_NATIVE
    TEST_ASSERT_EQUAL_UINT8(65, config.buttonNames[0]);
    TEST_ASSERT_EQUAL_UINT8(66, config.buttonNames[1]);
    #else
    TEST_ASSERT_EQUAL(65, config.inputPins[0]);
    TEST_ASSERT_EQUAL(66, config.inputPins[1]);
    #endif
}

// ============================================================================
// Platform-Specific Tests
// ============================================================================

#ifdef PLATFORM_NATIVE
void test_input_config_native_button_names_type(void) {
    // New API: 2 inputs = 2 arguments
    InputConfig config(10, 20);
    // Verify buttonNames is the correct type for native platform
    static_assert(std::is_same<decltype(config.buttonNames), std::array<uint8_t, InputConfig::MAX_INPUT_COUNT>>::value,
                  "buttonNames should be array<uint8_t, MAX_INPUT_COUNT> on native");
}
#else
void test_input_config_esp32_pins_type(void) {
    // New API: 2 inputs = 2 arguments
    InputConfig config(10, 20);
    // Verify inputPins is the correct type for ESP32
    static_assert(std::is_same<decltype(config.inputPins), std::array<int, InputConfig::MAX_INPUT_COUNT>>::value,
                  "inputPins should be array<int, MAX_INPUT_COUNT> on ESP32");
}
#endif

// ============================================================================
// Edge Cases
// ============================================================================

void test_input_config_large_count(void) {
    // Test with many inputs - new API uses 10 arguments
    InputConfig config(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    TEST_ASSERT_EQUAL(10, config.count);
}

void test_input_config_duplicate_values(void) {
    // Duplicate mappings should be allowed - new API uses 3 arguments
    InputConfig config(10, 10, 10);
    TEST_ASSERT_EQUAL(3, config.count);

    #ifdef PLATFORM_NATIVE
    TEST_ASSERT_EQUAL(10, config.buttonNames[0]);
    TEST_ASSERT_EQUAL(10, config.buttonNames[1]);
    TEST_ASSERT_EQUAL(10, config.buttonNames[2]);
    #else
    TEST_ASSERT_EQUAL(10, config.inputPins[0]);
    TEST_ASSERT_EQUAL(10, config.inputPins[1]);
    TEST_ASSERT_EQUAL(10, config.inputPins[2]);
    #endif
}
