# TouchConfig

<Badge type="info" text="Struct" />

**Source:** `TouchConfig.h`

## Description

Configuration for touch controller

Add to DisplayConfig or use standalone.
Define one of TOUCH_DRIVER_XPT2046 or TOUCH_DRIVER_GT911 in build flags.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `controller` | `TouchController` | Active controller |

## Methods

### `static TouchConfig createXPT2046(uint8_t cs, uint8_t irq = 255)`

**Description:**

Constructor for XPT2046

### `static TouchConfig createGT911(uint8_t irq = 4)`

**Description:**

Constructor for GT911
