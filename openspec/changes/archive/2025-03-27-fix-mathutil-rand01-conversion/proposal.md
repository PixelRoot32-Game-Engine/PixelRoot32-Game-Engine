## Why

The `rand01()` functions in `MathUtil.h` fail to compile on the native platform (where `Scalar` is `float`). The functions return `Fixed16::fromRaw()` in the `else` branch, but the return type is `Scalar`. Since `Fixed16` has an explicit conversion operator to `float`, implicit conversion fails, causing compilation errors in native_test environment.

## What Changes

- **Fix implicit conversion error**: Add `static_cast<float>()` to `Fixed16::fromRaw()` return values in two locations:
  - Global function `rand01()` at line 245
  - `Random::rand01()` method at line 333

## Capabilities

### New Capabilities
<!-- No new capabilities - this is a bug fix -->

### Modified Capabilities
<!-- No spec modifications - this is an implementation fix only -->

## Impact

- **File**: `include/math/MathUtil.h`
- **Functions**: `rand01()` (global) and `Random::rand01()` (member)
- **Platform**: Affects native_test environment compilation
- **Runtime**: No behavioral changes - purely a type conversion fix
