# Camera Effect Demo Example

Interactive menu-based demo that showcases all **`CameraEffectsSystem`** effects: **shake**, **punch** (4 directions), **offset**, and **cancel all**. Select an effect from the menu, watch it activate for a few seconds, then return to the menu. Built as a visual reference for developers integrating camera effects into their own games.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_CAMERA_EFFECTS`**
- **`PIXELROOT32_ENABLE_UI_SYSTEM`**

Both are enabled by default in [`PlatformDefaults.h`](../../include/platforms/PlatformDefaults.h). See **`platformio.ini`** in this folder for `native` and **`esp32dev`** presets.

The scene expects **`extern pixelroot32::core::Engine engine`** (see `src/platforms/native.h` / `esp32_dev.h` and `main.cpp`).

## Platforms

| Environment | Notes |
|-------------|--------|
| **`native`** | SDL2 window, 240×240 logical size (paths to SDL on Windows may need adjustment in `platformio.ini`). |
| **`esp32dev`** | **ST7789** TFT 240×240, TFT_eSPI-style pin defines in `platformio.ini`. |

The engine version or Git branch is set in **`lib_deps`** in `platformio.ini`.

## Controls

- **UP / DOWN** — navigate effect list (buttons **0** and **1** in `InputManager` order).
- **SELECT** — trigger the selected effect (button **5**).
- **BACK** — return to menu from active effect (button **4**).

## Features

- **`CameraEffectsSystem`**: shake, punch (up/down/left/right), offset, cancel all
- **`Camera2D`** integration via `apply(renderer, getCameraEffectOffset())`
- **`UIVerticalLayout`** menu with `UIButton` navigation (same pattern as `music_demo`)
- Auto-cancel after 2 seconds, instant cancel via "Cancel All"
- Static vs dynamic draw ordering to visualize effect displacement

## Effects

| Effect | Preset | Description |
|--------|--------|-------------|
| **Shake** | amp=8, dur=2000ms | Random oscillation with linear decay |
| **Punch Up** | amp=6, dur=1500ms | Directional impulse upward |
| **Punch Down** | amp=6, dur=1500ms | Directional impulse downward |
| **Punch Left** | amp=6, dur=1500ms | Directional impulse leftward |
| **Punch Right** | amp=6, dur=1500ms | Directional impulse rightward |
| **Offset** | amp=4, dur=2000ms | Constant displacement (default RIGHT) |
| **Cancel All** | — | Immediately clears all active effects |

## Documentation links

- [Graphics — CameraEffectsSystem](../../docs/api/graphics.md#cameraeffects)
- [Graphics — Camera2D](../../docs/api/graphics.md#camera2d)
- [Core — Scene / entities](../../docs/api/core.md)
- [Architecture](../../docs/architecture/architecture-index.md)

## Build

Run from **`examples/camera-effect-demo`**:

```bash
pio run -e native
pio run -e esp32dev
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
