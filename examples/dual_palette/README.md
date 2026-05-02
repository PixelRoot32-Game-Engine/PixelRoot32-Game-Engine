# Dual Palette Example

Demonstrates **dual palette mode**: the **background** and **sprite** paths can use **different color RAM tables**, so the same color **index** maps to **different RGB** on each layer (in this demo, NES-style colors on the background and Game Boy–style colors on sprites, per the scene comments).

Runtime entry point calls **`pixelroot32::graphics::enableDualPaletteMode(true)`** in `DualPaletteTestScene::init()` (see [`src/DualPaletteTestScene.cpp`](src/DualPaletteTestScene.cpp)).

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_SCENE_ARENA`**

See **`platformio.ini`** for **`native`** and **`esp32dev`**. Dual palette itself is toggled in code, not an extra `-D` in this project.

**`extern pixelroot32::core::Engine engine`** is provided by `src/platforms/native.h` or `esp32_dev.h`.

## Platforms

| Environment | Display |
|-------------|---------|
| **`native`** | SDL2, 240×240 |
| **`esp32dev`** | **ST7789** 240×240 (pins in `platformio.ini`) |

## Features

- **`Scene`** with layered entities: full-screen **background** shapes + **8×8 1bpp-style** test sprites
- **Dual palette** API (`enableDualPaletteMode`)
- **Scene arena** enabled for allocation patterns consistent with other advanced examples

## Documentation links

- [Graphics API](../../docs/api/graphics.md)
- [Architecture](../../docs/architecture/overview.md)

## Build

From **`examples/dual_palette`**:

```bash
pio run -e native
pio run -e esp32dev
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
