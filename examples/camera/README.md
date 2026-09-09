# Camera Demo Example

> **⚠️ Demonstration example** — This project is provided **as an example** to showcase the capabilities of the PixelRoot32 Game Engine and what you can build with it. It may **not be 100% functional or finished**; some features can be incomplete, experimental, or work in progress.

One scene, one subject: **`Camera2D`** in a world wider than the screen.

The platformer around it — a cube, a ground strip, three platforms — exists only
to give the camera something to follow. What the example actually teaches is how
the three camera APIs coexist:

- **`Camera2D`** — `followTarget()` with smoothing, horizontal bounds, locked vertical scroll.
- **`CameraEffects`** — shake, punch and offset, as a per-frame *offset*.
- **`CameraTween`** — a scripted pan, as a *position*.

That last distinction is the point of the example. See
[Effects and tweens](#effects-and-tweens).

Parallax is here for the same reason the platformer is: three layers scrolling at
their own rates are the cheapest way to see that the camera is moving at all.

> **Looking for scene transitions?** They used to live in a second scene in this
> example, which made it a transitions demo wearing a camera demo's clothes.
> Transitions are demonstrated in
> [PixelRoot32-Demo-Projects](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects),
> where complete projects belong.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_SCENE_ARENA`**
- **`PIXELROOT32_ENABLE_CAMERA_EFFECTS=1`** — shake / punch / offset
- **`PIXELROOT32_ENABLE_CAMERA_TWEEN=1`** — the scripted camera pan
- **`PIXELROOT32_ENABLE_PHYSICS=1`** — the follow target needs to move and land

`lib/platformio.ini` also turns **off** what the subject does not need: audio,
particles, the UI system, and — since there is only one scene —
`PIXELROOT32_ENABLE_SCENE_TRANSITIONS`, which otherwise defaults to `1`. Measured
on `esp32dev`, switching that last one off saves **1,552 bytes of flash and 40
bytes of RAM**. The flag list is part of the lesson: it tells you what each
feature costs.

Everything else follows the defaults in
[`PlatformDefaults.h`](../../include/platforms/PlatformDefaults.h). See
**`platformio.ini`** in this folder for the `native` and `esp32dev` presets.

The scene expects **`extern pixelroot32::core::Engine engine`** (see
`src/platforms/native.h` / `esp32_dev.h` and `main.cpp`).

## Platforms

| Environment | Notes |
|-------------|--------|
| **`native`** | SDL2 window, 240×240 logical size. |
| **`esp32dev`** | **ST7789** TFT 240×240, TFT_eSPI-style pin defines in `platformio.ini`. |

The engine version or Git branch is set in **`lib_deps`** in `platformio.ini`.

## Controls

- **Left / Right** — move (buttons **2** and **3** in `InputManager` order).
- **Jump** — button **4** (edge-triggered after release so holding does not spam jump).
- **B** — button **5**, fires the next camera effect in the cycle: shake, punch up/down/left/right, offset, then back to shake. The active one is named in the top-left corner.
- **Up** — button **0**, pans the camera to the third platform, holds, and pans back.
- **Down** — button **1**, cancels every active effect.

Landing after a fall fires a short downward punch on its own, which is the more
realistic way to use the capability: an effect keyed to a game event rather than
to a button.

## Effects and tweens

The two capabilities look similar and behave very differently, which is the
reason they share one example.

**An effect is an offset, not a position.** `cameraEffects` resolves to a single
`Vector2` per frame, read through `getCameraEffectOffset()` and added to the
display offset at draw time. The camera's own position is never touched, so
`followTarget()` keeps working underneath a shake with no coordination at all.
Note that the offset is added to every parallax layer here — shaking only the
foreground would visibly tear the background away from it. The HUD is drawn after
the offset is cleared, so it stays rock steady while the world shakes.

**A tween is a position.** `CameraTween::update()` calls `camera.setPosition()`
directly. That means it competes with `followTarget()` for the same value, and
calling both in the same frame makes the follow win every time and the tween
appear to do nothing. This example suspends following for the duration of the
pan (`TourStage != Idle`), which is the pattern to copy.

The return leg targets wherever the player is when the hold ends, not where the
camera started — the player is free to keep walking during the pan, and returning
to a stale position would snap the camera on the next follow frame.

## Features

- **`Camera2D`**: follow target with smoothing, horizontal bounds, locked vertical scroll
- **Parallax**: three layers at 0.4×, 0.7× and 1.0× of the camera position
- **`CameraEffects`**: shake, four-way punch, offset, and a landing punch driven by a game event
- **`CameraTween`**: a two-leg scripted pan with `EaseInOutQuad`, holding at the target
- **`KinematicActor`** player cube (`PlayerCube`), gravity and one-way platform collision masks
- **Scene arena** for stable entity storage (fixed-size array, no heap allocation)

## File Structure

```
src/
├── CameraDemoScene.h/.cpp     — the scene: camera, parallax, effects, tween
├── PlayerCube.h/.cpp          — KinematicActor with gravity/jump/movement
├── GameConstants.h            — tile size, player dimensions, physics, effect and tween presets
├── GameLayers.h               — collision layer bitmasks
└── platforms/
    ├── native.h               — SDL2 engine wiring
    └── esp32_dev.h            — ESP32 engine wiring
```

## Documentation links

- [Graphics — Camera2D](../../docs/api/graphics.md#camera2d)
- [Core — Scene / entities](../../docs/api/core.md)
- [Physics — kinematic & static actors](../../docs/api/physics.md)
- [Architecture](../../docs/architecture/architecture-index.md)

## Build

Run from **`examples/camera`**:

```bash
pio run -e native
pio run -e esp32dev
```

> **Windows note.** The `native` environment compiles with MSYS2's MinGW `g++`.
> If `pio run -e native` fails with a bare `*** [...] Error 1` and no compiler
> diagnostic, `g++` is not on `PATH`. Add it before building:
> `export PATH="/c/msys64/mingw64/bin:$PATH"`.

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
