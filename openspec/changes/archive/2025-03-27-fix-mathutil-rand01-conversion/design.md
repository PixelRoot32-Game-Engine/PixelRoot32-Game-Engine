## Context

The `MathUtil.h` header provides random number generation utilities through the `rand01()` function (global) and `Random::rand01()` method (struct member). Both functions use `if constexpr` to handle different `Scalar` types:
- For `float` (native platform): returns `static_cast<float>(r) / static_cast<float>(UINT32_MAX)`
- For `Fixed16` (ESP32): returns `Fixed16::fromRaw(static_cast<int16_t>(r >> 16))`

The bug occurs because the `else` branch returns a `Fixed16` object, but the function return type is `Scalar` (which is `float` on native platform). The `Fixed16` class has an `explicit` conversion operator to `float`, preventing implicit conversion.

## Goals / Non-Goals

**Goals:**
- Fix compilation error on native_test environment
- Maintain identical runtime behavior for both platforms
- Ensure explicit cast makes the conversion intentional and clear

**Non-Goals:**
- No algorithm changes to random number generation
- No API signature changes
- No new functionality or features

## Decisions

**Decision**: Add `static_cast<float>()` wrapper around `Fixed16::fromRaw()` return values

**Rationale**: 
- The `if constexpr` branch already handles the float path correctly
- The `else` branch is meant to return a value convertible to Scalar, but the explicit conversion operator blocks implicit conversion
- `static_cast<float>()` explicitly invokes the conversion operator, satisfying the compiler
- This is a minimal fix that doesn't change any logic or behavior

**Alternative considered**: Change `Fixed16` conversion operator from `explicit` to implicit
- **Rejected**: The explicit operator is intentional to prevent accidental precision loss in embedded contexts

## Risks / Trade-offs

**Risk**: Cast might hide future type mismatches
→ **Mitigation**: The `if constexpr` pattern ensures compile-time type safety; the cast is only applied in the Fixed16 branch where we explicitly want conversion to float for native testing

**Risk**: Runtime performance impact on native
→ **Mitigation**: The float path doesn't hit the else branch; this cast only executes in test scenarios that exercise the Fixed16 code path on native (which shouldn't happen in normal operation)
