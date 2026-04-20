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
- **Display bottleneck benchmarking**: Reports per-frame and average region/pixel counts plus dirty ratio every ~2 seconds to compare optimizations

## Benchmarking

The hello_world example now includes built-in benchmarking for the display bottleneck optimization.
It reports metrics every ~2 seconds (60 frames at 30 FPS) to the serial console, allowing you to
compare the original full-frame behavior against the partial-update and reduced-color-depth
optimizations.

### How to Run the Benchmark

1. **Build with optimizations disabled (baseline):**
        pio run -e native --build-flags "-DENABLE_PARTIAL_UPDATES=0 -DDISPLAY_COLOR_DEPTH=16"

2. **Build with optimizations enabled:**
        pio run -e native --build-flags "-DENABLE_PARTIAL_UPDATES=1 -DDISPLAY_COLOR_DEPTH=16"

3. **Experiment with color depth (while keeping partial updates enabled):**
        pio run -e native --build-flags "-DENABLE_PARTIAL_UPDATES=1 -DDISPLAY_COLOR_DEPTH=8"
        pio run -e native --build-flags "-DENABLE_PARTIAL_UPDATES=1 -DDISPLAY_COLOR_DEPTH=24"

4. **On real ESP32 hardware (with profiling):**
        pio run -e esp32dev --build-flags "-DENABLE_PARTIAL_UPDATES=1 -DDISPLAY_COLOR_DEPTH=16 -DPIXELROOT32_ENABLE_PROFILING"

### What the Benchmark Shows

The output includes lines such as:
        [INFO] === DISPLAY BOTTLENECK BENCHMARK ===
        [INFO] Partial Updates: ENABLED
        [INFO] Current Frame - Regions: 0, Pixels Sent: 0
        [INFO] Average - Regions: 0, Pixels Sent: 0
        [INFO] Frames with optimization: 30, without: 30
        [INFO] Dirty Ratio: 0.0% (0/115200 pixels)
        [INFO] =====================================

- **Partial Updates**: Shows whether the engine is allowed to send only dirty regions (reflects the ENABLE_PARTIAL_UPDATES flag).
- **Current Frame - Regions / Pixels Sent**: Granular view of the most recent frame.
- **Average - Regions / Pixels Sent**: Running mean over the benchmark interval (60 frames).
- **Frames with optimization / without**: Counts how many frames fell into each mode within the interval.
- **Dirty Ratio**: Percentage of the logical screen that actually needed to be transmitted.

### Notes

- The benchmark runs automatically; no code changes are required.
- The native build (PC simulation) will show region/pixel counts and dirty ratio, letting you verify the algorithm works.
- On real hardware, the SPI transfer time (when profiling is enabled) will show a clear reduction when only a small region is dirty.

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