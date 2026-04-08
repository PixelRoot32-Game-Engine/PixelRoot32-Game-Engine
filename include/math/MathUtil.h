/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "math/Scalar.h"
#include <cmath>
#include <cstdint>
#include <type_traits>

/**
 * @namespace math
 * @brief Collection of math helper functions and constants.
 */
namespace pixelroot32::math {

// Internal PRNG state
namespace detail {
    inline uint32_t& prng_state() {
        static uint32_t state = 0xDEADBEEF;
        return state;
    }

    // Unified Xorshift32 core algorithm
    inline uint32_t xorshift32(uint32_t& state) {
        // Prevent state from becoming 0 (would break PRNG)
        if (state == 0) state = 0xDEADBEEF;
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    // Global Xorshift32 next() using unified function
    inline uint32_t xorshift32_next() {
        return xorshift32(prng_state());
    }
}

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
    inline T rsqrt_impl(T x) {
        // Reciprocal square root: 1/sqrt(x)
        // Uses sqrt-based implementation for accuracy
        if constexpr (std::is_same_v<T, float>) {
            return static_cast<T>(1.0f) / std::sqrt(x);
        } else {
            // Fixed16: compute sqrt then invert
            return static_cast<T>(toScalar(1.0f)) / static_cast<T>(Fixed16::sqrt(x.toFloat()));
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

    template <typename T>
    inline int roundToInt(T v) {
            if constexpr (std::is_same_v<T, float>) {
                return static_cast<int>(std::round(v));
            } else {
                return v.roundToInt();
            }
    }
    
    template <typename T>
    inline int floorToInt(T v) {
        if constexpr (std::is_same_v<T, float>) {
            return static_cast<int>(std::floor(v));
        } else {
            return v.floorToInt();
        }
    }

    template <typename T>
    inline int ceilToInt(T v) {
        if constexpr (std::is_same_v<T, float>) {
            return static_cast<int>(std::ceil(v));
        } else {
            return v.ceilToInt();
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
 * @brief Reciprocal square root function: 1/sqrt(x)
 * Faster than computing sqrt(x) then dividing.
 * @param x Input value (must be > 0)
 * @return 1/sqrt(x)
 */
inline Scalar rsqrt(Scalar x) {
    return detail::rsqrt_impl(x);
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

/**
 * @brief Round to int.
 */
inline int roundToInt(Scalar v) {
    return detail::roundToInt(v);
}

/**
 * @brief Floor to int.
 */
inline int floorToInt(Scalar v) {
    return detail::floorToInt(v);
}

/**
 * @brief Ceil to int.
 */
inline int ceilToInt(Scalar v) {
    return detail::ceilToInt(v);
}

// ==================== PRNG SYSTEM ====================

/**
 * @brief Set the PRNG seed
 * @param seed The seed value. If 0, a fallback constant is used.
 * @warning NOT thread-safe. For concurrent use, create Random instances.
 */
inline void set_seed(uint32_t seed) {
    detail::prng_state() = (seed == 0) ? 0xDEADBEEF : seed;
}

/**
 * @brief Generate random Scalar in range [0, 1]
 * Uses bit-shifting for Fixed16 path to avoid float operations.
 * @return Random value in [0, 1] range.
 * @warning NOT thread-safe. For concurrent use, create Random instances.
 */
inline Scalar rand01() {
    uint32_t r = detail::xorshift32_next();
    if constexpr (std::is_same_v<Scalar, float>) {
        return static_cast<float>(r) / static_cast<float>(UINT32_MAX);
    } else {
        // Fixed16: use high 16 bits directly, no float conversion
        return static_cast<float>(Fixed16::fromRaw(static_cast<int16_t>(r >> 16)));
    }
}

/**
 * @brief Generate random Scalar in range [min, max]
 * @param min Minimum value (inclusive).
 * @param max Maximum value (inclusive).
 * @return Random value in [min, max] range.
 * @warning NOT thread-safe. For concurrent use, create Random instances.
 */
inline Scalar rand_range(Scalar min, Scalar max) {
    return min + rand01() * (max - min);
}

/**
 * @brief Generate random integer in range [min, max]
 * Uses rejection sampling for bias-free uniform distribution.
 * @param min Minimum value (inclusive).
 * @param max Maximum value (inclusive).
 * @return Random integer in [min, max] range.
 * @warning NOT thread-safe. For concurrent use, create Random instances.
 */
inline int32_t rand_int(int32_t min, int32_t max) {
    uint32_t range = static_cast<uint32_t>(max - min + 1);
    // Rejection sampling to eliminate modulo bias
    uint32_t threshold = (UINT32_MAX / range) * range;
    uint32_t r;
    do {
        r = detail::xorshift32_next();
    } while (r >= threshold);
    return min + static_cast<int32_t>(r % range);
}

/**
 * @brief Return true with probability p
 * @param p Probability in range [0, 1].
 * @return true with probability p, false otherwise.
 * @warning NOT thread-safe. For concurrent use, create Random instances.
 */
inline bool rand_chance(Scalar p) {
    return rand01() < p;
}

/**
 * @brief Return random sign -1 or 1
 * @return -1 or 1 as Scalar.
 * @warning NOT thread-safe. For concurrent use, create Random instances.
 */
inline Scalar rand_sign() {
    return (detail::xorshift32_next() & 1) ? toScalar(1) : toScalar(-1);
}

/**
 * @brief Instance-based random number generator
 * 
 * Provides independent RNG state for scenarios requiring multiple
 * separate random sequences (e.g., per-entity RNG).
 */
struct Random {
    uint32_t state;

    /**
     * @brief Constructor with seed parameter
     * @param seed Initial seed value. If 0, uses fallback constant.
     */
    explicit Random(uint32_t seed = 0xDEADBEEF)
        : state(seed == 0 ? 0xDEADBEEF : seed) {}

    /**
     * @brief Generate next random value using Xorshift32
     * @return Random uint32_t value
     */
    uint32_t next() {
        return detail::xorshift32(state);
    }

    /**
     * @brief Generate random Scalar in range [0, 1]
     * Uses bit-shifting for Fixed16 path to avoid float operations.
     * @return Random value in [0, 1] range
     */
    Scalar rand01() {
        uint32_t r = next();
        if constexpr (std::is_same_v<Scalar, float>) {
            return static_cast<float>(r) / static_cast<float>(UINT32_MAX);
        } else {
            // Fixed16: use high 16 bits directly, no float conversion
            return static_cast<float>(Fixed16::fromRaw(static_cast<int16_t>(r >> 16)));
        }
    }

    /**
     * @brief Generate random Scalar in range [min, max]
     * @param min Minimum value (inclusive)
     * @param max Maximum value (inclusive)
     * @return Random value in [min, max] range
     */
    Scalar rand_range(Scalar min, Scalar max) {
        return min + rand01() * (max - min);
    }

    /**
     * @brief Generate random integer in range [min, max]
     * Uses rejection sampling for bias-free uniform distribution.
     * @param min Minimum value (inclusive)
     * @param max Maximum value (inclusive)
     * @return Random integer in [min, max] range
     */
    int32_t rand_int(int32_t min, int32_t max) {
        uint32_t range = static_cast<uint32_t>(max - min + 1);
        // Rejection sampling to eliminate modulo bias
        uint32_t threshold = (UINT32_MAX / range) * range;
        uint32_t r;
        do {
            r = next();
        } while (r >= threshold);
        return min + static_cast<int32_t>(r % range);
    }

    /**
     * @brief Return true with probability p
     * @param p Probability in range [0, 1]
     * @return true with probability p, false otherwise
     */
    bool rand_chance(Scalar p) {
        return rand01() < p;
    }

    /**
     * @brief Return random sign -1 or 1
     * @return -1 or 1 as Scalar
     */
    Scalar rand_sign() {
        return (next() & 1) ? toScalar(1) : toScalar(-1);
    }
};

} // namespace pixelroot32::math