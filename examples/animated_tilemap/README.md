# Animated Tilemap Example

Demonstrates **4bpp** animated tilemaps, multi-palette setup (`ColorPaletteManager`), optional **scene arena**, and the engine **`StaticTilemapLayerCache`** when the driver exposes a direct logical 8bpp sprite buffer (typical **ESP32 + TFT_eSPI**).

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_TILE_ANIMATIONS`**
- **`PIXELROOT32_ENABLE_4BPP_SPRITES`** (this scene’s tilemaps are 4bpp)
- **`PIXELROOT32_ENABLE_2BPP_SPRITES`** — also enabled in **`platformio.ini`** so the packed sprite/tile driver configuration matches other multi-bpp samples.

See **`platformio.ini`** in this folder for **`native`**, **`esp32dev`**, and **`esp32cyd`** presets. **`esp32dev`** and **`esp32cyd`** enable **`PIXELROOT32_ENABLE_PROFILING`** (serial heartbeat: Update / Draw / Present); **`esp32dev`** omits the debug overlay so FPS numbers are closer to real draw cost. Touch remains **disabled** for this example (`TOUCH_CS=-1`).

## How this scene uses the engine cache

Snapshot logic is **`pixelroot32::graphics::StaticTilemapLayerCache`** (`graphics/StaticTilemapLayerCache.h`), not hand-rolled `memcpy` in the scene.

1. **`init()`**: `tilemapLayerCache.clear()`, then after level/palette setup **`invalidate()`** and **`allocateForRenderer(engine.getRenderer())`** (reserves **W×H** bytes once; `false` → full-draw fallback, no crash).
2. **`draw()`**: builds **`TileMap4bppDrawSpec`** arrays — **background** = **static** (snapshot), **ground + details** = **dynamic** (both use **`animManager`**) — and passes camera samples **`-renderer.getXOffset()`** / **`-renderer.getYOffset()`**, then **`Scene::draw(renderer)`**.
3. **`invalidateStaticLayerCache()`** forwards to **`tilemapLayerCache.invalidate()`**.

**Global `Engine`:** `AnimatedTilemapScene.cpp` expects **`extern pixelroot32::core::Engine engine`** (provided by your platform file, e.g. **`platforms/esp32_dev.h`** / **`native.h`** via **`main.cpp`**).

## Invalidation and performance (important)

| Situation | What to do |
|-----------|------------|
| Something in the **static** group must change visually (tiles, palette, mask, or **`step(deltaTime)`** on an animator bound to **background** only in this split) | Call **`invalidateStaticLayerCache()`** (or the fast path can show stale pixels). |
| **Ground** and **details** animate (dynamic group) | No per-frame **`invalidate()`** needed; they are redrawn every frame after **`memcpy`** restore. |
| **Ground** is in the **static** group **and** you **`step(deltaTime)`** its animator every frame | Either **invalidate every frame** or move **ground** to **dynamic** — this example uses the latter. |
| Camera / scroll only | Rebuild follows offset samples; no invalidation needed **only** for scroll. |
| **`getSpriteBuffer()`** is **`nullptr`** (e.g. some native paths) | All layers drawn every frame; snapshot unused. |

**Current example code:** `update()` steps both animators; **`draw()`** keeps **background** in the static group and **ground + details** in the dynamic group so the ESP32 fast path can **`memcpy`** restore the snapshot and only raster the animated layers (until the camera offset changes, which forces a rebuild).

### Opción A: omitir `draw` + `present` sin cambio visual

La escena implementa **`Scene::shouldRedrawFramebuffer()`**: combina **`TileAnimationManager::getVisualSignature()`** (ground + details) y el offset de cámara del **`Renderer`**. Si coincide con el último frame presentado **y** no hay entidades en la escena, el **`Engine`** no llama a **`draw()`** ni **`present()`** ese tick (ahorra casi todo el coste SPI en los ticks entre avances de frame de animación). Con **`PIXELROOT32_ENABLE_DEBUG_OVERLAY`** el motor siempre redibuja.

## Disabling the snapshot

- **Compile-time:** `-DPIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE=0` (default is **`1`** in **`PlatformDefaults.h`**). Saves ~**W×H** bytes RAM.
- **Runtime:** call **`setFramebufferCacheEnabled(false)`** on your **`StaticTilemapLayerCache`** instance. This example keeps the cache **private**; add a small public wrapper on **`AnimatedTilemapScene`** if you need toggling from game code.

## Documentation links

- API detail: [Graphics module — `StaticTilemapLayerCache`](../../docs/api/API_GRAPHICS.md#multi-layer-4bpp-tilemap-framebuffer-snapshot-statictilemaplayercache)
- Pipeline / layering: [Architecture — ESP32 rendering and tilemap caching](../../docs/ARCHITECTURE.md#esp32-rendering-pipeline-and-tilemap-caching)
- Static cache concept: [Architecture — Static tilemap layer cache](../../docs/ARCHITECTURE.md#static-tilemap-layer-cache-engine--scenes)

## Features

- Animated tilemaps + **`TileAnimationManager`**
- Multi-palette / **`ColorPaletteManager`**
- Optional **`PIXELROOT32_ENABLE_SCENE_ARENA`** in this example’s **`platformio.ini`**
- 2bpp/4bpp build flags enabled for the driver stack; layer data here is **4bpp**
- Engine **`StaticTilemapLayerCache`** (~**W×H** RAM when allocated and flag enabled)

## Build

Run from **`examples/animated_tilemap`** (this directory):

```bash
# Native (SDL2) — paths in platformio.ini may need local SDL include/lib on Windows
pio run -e native

# ESP32 (ST7789 240×240)
pio run -e esp32dev

# ESP32 CYD-style (ILI9341 240×320)
pio run -e esp32cyd
```
