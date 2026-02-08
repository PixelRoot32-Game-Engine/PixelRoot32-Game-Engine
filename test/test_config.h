/**
 * @file test_config.h
 * @brief Shared test configuration and utilities for PixelRoot32 Game Engine tests
 * @version 1.0
 * @date 2026-02-08
 */

#pragma once

#include <unity.h>
#include <cmath>

// =============================================================================
// Float comparison utilities (for floating point tests)
// =============================================================================

#define FLOAT_EPSILON 0.0001f
#define DOUBLE_EPSILON 0.0000001

/**
 * @brief Compare two floats for equality within epsilon
 */
inline bool float_eq(float a, float b, float epsilon = FLOAT_EPSILON) {
    return std::fabs(a - b) < epsilon;
}

/**
 * @brief Compare two doubles for equality within epsilon
 */
inline bool double_eq(double a, double b, double epsilon = DOUBLE_EPSILON) {
    return std::fabs(a - b) < epsilon;
}

// =============================================================================
// Unity Test Framework Extensions
// =============================================================================

/**
 * @brief Assert that two floats are equal within epsilon
 */
#define TEST_ASSERT_FLOAT_EQUAL(expected, actual) \
    TEST_ASSERT_TRUE_MESSAGE(float_eq(expected, actual), \
        "Float values not equal within epsilon")

/**
 * @brief Assert that two floats are equal within custom epsilon
 */
#define TEST_ASSERT_FLOAT_EQUAL_EPS(expected, actual, eps) \
    TEST_ASSERT_TRUE_MESSAGE(float_eq(expected, actual, eps), \
        "Float values not equal within custom epsilon")

// =============================================================================
// Test Categories
// =============================================================================

/**
 * @brief Mark a test as implementation pending
 */
#define TEST_PENDING() \
    TEST_IGNORE_MESSAGE("Test implementation pending")

/**
 * @brief Mark a test that requires hardware
 */
#define TEST_REQUIRES_HARDWARE() \
    TEST_IGNORE_MESSAGE("Test requires hardware - skipped in native environment")

// =============================================================================
// Common Test Data
// =============================================================================

namespace test_data {
    // Common float values for testing
    constexpr float PI = 3.14159265f;
    constexpr float EPSILON = 0.0001f;
    
    // Screen dimensions for graphics tests
    constexpr int SCREEN_WIDTH = 240;
    constexpr int SCREEN_HEIGHT = 240;
    
    // Entity dimensions
    constexpr int DEFAULT_ENTITY_WIDTH = 32;
    constexpr int DEFAULT_ENTITY_HEIGHT = 32;
}

// =============================================================================
// Test Helpers
// =============================================================================

/**
 * @brief Setup function called before each test
 */
inline void test_setup(void) {
    // Global test setup - called before each test
}

/**
 * @brief Teardown function called after each test
 */
inline void test_teardown(void) {
    // Global test teardown - called after each test
}

// =============================================================================
// Platform Detection
// =============================================================================

#ifdef PLATFORM_NATIVE
    #define IS_NATIVE_TEST 1
    #define IS_ESP32_TEST 0
#else
    #define IS_NATIVE_TEST 0
    #define IS_ESP32_TEST 1
#endif
