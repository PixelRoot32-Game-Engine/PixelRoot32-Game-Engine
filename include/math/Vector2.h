/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file Vector2.h
 * @brief 2D vector using the configured Scalar type (float or Fixed16).
 */
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

    /** @brief Default constructor, initializes to (0, 0). */
    constexpr Vector2() : x(toScalar(0)), y(toScalar(0)) {}
    /** @brief Constructor with given x and y components.
     *  @param _x X component.
     *  @param _y Y component. */
    constexpr Vector2(Scalar _x, Scalar _y) : x(_x), y(_y) {}
    /** @brief Copy constructor.
     *  @param other The other vector to copy from. */
    constexpr Vector2(const Vector2& other) : x(other.x), y(other.y) {}
    /** @brief Constructor with integer components.
     *  @param _x X component.
     *  @param _y Y component. */
    constexpr Vector2(int _x, int _y) : x(toScalar(_x)), y(toScalar(_y)) {}
    /** @brief Default assignment operator.
     *  @param other The other vector to copy from.
     *  @return Reference to self. */
    constexpr Vector2& operator=(const Vector2&) = default;

    /** @brief Returns vector (0, 0). @return (0, 0) vector. */
    static constexpr Vector2 ZERO() { return {toScalar(0), toScalar(0)}; }
    /** @brief Returns vector (1, 1). @return (1, 1) vector. */
    static constexpr Vector2 ONE() { return {toScalar(1), toScalar(1)}; }
    /** @brief Returns vector (0, -1). @return (0, -1) vector. */
    static constexpr Vector2 UP() { return {toScalar(0), toScalar(-1)}; }
    /** @brief Returns vector (0, 1). @return (0, 1) vector. */
    static constexpr Vector2 DOWN() { return {toScalar(0), toScalar(1)}; }
    /** @brief Returns vector (-1, 0). @return (-1, 0) vector. */
    static constexpr Vector2 LEFT() { return {toScalar(-1), toScalar(0)}; }
    /** @brief Returns vector (1, 0). @return (1, 0) vector. */
    static constexpr Vector2 RIGHT() { return {toScalar(1), toScalar(0)}; }

    /** @brief Vector addition. @param other Vector to add. @return Resulting vector. */
    constexpr Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    /** @brief Vector subtraction. @param other Vector to subtract. @return Resulting vector. */
    constexpr Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    /** @brief Unary negation. @return Negated vector. */
    constexpr Vector2 operator-() const {
        return Vector2(-x, -y);
    }

    /** @brief Scalar multiplication. @param scalar Value to multiply by. @return Resulting vector. */
    constexpr Vector2 operator*(Scalar scalar) const {
        return Vector2(x * scalar, y * scalar);
    }

    /** @brief Scalar division. @param scalar Value to divide by. @return Resulting vector. */
    constexpr Vector2 operator/(Scalar scalar) const {
        return Vector2(x / scalar, y / scalar);
    }

    /** @brief Compound vector addition. @param other Vector to add. @return Reference to self. */
    constexpr Vector2& operator+=(const Vector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    /** @brief Compound vector subtraction. @param other Vector to subtract. @return Reference to self. */
    constexpr Vector2& operator-=(const Vector2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    /** @brief Compound scalar multiplication. @param scalar Value to multiply by. @return Reference to self. */
    constexpr Vector2& operator*=(Scalar scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    /** @brief Compound scalar division. @param scalar Value to divide by. @return Reference to self. */
    constexpr Vector2& operator/=(Scalar scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    /** @brief Equality check. @param other Vector to compare. @return True if equal. */
    constexpr bool operator==(const Vector2& other) const {
        return x == other.x && y == other.y;
    }

    /** @brief Inequality check. @param other Vector to compare. @return True if not equal. */
    constexpr bool operator!=(const Vector2& other) const {
        return !(*this == other);
    }

    /** @brief Computes squared length. @return Squared length of the vector. */
    constexpr Scalar lengthSquared() const {
        return x * x + y * y;
    }

    /** @brief Computes length (magnitude). @return Length of the vector. */
    inline Scalar length() const {
        return sqrt(lengthSquared());
    }

    /** @brief Normalizes the vector in place. */
    inline void normalize() {
        Scalar len = length();
        if (len > toScalar(0)) {
            *this /= len;
        }
    }

    /** @brief Returns a normalized copy. @return Normalized vector. */
    inline Vector2 normalized() const {
        Vector2 v = *this;
        v.normalize();
        return v;
    }

    /** @brief Dot product with another vector. @param other Vector to compute dot product with. @return Dot product result. */
    inline Scalar dot(const Vector2& other) const {
        return x * other.x + y * other.y;
    }

    /** @brief 2D Cross product with another vector. @param other Vector to compute cross product with. @return Cross product result. */
    inline Scalar cross(const Vector2& other) const {
        return x * other.y - y * other.x;
    }

    /** @brief Angle of the vector. @return Angle in radians. */
    inline Scalar angle() const {
        return math::atan2(y, x);
    }

    /** @brief Angle to another vector. @param to The target vector. @return Angle difference in radians. */
    inline Scalar angle_to(const Vector2& to) const {
        return math::atan2(cross(to), dot(to));
    }

    /** @brief Angle to a point. @param to Target point. @return Angle in radians. */
    inline Scalar angle_to_point(const Vector2& to) const {
        return (to - *this).angle();
    }

    /** @brief Direction to another point. @param to Target point. @return Normalized direction vector. */
    inline Vector2 direction_to(const Vector2& to) const {
        Vector2 ret(to.x - x, to.y - y);
        ret.normalize();
        return ret;
    }

    /** @brief Distance to another point. @param to Target point. @return Distance. */
    inline Scalar distance_to(const Vector2& to) const {
        return (to - *this).length();
    }

    /** @brief Squared distance to another point. @param to Target point. @return Squared distance. */
    inline Scalar distance_squared_to(const Vector2& to) const {
        return (to - *this).lengthSquared();
    }

    /** @brief Returns vector with length limited. @param max_len Maximum allowed length. @return Resulting vector. */
    inline Vector2 limit_length(Scalar max_len = toScalar(1)) const {
        Scalar len_sq = lengthSquared();
        if (len_sq > max_len * max_len && len_sq > toScalar(0)) {
            Scalar len = math::sqrt(len_sq);
            return *this * (max_len / len);
        }
        return *this;
    }

    /** @brief Clamps components between bounds. @param min Minimum bounds. @param max Maximum bounds. @return Clamped vector. */
    inline Vector2 clamp(Vector2 min, Vector2 max) const {
        return Vector2(
            math::clamp(x, min.x, max.x),
            math::clamp(y, min.y, max.y)
        );
    }

    /** @brief Linearly interpolates towards another vector. @param to Target vector. @param weight Interpolation weight. @return Interpolated vector. */
    inline Vector2 lerp(const Vector2& to, Scalar weight) const {
        return Vector2(
            math::lerp(x, to.x, weight),
            math::lerp(y, to.y, weight)
        );
    }

    /** @brief Rotates vector by an angle. @param phi Angle in radians. @return Rotated vector. */
    inline Vector2 rotated(Scalar phi) const {
        Scalar sine = math::sin(phi);
        Scalar cosine = math::cos(phi);
        return Vector2(
            x * cosine - y * sine,
            x * sine + y * cosine
        );
    }

    /** @brief Moves toward another point by a delta. @param to Target point. @param delta Step amount. @return Resulting vector. */
    inline Vector2 move_toward(const Vector2& to, Scalar delta) const {
        Vector2 v = *this;
        Vector2 vd = to - v;
        Scalar len = vd.length();
        return len <= delta || len < math::kEpsilon ? to : v + vd / len * delta;
    }

    /** @brief Slides vector along a surface normal. @param n Surface normal. @return Slid vector. */
    inline Vector2 slide(const Vector2& n) const {
        return *this - n * this->dot(n);
    }

    /** @brief Reflects vector across a surface normal. @param n Surface normal. @return Reflected vector. */
    inline Vector2 reflect(const Vector2& n) const {
        return *this - n * this->dot(n) * toScalar(2);
    }

    /** @brief Projects vector onto another. @param b Vector to project onto. @return Projected vector. */
    inline Vector2 project(const Vector2& b) const {
        return b * (dot(b) / b.lengthSquared());
    }

    /** @brief Returns absolute values of components. @return Absolute vector. */
    inline Vector2 abs() const {
        return Vector2(math::abs(x), math::abs(y));
    }

    /** @brief Returns signs of components. @return Sign vector. */
    inline Vector2 sign() const {
        return Vector2(math::sign(x), math::sign(y));
    }

    /** @brief Checks if vector is normalized. @return True if approximately normalized. */
    inline bool is_normalized() const {
        return math::abs(lengthSquared() - toScalar(1)) < math::kEpsilon;
    }

    /** @brief Checks if vector is near zero. @return True if approximately zero. */
    inline bool is_zero_approx() const {
        return math::is_zero_approx(x) && math::is_zero_approx(y);
    }

    /** @brief Checks if approximately equal to another. @param other Vector to compare. @return True if approximately equal. */
    inline bool is_equal_approx(const Vector2& other) const {
        return math::is_equal_approx(x, other.x) && math::is_equal_approx(y, other.y);
    }
};

} // namespace pixelroot32::math
