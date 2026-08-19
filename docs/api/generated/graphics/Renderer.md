# Renderer

<Badge type="info" text="Class" />

**Source:** `Renderer.h`

## Description

High-level graphics rendering system.

The Renderer class provides a unified API for drawing shapes, text, and images.
It abstracts the underlying hardware implementation (DrawSurface) and manages
display configuration, including rotation and offsets.

## Methods

### `void init()`

**Description:**

Initializes the renderer and the underlying draw surface.

### `void beginFrame()`

**Description:**

Prepares the buffer for a new frame (clears screen).

### `void forceFullRedraw()`

**Description:**

Forces a full clear on the next beginFrame when `PIXELROOT32_ENABLE_DIRTY_REGIONS` is on (no-op otherwise).

### `void resetFramebufferClearSuppressionAdvice()`

### `void accumulateFramebufferClearSuppressionAdvice(bool skipClearDueToMemcpyRestore)`

**Description:**

Accumulate framebuffer clear suppression advice from a scene.

**Parameters:**

- `skipClearDueToMemcpyRestore`: True if the scene will restore framebuffer via memcpy

### `bool restoreDirtyCellsFromSnapshot(const uint8_t* snapshot)`

**Description:**

Repaints only the cells dirtied by the PREVIOUS frame, taking
       their pixels from a static-layer snapshot.

**Parameters:**

- `snapshot`: Framebuffer-sized image of the static layers alone.

**Returns:** true when the selective restore ran.

### `void endFrame()`

**Description:**

Finalizes the frame and sends the buffer to the display.

### `DrawSurface& getDrawSurface()`

**Description:**

Gets the underlying DrawSurface implementation.

**Returns:** Reference to the DrawSurface.

### `void drawText(std::string_view text, int16_t x, int16_t y, Color color, uint8_t size)`

**Description:**

Draws a string of text using the default font.

**Parameters:**

- `text`: The text to draw.
- `x`: X coordinate.
- `y`: Y coordinate.
- `color`: Text color.
- `size`: Text size multiplier.

### `void drawText(std::string_view text, int16_t x, int16_t y, Color color, uint8_t size, const Font* font)`

**Description:**

Draws a string of text using a specific font.

**Parameters:**

- `text`: The text to draw.
- `x`: X coordinate.
- `y`: Y coordinate.
- `color`: Text color.
- `size`: Text size multiplier.
- `font`: Pointer to the font to use. If nullptr, uses the default font.

### `void drawTextCentered(std::string_view text, int16_t y, Color color, uint8_t size)`

**Description:**

Draws text centered horizontally at a given Y coordinate using the default font.

**Parameters:**

- `text`: The text to draw.
- `y`: Y coordinate.
- `color`: Text color.
- `size`: Text size.

### `void drawTextCentered(std::string_view text, int16_t y, Color color, uint8_t size, const Font* font)`

**Description:**

Draws text centered horizontally at a given Y coordinate using a specific font.

**Parameters:**

- `text`: The text to draw.
- `y`: Y coordinate.
- `color`: Text color.
- `size`: Text size.
- `font`: Pointer to the font to use. If nullptr, uses the default font.

### `void drawFilledCircle(int x, int y, int radius, Color color)`

**Description:**

Draws a filled circle.

**Parameters:**

- `x`: Center X coordinate.
- `y`: Center Y coordinate.
- `radius`: Radius of the circle.
- `color`: Fill color.

### `void drawCircle(int x, int y, int radius, Color color)`

**Description:**

Draws a circle outline.

**Parameters:**

- `x`: Center X coordinate.
- `y`: Center Y coordinate.
- `radius`: Radius of the circle.
- `color`: Outline color.

### `void drawRectangle(int x, int y, int width, int height, Color color)`

**Description:**

Draws a rectangle outline.

**Parameters:**

- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `width`: Width of the rectangle.
- `height`: Height of the rectangle.
- `color`: Outline color.

### `void drawFilledRectangle(int x, int y, int width, int height, Color color)`

**Description:**

Draws a filled rectangle.

**Parameters:**

- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `width`: Width of the rectangle.
- `height`: Height of the rectangle.
- `color`: Fill color.

### `void drawFilledRectangleW(int x, int y, int width, int height, uint16_t color)`

**Description:**

Draws a filled rectangle with a 16-bit color.

**Parameters:**

- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `width`: Width of the rectangle.
- `height`: Height of the rectangle.
- `color`: Fill color (RGB565).

### `void drawLine(int x1, int y1, int x2, int y2, Color color)`

**Description:**

Draws a line between two points.

**Parameters:**

- `x1`: Start X.
- `y1`: Start Y.
- `x2`: End X.
- `y2`: End Y.
- `color`: Line color.

### `void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, Color color)`

**Description:**

Draws a bitmap image.

**Parameters:**

- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `width`: Width of the bitmap.
- `height`: Height of the bitmap.
- `bitmap`: Pointer to the bitmap data.
- `color`: Color to draw the bitmap pixels (if monochrome) or ignored.

### `void drawPixel(int x, int y, Color color)`

**Description:**

Draws a single pixel.

**Parameters:**

- `x`: X coordinate.
- `y`: Y coordinate.
- `color`: Pixel color.

### `void setDisplaySize(int w, int h)`

**Description:**

Sets the logical display size (rendering resolution).

**Parameters:**

- `w`: Logical width.
- `h`: Logical height.

### `int getLogicalWidth() const`

### `int getLogicalHeight() const`

### `void setDisplayOffset(int x, int y)`

**Description:**

Sets a global offset for all drawing operations.

**Parameters:**

- `x`: X offset.
- `y`: Y offset.

### `void setContrast(uint8_t level)`

**Description:**

Sets the display contrast (brightness).

**Parameters:**

- `level`: Contrast level (0-255).

### `void setFont(const uint8_t* font)`

**Description:**

Sets the font for text rendering.

**Parameters:**

- `font`: Pointer to the font data array.

### `int getXOffset() const`

**Description:**

Gets the current global X offset.

**Returns:** The X offset.

### `int getYOffset() const`

**Description:**

Gets the current global Y offset.

**Returns:** The Y offset.

### `void setRenderContext(PaletteContext* context)`

**Description:**

Sets the render context for palette selection.

**Parameters:**

- `context`: The palette context to use (Background or Sprite).
                Pass nullptr to use method-specific defaults.

### `PaletteContext* getRenderContext() const`

**Description:**

Gets the current render context.

**Returns:** Pointer to the current context, or nullptr if using defaults.

### `void setSpritePaletteSlotContext(uint8_t slot)`

**Description:**

Sets the sprite palette slot context for multi-palette sprites.

**Parameters:**

- `slot`: Palette slot (0-7). To disable context, call with 0 or use default.

### `uint8_t getSpritePaletteSlotContext() const`

**Description:**

Gets the current sprite palette slot context.

**Returns:** Current palette slot, or 0xFF if context is inactive.

### `void drawSprite(const Sprite& sprite, int x, int y, Color color, bool flipX = false)`

**Description:**

Draws a 1bpp monochrome sprite using the Sprite descriptor.

**Parameters:**

- `sprite`: Sprite descriptor (data, width, height).
- `x`: Top-left X coordinate in logical screen space.
- `y`: Top-left Y coordinate in logical screen space.
- `color`: Color used for "on" pixels.
- `flipX`: If true, sprite is mirrored horizontally.

### `void drawSprite(const Sprite& sprite, int x, int y, float scaleX, float scaleY, Color color, bool flipX = false)`

**Description:**

Draws a scaled 1bpp monochrome sprite.

**Parameters:**

- `sprite`: Sprite descriptor.
- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `scaleX`: Horizontal scaling factor (e.g., 1.25).
- `scaleY`: Vertical scaling factor (e.g., 1.25).
- `color`: Color used for "on" pixels.
- `flipX`: If true, sprite is mirrored horizontally before scaling.

### `void drawSprite(const Sprite2bpp& sprite, int x, int y, uint8_t paletteSlot = 0, bool flipX = false)`

**Description:**

Draws a 2bpp sprite using a specific palette slot.

**Parameters:**

- `sprite`: The 2bpp sprite descriptor.
- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `paletteSlot`: The palette slot to use.
- `flipX`: True to mirror horizontally.

### `void drawSprite(const Sprite4bpp& sprite, int x, int y, uint8_t paletteSlot = 0, bool flipX = false)`

**Description:**

Draws a 4bpp sprite using a specific palette slot.

**Parameters:**

- `sprite`: The 4bpp sprite descriptor.
- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `paletteSlot`: The palette slot to use.
- `flipX`: True to mirror horizontally.

### `void drawSprite(const Sprite2bpp& sprite, int x, int y, bool flipX)`

**Description:**

Draws a 2bpp sprite (legacy).

**Parameters:**

- `sprite`: The 2bpp sprite descriptor.
- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `flipX`: True to mirror horizontally.

### `void drawSprite(const Sprite4bpp& sprite, int x, int y, bool flipX)`

**Description:**

Draws a 4bpp sprite (legacy).

**Parameters:**

- `sprite`: The 4bpp sprite descriptor.
- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `flipX`: True to mirror horizontally.

### `void drawMultiSprite(const MultiSprite& sprite, int x, int y)`

**Description:**

Draws a multi-layer sprite composed of several 1bpp layers.

**Parameters:**

- `sprite`: Multi-layer sprite descriptor.
- `x`: Top-left X coordinate in logical screen space.
- `y`: Top-left Y coordinate in logical screen space.

### `void drawMultiSprite(const MultiSprite& sprite, int x, int y, float scaleX, float scaleY)`

**Description:**

Draws a scaled multi-layer sprite.

**Parameters:**

- `sprite`: Multi-layer sprite descriptor.
- `x`: Top-left X coordinate.
- `y`: Top-left Y coordinate.
- `scaleX`: Horizontal scaling factor.
- `scaleY`: Vertical scaling factor.

### `void drawTileMap(const TileMap& map, int originX, int originY, Color color = Color::White, LayerType layerType = LayerType::Dynamic)`

**Description:**

Draws a tilemap of 1bpp sprites.

**Parameters:**

- `layerType`: Static layers assume full coverage (no dirty marks); Dynamic marks dirt (animated maps mark only tiles whose frame changed vs last step snapshot when possible).

### `void drawTileMap(const TileMap2bpp& map, int originX, int originY, LayerType layerType = LayerType::Dynamic)`

**Description:**

Draws a tilemap of 2bpp sprites.

### `void drawTileMap(const TileMap4bpp& map, int originX, int originY, LayerType layerType = LayerType::Dynamic)`

**Description:**

Draws a tilemap of 4bpp sprites.

### `void drawTileMap(const TileMap4bpp& map, int originX, int originY, LayerType layerType, const pixelroot32::math::ProjectionSpec& projection)`

**Description:**

Draws a tilemap of 4bpp sprites through a projection basis.

**Parameters:**

- `projection`: Places, culls and marks cells through this basis
       instead of the axis-aligned grid.

       Draw order is the caller's responsibility: this path iterates
       cells row-major and never sorts. Row-major is a correct
       back-to-front paint order only when `math::rowMajorIsPainterOrder(projection)`
       holds -- i.e. a `+1` step along either cell axis moves a tile
       strictly forward on screen (`axisXy > 0 && axisYy > 0`). It does
       NOT hold for every valid spec: an orthogonal or oblique basis
       (`axisXy == 0`) returns `false` from that predicate and is still
       painted correctly here whenever its art fills its cell and
       never overhangs it. The predicate is sufficient, not necessary --
       assert it at the spec's declaration site when tiles can overhang
       their cell, not unconditionally.

::: tip
`map.tileFootY` is what this path anchors from. The PixelRoot32
      Tilemap Editor does not export a foot-anchor table today, so an
      editor-exported map has `tileFootY == nullptr` and every tile
      anchors at its top-left corner (`footYFor()` returns 0
      uniformly) -- correct for a uniform-height tileset, wrong for
      one with mixed tile heights. Closing that export gap is a later,
      separate change.
:::

### `void setOffsetBypass(bool bypass)`

**Description:**

Enables or disables ignoring global offsets for subsequent draw calls.

**Parameters:**

- `bypass`: True to ignore offsets, false to apply them (default).

### `bool isOffsetBypassEnabled() const`

**Description:**

Checks if offset bypass is currently enabled.

**Returns:** True if offsets are being ignored.

### `void setDebugDirtyCellOverlay(bool enabled)`

**Description:**

When PIXELROOT32_DEBUG_MODE is defined and enabled, outlines curr dirty cells before present.

### `bool isDebugDirtyCellOverlayEnabled() const`
