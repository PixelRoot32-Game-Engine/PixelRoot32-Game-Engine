## Context

The PixelRoot32 game engine needs a deterministic random number generator for gameplay mechanics (procedural generation, dice rolls, random events) and testing. Current codebase uses no dedicated RNG, and `std::rand` is unsuitable for embedded ESP32 due to platform dependence and non-deterministic behavior.

Target constraints:
- ESP32 (Xtensa LX6) with FPU
- Native PC builds with SDL2
- C++17 standard
- No dynamic memory allocation preferred
- Must integrate with existing Scalar type (float on ESP32, Fixed16 on non-FPU)

## Goals / Non-Goals

**Goals:**
- Implement Xorshift32 algorithm for deterministic, reproducible sequences
- Provide seed control for reproducibility in testing and replays
- Integrate with Scalar abstraction (float/Fixed16)
- Keep implementation lightweight (no external libraries)
- Enable future extensibility (instance-based RNG)

**Non-Goals:**
- Cryptographically secure random numbers (not needed for game logic)
- Multi-threaded RNG (single-threaded embedded context)
- Platform-specific hardware RNG integration

## Decisions

### D1: Xorshift32 Algorithm

**Decision:** Use Xorshift32 instead of other PRNG algorithms.

**Rationale:**
- Very compact implementation (3-4 lines of code)
- Excellent statistical randomness for games
- Deterministic with seed input
- Uses only bitwise operations (xor, shift) - no multiplication/division
- Well-established algorithm with known properties
- No state initialization issues (unlike LCG)

**Alternatives Considered:**
- **LCG (Linear Congruential Generator)**: Simpler but worse statistical properties, more prone to patterns
- **PCG**: Better quality but more complex implementation
- **SplitMix64**: Better quality but requires 64-bit operations

### D2: Global State with Extensible Design

**Decision:** Implement global seed with optional Random struct for instance-based RNG.

**Rationale:**
- Simple API for common use cases (random events, dice)
- Instance-based approach possible for more complex scenarios (multiple entities with independent RNG)
- Backward compatible with global function approach

**Alternatives Considered:**
- **Pure global state**: Not extensible, harder to test
- **Pure instance-based**: More verbose for simple use cases

### D3: Integration via Scalar Type

**Decision:** Return Scalar from random functions to support both float and Fixed16 transparently.

**Rationale:**
- Follows existing pattern in MathUtil (math operations return Scalar)
- Allows compile-time choice between float and Fixed16 based on platform
- No runtime overhead for type selection

**Alternatives Considered:**
- **Template-based**: More complex API, less consistent with existing code
- **Separate float/Fixed16 functions**: Duplication, inconsistent API

## Risks / Trade-offs

- **[Risk]** Zero seed handling
  - **Mitigation:** If seed == 0, use fallback constant (e.g., 0xDEADBEEF) to avoid trivial all-zero state in Xorshift

- **[Risk]** Fixed16 range coverage
  - **Mitigation:** For Fixed16, use integer math to generate values, then convert to Fixed16 at the end to preserve precision

- **[Risk]** Uniform distribution precision at boundaries
  - **Mitigation:** Use inclusive-exclusive range semantics carefully; ensure max value is reachable but not exceeded
