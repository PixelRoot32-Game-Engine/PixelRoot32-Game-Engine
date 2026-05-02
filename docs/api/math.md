# API Reference: Math Module

> **Source of truth:**
> - `include/math/Scalar.h`
> - `include/math/MathUtil.h`
> - `include/math/Vector2.h`
> - `include/math/Fixed16.h`

## Overview

The Math module provides foundational mathematical structures and utilities tailored for embedded game development. It includes the `Scalar` abstraction for handling either floating-point or fixed-point arithmetic, a 2D vector class (`Vector2`), common math utilities, and a fast Pseudo-Random Number Generator (PRNG).

## Key Concepts

### The `Scalar` Type

To support both FPU-equipped devices (like the ESP32) and FPU-less devices, PixelRoot32 uses a `Scalar` typedef throughout its physics and geometry systems.

- **Floating-Point Mode (Default)**: `Scalar` maps to `float`.
- **Fixed-Point Mode**: `Scalar` maps to `Fixed16` (a custom Q16.16 fixed-point implementation) to avoid soft-float overhead.

This abstraction ensures that engine code remains performant and cross-platform without `#ifdef` clutter in high-level game logic.

### Vector2

The standard 2D vector class (`Vector2`) is templated by default but typedef'd to `Vector2f` (using `float`) or a custom type based on the `Scalar` definition. It supports standard vector operations: addition, subtraction, dot product, cross product, length, normalization, and distance calculations.

### MathUtil

Provides optimized, commonly used math functions. On devices with `PIXELROOT32_HAS_FAST_RSQRT`, the engine uses a fast reciprocal square root algorithm for normalization, bypassing costly standard library calls.

## Pseudo-Random Number Generator (PRNG)

The engine provides a high-performance **Xoroshiro128+** implementation, replacing the standard `rand()` for game logic and particle systems.

**Why Xoroshiro128+?**
- Significantly faster than `rand()`.
- Generates higher quality noise with a longer period.
- Avoids thread contention (standard `rand()` uses a hidden global lock, which causes extreme frame drops when called concurrently from the Audio and Main cores).

### Thread Safety

> [!WARNING]  
> The **Global PRNG** (`MathUtil::random()`) is **NOT thread-safe**. Do not call global random functions from the audio thread or network thread. Doing so will cause race conditions and undefined behavior.

For multi-threaded environments, each thread (e.g., the audio synthesizer) must instantiate its own local `Random` object.

### Instance-Based RNG (`Random`)

A standalone struct wrapping the Xoroshiro128+ state. It allows you to create isolated random streams (e.g., for procedural generation where you want a reproducible seed independent of the global game state).

## Common Usage Patterns

### Vector Math

```cpp
using pixelroot32::math::Vector2;

Vector2 playerPos(10.0f, 20.0f);
Vector2 enemyPos(50.0f, 60.0f);

// Distance
float dist = playerPos.distanceTo(enemyPos);

// Direction vector
Vector2 direction = enemyPos - playerPos;
direction.normalize(); // Now a unit vector

// Move player
float speed = 150.0f;
float dt = 0.016f;
playerPos += direction * (speed * dt);
```

### Random Numbers

```cpp
using pixelroot32::math::MathUtil;

// Seed the global generator (usually done once at startup)
MathUtil::seedRandom(1337);

// Random float between 0.0 and 1.0
float chance = MathUtil::randomFloat();
if (chance < 0.25f) {
    // 25% chance
}

// Random integer between 1 and 10 (inclusive)
int damage = MathUtil::randomInt(1, 10);

// Random float within a range
float x = MathUtil::randomRange(100.0f, 200.0f);
```

### Reproducible Procedural Generation

```cpp
using pixelroot32::math::Random;

// Create a local generator with a specific seed
Random mapGen(987654321);

// Generate terrain heights (always the same for this seed)
for (int i = 0; i < 100; ++i) {
    float height = mapGen.randomRange(10.0f, 50.0f);
    // ...
}
```

## Constants

| Constant | Description |
|----------|-------------|
| `PI` | `3.14159265359f` |
| `TWO_PI` | `6.28318530718f` |
| `HALF_PI` | `1.57079632679f` |
| `DEG_TO_RAD` | `0.0174532925f` |
| `RAD_TO_DEG` | `57.295779513f` |

## Related Documentation

- [API Reference](index.md) - Main index
- [Physics Module](physics.md) - The primary consumer of the Math module