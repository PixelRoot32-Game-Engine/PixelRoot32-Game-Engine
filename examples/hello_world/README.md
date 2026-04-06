# Hello World Example

Minimal PixelRoot32 project: **`UILabel`** text, **button polling** through **`InputManager`**, and a **background color** that cycles every few frames. Intended as the smallest “engine boots → scene draws → input works” check.

## Requirements (build flags)

No special `PIXELROOT32_ENABLE_*` flags beyond what **`lib/platformio.ini`** / defaults pull in. Resolution is set with **`PHYSICAL_DISPLAY_WIDTH`**, **`PHYSICAL_DISPLAY_HEIGHT`** (**128×128**) in **`platformio.ini`**.

The scene uses **`extern pixelroot32::core::Engine engine`** from your platform file (`src/platforms/native.h` or `esp32_dev.h`) — same pattern as other examples.

## Platforms

| Environment | Display |
|-------------|---------|
| **`native`** | SDL2, 128×128 logical size |
| **`esp32dev`** | **ST7735** 128×128 (GreenTab3 profile), SPI pins in **`platformio.ini`**: MOSI **23**, SCLK **18**, DC **2**, RST **4**, CS **-1** |
| **`esp32s3`** | **ST7735** 128×128 (GreenTab3 profile), SPI pins in **`platformio.ini`**: MOSI **12**, MISO **14**, SCLK **13**, DC **10**, RST **11**, CS **9** |

> ⚠️ **Note for ESP32-S3**: The `env:esp32s3` uses Arduino Core **2.0.14** as a workaround for DMA freeze issues (see [espressif/arduino-esp32#9618](https://github.com/espressif/arduino-esp32/issues/9618)). This is configured in `platformio.ini` via `platform_packages`.



## Controls

- **D-pad / face buttons** — any press is shown in the second label (`checkButtonPress()` in [`HelloWorldScene.cpp`](src/HelloWorldScene.cpp)).
- Background advances on a fixed frame interval (`COLOR_CHANGE_INTERVAL`).

## Features

- **`Scene`** lifecycle (`init` / `update` / `draw`)
- **`UILabel`** and `Renderer` text/color APIs
- **InputManager** button bitmask

## Documentation links

- [Core API](../../docs/api/API_CORE.md)
- [UI API](../../docs/api/API_UI.md)
- [Graphics / Renderer](../../docs/api/API_GRAPHICS.md)

## Build

From **`examples/hello_world`**:

```bash
pio run -e native
pio run -e esp32dev
pio run -e esp32_s3
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
