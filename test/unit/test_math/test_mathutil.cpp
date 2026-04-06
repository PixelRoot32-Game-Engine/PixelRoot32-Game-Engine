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
    float result = lerp(0.0f, 10.0f, 0.5f);
    TEST_ASSERT_FLOAT_EQUAL(5.0f, result);
}

/**
 * @test Lerp at start (t=0)
 * @expected Returns start value exactly
 */
void test_mathutil_lerp_start(void) {
    float result = lerp(0.0f, 10.0f, 0.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, result);
}

/**
 * @test Lerp at end (t=1)
 * @expected Returns end value exactly
 */
void test_mathutil_lerp_end(void) {
    float result = lerp(0.0f, 10.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
}

/**
 * @test Lerp at 25%
 * @expected Returns 25% of the way from a to b
 */
void test_mathutil_lerp_quarter(void) {
    float result = lerp(0.0f, 10.0f, 0.25f);
    TEST_ASSERT_FLOAT_EQUAL(2.5f, result);
}

/**
 * @test Lerp at 75%
 * @expected Returns 75% of the way from a to b
 */
void test_mathutil_lerp_three_quarters(void) {
    float result = lerp(0.0f, 10.0f, 0.75f);
    TEST_ASSERT_FLOAT_EQUAL(7.5f, result);
}

/**
 * @test Lerp with negative start value
 * @expected Correctly interpolates with negative numbers
 */
void test_mathutil_lerp_negative_start(void) {
    float result = lerp(-10.0f, 10.0f, 0.5f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, result);
}

/**
 * @test Lerp with negative end value
 * @expected Correctly interpolates to negative numbers
 */
void test_mathutil_lerp_negative_end(void) {
    float result = lerp(0.0f, -10.0f, 0.5f);
    TEST_ASSERT_EQUAL_FLOAT(-5.0f, result);
}

/**
 * @test Lerp with both negative values
 * @expected Correctly interpolates between negatives
 */
void test_mathutil_lerp_both_negative(void) {
    float result = lerp(-20.0f, -10.0f, 0.5f);
    TEST_ASSERT_EQUAL_FLOAT(-15.0f, result);
}

/**
 * @test Lerp with t > 1 (extrapolation)
 * @expected Extrapolates beyond end value
 */
void test_mathutil_lerp_extrapolate_forward(void) {
    float result = lerp(0.0f, 10.0f, 1.5f);
    TEST_ASSERT_EQUAL_FLOAT(15.0f, result);
}

/**
 * @test Lerp with t < 0 (extrapolation backward)
 * @expected Extrapolates before start value
 */
void test_mathutil_lerp_extrapolate_backward(void) {
    float result = lerp(0.0f, 10.0f, -0.5f);
    TEST_ASSERT_EQUAL_FLOAT(-5.0f, result);
}

/**
 * @test Lerp with same start and end
 * @expected Returns that value for any t
 */
void test_mathutil_lerp_same_value(void) {
    float result = lerp(5.0f, 5.0f, 0.5f);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, result);
}

/**
 * @test Lerp with very small range
 * @expected Handles small values correctly
 */
void test_mathutil_lerp_small_range(void) {
    float result = lerp(0.0f, 0.001f, 0.5f);
    TEST_ASSERT_FLOAT_EQUAL(0.0005f, result);
}

/**
 * @test Lerp with large range
 * @expected Handles large values correctly
 */
void test_mathutil_lerp_large_range(void) {
    float result = lerp(0.0f, 1000000.0f, 0.5f);
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
    float result = clamp(5.0f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, result);
}

/**
 * @test Clamp value at minimum
 * @expected Returns minimum value exactly
 */
void test_mathutil_clamp_at_minimum(void) {
    float result = clamp(0.0f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, result);
}

/**
 * @test Clamp value at maximum
 * @expected Returns maximum value exactly
 */
void test_mathutil_clamp_at_maximum(void) {
    float result = clamp(10.0f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
}

/**
 * @test Clamp value below minimum
 * @expected Returns minimum value
 */
void test_mathutil_clamp_below_min(void) {
    float result = clamp(-5.0f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, result);
}

/**
 * @test Clamp value above maximum
 * @expected Returns maximum value
 */
void test_mathutil_clamp_above_max(void) {
    float result = clamp(15.0f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
}

/**
 * @test Clamp with negative range
 * @expected Works correctly with negative bounds
 */
void test_mathutil_clamp_negative_range(void) {
    float result = clamp(0.0f, -10.0f, -5.0f);
    TEST_ASSERT_EQUAL_FLOAT(-5.0f, result);
}

/**
 * @test Clamp value in negative range
 * @expected Returns value unchanged when within negative range
 */
void test_mathutil_clamp_within_negative_range(void) {
    float result = clamp(-7.0f, -10.0f, -5.0f);
    TEST_ASSERT_EQUAL_FLOAT(-7.0f, result);
}

/**
 * @test Clamp with zero range (min == max)
 * @expected Returns min/max value
 */
void test_mathutil_clamp_zero_range(void) {
    float result = clamp(5.0f, 10.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
}

/**
 * @test Clamp with very small epsilon above max
 * @expected Returns max value
 */
void test_mathutil_clamp_epsilon_above_max(void) {
    float result = clamp(10.0001f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
}

/**
 * @test Clamp with very small epsilon below min
 * @expected Returns min value
 */
void test_mathutil_clamp_epsilon_below_min(void) {
    float result = clamp(-0.0001f, 0.0f, 10.0f);
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
    TEST_ASSERT_FLOAT_EQUAL(3.14159265f, kPi);
}

/**
 * @test Degrees to radians conversion constant
 * @expected Conversion factor is correct
 */
void test_mathutil_deg_to_rad_constant(void) {
    float expected = 3.14159265f / 180.0f;
    TEST_ASSERT_FLOAT_EQUAL(expected, kDegToRad);
}

/**
 * @test Radians to degrees conversion constant
 * @expected Conversion factor is correct
 */
void test_mathutil_rad_to_deg_constant(void) {
    float expected = 180.0f / 3.14159265f;
    TEST_ASSERT_FLOAT_EQUAL(expected, kRadToDeg);
}

// =============================================================================
// Tests for Math::abs
// =============================================================================

void test_mathutil_abs_positive(void) {
    TEST_ASSERT_EQUAL_FLOAT(5.0f, pixelroot32::math::abs(5.0f));
}

void test_mathutil_abs_negative(void) {
    TEST_ASSERT_EQUAL_FLOAT(5.0f, pixelroot32::math::abs(-5.0f));
}

void test_mathutil_abs_zero(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pixelroot32::math::abs(0.0f));
}

// =============================================================================
// Tests for Math::sign
// =============================================================================

void test_mathutil_sign_positive(void) {
    TEST_ASSERT_EQUAL_FLOAT(1.0f, sign(10.0f));
}

void test_mathutil_sign_negative(void) {
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, sign(-10.0f));
}

void test_mathutil_sign_zero(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, sign(0.0f));
}

// =============================================================================
// Tests for Math::is_equal_approx
// =============================================================================

void test_mathutil_is_equal_approx_exact(void) {
    TEST_ASSERT_TRUE(is_equal_approx(1.0f, 1.0f));
}

void test_mathutil_is_equal_approx_close(void) {
    TEST_ASSERT_TRUE(is_equal_approx(1.0f, 1.000001f));
}

void test_mathutil_is_equal_approx_not_close(void) {
    TEST_ASSERT_FALSE(is_equal_approx(1.0f, 1.1f));
}

// =============================================================================
// Tests for Math::is_zero_approx
// =============================================================================

void test_mathutil_is_zero_approx_zero(void) {
    TEST_ASSERT_TRUE(is_zero_approx(0.0f));
}

void test_mathutil_is_zero_approx_close(void) {
    TEST_ASSERT_TRUE(is_zero_approx(0.000001f));
}

void test_mathutil_is_zero_approx_not_close(void) {
    TEST_ASSERT_FALSE(is_zero_approx(0.1f));
}

// =============================================================================
// Tests for Math::atan2
// =============================================================================

void test_mathutil_atan2_basic(void) {
    // atan2(0, 1) = 0
    TEST_ASSERT_FLOAT_WITHIN(kEpsilon, 0.0f, pixelroot32::math::atan2(0.0f, 1.0f));
    // atan2(1, 0) = PI/2
    TEST_ASSERT_FLOAT_WITHIN(kEpsilon, kPi / 2.0f, pixelroot32::math::atan2(1.0f, 0.0f));
    // atan2(0, -1) = PI
    TEST_ASSERT_FLOAT_WITHIN(kEpsilon, kPi, pixelroot32::math::atan2(0.0f, -1.0f));
    // atan2(-1, 0) = -PI/2
    TEST_ASSERT_FLOAT_WITHIN(kEpsilon, -kPi / 2.0f, pixelroot32::math::atan2(-1.0f, 0.0f));
}

// =============================================================================
// Tests for PRNG (Pseudo-Random Number Generator)
// =============================================================================

/**
 * @test PRNG deterministic behavior with same seed
 * @expected Same seed produces identical sequences
 */
void test_prng_deterministic_same_seed(void) {
    set_seed(12345);
    float val1 = rand01();
    float val2 = rand01();
    float val3 = rand01();
    
    set_seed(12345);
    float val1b = rand01();
    float val2b = rand01();
    float val3b = rand01();
    
    TEST_ASSERT_EQUAL_FLOAT(val1, val1b);
    TEST_ASSERT_EQUAL_FLOAT(val2, val2b);
    TEST_ASSERT_EQUAL_FLOAT(val3, val3b);
}

/**
 * @test PRNG different seeds produce different sequences
 * @expected Different seeds produce different first values
 */
void test_prng_different_seeds(void) {
    set_seed(12345);
    float val1 = rand01();
    
    set_seed(67890);
    float val2 = rand01();
    
    TEST_ASSERT_NOT_EQUAL(val1, val2);
}

/**
 * @test Global RNG and Random struct produce identical sequences from same seed
 * @expected Both produce the same values when initialized with same seed
 */
void test_prng_global_vs_random_struct(void) {
    set_seed(54321);
    float global1 = rand01();
    float global2 = rand01();
    
    Random rng(54321);
    float rng1 = rng.rand01();
    float rng2 = rng.rand01();
    
    TEST_ASSERT_EQUAL_FLOAT(global1, rng1);
    TEST_ASSERT_EQUAL_FLOAT(global2, rng2);
}

/**
 * @test Random struct instances are independent
 * @expected Each instance has its own state
 */
void test_prng_random_struct_independent(void) {
    Random rng1(11111);
    Random rng2(22222);
    
    float val1a = rng1.rand01();
    float val2a = rng2.rand01();
    
    // Both should produce different values (different seeds)
    TEST_ASSERT_NOT_EQUAL(val1a, val2a);
    
    // Continue generating from rng1 only
    float val1b = rng1.rand01();
    
    // rng2's next value should still be different from rng1's sequence
    float val2b = rng2.rand01();
    TEST_ASSERT_NOT_EQUAL(val1b, val2b);
}

/**
 * @test rand_int returns values within specified range
 * @expected All values are within [min, max]
 */
void test_prng_rand_int_range(void) {
    set_seed(99999);
    
    for (int i = 0; i < 100; i++) {
        int val = rand_int(1, 6);  // 6-sided die
        TEST_ASSERT_TRUE(val >= 1 && val <= 6);
    }
}

/**
 * @test rand_int distribution uniformity (basic check)
 * @expected Values are reasonably distributed across range
 */
void test_prng_rand_int_distribution(void) {
    set_seed(77777);
    
    int counts[6] = {0};  // For range 1-6
    const int iterations = 6000;
    
    for (int i = 0; i < iterations; i++) {
        int val = rand_int(1, 6);
        counts[val - 1]++;
    }
    
    // Each value should appear approximately 1000 times
    // Allow 20% tolerance (800-1200)
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_TRUE(counts[i] >= 800 && counts[i] <= 1200);
    }
}

/**
 * @test rand01 returns values in [0, 1] range
 * @expected All values are within valid range
 */
void test_prng_rand01_range(void) {
    set_seed(55555);
    
    for (int i = 0; i < 100; i++) {
        float val = rand01();
        TEST_ASSERT_TRUE(val >= 0.0f && val <= 1.0f);
    }
}

/**
 * @test rand_chance with probability 0 always returns false
 * @expected Always false
 */
void test_prng_rand_chance_zero(void) {
    set_seed(44444);
    
    for (int i = 0; i < 50; i++) {
        TEST_ASSERT_FALSE(rand_chance(0.0f));
    }
}

/**
 * @test rand_chance with probability 1 always returns true
 * @expected Always true
 */
void test_prng_rand_chance_one(void) {
    set_seed(33333);
    
    for (int i = 0; i < 50; i++) {
        TEST_ASSERT_TRUE(rand_chance(1.0f));
    }
}

/**
 * @test rand_sign returns only -1 or 1
 * @expected All values are either -1 or 1
 */
void test_prng_rand_sign_values(void) {
    set_seed(22222);
    
    for (int i = 0; i < 100; i++) {
        float val = rand_sign();
        TEST_ASSERT_TRUE(val == 1.0f || val == -1.0f);
    }
}

/**
 * @test rand_sign distribution (approx 50/50)
 * @expected Roughly equal number of -1 and 1
 */
void test_prng_rand_sign_distribution(void) {
    set_seed(11111);
    
    int positive = 0;
    int negative = 0;
    const int iterations = 1000;
    
    for (int i = 0; i < iterations; i++) {
        float val = rand_sign();
        if (val == 1.0f) positive++;
        else negative++;
    }
    
    // Should be roughly 50/50, allow 30% tolerance
    TEST_ASSERT_TRUE(positive >= 350 && positive <= 650);
    TEST_ASSERT_TRUE(negative >= 350 && negative <= 650);
}

/**
 * @test seed 0 uses fallback value
 * @expected set_seed(0) produces valid random sequence
 */
void test_prng_seed_zero_fallback(void) {
    set_seed(0);  // Should use fallback 0xDEADBEEF
    float val1 = rand01();
    
    // Verify we got a valid value in range
    TEST_ASSERT_TRUE(val1 >= 0.0f && val1 <= 1.0f);
    
    // Verify it's not all zeros (which would indicate broken state)
    TEST_ASSERT_NOT_EQUAL(0.0f, val1);
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

    // Abs tests
    RUN_TEST(test_mathutil_abs_positive);
    RUN_TEST(test_mathutil_abs_negative);
    RUN_TEST(test_mathutil_abs_zero);

    // Sign tests
    RUN_TEST(test_mathutil_sign_positive);
    RUN_TEST(test_mathutil_sign_negative);
    RUN_TEST(test_mathutil_sign_zero);

    // Approximation tests
    RUN_TEST(test_mathutil_is_equal_approx_exact);
    RUN_TEST(test_mathutil_is_equal_approx_close);
    RUN_TEST(test_mathutil_is_equal_approx_not_close);
    RUN_TEST(test_mathutil_is_zero_approx_zero);
    RUN_TEST(test_mathutil_is_zero_approx_close);
    RUN_TEST(test_mathutil_is_zero_approx_not_close);

    // Atan2 tests
    RUN_TEST(test_mathutil_atan2_basic);
    
    // PRNG tests
    RUN_TEST(test_prng_deterministic_same_seed);
    RUN_TEST(test_prng_different_seeds);
    RUN_TEST(test_prng_global_vs_random_struct);
    RUN_TEST(test_prng_random_struct_independent);
    RUN_TEST(test_prng_rand_int_range);
    RUN_TEST(test_prng_rand_int_distribution);
    RUN_TEST(test_prng_rand01_range);
    RUN_TEST(test_prng_rand_chance_zero);
    RUN_TEST(test_prng_rand_chance_one);
    RUN_TEST(test_prng_rand_sign_values);
    RUN_TEST(test_prng_rand_sign_distribution);
    RUN_TEST(test_prng_seed_zero_fallback);
    
    return UNITY_END();
}
