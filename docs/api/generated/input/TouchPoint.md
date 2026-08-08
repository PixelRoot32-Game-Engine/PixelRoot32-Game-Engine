# TouchPoint

<Badge type="info" text="Struct" />

**Source:** `TouchPoint.h`

## Description

Normalized touch data structure.

This struct is the CONTRACT between TouchAdapter and the engine.
It MUST remain unchanged regardless of the underlying touch controller.

Invariants:
- Coordinates always valid: 0 <= x <= W, 0 <= y <= H
- No extreme noise (filtered by adapter)
- pressed state consistent
- timestamps monotonic

## Properties

| Name | Type | Description |
|------|------|-------------|
| `x` | `int16_t` | X coordinate (0 = left edge) |
| `y` | `int16_t` | Y coordinate (0 = top edge) |
| `pressed` | `bool` | True if touch is active |
| `id` | `uint8_t` | Touch ID (0 for single-touch, 0-4 for multi-touch) |
| `ts` | `uint32_t` | Timestamp in milliseconds |

## Methods

### `bool isValid(int16_t maxX, int16_t maxY) const`

**Description:**

Check if this touch point is valid (within bounds)

**Parameters:**

- `maxX`: Maximum X value (display width - 1)
- `maxY`: Maximum Y value (display height - 1)

**Returns:** true if coordinates are in valid range

### `bool isEmpty() const`

**Description:**

Check if this is a null/empty touch point

**Returns:** true if not pressed
