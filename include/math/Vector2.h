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
    constexpr Vector2(const Vector2& other) : x(other.x), y(other.y) {}
    constexpr Vector2(int _x, int _y) : x(toScalar(_x)), y(toScalar(_y)) {}
    constexpr Vector2& operator=(const Vector2&) = default;

    // Common vectors
    static constexpr Vector2 ZERO() { return {toScalar(0), toScalar(0)}; }
    static constexpr Vector2 ONE() { return {toScalar(1), toScalar(1)}; }
    static constexpr Vector2 UP() { return {toScalar(0), toScalar(-1)}; }
    static constexpr Vector2 DOWN() { return {toScalar(0), toScalar(1)}; }
    static constexpr Vector2 LEFT() { return {toScalar(-1), toScalar(0)}; }
    static constexpr Vector2 RIGHT() { return {toScalar(1), toScalar(0)}; }

    // Arithmetic operators
    constexpr Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    constexpr Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    constexpr Vector2 operator-() const {
        return Vector2(-x, -y);
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

    inline Scalar cross(const Vector2& other) const {
        return x * other.y - y * other.x;
    }

    inline Scalar angle() const {
        return math::atan2(y, x);
    }

    inline Scalar angle_to(const Vector2& to) const {
        return math::atan2(cross(to), dot(to));
    }

    inline Scalar angle_to_point(const Vector2& to) const {
        return (to - *this).angle();
    }

    inline Vector2 direction_to(const Vector2& to) const {
        Vector2 ret(to.x - x, to.y - y);
        ret.normalize();
        return ret;
    }

    inline Scalar distance_to(const Vector2& to) const {
        return (to - *this).length();
    }

    inline Scalar distance_squared_to(const Vector2& to) const {
        return (to - *this).lengthSquared();
    }

    inline Vector2 limit_length(Scalar max_len = toScalar(1)) const {
        Scalar len_sq = lengthSquared();
        if (len_sq > max_len * max_len && len_sq > toScalar(0)) {
            Scalar len = math::sqrt(len_sq);
            return *this * (max_len / len);
        }
        return *this;
    }

    inline Vector2 clamp(Vector2 min, Vector2 max) const {
        return Vector2(
            math::clamp(x, min.x, max.x),
            math::clamp(y, min.y, max.y)
        );
    }

    inline Vector2 lerp(const Vector2& to, Scalar weight) const {
        return Vector2(
            math::lerp(x, to.x, weight),
            math::lerp(y, to.y, weight)
        );
    }

    inline Vector2 rotated(Scalar phi) const {
        Scalar sine = math::sin(phi);
        Scalar cosine = math::cos(phi);
        return Vector2(
            x * cosine - y * sine,
            x * sine + y * cosine
        );
    }

    inline Vector2 move_toward(const Vector2& to, Scalar delta) const {
        Vector2 v = *this;
        Vector2 vd = to - v;
        Scalar len = vd.length();
        return len <= delta || len < math::kEpsilon ? to : v + vd / len * delta;
    }

    inline Vector2 slide(const Vector2& n) const {
        return *this - n * this->dot(n);
    }

    inline Vector2 reflect(const Vector2& n) const {
        return *this - n * this->dot(n) * toScalar(2);
    }

    inline Vector2 project(const Vector2& b) const {
        return b * (dot(b) / b.lengthSquared());
    }

    inline Vector2 abs() const {
        return Vector2(math::abs(x), math::abs(y));
    }

    inline Vector2 sign() const {
        return Vector2(math::sign(x), math::sign(y));
    }

    inline bool is_normalized() const {
        return math::abs(lengthSquared() - toScalar(1)) < math::kEpsilon;
    }

    inline bool is_zero_approx() const {
        return math::is_zero_approx(x) && math::is_zero_approx(y);
    }

    inline bool is_equal_approx(const Vector2& other) const {
        return math::is_equal_approx(x, other.x) && math::is_equal_approx(y, other.y);
    }
};

} // namespace pixelroot32::math
