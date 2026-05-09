# TouchConfig

<Badge type="info" text="Struct" />

**Source:** `TouchConfig.h`

## Description

Configuration for a touch controller (XPT2046 or GT911).

Set controller, communication parameters, and calibration transform.
Coordinate mapping: screenX = rawX * scaleX + offsetX (and same for Y).
Raw coordinates outside display bounds are clamped before mapping.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `controller` | `TouchController` | Active controller type. |

## Methods

### `static TouchConfig createXPT2046(uint8_t cs, uint8_t irq = 255)`

**Description:**

Factory: XPT2046 SPI configuration.

**Parameters:**

- `cs`: SPI chip-select pin.
- `irq`: Interrupt pin (255 = unused).

**Returns:** Configured TouchConfig.

### `static TouchConfig createGT911(uint8_t irq = 4)`

**Description:**

Factory: GT911 I2C configuration.

**Parameters:**

- `irq`: Interrupt pin (default 4).

**Returns:** Configured TouchConfig.
