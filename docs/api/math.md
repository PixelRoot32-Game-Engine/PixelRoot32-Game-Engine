# API Reference: Math Module

This document covers the math utilities, scalar type abstraction, vector operations, and random number generation in PixelRoot32.

> **Note:** This is part of the [API Reference](../API_REFERENCE.md). See the main index for complete documentation.

---

## Scalar Type

**Namespace:** `pixelroot32::math`

`Scalar` is the fundamental numeric type used throughout the engine for physics, positioning, and logic.

- **On FPU platforms (ESP32, S3):** `Scalar` is an alias for `float`.
- **On non-FPU platforms (C3, S2, C6):** `Scalar` is an alias for `Fixed16`.

### Fixed16 (16.16 Fixed Point)

On platforms without a Hardware Floating Point Unit (FPU), the engine uses `Fixed16` for all calculations.

- **Storage**: 32-bit signed integer.
- **Precision**: 16 bits for the integer part, 16 bits for the fractional part (approx. 0.000015 resolution).
- **Literal**: Use the `_fp` suffix for literals on non-FPU platforms for compile-time conversion.
  *Example:* `Scalar gravity = 9.8_fp;`

---

## Helper Functions

- **`Scalar toScalar(float value)`**
    Converts a floating-point literal or variable to `Scalar`.
    *Usage:* `Scalar speed = toScalar(2.5f);`

- **`Scalar toScalar(int value)`**
    Converts an integer to `Scalar`.

- **`int toInt(Scalar value)`**
    Converts a `Scalar` back to an integer (truncating decimals).

- **`int roundToInt(Scalar value)`**
    Converts a `Scalar` to an integer, rounding to the nearest whole number. Essential for mapping logical positions to pixel coordinates without jitter.

- **`int floorToInt(Scalar value)`**
    Returns the largest integer less than or equal to the scalar value.

- **`int ceilToInt(Scalar value)`**
    Returns the smallest integer greater than or equal to the scalar value.

- **`float toFloat(Scalar value)`**
    Converts a `Scalar` to `float`. **Warning:** Use sparingly on non-FPU platforms.

- **`Scalar abs(Scalar v)`**
    Returns the absolute value.

- **`Scalar sqrt(Scalar v)`**
    Returns the square root. **Warning:** Expensive operation. Prefer squared distances for comparisons.

- **`Scalar rsqrt(Scalar v)`**
    Returns the reciprocal square root: `1/sqrt(v)`. Faster than `1/sqrt(v)` for normalization operations.
    *Example:* `Vector2 normalized = velocity * (1.0f / velocity.length());` becomes `velocity * rsqrt(velocity.lengthSquared());`
    Enable with `-D PIXELROOT32_HAS_FAST_RSQRT=1` in build flags.

- **`Scalar min(Scalar a, Scalar b)`**
    Returns the smaller of two values.

- **`Scalar max(Scalar a, Scalar b)`**
    Returns the larger of two values.

- **`Scalar clamp(Scalar v, Scalar minVal, Scalar maxVal)`**
    Clamps a value between a minimum and maximum.

- **`Scalar lerp(Scalar a, Scalar b, Scalar t)`**
    Linearly interpolates between `a` and `b` by `t` (where `t` is 0.0 to 1.0).

- **`Scalar sin(Scalar x)`**
    Returns the sine of the angle `x` (in radians).

- **`Scalar cos(Scalar x)`**
    Returns the cosine of the angle `x` (in radians).

- **`Scalar atan2(Scalar y, Scalar x)`**
    Returns the arc tangent of y/x (in radians).

- **`Scalar sign(Scalar x)`**
    Returns the sign of x (-1, 0, or 1).

- **`bool is_equal_approx(Scalar a, Scalar b)`**
    Returns true if a and b are approximately equal.

- **`bool is_zero_approx(Scalar x)`**
    Returns true if x is approximately zero.

### Constants

- **`Scalar kPi`**
    Value of PI (3.14159...).

- **`Scalar kDegToRad`**
    Conversion factor from degrees to radians.

- **`Scalar kRadToDeg`**
    Conversion factor from radians to degrees.

---

## Vector2

**Namespace:** `pixelroot32::math`

A 2D vector structure composed of two `Scalar` components.

### Members

- **`Scalar x`**
- **`Scalar y`**

### Methods

- **`Vector2(Scalar x, Scalar y)`**
    Constructor.

- **`Scalar lengthSquared() const`**
    Returns the squared magnitude of the vector. **Preferred over `length()` for comparisons.**

- **`Scalar length() const`**
    Returns the magnitude of the vector.

- **`Vector2 normalized() const`**
    Returns a normalized (unit length) version of the vector.

- **`Scalar dot(const Vector2& other) const`**
    Returns the dot product with another vector.

- **`Scalar cross(const Vector2& other) const`**
    Returns the cross product with another vector (2D analog).

- **`Scalar angle() const`**
    Returns the angle of the vector in radians.

- **`Scalar angle_to(const Vector2& to) const`**
    Returns the angle to another vector in radians.

- **`Scalar angle_to_point(const Vector2& to) const`**
    Returns the angle from this point to another point.

- **`Vector2 direction_to(const Vector2& to) const`**
    Returns the normalized direction vector pointing to the target.

- **`Scalar distance_to(const Vector2& to) const`**
    Returns the distance to another point.

- **`Scalar distance_squared_to(const Vector2& to) const`**
    Returns the squared distance to another point.

- **`Vector2 limit_length(Scalar max_len) const`**
    Returns the vector with its length limited to `max_len`.

- **`Vector2 clamp(Vector2 min, Vector2 max) const`**
    Returns the vector clamped between min and max vectors.

- **`Vector2 lerp(const Vector2& to, Scalar weight) const`**
    Linear interpolation between this vector and `to`.

- **`Vector2 rotated(Scalar phi) const`**
    Returns the vector rotated by `phi` radians.

- **`Vector2 move_toward(const Vector2& to, Scalar delta) const`**
    Moves the vector toward `to` by a maximum of `delta` distance.

- **`Vector2 slide(const Vector2& n) const`**
    Returns the component of the vector along the sliding plane defined by normal `n`.

- **`Vector2 reflect(const Vector2& n) const`**
    Returns the vector reflected across the plane defined by normal `n`.

- **`Vector2 project(const Vector2& b) const`**
    Returns the projection of this vector onto vector `b`.

- **`Vector2 abs() const`**
    Returns a new vector with absolute values of components.

- **`Vector2 sign() const`**
    Returns a new vector with sign of components.

- **`bool is_normalized() const`**
    Returns true if the vector is normalized.

- **`bool is_zero_approx() const`**
    Returns true if the vector is approximately zero.

- **`bool is_equal_approx(const Vector2& other) const`**
    Returns true if the vector is approximately equal to `other`.

---

## MathUtil

**Inherits:** None

Collection of helper functions.

### Public Methods

- **`float lerp(float a, float b, float t)`**
    Linear interpolation.
- **`float clamp(float v, float min, float max)`**
    Clamps a value between min and max.

---

## Random Number Generation (PRNG)

The `math` namespace provides a deterministic pseudo-random number generator (PRNG) based on the Xorshift32 algorithm.

### PRNG System Overview

The PRNG uses the **Xorshift32 algorithm**, a lightweight, high-quality pseudo-random number generator that:

- Uses only bitwise operations (XOR, shifts) - no multiplication or division
- Produces deterministic sequences based on seed value
- Has excellent statistical properties for game development
- Is compact and efficient for embedded systems (ESP32)

**Note:** This PRNG is suitable for gameplay mechanics (procedural generation, dice rolls, random events) but is **not cryptographically secure**.

---

### Thread Safety and Concurrency

**Global PRNG functions are NOT thread-safe.** The global PRNG state (`set_seed`, `rand01`, `rand_int`, etc.) should not be accessed concurrently from multiple threads or ISRs without external synchronization.

**For concurrent or multi-threaded scenarios, use the `Random` struct:**

```cpp
// Each thread has its own Random instance
thread_local Random threadRNG(generate_unique_seed());

// Safe concurrent access - no locks needed
void threadWorker() {
    int value = threadRNG.rand_int(1, 100);
}
```

**Recommendation:**

- Single-threaded games: Use global functions for simplicity
- Multi-threaded scenarios: Create `Random` instances per thread/context
- Per-entity randomness: Give each entity its own `Random` instance

---

### Implementation Quality

The PRNG implementation includes several quality improvements:

- **Bias-free integer generation**: Uses rejection sampling to ensure uniform distribution for any range
- **Fixed16 optimization**: On platforms without FPU, `rand01()` uses bit-shifting (no float operations)
- **Unified core algorithm**: Single `xorshift32()` function ensures consistent behavior
- **State validation**: Automatically prevents PRNG from entering invalid zero state

---

### Global PRNG Functions

- **`void set_seed(uint32_t seed)`**
    Initializes the PRNG with a specific seed. If seed is 0, uses fallback constant `0xDEADBEEF`.

    ```cpp
    // Initialize with specific seed for reproducible gameplay
    set_seed(12345);
    
    // Reset with 0 (will use fallback seed)
    set_seed(0);
    ```

- **`Scalar rand01()`**
    Returns random Scalar in range [0, 1]. Works with both float and Fixed16 Scalars.

    ```cpp
    Scalar roll = rand01();  // 0.0 to 1.0
    ```

- **`Scalar rand_range(Scalar min, Scalar max)`**
    Returns random Scalar in inclusive range [min, max].

    ```cpp
    // Random position between 10 and 100
    Scalar posX = rand_range(toScalar(10), toScalar(100));
    
    // Random damage between 5 and 15
    Scalar damage = rand_range(toScalar(5), toScalar(15));
    ```

- **`int32_t rand_int(int32_t min, int32_t max)`**
    Returns random integer in inclusive range [min, max].

    ```cpp
    // Roll a 6-sided die
    int roll = rand_int(1, 6);
    
    // Random array index
    int index = rand_int(0, arraySize - 1);
    ```

- **`bool rand_chance(Scalar p)`**
    Returns true with probability p (0.0 to 1.0).

    ```cpp
    // 30% chance to spawn power-up
    if (rand_chance(toScalar(0.3f))) {
        spawnPowerUp();
    }
    
    // Guaranteed event
    if (rand_chance(toScalar(1.0f))) { /* always true */ }
    ```

- **`Scalar rand_sign()`**
    Returns -1 or 1 as Scalar (50% probability each).

    ```cpp
    // Random direction
    Scalar direction = rand_sign();  // -1 or 1
    velocity.x = speed * direction;
    ```

---

### Instance-Based RNG: Random Struct

For scenarios requiring multiple independent random sequences (e.g., per-entity RNG, separate generators for different systems):

```cpp
// Create independent RNG instances
Random enemyRNG(12345);    // For enemy spawns
Random lootRNG(67890);     // For loot drops
Random visualRNG(11111);   // For visual effects

// Use independently - each has its own state
Scalar enemyX = enemyRNG.rand_range(toScalar(0), toScalar(100));
int lootTier = lootRNG.rand_int(1, 5);
```

The `Random` struct provides the same methods as global functions:

- `next()` - Generate next uint32_t value
- `rand01()` - Random Scalar in [0, 1]
- `rand_range(min, max)` - Random Scalar in range
- `rand_int(min, max)` - Random integer in range
- `rand_chance(p)` - Boolean with probability p
- `rand_sign()` - -1 or 1

---

### Common Usage Patterns

**Pattern 1: Seeded Procedural Generation**

```cpp
// Same seed always produces same level
set_seed(levelSeed);
for (int i = 0; i < 100; i++) {
    Scalar x = rand_range(toScalar(0), worldWidth);
    Scalar y = rand_range(toScalar(0), worldHeight);
    spawnTree(x, y);
}
```

**Pattern 2: Deterministic Dice Rolls**

```cpp
set_seed(turnNumber);  // Reproducible combat
int attackRoll = rand_int(1, 20);
int damageRoll = rand_int(1, 8);
```

**Pattern 3: Random Spawning with Probability**

```cpp
void update() {
    // 1% chance per frame to spawn enemy
    if (rand_chance(toScalar(0.01f))) {
        spawnEnemy();
    }
}
```

**Pattern 4: Shuffle Array (Fisher-Yates)**

```cpp
template<typename T>
void shuffleArray(T* array, int count) {
    for (int i = count - 1; i > 0; i--) {
        int j = rand_int(0, i);
        swap(array[i], array[j]);
    }
}
```

---

### Math Constants

- **`kPi`** - π (3.14159265)
- **`kDegToRad`** - Degrees to radians conversion factor
- **`kRadToDeg`** - Radians to degrees conversion factor
- **`kEpsilon`** - Small value for approximate equality checks

---

## Related Documentation

- [API Reference](API_REFERENCE.md) - Main index
- [Platform Compatibility Guide](PLATFORM_COMPATIBILITY.md)