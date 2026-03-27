## Why

The current PRNG implementation in MathUtil has several quality and efficiency issues that need addressing:
1. **Statistical bias** in `rand_int()` due to modulo operation with non-power-of-2 ranges
2. **Inefficient float operations** in `rand01()` even when using Fixed16 Scalar type
3. **Code duplication** between global RNG and Random struct implementations
4. **Undefined thread-safety** guarantees for the global state

These improvements are needed to ensure high-quality random distributions for gameplay mechanics while maintaining efficiency on ESP32 without FPU.

## What Changes

- **Fix bias in integer generation**: Replace modulo-based `rand_int()` with rejection sampling for uniform distribution
- **Optimize Fixed16 path**: Eliminate float operations in `rand01()` when Scalar is Fixed16
- **Unify PRNG core**: Centralize Xorshift32 algorithm to avoid code duplication
- **Document thread-safety**: Explicitly declare global RNG as not thread-safe
- **Add state validation**: Ensure PRNG never gets stuck in invalid states
- **Maintain API compatibility**: All existing function signatures remain unchanged

## Capabilities

### New Capabilities
- `prng-quality-improvements`: Enhanced PRNG with bias-free integer generation and Fixed16 optimization

### Modified Capabilities
- `prng-random-system`: Behavior improvements (better distribution, no API changes)

## Impact

- **Affected Code**: `include/math/MathUtil.h`
- **API Changes**: None - 100% backward compatible
- **Performance**: Improved on ESP32 (fewer float operations in Fixed16 mode)
- **Behavior**: Better statistical quality for integer random ranges
