/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Unit tests for platform timer abstractions
 */

#include <unity.h>
#include <platforms/mock/MockTiming.h>

void setUp(void) {
    // Create fresh mock timing provider for each test
    pixelroot32::platforms::mock::g_mockTiming = new pixelroot32::platforms::mock::MockTimingProvider();
}

void tearDown(void) {
    delete pixelroot32::platforms::mock::g_mockTiming;
    pixelroot32::platforms::mock::g_mockTiming = nullptr;
}

// ============================================================================
// Timer Initialization Tests
// ============================================================================

void test_timer_initializes_at_zero(void) {
    // Timer should start at 0
    TEST_ASSERT_EQUAL_UINT32(0, pixelroot32::platforms::mock::g_mockTiming->millis());
    TEST_ASSERT_EQUAL_UINT32(0, pixelroot32::platforms::mock::g_mockTiming->micros());
}

void test_timer_advance_milliseconds(void) {
    // Advance by 100ms
    pixelroot32::platforms::mock::g_mockTiming->advance(100);
    TEST_ASSERT_EQUAL_UINT32(100, pixelroot32::platforms::mock::g_mockTiming->millis());
    TEST_ASSERT_EQUAL_UINT32(100000, pixelroot32::platforms::mock::g_mockTiming->micros());
}

void test_timer_advance_multiple_times(void) {
    // Advance multiple times
    pixelroot32::platforms::mock::g_mockTiming->advance(50);
    pixelroot32::platforms::mock::g_mockTiming->advance(25);
    pixelroot32::platforms::mock::g_mockTiming->advance(25);
    TEST_ASSERT_EQUAL_UINT32(100, pixelroot32::platforms::mock::g_mockTiming->millis());
}

void test_timer_set_time(void) {
    // Set specific time
    pixelroot32::platforms::mock::g_mockTiming->setTime(1000);
    TEST_ASSERT_EQUAL_UINT32(1000, pixelroot32::platforms::mock::g_mockTiming->millis());
    TEST_ASSERT_EQUAL_UINT32(1000000, pixelroot32::platforms::mock::g_mockTiming->micros());
}

void test_timer_reset(void) {
    // Advance time, then reset
    pixelroot32::platforms::mock::g_mockTiming->advance(500);
    TEST_ASSERT_EQUAL_UINT32(500, pixelroot32::platforms::mock::g_mockTiming->millis());
    
    pixelroot32::platforms::mock::g_mockTiming->reset();
    TEST_ASSERT_EQUAL_UINT32(0, pixelroot32::platforms::mock::g_mockTiming->millis());
    TEST_ASSERT_EQUAL_UINT32(0, pixelroot32::platforms::mock::g_mockTiming->micros());
}

// ============================================================================
// Microsecond Tests
// ============================================================================

void test_timer_micros_advance(void) {
    // Advance 1ms = 1000us
    pixelroot32::platforms::mock::g_mockTiming->advance(1);
    TEST_ASSERT_EQUAL_UINT32(1000, pixelroot32::platforms::mock::g_mockTiming->micros());
}

void test_timer_micros_precision(void) {
    // Test microsecond precision
    pixelroot32::platforms::mock::g_mockTiming->advance(123);
    TEST_ASSERT_EQUAL_UINT32(123000, pixelroot32::platforms::mock::g_mockTiming->micros());
}

// ============================================================================
// MockMillis Global Function Tests
// ============================================================================

void test_mockMillis_returns_zero_without_provider(void) {
    // Delete the provider
    delete pixelroot32::platforms::mock::g_mockTiming;
    pixelroot32::platforms::mock::g_mockTiming = nullptr;
    
    // Should return 0 when no provider exists
    TEST_ASSERT_EQUAL_UINT32(0, pixelroot32::platforms::mock::mockMillis());
}

void test_mockMillis_returns_provider_time(void) {
    pixelroot32::platforms::mock::g_mockTiming->setTime(42);
    TEST_ASSERT_EQUAL_UINT32(42, pixelroot32::platforms::mock::mockMillis());
}

// ============================================================================
// Delta Calculation Tests
// ============================================================================

void test_timer_delta_calculation(void) {
    // Initial time
    uint32_t t1 = pixelroot32::platforms::mock::g_mockTiming->millis();
    TEST_ASSERT_EQUAL_UINT32(0, t1);
    
    // Advance and measure delta
    pixelroot32::platforms::mock::g_mockTiming->advance(16); // typical frame time
    uint32_t t2 = pixelroot32::platforms::mock::g_mockTiming->millis();
    
    uint32_t delta = t2 - t1;
    TEST_ASSERT_EQUAL_UINT32(16, delta);
}

void test_timer_delta_multiple_frames(void) {
    uint32_t t1 = pixelroot32::platforms::mock::g_mockTiming->millis();
    
    // Simulate 60 frames at 16.67ms each
    for (int i = 0; i < 60; i++) {
        pixelroot32::platforms::mock::g_mockTiming->advance(16);
    }
    
    uint32_t t2 = pixelroot32::platforms::mock::g_mockTiming->millis();
    uint32_t totalDelta = t2 - t1;
    
    TEST_ASSERT_EQUAL_UINT32(960, totalDelta); // 60 * 16ms
}

// ============================================================================
// Edge Cases
// ============================================================================

void test_timer_advance_zero(void) {
    // Advancing by 0 should not change time
    pixelroot32::platforms::mock::g_mockTiming->advance(0);
    TEST_ASSERT_EQUAL_UINT32(0, pixelroot32::platforms::mock::g_mockTiming->millis());
}

void test_timer_advance_large_value(void) {
    // Test with large value (e.g., 1 hour in ms)
    pixelroot32::platforms::mock::g_mockTiming->advance(3600000);
    TEST_ASSERT_EQUAL_UINT32(3600000, pixelroot32::platforms::mock::g_mockTiming->millis());
}

void test_timer_millis_monotonic(void) {
    // Time should always increase
    uint32_t prev = pixelroot32::platforms::mock::g_mockTiming->millis();
    
    for (int i = 0; i < 10; i++) {
        pixelroot32::platforms::mock::g_mockTiming->advance(1);
        uint32_t current = pixelroot32::platforms::mock::g_mockTiming->millis();
        TEST_ASSERT_GREATER_THAN(prev, current);
        prev = current;
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // Timer Initialization
    RUN_TEST(test_timer_initializes_at_zero);
    RUN_TEST(test_timer_advance_milliseconds);
    RUN_TEST(test_timer_advance_multiple_times);
    RUN_TEST(test_timer_set_time);
    RUN_TEST(test_timer_reset);
    
    // Microsecond Tests
    RUN_TEST(test_timer_micros_advance);
    RUN_TEST(test_timer_micros_precision);
    
    // MockMillis Global Function
    RUN_TEST(test_mockMillis_returns_zero_without_provider);
    RUN_TEST(test_mockMillis_returns_provider_time);
    
    // Delta Calculation
    RUN_TEST(test_timer_delta_calculation);
    RUN_TEST(test_timer_delta_multiple_frames);
    
    // Edge Cases
    RUN_TEST(test_timer_advance_zero);
    RUN_TEST(test_timer_advance_large_value);
    RUN_TEST(test_timer_millis_monotonic);
    
    return UNITY_END();
}
