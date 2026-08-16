# Random

<Badge type="info" text="Struct" />

**Source:** `MathUtil.h`

## Description

Instance-based random number generator

Provides independent RNG state for scenarios requiring multiple
separate random sequences (e.g., per-entity RNG).

## Methods

### `explicit Random(uint32_t seed = 0xDEADBEEF)`

**Description:**

Constructor with seed parameter

**Parameters:**

- `seed`: Initial seed value. If 0, uses fallback constant.

### `uint32_t next()`

**Description:**

Generate next random value using Xorshift32

**Returns:** Random uint32_t value

### `Scalar rand01()`

**Description:**

Generate random Scalar in range [0, 1]
Uses bit-shifting for Fixed16 path to avoid float operations.

**Returns:** Random value in [0, 1] range

### `Scalar rand_range(Scalar min, Scalar max)`

**Description:**

Generate random Scalar in range [min, max]

**Parameters:**

- `min`: Minimum value (inclusive)
- `max`: Maximum value (inclusive)

**Returns:** Random value in [min, max] range

### `int32_t rand_int(int32_t min, int32_t max)`

**Description:**

Generate random integer in range [min, max]
Uses rejection sampling for bias-free uniform distribution.

**Parameters:**

- `min`: Minimum value (inclusive)
- `max`: Maximum value (inclusive)

**Returns:** Random integer in [min, max] range

### `bool rand_chance(Scalar p)`

**Description:**

Return true with probability p

**Parameters:**

- `p`: Probability in range [0, 1]

**Returns:** true with probability p, false otherwise

### `Scalar rand_sign()`

**Description:**

Return random sign -1 or 1

**Returns:** -1 or 1 as Scalar
