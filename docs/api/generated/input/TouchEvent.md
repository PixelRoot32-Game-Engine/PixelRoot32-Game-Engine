# TouchEvent

<Badge type="info" text="Struct" />

**Source:** `TouchEvent.h`

## Description

Compact touch event structure (12 bytes total, naturally aligned)

Memory layout (naturally aligned, no packing needed):
- timestamp: 4 bytes (offset 0)
- x:         2 bytes (offset 4)
- y:         2 bytes (offset 6)
- type:      1 byte (offset 8)
- flags:     1 byte (offset 9)
- id:        1 byte (offset 10)
- _padding:  1 byte (offset 11)
Total: 12 bytes

Invariants:
- timestamp always monotonically increasing per touch ID
- x, y always valid (within display bounds)
- type always non-None when queued

## Properties

| Name | Type | Description |
|------|------|-------------|
| `timestamp` | `uint32_t` | Timestamp in milliseconds |
| `x` | `int16_t` | X coordinate |
| `y` | `int16_t` | Y coordinate |
| `type` | `uint8_t` | Event type |
| `flags` | `uint8_t` | Event flags |
| `id` | `uint8_t` | Touch ID (0-4) |
| `_padding` | `uint8_t` | Explicit padding for alignment |

## Methods

### `, x(0)`

**Description:**

Default constructor - creates empty event

### `, y(0)`

### `, type(0)`

### `, flags(0)`

### `, id(0)`

### `, _padding(0)`

### `, x(xPos)`

**Description:**

Construct touch event with all fields

**Parameters:**

- `eventType`: Type of event
- `touchId`: Touch identifier
- `xPos`: X coordinate
- `yPos`: Y coordinate
- `ts`: Timestamp in ms
- `eventFlags`: Event flags

### `, y(yPos)`

### `, type(static_cast<uint8_t>(eventType))`

### `, flags(static_cast<uint8_t>(eventFlags))`

### `, id(touchId)`

### `TouchEventType getType() const`

**Description:**

Get event type as enum

**Returns:** Event type

### `TouchEventFlags getFlags() const`

**Description:**

Get event flags as enum

**Returns:** Event flags

### `void setType(TouchEventType eventType)`

**Description:**

Set event type from enum

**Parameters:**

- `eventType`: Type of event

### `void setFlags(TouchEventFlags eventFlags)`

**Description:**

Set event flags from enum

**Parameters:**

- `eventFlags`: Event flags

### `bool isValid() const`

**Description:**

Check if event is valid (has a type)

**Returns:** true if type is not None

### `bool isPrimary() const`

**Description:**

Check if this is a primary touch

**Returns:** true if Primary flag is set

### `bool isConsumed() const`

**Description:**

Check if event was consumed

**Returns:** true if Consumed flag is set

### `void consume()`

**Description:**

Mark event as consumed

### `void setPrimary()`

**Description:**

Set primary flag
