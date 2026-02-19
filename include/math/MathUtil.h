/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "math/Scalar.h"
#include <cmath>
#include <type_traits>

/**
 * @namespace math
 * @brief Collection of math helper functions and constants.
 */
namespace pixelroot32::math {

namespace detail {
    template <typename T>
    inline T sqrt_impl(T x) {
        if constexpr (std::is_same_v<T, float>) {
            return std::sqrt(x);
        } else {
            return static_cast<T>(Fixed16::sqrt(x));
        }
    }

    template <typename T>
    inline T sin_impl(T x) {
        if constexpr (std::is_same_v<T, float>) {
            return std::sin(x);
        } else {
            return static_cast<T>(std::sin(x.toFloat()));
        }
    }

    template <typename T>
    inline T cos_impl(T x) {
        if constexpr (std::is_same_v<T, float>) {
            return std::cos(x);
        } else {
            return static_cast<T>(std::cos(x.toFloat()));
        }
    }

    template <typename T>
    inline T atan2_impl(T y, T x) {
        if constexpr (std::is_same_v<T, float>) {
            return std::atan2(y, x);
        } else {
            return static_cast<T>(std::atan2(y.toFloat(), x.toFloat()));
        }
    }
}

constexpr Scalar kPi = toScalar(3.14159265f);
constexpr Scalar kDegToRad = toScalar(3.14159265f / 180.0f);
constexpr Scalar kRadToDeg = toScalar(180.0f / 3.14159265f);
constexpr Scalar kEpsilon = toScalar(0.00001f);

/**
 * @brief Returns the smaller of two values.
 */
inline Scalar min(Scalar a, Scalar b) {
    return (a < b) ? a : b;
}

/**
 * @brief Returns the larger of two values.
 */
inline Scalar max(Scalar a, Scalar b) {
    return (a > b) ? a : b;
}

/**
 * @brief Square root function adaptable for float or Fixed16.
*/
inline Scalar sqrt(Scalar x) {
    return detail::sqrt_impl(x);
}
/**
 * @brief Sine function.
 */
inline Scalar sin(Scalar x) {
    return detail::sin_impl(x);
}

/**
 * @brief Cosine function.
 */
inline Scalar cos(Scalar x) {
    return detail::cos_impl(x);
}

/**
 * @brief Arc Tangent 2 function.
 */
inline Scalar atan2(Scalar y, Scalar x) {
    return detail::atan2_impl(y, x);
}

/**
 * @brief Linear interpolation between two values.
 * @param a Start value.
 * @param b End value.
 * @param t Interpolation factor (usually 0.0 to 1.0).
 * @return Interpolated value.
 */
inline Scalar lerp(Scalar a, Scalar b, Scalar t) {
    return a + (b - a) * t;
}

/**
 * @brief Clamps a value between a minimum and maximum.
 * @param v Value to clamp.
 * @param min Minimum allowed value.
 * @param max Maximum allowed value.
 * @return The clamped value.
 */
inline Scalar clamp(Scalar v, Scalar min, Scalar max) {
    return (v < min) ? min : (v > max ? max : v);
}

/**
 * @brief Absolute value function.
 */
inline Scalar abs(Scalar x) {
    return (x < toScalar(0)) ? -x : x;
}

/**
 * @brief Returns the sign of the value (-1, 0, or 1).
 */
inline Scalar sign(Scalar x) {
    return (x < toScalar(0)) ? toScalar(-1) : (x > toScalar(0) ? toScalar(1) : toScalar(0));
}

/**
 * @brief Checks if two values are approximately equal.
 */
inline bool is_equal_approx(Scalar a, Scalar b) {
    return abs(a - b) < kEpsilon;
}

/**
 * @brief Checks if a value is approximately zero.
 */
inline bool is_zero_approx(Scalar x) {
    return abs(x) < kEpsilon;
}

} // namespace pixelroot32::math