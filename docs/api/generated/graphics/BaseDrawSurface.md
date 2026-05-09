# BaseDrawSurface

<Badge type="info" text="Class" />

**Source:** `BaseDrawSurface.h`

**Inherits from:** [DrawSurface](./DrawSurface.md)

## Description

Optional base class for DrawSurface implementations providing default primitive rendering.

Users can inherit from this class to avoid implementing every single primitive.
At minimum, a subclass must implement:
- init() — initialize the hardware or window
- drawPixel() — draw a single pixel (all other primitives delegate here)
- sendBuffer() — transfer framebuffer to physical display
- clearBuffer() — fill framebuffer with background color

The default implementations use Bresenham-style algorithms for lines,
rectangles, circles, and bitmaps, all building on drawPixel().
These are slow but correct; override in performance-critical drivers.

## Inheritance

[DrawSurface](./DrawSurface.md) → `BaseDrawSurface`

## Methods

### `void setTextColor(uint16_t color)`

**Description:**

Sets the text color.

**Parameters:**

- `color`: 16-bit RGB565 text color.

### `void setTextSize(uint8_t size)`

**Description:**

Sets the text size multiplier.

**Parameters:**

- `size`: Size multiplier (1 = normal, 2 = double, etc.).

### `void setCursor(int16_t x, int16_t y)`

**Description:**

Sets the text cursor position.

**Parameters:**

- `x`: X coordinate in pixels.
- `y`: Y coordinate in pixels.

### `void setContrast(uint8_t level)`

**Description:**

Sets the display brightness.

**Parameters:**

- `level`: Contrast level (0–255).

### `void setRotation(uint16_t rot)`

**Description:**

Sets the display rotation.

**Parameters:**

- `rot`: Rotation index (0, 1, 2, 3) or degrees (0, 90, 180, 270).

### `void setDisplaySize(int w, int h)`

**Description:**

Sets the logical rendering resolution.

**Parameters:**

- `w`: Logical width in pixels.
- `h`: Logical height in pixels.

### `void setPhysicalSize(int w, int h)`

**Description:**

Sets the physical display resolution.

**Parameters:**

- `w`: Physical width in pixels.
- `h`: Physical height in pixels.

### `void setOffset(int x, int y)`

**Description:**

Sets the display origin offset.

**Parameters:**

- `x`: Horizontal offset in pixels.
- `y`: Vertical offset in pixels.

### `void present()`

**Description:**

Transfers the framebuffer to the physical display.

### `uint16_t color565(uint8_t r, uint8_t g, uint8_t b)`

**Description:**

Converts 24-bit RGB to RGB565.

**Parameters:**

- `r`: Red component (0-255).
- `g`: Green component (0-255).
- `b`: Blue component (0-255).

**Returns:** 16-bit RGB565 color.

### `void drawLine(int x1, int y1, int x2, int y2, uint16_t color)`

**Description:**

Draws a line between two points (Bresenham algorithm).

**Parameters:**

- `x1`: Start X coordinate.
- `y1`: Start Y coordinate.
- `x2`: End X coordinate.
- `y2`: End Y coordinate.
- `color`: 16-bit RGB565 line color.

### `void drawRectangle(int x, int y, int w, int h, uint16_t color)`

**Description:**

Draws a rectangle outline.

**Parameters:**

- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `w`: Width in pixels.
- `h`: Height in pixels.
- `color`: 16-bit RGB565 outline color.

### `void drawFilledRectangle(int x, int y, int w, int h, uint16_t color)`

**Description:**

Draws a filled rectangle.

**Parameters:**

- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `w`: Width in pixels.
- `h`: Height in pixels.
- `color`: 16-bit RGB565 fill color.

### `void drawCircle(int x0, int y0, int r, uint16_t color)`

**Description:**

Draws a circle outline (midpoint algorithm).

**Parameters:**

- `x0`: Center X coordinate.
- `y0`: Center Y coordinate.
- `r`: Radius in pixels.
- `color`: 16-bit RGB565 outline color.

### `void drawFilledCircle(int x0, int y0, int r, uint16_t color)`

**Description:**

Draws a filled circle (midpoint algorithm).

**Parameters:**

- `x0`: Center X coordinate.
- `y0`: Center Y coordinate.
- `r`: Radius in pixels.
- `color`: 16-bit RGB565 fill color.

### `void drawBitmap(int x, int y, int w, int h, const uint8_t *bitmap, uint16_t color)`

**Description:**

Draws a 1bpp bitmap, rendering only non-zero pixels.

**Parameters:**

- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `w`: Width in pixels.
- `h`: Height in pixels.
- `bitmap`: Pointer to packed 1bpp row data (byte per row, MSB left).
- `color`: 16-bit RGB565 color for "on" pixels.
