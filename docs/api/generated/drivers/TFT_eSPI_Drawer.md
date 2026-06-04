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

### `void init()`

**Description:**

Initializes the TFT_eSPI library and the sprite buffer.
Sets up the SPI communication and allocates memory for the framebuffer.

### `void setRotation(uint16_t rotation)`

**Description:**

Sets the screen rotation.

**Parameters:**

- `rotation`: 0-3 corresponding to 0, 90, 180, 270 degrees.

### `void clearBuffer()`

**Description:**

Fills the sprite buffer with black color.

### `void sendBuffer()`

**Description:**

Pushes the sprite buffer to the physical display.
This is the "flip" operation in double buffering.

### `void drawFilledCircle(int x, int y, int radius, uint16_t color)`

### `void drawCircle(int x, int y, int radius, uint16_t color)`

### `void drawRectangle(int x, int y, int width, int height, uint16_t color)`

### `void drawFilledRectangle(int x, int y, int width, int height, uint16_t color)`

### `void drawLine(int x1, int y1, int x2, int y2, uint16_t color)`

### `void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color)`

### `void drawPixel(int x, int y, uint16_t color)`

### `void drawTileDirect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* data)`

**Description:**

Direct tile write to sprite buffer (optimized for tilemap rendering).

**Parameters:**

- `x`: Tile X position in sprite coordinates
- `y`: Tile Y position in sprite coordinates
- `width`: Tile width in pixels
- `height`: Tile height in pixels
- `data`: Pointer to 8bpp tile data (one byte per pixel, index into palette)

### `uint8_t* getSpriteBuffer()`

**Description:**

Get pointer to sprite buffer for direct manipulation.

**Returns:** Pointer to 8bpp sprite buffer, or nullptr if sprite not created

### `bool processEvents()`

**Description:**

Processes system events. Always true for embedded.

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
