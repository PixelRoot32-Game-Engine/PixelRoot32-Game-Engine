# Camera Demo Example

Side-scrolling platformer-style demo that showcases **`Camera2D`** (smoothing and horizontal bounds), **parallax background layers**, and **`KinematicActor`** movement with tile-based ground and one-way platforms (`StaticActor`, collision layers). The world is wider than the screen so the camera follows the player.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_SCENE_ARENA`**

Additional engine features (physics actors, tilemaps) follow defaults from [`PlatformDefaults.h`](../../include/platforms/PlatformDefaults.h). See **`platformio.ini`** in this folder for `native` and **`esp32dev`** presets.

The scene expects **`extern pixelroot32::core::Engine engine`** (see `src/platforms/native.h` / `esp32_dev.h` and `main.cpp`).

## Platforms

| Environment | Notes |
|-------------|--------|
| **`native`** | SDL2 window, 240×240 logical size (paths to SDL on Windows may need adjustment in `platformio.ini`). |
| **`esp32dev`** | **ST7789** TFT 240×240, TFT_eSPI-style pin defines in `platformio.ini`. |

The engine version or Git branch is set in **`lib_deps`** in `platformio.ini`.

## Controls

- **Left / Right** — move (buttons **2** and **3** in `InputManager` order).
- **Jump** — button **4** (edge-triggered after release so hold does not spam jump).

## Features

- **`Camera2D`**: follow target, bounds, locked vertical scroll in this demo
- **Parallax** layers (`GameLayers`) + tilemap strip for ground/platforms
- **`KinematicActor`** player cube (`PlayerCube`), gravity and one-way platform collision masks
- **Scene arena** for stable entity storage

## Documentation links

- [Graphics — Camera2D](../../docs/api/API_GRAPHICS.md#camera2d)
- [Core — Scene / entities](../../docs/api/API_CORE.md)
- [Physics — kinematic & static actors](../../docs/api/API_PHYSICS.md)
- [Architecture](../../docs/ARCHITECTURE.md)

## Build

Run from **`examples/camera`**:

```bash
pio run -e native
pio run -e esp32dev
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
