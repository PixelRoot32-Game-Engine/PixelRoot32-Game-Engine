#pragma once

#include <unity.h>
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
    InputConfig config(1, 10);
    TEST_ASSERT_EQUAL(1, config.count);
    
    #ifdef PLATFORM_NATIVE
    TEST_ASSERT_EQUAL(10, config.buttonNames[0]);
    #else
    TEST_ASSERT_EQUAL(10, config.inputPins[0]);
    #endif
}

void test_input_config_multiple_inputs(void) {
    InputConfig config(3, 10, 20, 30);
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
    InputConfig config(0);
    TEST_ASSERT_EQUAL(0, config.count);
}

void test_input_config_negative_count(void) {
    // Negative count should be treated as 0
    InputConfig config(-5, 10, 20);
    TEST_ASSERT_EQUAL(0, config.count);
}

// ============================================================================
// InputConfig Array Access Tests
// ============================================================================

void test_input_config_array_size_matches_count(void) {
    InputConfig config(4, 1, 2, 3, 4);
    
    #ifdef PLATFORM_NATIVE
    TEST_ASSERT_EQUAL(4, config.buttonNames.size());
    #else
    TEST_ASSERT_EQUAL(4, config.inputPins.size());
    #endif
}

void test_input_config_array_values_correct(void) {
    InputConfig config(2, 65, 66); // A and B keys
    
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
    InputConfig config(2, 10, 20);
    // Verify buttonNames is the correct type for native platform
    static_assert(std::is_same<decltype(config.buttonNames), std::vector<uint8_t>>::value, 
                  "buttonNames should be vector<uint8_t> on native");
}
#else
void test_input_config_esp32_pins_type(void) {
    InputConfig config(2, 10, 20);
    // Verify inputPins is the correct type for ESP32
    static_assert(std::is_same<decltype(config.inputPins), std::vector<int>>::value, 
                  "inputPins should be vector<int> on ESP32");
}
#endif

// ============================================================================
// Edge Cases
// ============================================================================

void test_input_config_large_count(void) {
    // Test with many inputs
    InputConfig config(10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    TEST_ASSERT_EQUAL(10, config.count);
}

void test_input_config_duplicate_values(void) {
    // Duplicate mappings should be allowed
    InputConfig config(3, 10, 10, 10);
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
