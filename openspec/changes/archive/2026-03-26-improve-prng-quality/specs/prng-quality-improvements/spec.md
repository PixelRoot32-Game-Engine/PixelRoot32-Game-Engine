## ADDED Requirements

### Requirement: Bias-free integer random generation
The system SHALL generate uniformly distributed integers without statistical bias using rejection sampling.

#### Scenario: Uniform distribution for non-power-of-2 ranges
- **WHEN** `rand_int(1, 6)` is called 60000 times
- **THEN** each value (1-6) appears approximately 10000 times with chi-square test passing

#### Scenario: No modulo bias present
- **WHEN** the implementation is examined
- **THEN** it does not use direct modulo without rejection sampling for ranges that don't divide 2^32 evenly

---

### Requirement: Optimized rand01 for Fixed16
The system SHALL generate random values in [0, 1] without using floating-point operations when Scalar is Fixed16.

#### Scenario: Fixed16 path avoids float operations
- **WHEN** `rand01()` is called with `Scalar` defined as `Fixed16`
- **THEN** no `float` division or conversion occurs at runtime
- **AND** the result uses `Fixed16::fromRaw()` with high bits from the random value

#### Scenario: Float path remains functional
- **WHEN** `rand01()` is called with `Scalar` defined as `float`
- **THEN** it returns a float in range [0.0, 1.0] using float division

---

### Requirement: Unified PRNG core algorithm
The system SHALL use a single shared implementation of the Xorshift32 algorithm for both global and instance-based RNG.

#### Scenario: Single core function exists
- **WHEN** examining the implementation
- **THEN** there is exactly one `xorshift32(uint32_t&)` function in `detail` namespace
- **AND** both global `rand01()` and `Random::rand01()` call this function

#### Scenario: Identical sequences from same seed
- **WHEN** `set_seed(12345)` is called and `rand01()` generates sequence A
- **AND** a `Random rng(12345)` instance generates sequence B with `rng.rand01()`
- **THEN** sequence A equals sequence B

---

### Requirement: Thread-safety explicitly defined
The system SHALL document that the global PRNG state is not thread-safe.

#### Scenario: Documentation warns about thread-safety
- **WHEN** reading the Doxygen comments for `set_seed()` and global random functions
- **THEN** it explicitly states "NOT thread-safe"
- **AND** it recommends using `Random` struct for concurrent scenarios

---

### Requirement: Safe state handling
The system SHALL prevent the PRNG from entering or remaining in an invalid state (e.g., state = 0).

#### Scenario: Zero seed uses fallback
- **WHEN** `set_seed(0)` is called
- **THEN** the internal state becomes `0xDEADBEEF` instead of 0

#### Scenario: Random struct validates seed
- **WHEN** a `Random` instance is created with seed 0
- **THEN** it uses the fallback seed internally

---

### Requirement: API backward compatibility
The system SHALL maintain all existing public function signatures and behavior.

#### Scenario: Existing code compiles without changes
- **WHEN** existing code using `rand01()`, `rand_range()`, `rand_int()`, `set_seed()` is compiled
- **THEN** it compiles without errors or warnings
- **AND** deterministic behavior with same seeds is preserved

---

### Requirement: ESP32 optimization without FPU
The system SHALL minimize floating-point operations on platforms without hardware FPU.

#### Scenario: Fixed16 mode avoids float in hot path
- **WHEN** the engine runs on a platform without FPU (Fixed16 mode)
- **THEN** critical random functions (`rand01`, `rand_range`) use only integer/bitwise operations
