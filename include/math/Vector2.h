#pragma once
#include "math/Scalar.h"
#include "math/MathUtil.h"

namespace pixelroot32::math {

/**
 * @struct Vector2
 * @brief 2D vector using the configured Scalar type (float or Fixed16).
 * 
 * Automatically adapts to the architecture's FPU availability.
 */
struct Vector2 {
    Scalar x;
    Scalar y;

    constexpr Vector2() : x(toScalar(0)), y(toScalar(0)) {}
    constexpr Vector2(Scalar _x, Scalar _y) : x(_x), y(_y) {}

    // Arithmetic operators
    constexpr Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    constexpr Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    constexpr Vector2 operator*(Scalar scalar) const {
        return Vector2(x * scalar, y * scalar);
    }

    constexpr Vector2 operator/(Scalar scalar) const {
        return Vector2(x / scalar, y / scalar);
    }

    // Compound assignment
    constexpr Vector2& operator+=(const Vector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr Vector2& operator-=(const Vector2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr Vector2& operator*=(Scalar scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vector2& operator/=(Scalar scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    // Comparison
    constexpr bool operator==(const Vector2& other) const {
        return x == other.x && y == other.y;
    }

    constexpr bool operator!=(const Vector2& other) const {
        return !(*this == other);
    }

    // Vector operations
    constexpr Scalar lengthSquared() const {
        return x * x + y * y;
    }

    inline Scalar length() const {
        return sqrt(lengthSquared());
    }

    inline void normalize() {
        Scalar len = length();
        if (len > toScalar(0)) {
            *this /= len;
        }
    }

    inline Vector2 normalized() const {
        Vector2 v = *this;
        v.normalize();
        return v;
    }

    inline Scalar dot(const Vector2& other) const {
        return x * other.x + y * other.y;
    }
};

} // namespace pixelroot32::math
