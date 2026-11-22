# Platformer Example

> **⚠️ Demonstration example** — This project is provided **as an example** to showcase the capabilities of the PixelRoot32 Game Engine and what you can build with it. It may **not be 100% functional or finished**; some features can be incomplete, experimental, or work in progress.

One screen, one subject: the **three physics actors** and the collision matrix
that connects them.

- **`KinematicActor`** — the character (`PlayerCharacter`). The script owns its
  velocity; the engine only resolves it against the level.
- **`RigidActor`** — the crates (`CrateActor`). The solver owns their velocity:
  they fall, rest on the shelf and stop the character. What they do *not* do is
  explained in [what the crates actually do](#what-the-crates-actually-do).
- **`StaticActor`** — the level (`LevelBlock`). Nothing moves it.

The level is exactly one logical screen (240×240), so there is no camera.
`Camera2D`, parallax and camera effects are the subject of
[`examples/camera`](../camera/README.md), not of this one.

If you come from **Godot**, the actor types are the ones you already know:

| Godot | PixelRoot32 |
|---|---|
| `CharacterBody2D` + `move_and_slide()` | `KinematicActor` + `moveAndSlide()` |
| `is_on_floor()` | `is_on_floor()` — same name; per-frame contact, no coyote time |
| `RigidBody2D` | `RigidActor` |
| `StaticBody2D` | `StaticActor` |
| `Area2D` | `SensorActor` — not used here; see [`examples/physics`](../physics/README.md) |
| `collision_layer` / `collision_mask` | `setCollisionLayer()` / `setCollisionMask()` |
| `one_way_collision` | `setOneWay(true)` |
| `CollisionShape2D` | `setHitboxDimensions()` / `setHitboxOffset()` (the default hitbox is the body) |

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_PHYSICS=1`** — every actor in the scene.

`lib/platformio.ini` also turns **off** what the subject does not need: audio,
particles, the UI system, camera effects, camera tweens and — since there is one
scene — `PIXELROOT32_ENABLE_SCENE_TRANSITIONS`. The flag list is part of the
lesson: it tells you what each feature costs.

Everything else follows the defaults in
[`PlatformDefaults.h`](../../include/platforms/PlatformDefaults.h). See
**`platformio.ini`** in this folder for the `native` and `esp32dev` presets.

The scene expects **`extern pixelroot32::core::Engine engine`** (see
`src/platforms/native.h` / `esp32_dev.h` and `main.cpp`).

## The collision matrix

`GameLayers.h` is the whole story of what touches what:

| Actor | Layer | Mask | Meaning |
|---|---|---|---|
| `PlayerCharacter` | `PLAYER` | `GROUND \| PLATFORM \| PROP` | Walks on solids, lands on one-way platforms, is stopped by crates |
| `LevelBlock` (solid) | `GROUND` | `PLAYER \| PROP` | Floor, walls, crate shelf |
| `LevelBlock` (one-way) | `PLATFORM` | `PLAYER` | Characters only |
| `CrateActor` | `PROP` | `GROUND \| PLAYER \| PROP` | Falls through one-way platforms, rests on solids, stacks when dropped on another crate |

The engine's filter is `(a.mask & b.layer) || (b.mask & a.layer)`, so one side
naming the other is enough for a contact. The distinctions that matter are still
written on both sides, which is what stops a new actor type from colliding with
something it should ignore.

**One-way platforms** use the engine's `setOneWay(true)`: the body is landed on
from above and passed through from below, and `is_on_floor()` stays correct. The
camera example instead swaps the player's mask per frame to get the same
behaviour; that trick is what you write when the platform type does not exist,
and it is not needed here.

**Gravity** is not one number. A `KinematicActor` has none — `GameConstants.h`
sets 400 units/s² and `PlayerCharacter::update()` integrates it. A `RigidActor`
gets a fixed 200 units/s² from `RigidActor::integrate()`, so the crates call
`setGravityScale(2.0f)` to fall at the same rate as the character. This is a real
difference in the API, not a tuning accident.

**Snap** keeps the character glued to the floor while walking (`SnapPolicy::Step`
with `MIN_SNAP`), and the snap vector must be zeroed on the jump frame or the
body is pulled straight back down.

**The jump is edge-triggered**: `InputManager::isButtonPressed()` is true on the
transition frame only, so holding the button does not repeat it. There is no
coyote time and no jump buffer — `is_on_floor()` reports last frame's contact and
nothing more.

**The jump reach, measured** — with `PLAYER_JUMP_VELOCITY=220` and
`PLAYER_GRAVITY=400`, the integrator peaks at **62 px** above the takeoff feet
(the continuous `v² / 2g` says 60.5 px; the extra ~2 px is the discrete step), and
a full-speed jump stays airborne for **99 px** of horizontal travel. Every gap
and step in the level table is inside that envelope: the vertical steps are
24 px, 32 px from the ground up to the first one-way platform, and the 56 px hop
from the ground to platform B leaves about **6 px** of margin. That last one is
the tightest jump in the level; platform B is also reachable from platform A,
where it is only 24 px up, so no route depends on the 6 px.

## What the crates actually do

The crates are a `RigidActor` showcase, and the limits are as instructive as the
behaviour:

- They **fall** through the one-way platforms (their mask has no `PLATFORM` bit)
  and **rest** on the shelf 0.5 px into it — the solver's resting penetration,
  stable for hundreds of frames and free of jitter (`setBounce(false)`, plus
  `setGravityScale(2.0f)` so their fixed 200 units/s² match the character's 400).
- They **stack**: dropping one crate above another leaves it about 1.5 px into
  the one below, and the pair holds — 0.1 px of drift over 600 frames.
- They **stop the character**. `moveAndSlide()` treats the crate as an obstacle,
  so the kinematic body cannot walk it along: the solver shoves the crate only
  far enough to undo the overlap (a few pixels) and then the character is stuck.
  Pushing a crate across the level is not something this engine does today.
- **Standing on a crate does not hold.** The character ends up about 5 px inside
  the crate and the crate about 5 px inside the shelf, with the feet level with
  the shelf's top edge, and it makes no difference whether the crate has mass 1
  or 200, or gravity scale 0.5 or 3. The overlap is resolved by moving the rigid
  body, and once the static shelf stops it there is no clean separation left.

That is the engine's current kinematic-vs-rigid behaviour, which is why the level
keeps the crates as obstacles on a shelf instead of building a puzzle out of
them.

## Controls

- **Left / Right** — move (buttons **2** and **3** in `InputManager` order).
- **A** — jump (button **4**), only while on the floor.
- **B** — reset (button **5**): re-runs `init()`, which is also the boot path, so
  the crates drop again exactly as they did the first time.

## Platforms

| Environment | Notes |
|-------------|--------|
| **`native`** | SDL2 window, 240×240 logical size. |
| **`esp32dev`** | **ST7789** TFT 240×240, TFT_eSPI-style pin defines in `platformio.ini`. |

The engine version or Git branch is set in **`lib_deps`** in `platformio.ini`.

## Features

- **`KinematicActor`** character with script-owned gravity, jump and floor snap
- **`RigidActor`** crates with mass, friction, restitution and a gravity scale
- **`StaticActor`** level built from a table, one body per run of tiles
- **Collision layers and masks** stated on both sides (`GameLayers.h`)
- **One-way platforms** through `setOneWay(true)`
- **On-screen legend** naming the three actor types in the colour they are drawn

## File Structure

```
src/
├── PlatformerScene.h/.cpp     — level table, actor wiring, HUD
├── PlayerCharacter.h/.cpp     — KinematicActor: gravity, run, jump, snap
├── CrateActor.h/.cpp          — RigidActor: material, gravity scale, drawing
├── LevelBlock.h               — StaticActor: colour and the one-way lip
├── GameConstants.h            — level size, player tuning, input mapping
├── GameLayers.h               — collision layers and the mask of each type
└── platforms/
    ├── native.h               — SDL2 engine wiring
    └── esp32_dev.h            — ESP32 engine wiring
```

## What this example does not do

- **No camera, no parallax** — see [camera](../camera/README.md).
- **No sensors or triggers** — `SensorActor` (the `Area2D` equivalent) and
  touch interaction are in [physics](../physics/README.md).
- **No tile collision** — the level is a handful of static runs, not a tilemap
  with per-tile attributes. See
  [animated_tilemap](../animated_tilemap/README.md) for tilemaps.
- **No pushing or stacking puzzle** — the character cannot push a crate along
  or ride one; see [what the crates actually do](#what-the-crates-actually-do).
- **No sprites, audio, particles or touch** — everything is drawn with
  primitives, which keeps the subject on the actors.

## Documentation links

- [Physics — kinematic, rigid and static actors](../../docs/api/physics.md)
- [Core — Scene / entities](../../docs/api/core.md)
- [Input API](../../docs/api/input.md)
- [Architecture](../../docs/architecture/architecture-index.md)

## Build

Run from **`examples/platformer`**:

```bash
pio run -e native
pio run -e esp32dev
```

`pio run -e native` only builds. To run it:

```bash
./.pio/build/native/program
```

> **Windows note.** The `native` environment compiles with MSYS2's MinGW `g++`.
> If `pio run -e native` fails with a bare `*** [...] Error 1` and no compiler
> diagnostic, `g++` is not on `PATH`. Add it before building:
> `export PATH="/c/msys64/mingw64/bin:$PATH"`.

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
