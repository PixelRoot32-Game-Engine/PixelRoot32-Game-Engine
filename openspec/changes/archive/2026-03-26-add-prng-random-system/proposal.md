## Why

The PixelRoot32 game engine currently lacks a deterministic, platform-independent random number generation system. Games require reproducible random sequences for features like procedural generation, deterministic gameplay, testing, and replay functionality. Using `std::rand` or `std::mt19937` is not suitable for ESP32 due to overhead and platform dependence.

## What Changes

- Add a lightweight PRNG system based on Xorshift32 algorithm to `MathUtil`
- Implement seed-based deterministic random number generation
- Add utility functions: `rand01()`, `rand_range()`, `rand_int()`, `set_seed()`
- Add optional helpers: `rand_chance()`, `rand_sign()`
- Ensure compatibility with the `Scalar` type system (float and Fixed16)

## Capabilities

### New Capabilities

- `prng-random-system`: Deterministic pseudo-random number generation using Xorshift32 algorithm, integrated with Scalar type abstraction

### Modified Capabilities

- `math-util`: Extend MathUtil with random number generation functions

## Impact

- **Affected Code**: `include/math/MathUtil.h`, `src/math/MathUtil.cpp`
- **Dependencies**: None (no external libraries)
- **Breaking Changes**: None
