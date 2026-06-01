# Camera Demo Example

Side-scrolling platformer-style demo that showcases **`Camera2D`** (smoothing and horizontal bounds), **parallax background layers**, **`KinematicActor`** movement with tile-based ground and one-way platforms (`StaticActor`, collision layers), and **directional iris scene transitions** between two scenes. The world is wider than the screen so the camera follows the player.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_SCENE_ARENA`**
- **`PIXELROOT32_ENABLE_SCENE_TRANSITIONS`** (enabled by default in `PlatformDefaults.h`)

Additional engine features (physics actors, tilemaps) follow defaults from [`PlatformDefaults.h`](../../include/platforms/PlatformDefaults.h). See **`platformio.ini`** in this folder for `native` and **`esp32dev`** presets.

The scene expects **`extern pixelroot32::core::Engine engine`** (see `src/platforms/native.h` / `esp32_dev.h` and `main.cpp`).

## Platforms

| Environment | Notes |
|-------------|--------|
| **`native`** | SDL2 window, 240×240 logical size. Transitions work via RGB565 pixel buffer path. |
| **`esp32dev`** | **ST7789** TFT 240×240, TFT_eSPI-style pin defines in `platformio.ini`. Transitions work via 8bpp sprite buffer path. |

The engine version or Git branch is set in **`lib_deps`** in `platformio.ini`.

## Controls

- **Left / Right** — move (buttons **2** and **3** in `InputManager` order).
- **Jump** — button **4** (edge-triggered after release so hold does not spam jump).

## Scenes

The demo contains two scenes with identical gameplay but different color palettes:

| | CameraDemoScene (Scene 1) | CameraDemoScene2 (Scene 2) |
|---|---|---|
| Parallax far | DarkBlue | Teal |
| Parallax mid | DarkGreen | Orange |
| Tilemap color | Brown | Gold |

Both scenes share the same tilemap layout (ground + 3 platforms), player physics, and camera follow.

### Scene Transitions

- **Scene 1 → Scene 2**: Iris transition closes from the **right edge**, opens from the **left edge**. Triggered when the player reaches the rightmost end of the level.
- **Scene 2 → Scene 1**: Iris transition closes from the **left edge**, opens from the **right edge**. Triggered when the player reaches the leftmost edge (position ≤ 0).

The iris effect uses directional centers (`irisOutCx/Cy`, `irisInCx/Cy`) stored in `SceneManager` and re-applied after each `TransitionEffect::init()` call.

### Transition Rendering Paths

The engine automatically selects the correct rendering path:

- **ESP32 (8bpp)**: `TransitionEffect::apply()` operates on the TFT_eSPI sprite buffer (palette-indexed).
- **Native/SDL2 (RGB565)**: `TransitionEffect::applyRGB565()` operates directly on the SDL2 pixel buffer when `getSpriteBuffer()` returns `nullptr`.

## Features

- **`Camera2D`**: follow target, bounds, locked vertical scroll in this demo
- **Parallax** layers + tilemap strip for ground/platforms
- **`KinematicActor`** player cube (`PlayerCube`), gravity and one-way platform collision masks
- **Scene arena** for stable entity storage (fixed-size array, no heap allocation)
- **Directional iris transitions** between two scenes with opposite center directions
- **`TransitionEffect`**: Fade (palette LUT) and Iris (circle wipe), zero-allocation

## File Structure

```
src/
├── CameraDemoScene.h/.cpp     — Scene 1 (platformer, transitions to Scene 2)
├── CameraDemoScene2.h/.cpp    — Scene 2 (identical gameplay, different colors, back to Scene 1)
├── PlayerCube.h/.cpp          — KinematicActor with gravity/jump/movement
├── GameConstants.h            — Tile size, player dimensions, physics constants
├── GameLayers.h               — Collision layer bitmasks
└── platforms/
    ├── native.h               — SDL2 engine wiring
    └── esp32_dev.h            — ESP32 engine wiring
```

## Documentation links

- [Graphics — Camera2D](../../docs/api/graphics.md#camera2d)
- [Graphics — TransitionEffect](../../docs/api/generated/graphics/TransitionEffect.md)
- [Core — Scene / entities](../../docs/api/core.md)
- [Core — SceneManager transitions](../../docs/api/generated/core/SceneManager.md)
- [Physics — kinematic & static actors](../../docs/api/physics.md)
- [Architecture](../../docs/architecture/architecture-index.md)

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
