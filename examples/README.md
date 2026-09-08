# PixelRoot32 — Examples

Self-contained **[PlatformIO](https://platformio.org/)** projects that show how to use the engine on **PC (SDL2)** and **ESP32-class boards**. Each folder has its own **`platformio.ini`**, **`src/`** entry point, and **`README.md`** with build flags, supported environments, and documentation links.

**Typical workflow:** open a project folder in PlatformIO (or run CLI from that folder), pick an environment (`native`, `esp32dev`, etc.), then:

```bash
cd <example-folder>
pio run -e <environment>
```

On Windows, **`native`** examples may need local **SDL2** include/lib paths in `platformio.ini` (see comments in [animated_tilemap](animated_tilemap/README.md)).

The engine revision for each example is defined in **`lib_deps`** inside that example’s `platformio.ini` (registry tag vs Git branch).

## Catalogue

| Example | What it demonstrates | PlatformIO environments |
|--------|----------------------|-------------------------|
| [camera](camera/) | `Camera2D` (smoothing, bounds), parallax, tile platforms, `KinematicActor`, **camera effects** (shake / punch / offset) and a scripted **`CameraTween`** pan | `native`, `esp32dev` |
| [sprites](sprites/) | 2bpp / 4bpp sprites and animation over a drawn background scene, cycling **single / dual / dual-inverted palette mode** with **A**, with the live background table shown as a 16-entry ramp | `native`, `esp32dev` |
| [mono_oled](mono_oled/) | **Monochrome 1-bit OLED** on ESP32-C3: `U8G2_Drawer` via `PIXELROOT32_USE_U8G2`, which renderer paths a monochrome panel gives up (no tilemap fast path, no sprite blit, no palette), a **72x40 logical screen offset inside a 128x64 controller framebuffer**, and one-button interaction | `native`, `esp32c3` |
| [physics](physics/) | `RigidActor` / `KinematicActor` / `StaticActor`, touch, optional touch UI (CYD), **layer-aware radius query** overlay, **collision-driven particle burst** | `native`, `esp32dev`, `esp32cyd` |
| [metroidvania](metroidvania/) | 4bpp tilemaps, `StaticTilemapLayerCache`, dirty regions, platformer player with gravity + climbing, **interaction triggers** + **gameplay event bus** on sensor pickups | `native`, `esp32dev` |
| [animated_tilemap](animated_tilemap/) | Tile animation, palettes, static tilemap framebuffer cache (reference depth) | `native`, `esp32dev`, `esp32cyd` |
| [iso_dungeon](iso_dungeon/) | **Isometric dungeon** built on a generic `ProjectionSpec`: exact tile-to-tile movement via `GridMotion` + the projection overload of `interpolatedWorld`, **projection-aware depth sorting** (`compareByDepthKey`) so the hero passes behind and in front of the altar, row-major painter's order with no sort, **three rooms connected by a `RoomGraph`** whose catalog validates itself with `static_assert`, programmatically generated 4bpp iso art | `native`, `esp32dev` |


## Suggested learning order

1. **sprites** — graphics and colour models (single, dual and dual-inverted palette).  
2. **camera** — scrolling, parallax, camera effects and tweens.  
3. **animated_tilemap** or **metroidvania** — tilemaps and caching (read **animated_tilemap** for the fullest tilemap write-up).  
4. **physics** — bodies, sensors, touch, area queries.  
5. **iso_dungeon** — projection, grid motion and projection-aware depth sorting.  
6. **mono_oled** — the other end of the hardware range: a 1-bit OLED on an ESP32-C3. Read it when you are choosing a display, not when you are learning the engine — it is mostly about which renderer paths a monochrome panel takes away.

For **complete games**, and for audio, UI, input, performance and gameplay topics
with a demo each, see [**PixelRoot32-Demo-Projects**](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main).

> **Display drivers.** `PIXELROOT32_USE_U8G2` selects `U8G2_Drawer` for
> monochrome panels; without it, ESP32 builds default to TFT_eSPI. Only
> [mono_oled](mono_oled/) sets it, and it is also the only `esp32c3` example. This flag is not part of the
> `PIXELROOT32_ENABLE_*` family below.

## Where each opt-in capability is demonstrated

Each row names the examples that actually define the flag in their
`platformio.ini` (or their `lib/platformio.ini` template). Start here when you
want to see a capability in use:

| Capability | Flag | Example |
|---|---|---|
| Grid space / motion | `GAMEPLAY_GRID_SPACE` | [iso_dungeon](iso_dungeon/) |
| State machine | `GAMEPLAY_STATE_MACHINE` | [metroidvania](metroidvania/) |
| Object pool | `GAMEPLAY_OBJECT_POOL` | *(Demo-Projects: [`gameplay/object_pool`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/gameplay/object_pool))* |
| Room graph | `GAMEPLAY_ROOM` | [iso_dungeon](iso_dungeon/) |
| Per-pixel tile collision | *(always on)* | *(Demo-Projects: [`gameplay/room_screen`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/gameplay/room_screen))* |
| Gameplay event bus | `GAMEPLAY_EVENTS` | [metroidvania](metroidvania/) |
| Interaction triggers | `INTERACTION_TRIGGERS` | [metroidvania](metroidvania/) |
| Spatial queries | `SPATIAL_QUERY` | [physics](physics/) |
| Depth sorting | `DEPTH_SORT` | [iso_dungeon](iso_dungeon/) |
| Cell-to-screen projection | `PROJECTION` | [iso_dungeon](iso_dungeon/) |
| Static layer snapshot | `STATIC_LAYER_SNAPSHOT` | [iso_dungeon](iso_dungeon/) |
| Dirty regions (selective clear) | `DIRTY_REGIONS` | [animated_tilemap](animated_tilemap/), [iso_dungeon](iso_dungeon/), [metroidvania](metroidvania/) |
| Particles | `PARTICLES` | [physics](physics/) |
| Camera effects | `CAMERA_EFFECTS` | [camera](camera/) |
| Camera tweens | `CAMERA_TWEEN` | [camera](camera/) |
| 12-bit colour wire format | `PIXELROOT32_TFT_12BIT_COLOR` | **not demonstrated** — see below |

Every flag above except the last takes the `PIXELROOT32_ENABLE_` prefix.

**Not currently demonstrated by any example:**

- **12-bit colour wire format.** `PIXELROOT32_TFT_12BIT_COLOR` is not part of the
  `PIXELROOT32_ENABLE_*` family; it is a `TFT_eSPI_Drawer` option that defaults
  to `0` in [`PlatformDefaults.h`](../include/platforms/PlatformDefaults.h).
  Nothing in this repository or in Demo-Projects switches it on.

> **Note on the projection flag.** The capability is `PIXELROOT32_ENABLE_PROJECTION`.
> The older name `PIXELROOT32_ENABLE_GAMEPLAY_PROJECTION` was renamed and now
> raises a `#error` at compile time — see
> [`PlatformDefaults.h`](../include/platforms/PlatformDefaults.h).

## Engine documentation

- [API reference index](../docs/api/index.md)  
- [Architecture](../docs/architecture/architecture-index.md)  
- Module docs under [`docs/api/`](../docs/api/) (Graphics, Physics, UI, Input, Audio, Core, …)

## Format reference for per-example READMEs

The **[animated_tilemap](animated_tilemap/README.md)** example is the template for depth: opening summary, **Requirements (build flags)**, optional technical subsection, **Documentation links**, **Features**, and **Build** commands. Scene intent is also described in each `src/*Scene.h` file.
