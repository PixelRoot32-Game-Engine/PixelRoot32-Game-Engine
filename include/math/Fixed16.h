/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * @file Fixed16.h
 * @brief Fixed-point 16.16 number implementation optimized for RISC-V.
 */
#pragma once
#include <cstdint>

namespace pixelroot32::math {

/**
 * @struct Fixed16
 * @brief Fixed-point 16.16 number implementation optimized for RISC-V.
 * 
 * Uses 32-bit integer storage: 16 bits for integer part, 16 bits for fractional part.
 * Designed for platforms without FPU (ESP32-C3, C2, C6).
 */
struct Fixed16 {
    int32_t raw;

    static constexpr int FRACTIONAL_BITS = 16;
    static constexpr int32_t ONE = 1 << FRACTIONAL_BITS;

    /** @brief Default constructor (initializes to 0). */
    constexpr Fixed16() : raw(0) {}
    
    /** @brief Raw value constructor.
     *  @param rawValue The raw 32-bit representation.
     *  @param isRaw Dummy parameter to distinguish from integer constructor. */
    constexpr explicit Fixed16(int32_t rawValue, bool /*isRaw*/) : raw(rawValue) {}
    
    /** @brief Construct from integer.
     *  @param v The integer value. */
    constexpr Fixed16(int v) : raw(v << FRACTIONAL_BITS) {}
    
    /** @brief Construct from float.
     *  @param v The float value. */
    constexpr Fixed16(float v) : raw(static_cast<int32_t>(v * ONE + (v >= 0 ? 0.5f : -0.5f))) {}
    
    /** @brief Construct from double.
     *  @param v The double value. */
    constexpr Fixed16(double v) : raw(static_cast<int32_t>(v * ONE + (v >= 0 ? 0.5 : -0.5))) {}

    /**
     * @brief Factory method to create a Fixed16 directly from a raw 32-bit value.
     * @param raw The raw 32-bit representation.
     * @return The created Fixed16 instance.
     */
    static constexpr Fixed16 fromRaw(int32_t raw) {
        return Fixed16(raw, true);
    }

    /**
     * @brief Converts to integer (truncating fractional part).
     * @return The integer value.
     */
    constexpr int toInt() const {
        return raw >> FRACTIONAL_BITS;
    }

    /**
     * @brief Converts to float.
     * @return The floating-point value.
     */
    constexpr float toFloat() const {
        return static_cast<float>(raw) / ONE;
    }

    /**
     * @brief Converts to double.
     * @return The double-precision floating-point value.
     */
    constexpr double toDouble() const {
        return static_cast<double>(raw) / ONE;
    }

    /**
     * @brief Rounds to nearest integer.
     * @return The rounded integer value.
     */
    constexpr int roundToInt() const {
        const int32_t half = 1 << (FRACTIONAL_BITS - 1);
        return (raw + (raw >= 0 ? half : -half)) >> FRACTIONAL_BITS;
    }

    /**
     * @brief Computes floor and returns as integer.
     * @return The floor integer value.
     */
    constexpr int floorToInt() const {
        return raw >> FRACTIONAL_BITS;
    }

    /**
     * @brief Computes ceiling and returns as integer.
     * @return The ceiling integer value.
     */
    constexpr int ceilToInt() const {
        const int32_t mask = (1 << FRACTIONAL_BITS) - 1;
        return (raw + mask) >> FRACTIONAL_BITS;
    }

    /**
     * @brief Cast to int.
     * @return The truncated integer value.
     */
    explicit constexpr operator int() const {
        return toInt();
    }

    /**
     * @brief Cast to float.
     * @return The floating-point value.
     */
    explicit constexpr operator float() const {
        return toFloat();
    }

    /**
     * @brief Cast to double.
     * @return The double value.
     */
    explicit constexpr operator double() const {
        return toDouble();
    }

    /**
     * @brief Addition operator.
     * @param other Value to add.
     * @return The sum.
     */
    constexpr Fixed16 operator+(const Fixed16& other) const {
        return Fixed16(raw + other.raw, true);
    }

    /**
     * @brief Subtraction operator.
     * @param other Value to subtract.
     * @return The difference.
     */
    constexpr Fixed16 operator-(const Fixed16& other) const {
        return Fixed16(raw - other.raw, true);
    }

    /**
     * @brief Multiplication operator.
     * @param other Value to multiply by.
     * @return The product.
     */
    constexpr Fixed16 operator*(const Fixed16& other) const {
        int64_t temp = static_cast<int64_t>(raw) * other.raw;
        return Fixed16(static_cast<int32_t>(temp >> FRACTIONAL_BITS), true);
    }

    /**
     * @brief Division operator.
     * @param other Value to divide by.
     * @return The quotient.
     */
    constexpr Fixed16 operator/(const Fixed16& other) const {
        if (other.raw == 0) return Fixed16(0, true); // Safety check
        int64_t temp = static_cast<int64_t>(raw) << FRACTIONAL_BITS;
        return Fixed16(static_cast<int32_t>(temp / other.raw), true);
    }

    /**
     * @brief Compound addition assignment.
     * @param other Value to add.
     * @return Reference to self.
     */
    constexpr Fixed16& operator+=(const Fixed16& other) {
        raw += other.raw;
        return *this;
    }

    /**
     * @brief Compound subtraction assignment.
     * @param other Value to subtract.
     * @return Reference to self.
     */
    constexpr Fixed16& operator-=(const Fixed16& other) {
        raw -= other.raw;
        return *this;
    }

    /**
     * @brief Compound multiplication assignment.
     * @param other Value to multiply by.
     * @return Reference to self.
     */
    constexpr Fixed16& operator*=(const Fixed16& other) {
        *this = *this * other;
        return *this;
    }

    /**
     * @brief Compound division assignment.
     * @param other Value to divide by.
     * @return Reference to self.
     */
    constexpr Fixed16& operator/=(const Fixed16& other) {
        *this = *this / other;
        return *this;
    }

    /**
     * @brief Equality operator.
     * @param other Value to compare.
     * @return True if equal.
     */
    constexpr bool operator==(const Fixed16& other) const { return raw == other.raw; }

    /**
     * @brief Inequality operator.
     * @param other Value to compare.
     * @return True if not equal.
     */
    constexpr bool operator!=(const Fixed16& other) const { return raw != other.raw; }

    /**
     * @brief Less-than operator.
     * @param other Value to compare.
     * @return True if less than.
     */
    constexpr bool operator<(const Fixed16& other) const { return raw < other.raw; }

    /**
     * @brief Greater-than operator.
     * @param other Value to compare.
     * @return True if greater than.
     */
    constexpr bool operator>(const Fixed16& other) const { return raw > other.raw; }

    /**
     * @brief Less-than-or-equal operator.
     * @param other Value to compare.
     * @return True if less than or equal.
     */
    constexpr bool operator<=(const Fixed16& other) const { return raw <= other.raw; }

    /**
     * @brief Greater-than-or-equal operator.
     * @param other Value to compare.
     * @return True if greater than or equal.
     */
    constexpr bool operator>=(const Fixed16& other) const { return raw >= other.raw; }

    /**
     * @brief Unary minus (negation) operator.
     * @return The negated value.
     */
    constexpr Fixed16 operator-() const {
        return Fixed16(-raw, true);
    }

    /**
     * @brief Computes the square root.
     * @param x The value to compute square root for.
     * @return The square root of x.
     */
    static Fixed16 sqrt(Fixed16 x) {
        if (x.raw <= 0) return Fixed16(0);

        // Calculate sqrt(raw * 2^16) using integer arithmetic
        // This gives us the result directly in 16.16 format
        // Because sqrt(X) in fixed point = sqrt(X_raw * 2^-16) 
        // = sqrt(X_raw) * 2^-8
        // We want result R such that R_raw = (sqrt(X_raw) * 2^-8) * 2^16 = sqrt(X_raw) * 2^8
        // = sqrt(X_raw * 2^16)
        
        uint64_t val = static_cast<uint64_t>(x.raw) << 16;
        uint64_t res = 0;
        uint64_t bit = 1ULL << 62; // The second-to-top bit

        // Align bit to the highest power of 4 <= val
        while (bit > val) {
            bit >>= 2;
        }
            
        while (bit != 0) {
            if (val >= res + bit) {
                val -= res + bit;
                res = (res >> 1) + bit;
            } else {
                res >>= 1;
            }
            bit >>= 2;
        }
        return Fixed16(static_cast<int32_t>(res), true);
    }
};

/**
 * @brief Fixed-point literal operator.
 * @param val The literal double value.
 * @return The Fixed16 representation.
 */
constexpr Fixed16 operator""_fp(long double val) {
    return Fixed16(static_cast<double>(val));
}

} // namespace pixelroot32::math
