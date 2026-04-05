# Sprites Demo Example

Demonstrates **2BPP** (4 colors) and **4BPP** (16 colors) **sprites** on the same scene, including **animation** and popup assets under `src/assets/`. Entities are owned by the scene and updated/drawn through the usual **`Scene`** pipeline.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_2BPP_SPRITES`**
- **`PIXELROOT32_ENABLE_4BPP_SPRITES`**

[`SpritesDemoScene.h`](src/SpritesDemoScene.h) is compiled only when at least one of these is defined (see `#if` guard in the header).

See **`platformio.ini`** for **`native`** and **`esp32dev`**.

## Platforms

| Environment | Display |
|-------------|---------|
| **`native`** | SDL2, 240×240 |
| **`esp32dev`** | **ST7789** 240×240 (TFT_eSPI defines in `platformio.ini`) |

## Features

- **2bpp and 4bpp** sprite drawing on one screen
- **Sprite animation** via demo entities
- **Asset headers** (`Sprites.h`, `SpritesPopup.h`) as reference for embedding bitmaps

## Documentation links

- [Graphics API — sprites](../../docs/api/API_GRAPHICS.md)
- [Core — Scene / Entity](../../docs/api/API_CORE.md)

## Build

From **`examples/sprites`**:

```bash
pio run -e native
pio run -e esp32dev
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
