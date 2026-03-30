/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Mock timing provider for controlled time steps in Engine tests
 */

#ifndef PIXELROOT32_MOCK_TIMING_H
#define PIXELROOT32_MOCK_TIMING_H

#include <cstdint>
#include <cstddef>

namespace pixelroot32::platforms::mock {

/**
 * @brief Mock timing provider for controlled time steps
 * 
 * Allows tests to control the passage of time for deterministic
 * testing of Engine's update loop and timing calculations.
 */
class MockTimingProvider {
public:
    MockTimingProvider() : currentMillis_(0), micros_(0) {}

    /**
     * @brief Advance time by a specific amount
     * @param ms Milliseconds to advance
     */
    void advance(uint32_t ms) {
        currentMillis_ += ms;
        micros_ += ms * 1000;
    }

    /**
     * @brief Set specific time value
     * @param ms Milliseconds to set
     */
    void setTime(uint32_t ms) {
        currentMillis_ = ms;
        micros_ = ms * 1000;
    }

    /**
     * @brief Get current simulated milliseconds
     */
    unsigned long millis() const {
        return currentMillis_;
    }

    /**
     * @brief Get current simulated microseconds
     */
    uint32_t micros() const {
        return micros_;
    }

    /**
     * @brief Reset timing to zero
     */
    void reset() {
        currentMillis_ = 0;
        micros_ = 0;
    }

private:
    uint32_t currentMillis_;
    uint32_t micros_;
};

/**
 * @brief Global mock timing instance for tests
 */
extern MockTimingProvider* g_mockTiming;

/**
 * @brief Get current time from mock provider or real clock
 */
inline unsigned long mockMillis() {
    if (g_mockTiming) {
        return g_mockTiming->millis();
    }
    return 0;
}

} // namespace pixelroot32::platforms::mock

#endif // PIXELROOT32_MOCK_TIMING_H
