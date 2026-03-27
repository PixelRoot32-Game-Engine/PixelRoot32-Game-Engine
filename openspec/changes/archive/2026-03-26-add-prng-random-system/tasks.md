## 1. PRNG Core Implementation

- [x] 1.1 Add internal PRNG state (uint32_t) to MathUtil
- [x] 1.2 Implement Xorshift32 next() function using only bitwise operations
- [x] 1.3 Implement set_seed(uint32_t seed) with fallback for seed == 0

## 2. Random Generation Functions

- [x] 2.1 Implement rand01() returning Scalar in range [0, 1]
- [x] 2.2 Implement rand_range(Scalar min, Scalar max) for float compatibility
- [x] 2.3 Implement rand_range(Scalar min, Scalar max) for Fixed16 compatibility
- [x] 2.4 Implement rand_int(int32_t min, int32_t max) returning integer in range

## 3. Utility Functions

- [x] 3.1 Implement rand_chance(Scalar p) returning boolean based on probability
- [x] 3.2 Implement rand_sign() returning -1 or 1 as Scalar

## 4. Extensibility (Optional)

- [x] 4.1 Create Random struct for instance-based RNG
- [x] 4.2 Add constructor taking seed parameter to Random struct

## 5. Testing and Verification

- [x] 5.1 Verify deterministic output with same seed
- [x] 5.2 Verify different output with different seed
- [x] 5.3 Test Fixed16 range coverage
- [x] 5.4 Compile and test on native platform

## 6. Documentation

- [x] 6.1 Document public API functions in MathUtil header with Doxygen-style comments
- [x] 6.2 Add usage examples to engine documentation (seed initialization, common patterns)
- [x] 6.3 Document algorithm choice (Xorshift32) and its properties
