# Physics Demo Example

Interactive **physics** sandbox: **`KinematicActor`** player, **`RigidActor`** boxes and circles (AABB and circle shape), **`StaticActor`** floors/walls, **sensors**, and **`ActorTouchController`** for dragging/spawning on touch-capable builds.

When **`PIXELROOT32_ENABLE_UI_SYSTEM`** is on (default in [`PlatformDefaults.h`](../../include/platforms/PlatformDefaults.h)), **`esp32cyd`** builds also expose **touch UI**: **`UITouchButton`** (full reset), **`UITouchSlider`** (dynamic spawn count), **`UITouchCheckbox`**, and horizontal/vertical **layouts** — see [`PhysicsDemoScene.h`](src/PhysicsDemoScene.h).

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_SCENE_ARENA`** — pre-allocated box/circle pools and arena-safe add/remove when the slider changes.
- **`PIXELROOT32_ENABLE_TOUCH=1`** — set for **`native`** and **`esp32cyd`** in `platformio.ini` so touch APIs compile and mouse/touch can drive the demo.
- **`esp32cyd`** additionally enables **`PIXELROOT32_ENABLE_DEBUG_OVERLAY`**, **`PIXELROOT32_DEBUG_MODE`**, **ILI9341** 240×320, and **XPT2046** touch (many tuning `-D`s in `platformio.ini`).

See **`platformio.ini`** for **`native`**, **`esp32dev`**, **`esp32cyd`**.

## Platforms

| Environment | Display / input |
|-------------|-----------------|
| **`native`** | SDL2, 240×240; touch flag enables the same code paths with **simulated** touch |
| **`esp32dev`** | **ST7789** 240×240 (no `PIXELROOT32_ENABLE_TOUCH` in this preset — use keyboard/GPIO per your `platforms/esp32_dev.h`) |
| **`esp32cyd`** | **ILI9341** 240×320 + **XPT2046** resistive touch |

## Controls / interaction

- **Move player** — bind your **`InputManager`** layout to the kinematic actor (see scene `update` / player handling).
- **Touch (native + CYD)** — `processTouchEvents` / `onUnconsumedTouchEvent` and **`ActorTouchController`** for world interaction; CYD also gets on-screen **UI widgets** when `PIXELROOT32_ENABLE_UI_SYSTEM` is true.

## Features

- **Rigid** dynamics (restitution, friction), **circle vs AABB** collision shape
- **Static** scenery with bounce flag on walls
- **Sensor-style** regions (as wired in the demo scene)
- **Optional touch HUD** (slider adjusts how many boxes/circles are registered without re-running `init()` from scratch)

## Documentation links

- [Physics API](../../docs/api/physics.md)
- [Input API](../../docs/api/input.md)
- [UI API](../../docs/api/ui.md)
- [Core — Scene](../../docs/api/core.md)

## Build

From **`examples/physics`**:

```bash
pio run -e native
pio run -e esp32dev
pio run -e esp32cyd
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
pio run -e esp32cyd --target upload
```
