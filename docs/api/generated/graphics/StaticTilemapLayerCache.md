# StaticTilemapLayerCache

<Badge type="info" text="Class" />

**Source:** `StaticTilemapLayerCache.h`

## Description

Centralized framebuffer snapshot for static 4bpp tilemap layers.

On drivers that expose a direct logical 8bpp sprite buffer (e.g. TFT_eSPI),
this avoids redrawing “static” layers every frame when the sampled camera
position is unchanged and the cache has not been invalidated.

Override points:
- Compile-time: set PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE to 0 in build flags.
- Run-time: setFramebufferCacheEnabled(false) per scene or platform init.
- No sprite buffer or failed allocation: automatically falls back to full draw.

Allocate during scene init() via allocateForLogicalSize() or allocateForRenderer()
so the game loop does not hit the heap (see ARCH_MEMORY_SYSTEM.md).

## Methods

### `void clear()`

### `bool allocateForLogicalSize(int width, int height)`

**Description:**

Pre-allocate the snapshot for a logical framebuffer of width * height bytes.

**Returns:** false if dimensions are invalid or allocation failed.

### `bool allocateForRenderer(const Renderer& renderer)`

**Description:**

Same as allocateForLogicalSize(renderer.getLogicalWidth/Height()).

### `void invalidate()`

### `void draw(Renderer& renderer, int cameraSampleX, int cameraSampleY, const TileMap4bppDrawSpec* staticLayers, std::size_t staticLayerCount, const TileMap4bppDrawSpec* dynamicLayers, std::size_t dynamicLayerCount)`

**Parameters:**

- `cameraSampleX`: Typically
- `cameraSampleY`: Typically

### `void adviseFramebufferBeforeBeginFrame(Renderer& renderer, int cameraSampleX, int cameraSampleY, const TileMap4bppDrawSpec* staticLayers, std::size_t staticLayerCount, const TileMap4bppDrawSpec* dynamicLayers, std::size_t dynamicLayerCount) const`

### `void setFramebufferCacheEnabled(bool enabled)`

### `bool isFramebufferCacheEnabled() const`
