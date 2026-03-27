## Context

The PRNG system was recently implemented in MathUtil using Xorshift32. While functional, the current implementation has quality and efficiency gaps:

- **Bias issue**: `rand_int()` uses modulo which creates uneven distribution when range is not a power of 2
- **Float inefficiency**: `rand01()` converts to float even when Scalar is Fixed16
- **Code duplication**: Global RNG and Random struct have separate Xorshift32 implementations
- **Undefined concurrency**: Thread-safety of global state is not documented

Target constraints:
- ESP32 (Xtensa LX6 and RISC-V variants)
- Native PC builds
- C++17 standard
- No dynamic memory allocation
- Must work with both float and Fixed16 Scalar types

## Goals / Non-Goals

**Goals:**
- Eliminate statistical bias in integer random generation
- Optimize Fixed16 path to avoid float operations
- Unify PRNG core logic between global and instance-based RNG
- Document thread-safety limitations explicitly
- Maintain 100% API compatibility (no breaking changes)
- Improve performance on ESP32 without FPU

**Non-Goals:**
- Add true hardware RNG integration (out of scope)
- Make PRNG cryptographically secure (not needed for games)
- Add thread-safety locks (would hurt performance on ESP32)
- Change any public API signatures

## Decisions

### D1: Rejection Sampling for Bias-Free Integer Generation

**Decision:** Use rejection sampling instead of modulo for `rand_int()`.

**Rationale:**
- Modulo creates bias when range doesn't evenly divide 2^32
- Rejection sampling guarantees uniform distribution at the cost of occasional retries
- For game use cases, the retry rate is acceptable (worst case < 50% for small ranges)

**Implementation:**
```cpp
// Reject values that would create bias
uint32_t range = static_cast<uint32_t>(max - min + 1);
uint32_t threshold = (UINT32_MAX / range) * range;
uint32_t r;
do {
    r = xorshift32_next(state);
} while (r >= threshold);
return min + static_cast<int32_t>(r % range);
```

**Alternatives Considered:**
- **Floating-point scaling**: Simpler but slower, especially on Fixed16 platforms
- **Lemire's method**: More complex, marginal benefit for our use case

### D2: Fixed-Specific rand01 Without Float Conversion

**Decision:** Use compile-time branching to avoid float operations in Fixed16 mode.

**Rationale:**
- Fixed16 stores values as int16_t with 8 fractional bits
- We can directly use high bits of random value for Fixed16 generation
- `static_cast<Fixed16>(r >> 16)` gives [0, 255] which maps to [0.0, 1.0] in Fixed16

**Implementation:**
```cpp
inline Scalar rand01() {
    uint32_t r = xorshift32_next();
    if constexpr (std::is_same_v<Scalar, float>) {
        return static_cast<float>(r) * (1.0f / static_cast<float>(UINT32_MAX));
    } else {
        // Fixed16: use high 16 bits, map to [0, 1] range
        return Fixed16::fromRaw(static_cast<int16_t>(r >> 16));
    }
}
```

### D3: Unified Xorshift32 Core Function

**Decision:** Create a single `xorshift32(uint32_t& state)` function used by both global and instance RNG.

**Rationale:**
- Eliminates code duplication
- Ensures identical behavior between global and Random struct
- Makes the core algorithm testable in isolation

**Implementation:**
```cpp
namespace detail {
    inline uint32_t xorshift32(uint32_t& state) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
}
```

### D4: Explicit Thread-Safety Documentation

**Decision:** Document that global RNG is NOT thread-safe and recommend Random struct for concurrent use.

**Rationale:**
- ESP32 games typically run single-threaded or with controlled concurrency
- Adding locks would add overhead for the common case
- Instance-based Random struct provides a clean solution for multi-threaded scenarios

**Documentation:**
- Add Doxygen comment: "NOT thread-safe. For concurrent use, create Random instances."
- Recommend Random struct in API documentation

## Risks / Trade-offs

- **[Risk]** Rejection sampling performance degradation
  - **Mitigation:** Worst-case retry rate is < 50%, typically much lower. Acceptable for game randomness.

- **[Risk]** Fixed16 precision reduction in rand01
  - **Mitigation:** Using 16 bits gives 65536 distinct values, sufficient for most game use cases.

- **[Risk]** Breaking existing behavior if implementation has bugs
  - **Mitigation:** Thorough testing, maintain deterministic behavior with same seeds.

- **[Risk]** Code size increase from rejection sampling loop
  - **Mitigation:** Minimal increase, loop typically executes once.
