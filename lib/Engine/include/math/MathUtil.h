#pragma once
#include <stdint.h>

/**
 * @namespace Math
 * @brief Collection of math helper functions and constants.
 */
namespace Math {

    constexpr float kPi = 3.14159265f;
    constexpr float kDegToRad = 180.0f / kPi;
    constexpr float kRadToDeg = kPi / 180.0f;

    /**
     * @brief Linear interpolation between two values.
     * @param a Start value.
     * @param b End value.
     * @param t Interpolation factor (usually 0.0 to 1.0).
     * @return Interpolated value.
     */
    inline float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    /**
     * @brief Clamps a value between a minimum and maximum.
     * @param v Value to clamp.
     * @param min Minimum allowed value.
     * @param max Maximum allowed value.
     * @return The clamped value.
     */
    inline float clamp(float v, float min, float max) {
        return (v < min) ? min : (v > max ? max : v);
    }
}
