## ADDED Requirements

### Requirement: PRNG generates deterministic sequences
The system SHALL generate reproducible sequences of pseudo-random numbers when initialized with a specific seed value.

#### Scenario: Same seed produces same sequence
- **WHEN** seed is set to 12345 and rand01() is called 3 times
- **THEN** the three values returned are identical across separate program executions

#### Scenario: Different seed produces different sequence
- **WHEN** seed is set to 12345 and rand01() is called
- **AND** seed is set to 67890 and rand01() is called
- **THEN** the values returned are different

### Requirement: PRNG uses Xorshift32 algorithm
The system SHALL implement the Xorshift32 algorithm using only bitwise operations (xor, shift).

#### Scenario: Algorithm uses only bitwise operations
- **WHEN** the PRNG implementation is examined
- **THEN** no multiplication, division, or modulo operations are used in the core algorithm
- **AND** only uint32_t state is maintained

### Requirement: rand01 returns value in range [0, 1]
The system SHALL return a Scalar value in the inclusive range [0, 1] when rand01() is called.

#### Scenario: Returns value in valid range
- **WHEN** rand01() is called
- **THEN** the returned value is >= 0.0 AND <= 1.0

#### Scenario: Works with float Scalar
- **WHEN** Scalar is defined as float and rand01() is called
- **THEN** the returned value is a float in range [0, 1]

#### Scenario: Works with Fixed16 Scalar
- **WHEN** Scalar is defined as Fixed16 and rand01() is called
- **THEN** the returned value is a Fixed16 in range [0, 1]

### Requirement: rand_range returns value in inclusive range
The system SHALL return a Scalar value within the inclusive range [min, max] when rand_range(min, max) is called.

#### Scenario: Returns value in specified range
- **WHEN** rand_range(5.0, 10.0) is called
- **THEN** the returned value is >= 5.0 AND <= 10.0

#### Scenario: Works with Scalar inputs
- **WHEN** rand_range(Scalar(5), Scalar(10)) is called
- **THEN** the returned value is a Scalar in range [5, 10]

### Requirement: rand_int returns integer in inclusive range
The system SHALL return an integer value within the inclusive range [min, max] when rand_int(min, max) is called.

#### Scenario: Returns integer in range
- **WHEN** rand_int(1, 6) is called multiple times
- **THEN** all returned values are integers >= 1 AND <= 6

### Requirement: set_seed initializes or resets the PRNG state
The system SHALL initialize or reset the PRNG state when set_seed(seed) is called.

#### Scenario: Seed 0 is handled safely
- **WHEN** set_seed(0) is called
- **THEN** the PRNG does not produce all-zero sequences
- **AND** subsequent calls to rand01() return valid values

#### Scenario: Setting seed resets the sequence
- **WHEN** seed is set to 100 and rand01() returns value A
- **AND** seed is set to 100 again
- **AND** rand01() is called
- **THEN** the returned value equals A

### Requirement: rand_chance returns boolean based on probability
The system SHALL return true or false based on the given probability p when rand_chance(p) is called.

#### Scenario: Probability 1.0 always returns true
- **WHEN** rand_chance(1.0) is called
- **THEN** the returned value is always true

#### Scenario: Probability 0.0 always returns false
- **WHEN** rand_chance(0.0) is called
- **THEN** the returned value is always false

### Requirement: rand_sign returns -1 or 1 as Scalar
The system SHALL return either -1 or 1 as a Scalar value when rand_sign() is called.

#### Scenario: Returns valid sign values
- **WHEN** rand_sign() is called multiple times
- **THEN** all returned values are either -1 or 1
