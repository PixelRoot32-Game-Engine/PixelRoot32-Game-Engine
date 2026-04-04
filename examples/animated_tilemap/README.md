# Animated Tilemap Example

Demonstrates **4bpp** animated tilemaps, multi-palette setup (`ColorPaletteManager`), optional **scene arena**, and the engine **`StaticTilemapLayerCache`** when the driver exposes a direct logical 8bpp sprite buffer (typical **ESP32 + TFT_eSPI**).

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_TILE_ANIMATIONS`**
- **`PIXELROOT32_ENABLE_4BPP_SPRITES`** (this scene’s tilemaps are 4bpp)

See **`platformio.ini`** in this folder for **`native`**, **`esp32dev`**, and **`esp32cyd`** presets.

## How this scene uses the engine cache

Snapshot logic is **`pixelroot32::graphics::StaticTilemapLayerCache`** (`graphics/StaticTilemapLayerCache.h`), not hand-rolled `memcpy` in the scene.

1. **`init()`**: `tilemapLayerCache.clear()`, then after level/palette setup **`invalidate()`** and **`allocateForRenderer(engine.getRenderer())`** (reserves **W×H** bytes once; `false` → full-draw fallback, no crash).
2. **`draw()`**: builds **`TileMap4bppDrawSpec`** arrays — **background + ground** = **static** group, **details** = **dynamic** — and passes camera samples **`-renderer.getXOffset()`** / **`-renderer.getYOffset()`**, then **`Scene::draw(renderer)`**.
3. **`invalidateStaticLayerCache()`** forwards to **`tilemapLayerCache.invalidate()`**.

**Global `Engine`:** `AnimatedTilemapScene.cpp` expects **`extern pixelroot32::core::Engine engine`** (provided by your platform file, e.g. **`platforms/esp32_dev.h`** / **`native.h`** via **`main.cpp`**).

## Invalidation and performance (important)

| Situation | What to do |
|-----------|------------|
| Something in the **static** group must change visually (tiles, palette, mask, or **`step()`** on an animator bound to **background** or **ground**) | Call **`invalidateStaticLayerCache()`** (or the fast path can show stale pixels). |
| Only **details** (dynamic group) animate | You **do not** need invalidation for correctness; omit **`invalidateStaticLayerCache()`** in **`update()`** if ground/background are truly static so the **restore** branch can run when the camera is stable. |
| **Ground** is in the **static** group **and** you **`step()`** its animator every frame | You must **invalidate every frame** (as in the current **`AnimatedTilemapScene.cpp`**) or move **ground** to the **dynamic** specs in **`draw()`** — otherwise water/fire on ground would look wrong. |
| Camera / scroll only | Rebuild follows offset samples; no invalidation needed **only** for scroll. |
| **`getSpriteBuffer()`** is **`nullptr`** (e.g. some native paths) | All layers drawn every frame; snapshot unused. |

**Current example code:** `update()` calls **`getGroundAnimManager().step()`**, **`getDetailsAnimManager().step()`**, then **`invalidateStaticLayerCache()`** every frame. That keeps **ground** animations correct while **ground** stays in the static group, at the cost of **rebuilding the snapshot every frame** on the ESP32 fast path. To maximize **`memcpy`** restore wins, restrict **`step()` + invalidate** to what your **static** group actually needs (or reclassify layers).

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
