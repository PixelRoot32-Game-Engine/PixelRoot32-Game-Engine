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

    // Constructors
    constexpr Fixed16() : raw(0) {}
    constexpr explicit Fixed16(int32_t rawValue, bool /*isRaw*/) : raw(rawValue) {}
    constexpr Fixed16(int v) : raw(v << FRACTIONAL_BITS) {}
    constexpr Fixed16(float v) : raw(static_cast<int32_t>(v * ONE + (v >= 0 ? 0.5f : -0.5f))) {}
    constexpr Fixed16(double v) : raw(static_cast<int32_t>(v * ONE + (v >= 0 ? 0.5 : -0.5))) {}

    // Factory
    static constexpr Fixed16 fromRaw(int32_t raw) {
        return Fixed16(raw, true);
    }

    // Conversions
    constexpr int toInt() const {
        return raw >> FRACTIONAL_BITS;
    }

    constexpr float toFloat() const {
        return static_cast<float>(raw) / ONE;
    }

    constexpr double toDouble() const {
        return static_cast<double>(raw) / ONE;
    }

    explicit constexpr operator int() const {
        return toInt();
    }

    explicit constexpr operator float() const {
        return toFloat();
    }

    explicit constexpr operator double() const {
        return toDouble();
    }

    // Arithmetic Operators
    constexpr Fixed16 operator+(const Fixed16& other) const {
        return Fixed16(raw + other.raw, true);
    }

    constexpr Fixed16 operator-(const Fixed16& other) const {
        return Fixed16(raw - other.raw, true);
    }

    constexpr Fixed16 operator*(const Fixed16& other) const {
        int64_t temp = static_cast<int64_t>(raw) * other.raw;
        return Fixed16(static_cast<int32_t>(temp >> FRACTIONAL_BITS), true);
    }

    constexpr Fixed16 operator/(const Fixed16& other) const {
        if (other.raw == 0) return Fixed16(0, true); // Safety check
        int64_t temp = static_cast<int64_t>(raw) << FRACTIONAL_BITS;
        return Fixed16(static_cast<int32_t>(temp / other.raw), true);
    }

    // Compound Assignment
    constexpr Fixed16& operator+=(const Fixed16& other) {
        raw += other.raw;
        return *this;
    }

    constexpr Fixed16& operator-=(const Fixed16& other) {
        raw -= other.raw;
        return *this;
    }

    constexpr Fixed16& operator*=(const Fixed16& other) {
        *this = *this * other;
        return *this;
    }

    constexpr Fixed16& operator/=(const Fixed16& other) {
        *this = *this / other;
        return *this;
    }

    // Comparison Operators
    constexpr bool operator==(const Fixed16& other) const { return raw == other.raw; }
    constexpr bool operator!=(const Fixed16& other) const { return raw != other.raw; }
    constexpr bool operator<(const Fixed16& other) const { return raw < other.raw; }
    constexpr bool operator>(const Fixed16& other) const { return raw > other.raw; }
    constexpr bool operator<=(const Fixed16& other) const { return raw <= other.raw; }
    constexpr bool operator>=(const Fixed16& other) const { return raw >= other.raw; }

    // Unary Operators
    constexpr Fixed16 operator-() const {
        return Fixed16(-raw, true);
    }

    // Math Functions
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

// Literal operator for convenience (e.g., 1.5_fp)
constexpr Fixed16 operator""_fp(long double val) {
    return Fixed16(static_cast<double>(val));
}

} // namespace pixelroot32::math
