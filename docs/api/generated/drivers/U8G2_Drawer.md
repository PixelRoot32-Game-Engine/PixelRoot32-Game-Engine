# U8G2_Drawer

<Badge type="info" text="Class" />

**Source:** `U8G2_Drawer.h`

**Inherits from:** [BaseDrawSurface](../graphics/BaseDrawSurface.md)

## Description

Implementation of DrawSurface using the U8G2 library for monochromatic OLED displays.

## Inheritance

[BaseDrawSurface](../graphics/BaseDrawSurface.md) → `U8G2_Drawer`

## Methods

### `U8G2* getU8g2() const`

### `bool needsScaling() const`

**Description:**

Checks if scaling is needed.

**Returns:** true if logical != physical resolution.

### `void buildScaleLUTs()`

**Description:**

Builds the X and Y scaling lookup tables.

### `void freeScalingBuffers()`

**Description:**

Frees scaling-related memory.

### `void sendBufferScaled()`

**Description:**

Sends the buffer using software scaling.

### `inline uint8_t rgb565To1Bit(uint16_t color)`

**Description:**

Internal helper to convert RGB565 to 1-bit monochromatic.

**Returns:** 1 for "on", 0 for "off".
