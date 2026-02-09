/**
 * @file test_mathutil.cpp
 * @brief Unit tests for math/MathUtil module
 * @version 1.0
 * @date 2026-02-08
 * 
 * Tests for mathematical utility functions including:
 * - Linear interpolation (lerp)
 * - Value clamping
 * - Constants (PI, conversion factors)
 */

#include <unity.h>
#include "math/MathUtil.h"
#include "../../test_config.h"

using namespace pixelroot32::math;

// =============================================================================
// Setup / Teardown
// =============================================================================

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Tests for Math::lerp
// =============================================================================

/**
 * @test Basic lerp at 50% (t=0.5)
 * @expected Returns midpoint between a and b
 */
void test_mathutil_lerp_basic(void) {
    float result = Math::lerp(0.0f, 10.0f, 0.5f);
    TEST_ASSERT_FLOAT_EQUAL(5.0f, result);
}

/**
 * @test Lerp at start (t=0)
 * @expected Returns start value exactly
 */
void test_mathutil_lerp_start(void) {
    float result = Math::lerp(0.0f, 10.0f, 0.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, result);
}

/**
 * @test Lerp at end (t=1)
 * @expected Returns end value exactly
 */
void test_mathutil_lerp_end(void) {
    float result = Math::lerp(0.0f, 10.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
}

/**
 * @test Lerp at 25%
 * @expected Returns 25% of the way from a to b
 */
void test_mathutil_lerp_quarter(void) {
    float result = Math::lerp(0.0f, 10.0f, 0.25f);
    TEST_ASSERT_FLOAT_EQUAL(2.5f, result);
}

/**
 * @test Lerp at 75%
 * @expected Returns 75% of the way from a to b
 */
void test_mathutil_lerp_three_quarters(void) {
    float result = Math::lerp(0.0f, 10.0f, 0.75f);
    TEST_ASSERT_FLOAT_EQUAL(7.5f, result);
}

/**
 * @test Lerp with negative start value
 * @expected Correctly interpolates with negative numbers
 */
void test_mathutil_lerp_negative_start(void) {
    float result = Math::lerp(-10.0f, 10.0f, 0.5f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, result);
}

/**
 * @test Lerp with negative end value
 * @expected Correctly interpolates to negative numbers
 */
void test_mathutil_lerp_negative_end(void) {
    float result = Math::lerp(0.0f, -10.0f, 0.5f);
    TEST_ASSERT_EQUAL_FLOAT(-5.0f, result);
}

/**
 * @test Lerp with both negative values
 * @expected Correctly interpolates between negatives
 */
void test_mathutil_lerp_both_negative(void) {
    float result = Math::lerp(-20.0f, -10.0f, 0.5f);
    TEST_ASSERT_EQUAL_FLOAT(-15.0f, result);
}

/**
 * @test Lerp with t > 1 (extrapolation)
 * @expected Extrapolates beyond end value
 */
void test_mathutil_lerp_extrapolate_forward(void) {
    float result = Math::lerp(0.0f, 10.0f, 1.5f);
    TEST_ASSERT_EQUAL_FLOAT(15.0f, result);
}

/**
 * @test Lerp with t < 0 (extrapolation backward)
 * @expected Extrapolates before start value
 */
void test_mathutil_lerp_extrapolate_backward(void) {
    float result = Math::lerp(0.0f, 10.0f, -0.5f);
    TEST_ASSERT_EQUAL_FLOAT(-5.0f, result);
}

/**
 * @test Lerp with same start and end
 * @expected Returns that value for any t
 */
void test_mathutil_lerp_same_value(void) {
    float result = Math::lerp(5.0f, 5.0f, 0.5f);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, result);
}

/**
 * @test Lerp with very small range
 * @expected Handles small values correctly
 */
void test_mathutil_lerp_small_range(void) {
    float result = Math::lerp(0.0f, 0.001f, 0.5f);
    TEST_ASSERT_FLOAT_EQUAL(0.0005f, result);
}

/**
 * @test Lerp with large range
 * @expected Handles large values correctly
 */
void test_mathutil_lerp_large_range(void) {
    float result = Math::lerp(0.0f, 1000000.0f, 0.5f);
    TEST_ASSERT_EQUAL_FLOAT(500000.0f, result);
}

// =============================================================================
// Tests for Math::clamp
// =============================================================================

/**
 * @test Clamp value within range
 * @expected Returns value unchanged
 */
void test_mathutil_clamp_within_range(void) {
    float result = Math::clamp(5.0f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, result);
}

/**
 * @test Clamp value at minimum
 * @expected Returns minimum value exactly
 */
void test_mathutil_clamp_at_minimum(void) {
    float result = Math::clamp(0.0f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, result);
}

/**
 * @test Clamp value at maximum
 * @expected Returns maximum value exactly
 */
void test_mathutil_clamp_at_maximum(void) {
    float result = Math::clamp(10.0f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
}

/**
 * @test Clamp value below minimum
 * @expected Returns minimum value
 */
void test_mathutil_clamp_below_min(void) {
    float result = Math::clamp(-5.0f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, result);
}

/**
 * @test Clamp value above maximum
 * @expected Returns maximum value
 */
void test_mathutil_clamp_above_max(void) {
    float result = Math::clamp(15.0f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
}

/**
 * @test Clamp with negative range
 * @expected Works correctly with negative bounds
 */
void test_mathutil_clamp_negative_range(void) {
    float result = Math::clamp(0.0f, -10.0f, -5.0f);
    TEST_ASSERT_EQUAL_FLOAT(-5.0f, result);
}

/**
 * @test Clamp value in negative range
 * @expected Returns value unchanged when within negative range
 */
void test_mathutil_clamp_within_negative_range(void) {
    float result = Math::clamp(-7.0f, -10.0f, -5.0f);
    TEST_ASSERT_EQUAL_FLOAT(-7.0f, result);
}

/**
 * @test Clamp with zero range (min == max)
 * @expected Returns min/max value
 */
void test_mathutil_clamp_zero_range(void) {
    float result = Math::clamp(5.0f, 10.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
}

/**
 * @test Clamp with very small epsilon above max
 * @expected Returns max value
 */
void test_mathutil_clamp_epsilon_above_max(void) {
    float result = Math::clamp(10.0001f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
}

/**
 * @test Clamp with very small epsilon below min
 * @expected Returns min value
 */
void test_mathutil_clamp_epsilon_below_min(void) {
    float result = Math::clamp(-0.0001f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, result);
}

// =============================================================================
// Tests for Math Constants
// =============================================================================

/**
 * @test PI constant accuracy
 * @expected PI matches expected value
 */
void test_mathutil_pi_constant(void) {
    TEST_ASSERT_FLOAT_EQUAL(3.14159265f, Math::kPi);
}

/**
 * @test Degrees to radians conversion constant
 * @expected Conversion factor is correct
 */
void test_mathutil_deg_to_rad_constant(void) {
    float expected = 180.0f / Math::kPi;
    TEST_ASSERT_FLOAT_EQUAL(expected, Math::kDegToRad);
}

/**
 * @test Radians to degrees conversion constant
 * @expected Conversion factor is correct
 */
void test_mathutil_rad_to_deg_constant(void) {
    float expected = Math::kPi / 180.0f;
    TEST_ASSERT_FLOAT_EQUAL(expected, Math::kRadToDeg);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    
    // Lerp tests
    RUN_TEST(test_mathutil_lerp_basic);
    RUN_TEST(test_mathutil_lerp_start);
    RUN_TEST(test_mathutil_lerp_end);
    RUN_TEST(test_mathutil_lerp_quarter);
    RUN_TEST(test_mathutil_lerp_three_quarters);
    RUN_TEST(test_mathutil_lerp_negative_start);
    RUN_TEST(test_mathutil_lerp_negative_end);
    RUN_TEST(test_mathutil_lerp_both_negative);
    RUN_TEST(test_mathutil_lerp_extrapolate_forward);
    RUN_TEST(test_mathutil_lerp_extrapolate_backward);
    RUN_TEST(test_mathutil_lerp_same_value);
    RUN_TEST(test_mathutil_lerp_small_range);
    RUN_TEST(test_mathutil_lerp_large_range);
    
    // Clamp tests
    RUN_TEST(test_mathutil_clamp_within_range);
    RUN_TEST(test_mathutil_clamp_at_minimum);
    RUN_TEST(test_mathutil_clamp_at_maximum);
    RUN_TEST(test_mathutil_clamp_below_min);
    RUN_TEST(test_mathutil_clamp_above_max);
    RUN_TEST(test_mathutil_clamp_negative_range);
    RUN_TEST(test_mathutil_clamp_within_negative_range);
    RUN_TEST(test_mathutil_clamp_zero_range);
    RUN_TEST(test_mathutil_clamp_epsilon_above_max);
    RUN_TEST(test_mathutil_clamp_epsilon_below_min);
    
    // Constant tests
    RUN_TEST(test_mathutil_pi_constant);
    RUN_TEST(test_mathutil_deg_to_rad_constant);
    RUN_TEST(test_mathutil_rad_to_deg_constant);
    
    return UNITY_END();
}
