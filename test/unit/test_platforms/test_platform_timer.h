#pragma once

#include <unity.h>
#include <platforms/mock/MockTiming.h>

using namespace pixelroot32::platforms::mock;

// ============================================================================
// Timer Initialization Tests
// ============================================================================

void test_timer_initializes_at_zero(void) {
    // Timer should start at 0
    TEST_ASSERT_EQUAL_UINT32(0, g_mockTiming->millis());
    TEST_ASSERT_EQUAL_UINT32(0, g_mockTiming->micros());
}

void test_timer_advance_milliseconds(void) {
    // Advance by 100ms
    g_mockTiming->advance(100);
    TEST_ASSERT_EQUAL_UINT32(100, g_mockTiming->millis());
    TEST_ASSERT_EQUAL_UINT32(100000, g_mockTiming->micros());
}

void test_timer_advance_multiple_times(void) {
    // Advance multiple times
    g_mockTiming->advance(50);
    g_mockTiming->advance(25);
    g_mockTiming->advance(25);
    TEST_ASSERT_EQUAL_UINT32(100, g_mockTiming->millis());
}

void test_timer_set_time(void) {
    // Set specific time
    g_mockTiming->setTime(1000);
    TEST_ASSERT_EQUAL_UINT32(1000, g_mockTiming->millis());
    TEST_ASSERT_EQUAL_UINT32(1000000, g_mockTiming->micros());
}

void test_timer_reset(void) {
    // Advance time, then reset
    g_mockTiming->advance(500);
    TEST_ASSERT_EQUAL_UINT32(500, g_mockTiming->millis());
    
    g_mockTiming->reset();
    TEST_ASSERT_EQUAL_UINT32(0, g_mockTiming->millis());
    TEST_ASSERT_EQUAL_UINT32(0, g_mockTiming->micros());
}

// ============================================================================
// Microsecond Tests
// ============================================================================

void test_timer_micros_advance(void) {
    // Advance 1ms = 1000us
    g_mockTiming->advance(1);
    TEST_ASSERT_EQUAL_UINT32(1000, g_mockTiming->micros());
}

void test_timer_micros_precision(void) {
    // Test microsecond precision
    g_mockTiming->advance(123);
    TEST_ASSERT_EQUAL_UINT32(123000, g_mockTiming->micros());
}

// ============================================================================
// MockMillis Global Function Tests
// ============================================================================

void test_mockMillis_returns_zero_without_provider(void) {
    // Delete the provider
    delete g_mockTiming;
    g_mockTiming = nullptr;
    
    // Should return 0 when no provider exists
    TEST_ASSERT_EQUAL_UINT32(0, pixelroot32::platforms::mock::mockMillis());
}

void test_mockMillis_returns_provider_time(void) {
    g_mockTiming->setTime(42);
    TEST_ASSERT_EQUAL_UINT32(42, pixelroot32::platforms::mock::mockMillis());
}

// ============================================================================
// Delta Calculation Tests
// ============================================================================

void test_timer_delta_calculation(void) {
    // Initial time
    uint32_t t1 = g_mockTiming->millis();
    TEST_ASSERT_EQUAL_UINT32(0, t1);
    
    // Advance and measure delta
    g_mockTiming->advance(16); // typical frame time
    uint32_t t2 = g_mockTiming->millis();
    
    uint32_t delta = t2 - t1;
    TEST_ASSERT_EQUAL_UINT32(16, delta);
}

void test_timer_delta_multiple_frames(void) {
    uint32_t t1 = g_mockTiming->millis();
    
    // Simulate 60 frames at 16.67ms each
    for (int i = 0; i < 60; i++) {
        g_mockTiming->advance(16);
    }
    
    uint32_t t2 = g_mockTiming->millis();
    uint32_t totalDelta = t2 - t1;
    
    TEST_ASSERT_EQUAL_UINT32(960, totalDelta); // 60 * 16ms
}

// ============================================================================
// Edge Cases
// ============================================================================

void test_timer_advance_zero(void) {
    // Advancing by 0 should not change time
    g_mockTiming->advance(0);
    TEST_ASSERT_EQUAL_UINT32(0, g_mockTiming->millis());
}

void test_timer_advance_large_value(void) {
    // Test with large value (e.g., 1 hour in ms)
    g_mockTiming->advance(3600000);
    TEST_ASSERT_EQUAL_UINT32(3600000, g_mockTiming->millis());
}

void test_timer_millis_monotonic(void) {
    // Time should always increase
    uint32_t prev = g_mockTiming->millis();
    
    for (int i = 0; i < 10; i++) {
        g_mockTiming->advance(1);
        uint32_t current = g_mockTiming->millis();
        TEST_ASSERT_GREATER_THAN(prev, current);
        prev = current;
    }
}
