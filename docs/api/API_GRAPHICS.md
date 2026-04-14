# API Reference: Graphics Module

This document covers the rendering system, sprites, tilemaps, colors, fonts, and particle system in PixelRoot32.

> **Note:** This is part of the [API Reference](../API_REFERENCE.md). See the main index for complete documentation.

---

## Renderer

**Inherits:** None

High-level graphics rendering system. Provides a unified API for drawing shapes, text, and images, abstracting the underlying hardware implementation.

### Public Methods

- **`void beginFrame()`**
    Prepares the buffer for a new frame (clears screen). On drivers that support it (e.g. **TFT_eSPI_Drawer**), refreshes the internal pointer used for **direct logical framebuffer writes** (`DrawSurface::getSpriteBuffer()`) before clearing, so **2bpp / 4bpp** tile and sprite paths can avoid per-pixel virtual `drawPixel` calls.

- **`void endFrame()`**
    Finalizes the frame and sends the buffer to the display.

- **`void setOffsetBypass(bool bypass)`**
    Enables or disables camera offset bypass. When enabled, subsequent draw calls will ignore global x/y offsets (scrolling). This is typically managed automatically by `UILayout` when `fixedPosition` is enabled.

- **`bool isOffsetBypassEnabled() const`**
    Returns whether the offset bypass is currently active.

- **`void drawText(std::string_view text, int16_t x, int16_t y, Color color, uint8_t size)`**
    Draws a string of text using the native bitmap font system. Uses the default font set in `FontManager`, or a custom font if provided via the overloaded version.
  - **text**: The string to render (ASCII characters 32-126 are supported).
  - **x, y**: Position where text starts (top-left corner).
  - **color**: Color from the `Color` enum (uses sprite palette context).
  - **size**: Scale multiplier (1 = normal, 2 = double, 3 = triple, etc.).

- **`void drawText(std::string_view text, int16_t x, int16_t y, Color color, uint8_t size, const Font* font)`**
    Draws text using a specific font. If `font` is `nullptr`, uses the default font from `FontManager`.

- **`void drawTextCentered(std::string_view text, int16_t y, Color color, uint8_t size)`**
    Draws text centered horizontally at a given Y coordinate using the default font.

- **`void drawTextCentered(std::string_view text, int16_t y, Color color, uint8_t size, const Font* font)`**
    Draws text centered horizontally using a specific font. If `font` is `nullptr`, uses the default font from `FontManager`.

- **`void drawFilledCircle(int x, int y, int radius, uint16_t color)`**
    Draws a filled circle.

- **`void drawCircle(int x, int y, int radius, uint16_t color)`**
    Draws a circle outline.

- **`void drawRectangle(int x, int y, int width, int height, uint16_t color)`**
    Draws a rectangle outline.

- **`void drawFilledRectangle(int x, int y, int width, int height, uint16_t color)`**
    Draws a filled rectangle.

- **`void drawLine(int x1, int y1, int x2, int y2, uint16_t color)`**
    Draws a line between two points.

- **`void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color)`**
    Draws a bitmap image.

- **`void drawPixel(int x, int y, uint16_t color)`**
    Draws a single pixel.

- **`void setOffset(int x, int y)`**
    Sets the hardware alignment offset for the display.

- **`void setRotation(uint8_t rotation)`**
    Sets the hardware rotation of the display.

- **`void drawSprite(const Sprite& sprite, int x, int y, Color color, bool flipX = false)`**
    Draws a 1bpp monochrome sprite described by a `Sprite` struct using a palette `Color`. Bit 0 of each row is the leftmost pixel, bit (`width - 1`) is the rightmost pixel.

- **`void drawSprite(const Sprite2bpp& sprite, int x, int y, uint8_t paletteSlot = 0, bool flipX = false)`**
    Available when `PIXELROOT32_ENABLE_2BPP_SPRITES` is defined. Draws a packed 2bpp sprite using the specified sprite palette slot. Index `0` is treated as transparent.

- **`void drawSprite(const Sprite4bpp& sprite, int x, int y, uint8_t paletteSlot = 0, bool flipX = false)`**
    Available when `PIXELROOT32_ENABLE_4BPP_SPRITES` is defined. Draws a packed 4bpp sprite using the specified sprite palette slot. Index `0` is treated as transparent.

- **`void drawSprite(const Sprite2bpp& sprite, int x, int y, bool flipX = false)`**
    Legacy overload for backward compatibility. Equivalent to `drawSprite(sprite, x, y, 0, flipX)`.

- **`void drawSprite(const Sprite4bpp& sprite, int x, int y, bool flipX = false)`**
    Legacy overload for backward compatibility. Equivalent to `drawSprite(sprite, x, y, 0, flipX)`.

- **`void setSpritePaletteSlotContext(uint8_t slot)`**
    Sets the sprite palette slot context for multi-palette sprites. When active, all subsequent `drawSprite` calls for 2bpp/4bpp sprites will use this slot regardless of the `paletteSlot` parameter.

- **`uint8_t getSpritePaletteSlotContext() const`**
    Gets the current sprite palette slot context.

- **`void drawMultiSprite(const MultiSprite& sprite, int x, int y)`**
    Draws a layered sprite composed of multiple 1bpp `SpriteLayer` entries.

- **`void drawTileMap(const TileMap& map, int originX, int originY, Color color)`**
    Draws a tile-based background using a compact `TileMap` descriptor built on 1bpp `Sprite` tiles. Includes automatic Viewport Culling.

- **`void drawTileMap(const TileMap2bpp& map, int originX, int originY)`**
    Available when `PIXELROOT32_ENABLE_2BPP_SPRITES` is defined. Draws a 2bpp tilemap.

- **`void drawTileMap(const TileMap4bpp& map, int originX, int originY)`**
    Available when `PIXELROOT32_ENABLE_4BPP_SPRITES` is defined. Draws a 4bpp tilemap.

- **`void setDisplaySize(int w, int h)`**
    Sets the logical display size.

- **`void setDisplayOffset(int x, int y)`**
    Sets a global offset for all drawing operations.

- **`void setContrast(uint8_t level)`**
    Sets the display contrast/brightness (0-255).

---

## Platform Optimizations (ESP32)

The engine includes several low-level optimizations for the ESP32 platform to maximize performance:

- **DMA Support**: Buffer transfers to the display are handled via DMA (`pushImageDMA`), allowing the CPU to process the next frame while the current one is being sent to the hardware.
- **IRAM Execution**: Critical rendering functions (`drawPixel`, `drawSpriteInternal`, `resolveColor`, `drawTileMap`) are decorated with `IRAM_ATTR` to run from internal RAM, bypassing the slow SPI Flash latency.
- **Palette Caching**: Tilemaps cache the resolved RGB565 LUT per tile.
- **Viewport Culling**: All tilemap rendering functions automatically skip tiles that are outside the current screen boundaries.
- **Direct logical framebuffer**: **`DrawSurface::getSpriteBuffer()`** exposes the **TFT_eSPI** 8bpp sprite memory when available; **`Renderer::beginFrame()`** caches that pointer so **2bpp / 4bpp** rasterization can write packed pixels directly (same packing as **`TFT_eSprite::drawPixel`** for 8bpp). **`DrawSurface::drawTileDirect()`** allows blitting pre-packed 8bpp tile rows where the driver implements it.

### Multi-layer 4bpp tilemap framebuffer snapshot: `StaticTilemapLayerCache`

**Header:** `graphics/StaticTilemapLayerCache.h` (engine API).

Use this when a **direct logical 8bpp sprite buffer** exists (`DrawSurface::getSpriteBuffer()` after `beginFrame`) to avoid redrawing “static” **4bpp** tilemaps every frame: the engine draws the static group, copies the framebuffer into an internal buffer, then each frame restores with **`memcpy`** and redraws only the **dynamic** group until the sampled camera changes or you **invalidate**.

| Type / method | Role |
|---------------|------|
| **`TileMap4bppDrawSpec`** | `{ const TileMap4bpp* map; int originX; int originY; }` — `map == nullptr` entries are skipped. |
| **`allocateForLogicalSize(w,h)`** / **`allocateForRenderer(renderer)`** | Pre-allocate **W×H** bytes during **`Scene::init()`** (not in `draw`/`update`). Returns `false` if allocation fails → full-draw fallback. |
| **`invalidate()`** | Mark cache stale (tile/palette/mask changes, or **`step()`** on animators bound to **static** layers). |
| **`draw(renderer, cameraSampleX, cameraSampleY, staticSpecs, staticCount, dynamicSpecs, dynamicCount)`** | Camera samples are typically **`-renderer.getXOffset()`** / **`-renderer.getYOffset()`** so scroll triggers rebuild. |
| **`setFramebufferCacheEnabled(false)`** | Runtime opt-out per scene (e.g. profiling); compile-time: **`PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE=0`**. |

**Memory:** about **W×H** bytes (malloc-backed in `allocate*`; no heap use inside `draw`). If **`getSpriteBuffer()`** is **`nullptr`**, the implementation draws all groups every frame (same as SDL2 / non-sprite drivers).

**Example:** **`examples/animated_tilemap`** — `AnimatedTilemapScene` holds a **`StaticTilemapLayerCache`**, calls **`allocateForRenderer(engine.getRenderer())`** in **`init()`**, builds **`TileMap4bppDrawSpec`** arrays for **background** (static) and **ground + details** (dynamic), and exposes **`invalidateStaticLayerCache()`** as a thin wrapper over **`invalidate()`** when the static group changes.

For the full pipeline diagram and layering context, see [Architecture — ESP32 rendering pipeline and tilemap caching](../ARCHITECTURE.md#esp32-rendering-pipeline-and-tilemap-caching).

---

## Camera2D

**Inherits:** None

The `Camera2D` class provides a 2D camera system for managing the viewport and scrolling of the game world. It handles coordinate transformations and target following with configurable dead zones.

### Public Methods

- **`Camera2D(int viewportWidth, int viewportHeight)`**
    Constructs a new `Camera2D` with the specified viewport dimensions.

- **`void setBounds(float minX, float maxX)`**
    Sets the horizontal boundaries for the camera.

- **`void setVerticalBounds(float minY, float maxY)`**
    Sets the vertical boundaries for the camera.

- **`void setPosition(float x, float y)`**
    Sets the camera's position directly.

- **`void followTarget(float targetX)`**
    Updates the camera position to follow a target's x coordinate.

- **`void followTarget(float targetX, float targetY)`**
    Updates the camera position to follow a target's x and y coordinates.

- **`float getX() const`**
    Returns the current x position of the camera.

- **`float getY() const`**
    Returns the current y position of the camera.

- **`void apply(Renderer& renderer) const`**
    Applies the camera's transformation to the renderer.

- **`void setViewportSize(int width, int height)`**
    Updates the viewport size.

---

## Color

**Inherits:** None

The `Color` module manages the engine's color palettes and provides the `Color` enumeration for referencing colors within the active palette.

### PaletteType (Enum)

- `PR32` (Default): The standard PixelRoot32 palette.
- `NES`: Nintendo Entertainment System inspired palette.
- `GB`: Game Boy inspired palette (4 greens).
- `GBC`: Game Boy Color inspired palette.
- `PICO8`: PICO-8 fantasy console palette.

### Public Methods

- **`static void setPalette(PaletteType type)`**
    Sets the active color palette for the engine (Single Palette Mode).

- **`static void setCustomPalette(const uint16_t* palette)`**
    Sets a custom color palette defined by the user.

- **`static void enableDualPaletteMode(bool enable)`**
    Enables or disables dual palette mode.

- **`static void setBackgroundPalette(PaletteType palette)`**
    Sets the background palette (for backgrounds, tilemaps, etc.).

- **`static void setSpritePalette(PaletteType palette)`**
    Sets the sprite palette (for sprites, characters, etc.).

- **`static void setDualPalette(PaletteType bgPalette, PaletteType spritePalette)`**
    Convenience function that sets both background and sprite palettes at once.

- **`static uint16_t resolveColor(Color color)`**
    Converts a `Color` enum value to its corresponding RGB565 `uint16_t` representation.

- **`static uint16_t resolveColor(Color color, PaletteContext context)`**
    Converts a `Color` enum value to RGB565 based on the context (dual palette mode).

### Color (Enum)

- `Black`, `White`, `LightGray`, `DarkGray`
- `Red`, `DarkRed`, `Green`, `DarkGreen`, `Blue`, `DarkBlue`
- `Yellow`, `Orange`, `Brown`
- `Purple`, `Pink`, `Cyan`
- `LightBlue`, `LightGreen`, `LightRed`
- `Navy`, `Teal`, `Olive`
- `Gold`, `Silver`
- `Transparent` (special value, not rendered)
- `DebugRed`, `DebugGreen`, `DebugBlue` (debug colors)

---

## Font System

The engine includes a native bitmap font system that uses 1bpp sprites to render text.

### Font Structure

**Type:** `struct Font`

- **`const Sprite* glyphs`**: Array of sprite structures, one per character.
- **`uint8_t firstChar`**: First character code in the font (e.g., 32 for space).
- **`uint8_t lastChar`**: Last character code in the font (e.g., 126 for tilde).
- **`uint8_t glyphWidth`**: Fixed width of each glyph in pixels.
- **`uint8_t glyphHeight`**: Fixed height of each glyph in pixels.
- **`uint8_t spacing`**: Horizontal spacing between characters in pixels.
- **`uint8_t lineHeight`**: Vertical line height.

### FontManager

**Type:** `class FontManager`

Static utility class for managing fonts and calculating text dimensions.

- **`static void setDefaultFont(const Font* font)`**
    Sets the default font used by `Renderer::drawText()`.

- **`static const Font* getDefaultFont()`**
    Returns the currently active default font.

- **`static int16_t textWidth(const Font* font, std::string_view text, uint8_t size = 1)`**
    Calculates the pixel width of a text string.

- **`static bool isCharSupported(char c, const Font* font = nullptr)`**
    Checks if a character is supported by the font.

### Built-in Font: FONT_5X7

A built-in 5x7 pixel bitmap font containing ASCII characters from space (32) to tilde (126).

---

## Sprite Structures

### Sprite

Compact descriptor for monochrome bitmapped sprites used by `Renderer::drawSprite`.

- **`const uint16_t* data`**  
  Pointer to an array of 16-bit rows. Each `uint16_t` packs pixels for one row.

- **`uint8_t width`**  
  Sprite width in pixels (typically ≤ 16).

- **`uint8_t height`**  
  Sprite height in pixels.

### Sprite2bpp

Optional descriptor for packed 2bpp sprites, enabled when `PIXELROOT32_ENABLE_2BPP_SPRITES` is defined.

- **`const uint8_t* data`**: Packed 2bpp bitmap data.
- **`const Color* palette`**: Sprite-local palette.
- **`uint8_t width, height`**: Dimensions.
- **`uint8_t paletteSize`**: Number of palette entries.

### Sprite4bpp

Optional descriptor for packed 4bpp sprites, enabled when `PIXELROOT32_ENABLE_4BPP_SPRITES` is defined.

- **`const uint8_t* data`**: Packed 4bpp bitmap data.
- **`const Color* palette`**: Sprite-local palette.
- **`uint8_t width, height`**: Dimensions.
- **`uint8_t paletteSize`**: Number of palette entries.

### SpriteLayer

Single monochrome layer used by layered sprites (`MultiSprite`).

### MultiSprite

Multi-layer, multi-color sprite built from one or more `SpriteLayer` entries.

---

## TileMap

### TileMapGeneric (Template)

Generic descriptor for tile-based backgrounds.

#### Template Parameters

- **`T`**: The sprite type used for tiles (e.g., `Sprite`, `Sprite2bpp`, `Sprite4bpp`).

#### Properties

- **`uint8_t* indices`**: Array of tile indices.
- **`uint8_t width, height`**: Dimensions in tiles.
- **`const T* tiles`**: Pointer to the tileset array.
- **`uint8_t tileWidth, tileHeight`**: Tile dimensions in pixels.
- **`uint16_t tileCount`**: Number of unique tiles.
- **`uint8_t* runtimeMask`**: Optional bitmask for runtime tile activation.
- **`const uint8_t* paletteIndices`**: Optional per-cell background palette index.
- **`TileAnimationManager* animManager`**: Optional pointer for tile animations.

### Type Aliases

- **`TileMap`** = `TileMapGeneric<Sprite>` (1bpp)
- **`TileMap2bpp`** = `TileMapGeneric<Sprite2bpp>` (2bpp, conditional)
- **`TileMap4bpp`** = `TileMapGeneric<Sprite4bpp>` (4bpp, conditional)

---

## Tile Animation System

The Tile Animation System enables frame-based tile animations (water, lava, fire, etc.) while maintaining static tilemap data and ESP32-optimized performance.

### TileAnimation

**Namespace:** `pixelroot32::graphics`

- **`uint8_t baseTileIndex`**: First tile in the animation sequence.
- **`uint8_t frameCount`**: Number of frames in the animation.
- **`uint8_t frameDuration`**: Number of game frames to display each animation frame.

### TileAnimationManager

Manages tile animations for a tilemap.

#### Public Methods

- **`void step()`**  
  Advances all animations by one step. Call once per frame in `Scene::update()`.

- **`void reset()`**  
  Resets all animations to frame 0.

- **`uint8_t resolveFrame(uint8_t tileIndex) const`**  
  Resolves tile index to current animated frame. O(1) lookup.

---

## Tilemap rendering notes

`Renderer::drawTileMap` always applies **viewport culling** and per-tile rasterization. Optional **`drawTileDirect()`** on `DrawSurface` (when implemented by the driver) can blit pre-packed 8bpp tile rows into the logical sprite buffer. For static **4bpp** layer reuse, use **`StaticTilemapLayerCache`** (section above) and **`PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE`**.

---

## Tile Attribute System

The tile attribute system provides runtime access to custom metadata attached to tiles in tilemaps.

### TileAttribute

- **`const char* key`**: Attribute key (PROGMEM string).
- **`const char* value`**: Attribute value (PROGMEM string).

### TileAttributeEntry

- **`uint16_t x, y`**: Tile coordinates in layer space.
- **`uint8_t num_attributes`**: Number of attributes.
- **`const TileAttribute* attributes`**: PROGMEM array of key-value pairs.

### LayerAttributes

- **`const char* layer_name`**: Layer name (PROGMEM string).
- **`uint16_t num_tiles_with_attributes`**: Number of tiles with attributes.
- **`const TileAttributeEntry* tiles`**: PROGMEM array of tiles with attributes.

### Query Functions

**Namespace:** `pixelroot32::graphics`

- **`const char* get_tile_attribute(const LayerAttributes* layers, uint8_t num_layers, uint8_t layer_idx, uint16_t x, uint16_t y, const char* key)`**
    Returns the value of a specific attribute for a tile.

- **`bool tile_has_attributes(const LayerAttributes* layers, uint8_t num_layers, uint8_t layer_idx, uint16_t x, uint16_t y)`**
    Returns `true` if the tile has any attributes.

> [!IMPORTANT]
> Since attributes are stored in Flash memory on ESP32, you must use **`PIXELROOT32_STRCMP_P`** or **`PIXELROOT32_MEMCPY_P`** to compare or copy the returned values.

---

## Particle System

The particle system provides lightweight visual effects using a fixed-size pool of particles.

> **Note**: Only available if `PIXELROOT32_ENABLE_PARTICLES=1`

### Particle

**Namespace:** `pixelroot32::graphics::particles`

- **`Vector2 position`**: Current position.
- **`Vector2 velocity`**: Velocity vector.
- **`uint16_t color`**: Current color (RGB565).
- **`Color startColor`**: Initial color for interpolation.
- **`Color endColor`**: Final color for interpolation.
- **`uint8_t life`**: Current remaining life (frames or ticks).
- **`uint8_t maxLife`**: Total life duration.
- **`bool active`**: Whether the particle is currently in use.

### ParticleConfig

**Namespace:** `pixelroot32::graphics::particles`

- **`Color startColor`**: Color at the beginning.
- **`Color endColor`**: Color at the end.
- **`Scalar minSpeed, maxSpeed`**: Initial speed range.
- **`Scalar gravity`**: Gravity force applied to Y velocity.
- **`Scalar friction`**: Air resistance factor (0.0 - 1.0).
- **`uint8_t minLife, maxLife`**: Lifetime range.
- **`bool fadeColor`**: If true, interpolates color.
- **`Scalar minAngleDeg, maxAngleDeg`**: Emission angle range.

### ParticleEmitter

**Inherits:** [Entity](API_CORE.md#entity)

Manages a pool of particles to create visual effects.

- **`ParticleEmitter(Vector2 position, const ParticleConfig& cfg)`**
    Constructs a new ParticleEmitter.

- **`void update(unsigned long deltaTime) override`**
    Updates all active particles.

- **`void draw(Renderer& renderer) override`**
    Renders all active particles.

- **`void burst(Vector2 position, int count)`**
    Emits a burst of particles from a specific location.

### ParticlePresets

Namespace containing predefined `ParticleConfig` constants:

- `Fire`, `Explosion`, `Sparks`, `Smoke`, `Dust`

---

## SpriteAnimation

Lightweight, step-based animation controller for sprite frames.

### Properties

- **`const SpriteAnimationFrame* frames`**: Pointer to frame table.
- **`uint8_t frameCount`**: Number of frames.
- **`uint8_t current`**: Current frame index.

### Public Methods

- **`void reset()`**: Resets animation to first frame.
- **`void step()`**: Advances by one frame, wraps at end.
- **`const SpriteAnimationFrame& getCurrentFrame() const`**: Returns current frame.
- **`const Sprite* getCurrentSprite() const`**: Convenience accessor.

---

## DisplayConfig

Configuration settings for initializing displays.

- **`DisplayType type`**: The display type (ST7789, ST7735, NONE, CUSTOM, etc.)
- **`int rotation`**: Display rotation (0-3 or degrees)
- **`uint16_t physicalWidth, physicalHeight`**: Actual hardware resolution
- **`uint16_t logicalWidth, logicalHeight`**: Rendering resolution
- **`int xOffset, yOffset`**: Alignment offsets

---

## DrawSurface

Abstract interface for platform-specific drawing operations.

### Public Methods

- **`virtual void init()`**: Initializes the hardware or window.
- **`virtual void setRotation(uint8_t rotation)`**: Sets display rotation.
- **`virtual void clearBuffer()`**: Clears the frame buffer.
- **`virtual void sendBuffer()`**: Sends the frame buffer to the display.
- **`virtual void drawPixel(int x, int y, uint16_t color)`**: Draws a single pixel.
- **`virtual uint8_t* getSpriteBuffer()`**: Returns a pointer to the **logical** framebuffer for direct CPU writes when supported (**TFT_eSPI_Drawer** 8bpp sprite); default returns **`nullptr`** (e.g. **SDL2_Drawer**).
- **`virtual void drawTileDirect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data)`**: Optional 8bpp tile blit into that buffer; default no-op.
- **`virtual void drawLine(...)`**, **`drawRectangle(...)`**, **`drawCircle(...)`**, etc.

---

## BaseDrawSurface

**Inherits:** [DrawSurface](#drawsurface)

Optional base class that provides default primitive rendering.

- At minimum, implement: `init()`, `drawPixel()`, `sendBuffer()`, `clearBuffer()`
- Default implementations use `drawPixel()` - slow but functional.

---

## Related Documentation

- [API Reference](../API_REFERENCE.md) - Main index
- [API Core](API_CORE.md) - Engine, Entity, Scene
- [API Physics](API_PHYSICS.md) - Collision system
- [API UI](API_UI.md) - User interface system