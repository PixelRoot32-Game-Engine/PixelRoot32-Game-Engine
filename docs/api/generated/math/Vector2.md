# Vector2

<Badge type="info" text="Struct" />

**Source:** `Vector2.h`

## Description

2D vector using the configured Scalar type (float or Fixed16).

Automatically adapts to the architecture's FPU availability.

## Methods

### `constexpr Vector2()`

**Description:**

Default constructor, initializes to (0, 0).

### `constexpr Vector2(Scalar _x, Scalar _y)`

**Description:**

Constructor with given x and y components.

**Parameters:**

- `_x`: X component.
- `_y`: Y component.

### `constexpr Vector2(const Vector2& other)`

**Description:**

Copy constructor.

**Parameters:**

- `other`: The other vector to copy from.

### `constexpr Vector2(int _x, int _y)`

**Description:**

Constructor with integer components.

**Parameters:**

- `_x`: X component.
- `_y`: Y component.

### `static constexpr Vector2 ZERO()`

**Description:**

Returns vector (0, 0). @return (0, 0) vector.

**Returns:** (0, 0) vector.

### `static constexpr Vector2 ONE()`

**Description:**

Returns vector (1, 1). @return (1, 1) vector.

**Returns:** (1, 1) vector.

### `static constexpr Vector2 UP()`

**Description:**

Returns vector (0, -1). @return (0, -1) vector.

**Returns:** (0, -1) vector.

### `static constexpr Vector2 DOWN()`

**Description:**

Returns vector (0, 1). @return (0, 1) vector.

**Returns:** (0, 1) vector.

### `static constexpr Vector2 LEFT()`

**Description:**

Returns vector (-1, 0). @return (-1, 0) vector.

**Returns:** (-1, 0) vector.

### `static constexpr Vector2 RIGHT()`

**Description:**

Returns vector (1, 0). @return (1, 0) vector.

**Returns:** (1, 0) vector.

### `constexpr Scalar lengthSquared() const`

**Description:**

Computes squared length. @return Squared length of the vector.

**Returns:** Squared length of the vector.

### `inline Scalar length() const`

**Description:**

Computes length (magnitude). @return Length of the vector.

**Returns:** Length of the vector.

### `inline void normalize()`

**Description:**

Normalizes the vector in place.

### `inline Vector2 normalized() const`

**Description:**

Returns a normalized copy. @return Normalized vector.

**Returns:** Normalized vector.

### `inline Scalar dot(const Vector2& other) const`

**Description:**

Dot product with another vector. @param other Vector to compute dot product with. @return Dot product result.

**Parameters:**

- `other`: Vector to compute dot product with.

**Returns:** Dot product result.

### `inline Scalar cross(const Vector2& other) const`

**Description:**

2D Cross product with another vector. @param other Vector to compute cross product with. @return Cross product result.

**Parameters:**

- `other`: Vector to compute cross product with.

**Returns:** Cross product result.

### `inline Scalar angle() const`

**Description:**

Angle of the vector. @return Angle in radians.

**Returns:** Angle in radians.

### `inline Scalar angle_to(const Vector2& to) const`

**Description:**

Angle to another vector. @param to The target vector. @return Angle difference in radians.

**Parameters:**

- `to`: The target vector.

**Returns:** Angle difference in radians.

### `inline Scalar angle_to_point(const Vector2& to) const`

**Description:**

Angle to a point. @param to Target point. @return Angle in radians.

**Parameters:**

- `to`: Target point.

**Returns:** Angle in radians.

### `inline Vector2 direction_to(const Vector2& to) const`

**Description:**

Direction to another point. @param to Target point. @return Normalized direction vector.

**Parameters:**

- `to`: Target point.

**Returns:** Normalized direction vector.

### `Vector2 ret(to.x - x, to.y - y)`

### `inline Scalar distance_to(const Vector2& to) const`

**Description:**

Distance to another point. @param to Target point. @return Distance.

**Parameters:**

- `to`: Target point.

**Returns:** Distance.

### `inline Scalar distance_squared_to(const Vector2& to) const`

**Description:**

Squared distance to another point. @param to Target point. @return Squared distance.

**Parameters:**

- `to`: Target point.

**Returns:** Squared distance.

### `inline Vector2 limit_length(Scalar max_len = toScalar(1)) const`

**Description:**

Returns vector with length limited. @param max_len Maximum allowed length. @return Resulting vector.

**Parameters:**

- `max_len`: Maximum allowed length.

**Returns:** Resulting vector.

### `inline Vector2 clamp(Vector2 min, Vector2 max) const`

**Description:**

Clamps components between bounds. @param min Minimum bounds. @param max Maximum bounds. @return Clamped vector.

**Parameters:**

- `min`: Minimum bounds.

**Returns:** Clamped vector.

### `inline Vector2 lerp(const Vector2& to, Scalar weight) const`

**Description:**

Linearly interpolates towards another vector. @param to Target vector. @param weight Interpolation weight. @return Interpolated vector.

**Parameters:**

- `to`: Target vector.

**Returns:** Interpolated vector.

### `inline Vector2 rotated(Scalar phi) const`

**Description:**

Rotates vector by an angle. @param phi Angle in radians. @return Rotated vector.

**Parameters:**

- `phi`: Angle in radians.

**Returns:** Rotated vector.

### `inline Vector2 move_toward(const Vector2& to, Scalar delta) const`

**Description:**

Moves toward another point by a delta. @param to Target point. @param delta Step amount. @return Resulting vector.

**Parameters:**

- `to`: Target point.

**Returns:** Resulting vector.

### `inline Vector2 slide(const Vector2& n) const`

**Description:**

Slides vector along a surface normal. @param n Surface normal. @return Slid vector.

**Parameters:**

- `n`: Surface normal.

**Returns:** Slid vector.

### `inline Vector2 reflect(const Vector2& n) const`

**Description:**

Reflects vector across a surface normal. @param n Surface normal. @return Reflected vector.

**Parameters:**

- `n`: Surface normal.

**Returns:** Reflected vector.

### `inline Vector2 project(const Vector2& b) const`

**Description:**

Projects vector onto another. @param b Vector to project onto. @return Projected vector.

**Parameters:**

- `b`: Vector to project onto.

**Returns:** Projected vector.

### `inline Vector2 abs() const`

**Description:**

Returns absolute values of components. @return Absolute vector.

**Returns:** Absolute vector.

### `inline Vector2 sign() const`

**Description:**

Returns signs of components. @return Sign vector.

**Returns:** Sign vector.

### `inline bool is_normalized() const`

**Description:**

Checks if vector is normalized. @return True if approximately normalized.

**Returns:** True if approximately normalized.

### `inline bool is_zero_approx() const`

**Description:**

Checks if vector is near zero. @return True if approximately zero.

**Returns:** True if approximately zero.

### `inline bool is_equal_approx(const Vector2& other) const`

**Description:**

Checks if approximately equal to another. @param other Vector to compare. @return True if approximately equal.

**Parameters:**

- `other`: Vector to compare.

**Returns:** True if approximately equal.
