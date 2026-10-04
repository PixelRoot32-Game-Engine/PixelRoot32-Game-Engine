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
| `spiClockHz` | `uint32_t` | SPI clock frequency in Hz (default 2.5 MHz). |
| `csPin` | `uint8_t` | SPI chip-select pin. |
| `irqPin` | `uint8_t` | Touch interrupt pin (255 = unused). |
| `i2cClockHz` | `uint32_t` | I2C clock frequency in Hz (default 400 kHz). |
| `i2cAddress` | `uint8_t` | GT911 I2C address (default 0x5D; alternate 0x14). |
| `scaleX` | `float` | Horizontal scale (raw → display coordinate multiplier). |
| `scaleY` | `float` | Vertical scale (raw → display coordinate multiplier). |
| `offsetX` | `int16_t` | Horizontal offset in display pixels after scaling. |
| `offsetY` | `int16_t` | Vertical offset in display pixels after scaling. |
| `displayWidth` | `uint16_t` | Physical display width in pixels. |
| `displayHeight` | `uint16_t` | Physical display height in pixels. |

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
