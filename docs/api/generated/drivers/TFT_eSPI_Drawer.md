# TFT_eSPI_Drawer

<Badge type="info" text="Class" />

**Source:** `TFT_eSPI_Drawer.h`

**Inherits from:** [BaseDrawSurface](../graphics/BaseDrawSurface.md)

## Description

Concrete implementation of DrawSurface for ESP32 using the TFT_eSPI library.

This class handles low-level interaction with the display hardware via SPI.
It uses a sprite (framebuffer) to minimize flickering and tearing.

## Inheritance

[BaseDrawSurface](../graphics/BaseDrawSurface.md) → `TFT_eSPI_Drawer`

## Methods

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

Sends the buffer using hardware DMA and software scaling.

### `void scaleLine(const uint8_t* spriteBase, int srcY, uint16_t* dst)`

**Description:**

Scales a single line from 8bpp logical to 16bpp physical.
