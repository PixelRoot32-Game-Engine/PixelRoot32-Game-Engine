# DrawSurface

<Badge type="info" text="Class" />

**Source:** `DrawSurface.h`

## Description

Abstract interface for platform-specific drawing operations.

This class defines the contract for any graphics driver (e.g., TFT_eSPI for ESP32,
SDL2 for Windows). It implements the Bridge pattern, allowing the Renderer to
remain platform-agnostic.

## Methods

### `virtual void init()`

**Description:**

Initializes the hardware or window.

### `virtual void setRotation(uint16_t rotation)`

**Description:**

Sets the display rotation.

**Parameters:**

- `rotation`: Rotation value. Can be index (0-3) or degrees (0, 90, 180, 270).

### `virtual void clearBuffer()`

**Description:**

Clears the frame buffer (fills with black or background color).

### `virtual void sendBuffer()`

**Description:**

Sends the frame buffer to the physical display.

### `virtual void drawFilledCircle(int x, int y, int radius, uint16_t color)`

**Description:**

Draws a filled circle.

**Parameters:**

- `x`: Center X coordinate.
- `y`: Center Y coordinate.
- `radius`: Radius of the circle.
- `color`: The fill color.

### `virtual void drawCircle(int x, int y, int radius, uint16_t color)`

**Description:**

Draws a circle outline.

**Parameters:**

- `x`: Center X coordinate.
- `y`: Center Y coordinate.
- `radius`: Radius of the circle.
- `color`: The outline color.

### `virtual void drawRectangle(int x, int y, int width, int height, uint16_t color)`

**Description:**

Draws a rectangle outline.

**Parameters:**

- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `width`: Width of the rectangle.
- `height`: Height of the rectangle.
- `color`: The outline color.

### `virtual void drawFilledRectangle(int x, int y, int width, int height, uint16_t color)`

**Description:**

Draws a filled rectangle.

**Parameters:**

- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `width`: Width of the rectangle.
- `height`: Height of the rectangle.
- `color`: The fill color.

### `virtual void drawLine(int x1, int y1, int x2, int y2, uint16_t color)`

**Description:**

Draws a line between two points.

**Parameters:**

- `x1`: Start X coordinate.
- `y1`: Start Y coordinate.
- `x2`: End X coordinate.
- `y2`: End Y coordinate.
- `color`: The line color.

### `virtual void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color)`

**Description:**

Draws a bitmap.

**Parameters:**

- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `width`: Width of the bitmap.
- `height`: Height of the bitmap.
- `bitmap`: Pointer to the bitmap data.
- `color`: The color to draw.

### `virtual void drawPixel(int x, int y, uint16_t color)`

**Description:**

Draws a single pixel.

**Parameters:**

- `x`: The X coordinate.
- `y`: The Y coordinate.
- `color`: The pixel color.

### `virtual void drawTileDirect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* data)`

**Description:**

Direct tile write to sprite buffer (optimized for tilemap rendering).

**Parameters:**

- `x`: Tile X position in sprite coordinates
- `y`: Tile Y position in sprite coordinates
- `width`: Tile width in pixels
- `height`: Tile height in pixels
- `data`: Pointer to 8bpp tile data (one byte per pixel, index into palette)

### `virtual uint8_t* getSpriteBuffer()`

**Description:**

Get pointer to sprite buffer for direct manipulation.

**Returns:** Pointer to 8bpp sprite buffer, or nullptr if not supported

### `virtual void setContrast(uint8_t level)`

**Description:**

Sets the display contrast/brightness.

**Parameters:**

- `level`: Contrast level (0-255).

### `virtual void setTextColor(uint16_t color)`

**Description:**

Sets the text color.

**Parameters:**

- `color`: The text color.

### `virtual void setTextSize(uint8_t size)`

**Description:**

Sets the text size.

**Parameters:**

- `size`: The text size multiplier.

### `virtual void setCursor(int16_t x, int16_t y)`

**Description:**

Sets the cursor position for text drawing.

**Parameters:**

- `x`: The X coordinate.
- `y`: The Y coordinate.

### `virtual uint16_t color565(uint8_t r, uint8_t g, uint8_t b)`

**Description:**

Converts RGB888 color to RGB565 format.

**Parameters:**

- `r`: Red component (0-255).
- `g`: Green component (0-255).
- `b`: Blue component (0-255).

**Returns:** 16-bit color value.

### `virtual void setDisplaySize(int w, int h)`

**Description:**

Sets the logical display size (rendering resolution).

**Parameters:**

- `w`: Width of the logical framebuffer.
- `h`: Height of the logical framebuffer.

### `virtual void setPhysicalSize(int w, int h)`

**Description:**

Sets the physical display size (hardware resolution).

**Parameters:**

- `w`: Physical display width.
- `h`: Physical display height.

### `virtual void setOffset(int x, int y)`

**Description:**

Sets the display offset (positioning of the active area).

**Parameters:**

- `x`: X offset.
- `y`: Y offset.

### `virtual bool processEvents()`

**Description:**

Processes platform events (e.g., SDL window events).

**Returns:** false if the application should quit, true otherwise.

### `virtual void present()`

**Description:**

Swaps buffers (for double-buffered systems like SDL).
