# Fixed16

<Badge type="info" text="Struct" />

**Source:** `Fixed16.h`

## Description

Fixed-point 16.16 number implementation optimized for RISC-V.

Uses 32-bit integer storage: 16 bits for integer part, 16 bits for fractional part.
Designed for platforms without FPU (ESP32-C3, C2, C6).

## Methods

### `constexpr Fixed16()`

**Description:**

Default constructor (initializes to 0).

### `constexpr explicit Fixed16(int32_t rawValue, bool /*isRaw*/)`

**Description:**

Raw value constructor.

**Parameters:**

- `rawValue`: The raw 32-bit representation.
- `isRaw`: Dummy parameter to distinguish from integer constructor.

### `constexpr Fixed16(int v)`

**Description:**

Construct from integer.

**Parameters:**

- `v`: The integer value.

### `constexpr Fixed16(float v)`

**Description:**

Construct from float.

**Parameters:**

- `v`: The float value.

### `constexpr Fixed16(double v)`

**Description:**

Construct from double.

**Parameters:**

- `v`: The double value.

### `static constexpr Fixed16 fromRaw(int32_t raw)`

**Description:**

Factory method to create a Fixed16 directly from a raw 32-bit value.

**Parameters:**

- `raw`: The raw 32-bit representation.

**Returns:** The created Fixed16 instance.

### `constexpr int toInt() const`

**Description:**

Converts to integer (truncating fractional part).

**Returns:** The integer value.

### `constexpr float toFloat() const`

**Description:**

Converts to float.

**Returns:** The floating-point value.

### `constexpr double toDouble() const`

**Description:**

Converts to double.

**Returns:** The double-precision floating-point value.

### `constexpr int roundToInt() const`

**Description:**

Rounds to nearest integer.

**Returns:** The rounded integer value.

### `constexpr int floorToInt() const`

**Description:**

Computes floor and returns as integer.

**Returns:** The floor integer value.

### `constexpr int ceilToInt() const`

**Description:**

Computes ceiling and returns as integer.

**Returns:** The ceiling integer value.

### `explicit constexpr operator int() const`

**Description:**

Cast to int.

**Returns:** The truncated integer value.

### `explicit constexpr operator float() const`

**Description:**

Cast to float.

**Returns:** The floating-point value.

### `explicit constexpr operator double() const`

**Description:**

Cast to double.

**Returns:** The double value.

### `static Fixed16 sqrt(Fixed16 x)`

**Description:**

Computes the square root.

**Parameters:**

- `x`: The value to compute square root for.

**Returns:** The square root of x.
