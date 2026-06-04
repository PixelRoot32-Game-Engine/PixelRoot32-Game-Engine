# U8G2_Drawer

<Badge type="info" text="Class" />

**Source:** `U8G2_Drawer.h`

**Inherits from:** [BaseDrawSurface](../graphics/BaseDrawSurface.md)

## Description

Implementation of DrawSurface using the U8G2 library for monochromatic OLED displays.

## Inheritance

[BaseDrawSurface](../graphics/BaseDrawSurface.md) → `U8G2_Drawer`

## Methods

### `void init()`

### `void setRotation(uint16_t rotation)`

### `void drawPixel(int x, int y, uint16_t color)`

### `void clearBuffer()`

### `void sendBuffer()`

### `void drawLine(int x1, int y1, int x2, int y2, uint16_t color)`

### `void drawRectangle(int x, int y, int w, int h, uint16_t color)`

### `void drawFilledRectangle(int x, int y, int w, int h, uint16_t color)`

### `void drawCircle(int x0, int y0, int r, uint16_t color)`

### `void drawFilledCircle(int x0, int y0, int r, uint16_t color)`

### `void drawBitmap(int x, int y, int w, int h, const uint8_t *bitmap, uint16_t color)`

**Description:**

Support for U8G2 native XBM bitmaps.
Note: This assumes the bitmap is in XBM format if color is 1, 
otherwise it uses the base implementation.

### `void setDisplaySize(int w, int h)`

### `void setPhysicalSize(int w, int h)`

### `U8G2* getU8g2() const`

### `void drawTileDirect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* data)`

### `uint8_t* getSpriteBuffer()`

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
