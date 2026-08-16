# Physics Demo Example

> **⚠️ Demonstration example** — This project is provided **as an example** to showcase the capabilities of the PixelRoot32 Game Engine and what you can build with it. It may **not be 100% functional or finished**; some features can be incomplete, experimental, or work in progress.

Interactive **physics** sandbox: **`KinematicActor`** player, **`RigidActor`** boxes and circles (AABB and circle shape), **`StaticActor`** floors/walls, **sensors**, and **`ActorTouchController`** for dragging/spawning on touch-capable builds.

When **`PIXELROOT32_ENABLE_UI_SYSTEM`** is on (default in [`PlatformDefaults.h`](../../include/platforms/PlatformDefaults.h)), **`esp32cyd`** builds also expose **touch UI**: **`UITouchButton`** (full reset), **`UITouchSlider`** (dynamic spawn count), **`UITouchCheckbox`**, and horizontal/vertical **layouts** — see [`PhysicsDemoScene.h`](src/PhysicsDemoScene.h).

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_SCENE_ARENA`** — pre-allocated box/circle pools and arena-safe add/remove when the slider changes.
- **`PIXELROOT32_ENABLE_TOUCH=1`** — set for **`native`** and **`esp32cyd`** in `platformio.ini` so touch APIs compile and mouse/touch can drive the demo.
- **`PIXELROOT32_ENABLE_SPATIAL_QUERY=1`** — the proximity scan described below.
- **`esp32cyd`** additionally enables **`PIXELROOT32_ENABLE_DEBUG_OVERLAY`**, **`PIXELROOT32_DEBUG_MODE`**, **ILI9341** 240×320, and **XPT2046** touch (many tuning `-D`s in `platformio.ini`).

## Proximity scan (spatial queries)

Press **B** (button **5**) to toggle a radius query centred on the player. The
circle is the query, and every actor it returns is boxed in magenta.

Worth knowing before you use `queryRadius()` in a game:

- **It has no anchor actor, so only your mask applies.** `checkCollision()`
  tests both sides (`a.mask & b.layer || b.mask & a.layer`) because both sides
  are actors. An area query has no second actor to ask, so the test is just
  `(mask & other->layer)`. This is a real behavioural difference, not an
  oversight.
- **The player is in its own results.** There is no actor to exclude, so filter
  it out yourself if the query drives damage or targeting.
- **Radius is bounded.** Squared-distance terms have to fit Q16.16, so the
  radius is clamped to `SPATIAL_QUERY_MAX_RADIUS` (default 128). Debug builds
  assert; release builds clamp. Distances are compared squared — never `sqrt`.
- **Run it after `Scene::update()`**, as this demo does, or the grid still holds
  last frame's positions.

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
