## 1. PRNG Core Implementation

- [ ] 1.1 Add internal PRNG state (uint32_t) to MathUtil
- [ ] 1.2 Implement Xorshift32 next() function using only bitwise operations
- [ ] 1.3 Implement set_seed(uint32_t seed) with fallback for seed == 0

## 2. Random Generation Functions

- [ ] 2.1 Implement rand01() returning Scalar in range [0, 1]
- [ ] 2.2 Implement rand_range(Scalar min, Scalar max) for float compatibility
- [ ] 2.3 Implement rand_range(Scalar min, Scalar max) for Fixed16 compatibility
- [ ] 2.4 Implement rand_int(int32_t min, int32_t max) returning integer in range

## 3. Utility Functions

- [ ] 3.1 Implement rand_chance(Scalar p) returning boolean based on probability
- [ ] 3.2 Implement rand_sign() returning -1 or 1 as Scalar

## 4. Extensibility (Optional)

- [ ] 4.1 Create Random struct for instance-based RNG
- [ ] 4.2 Add constructor taking seed parameter to Random struct

## 5. Testing and Verification

- [ ] 5.1 Verify deterministic output with same seed
- [ ] 5.2 Verify different output with different seed
- [ ] 5.3 Test Fixed16 range coverage
- [ ] 5.4 Compile and test on native platform

## 6. Documentation

- [ ] 6.1 Document public API functions in MathUtil header with Doxygen-style comments
- [ ] 6.2 Add usage examples to engine documentation (seed initialization, common patterns)
- [ ] 6.3 Document algorithm choice (Xorshift32) and its properties
- [ ] 6.4 Update CHANGELOG.md with new PRNG system addition
- [ ] 6.5 Create README section for random number generation usage
