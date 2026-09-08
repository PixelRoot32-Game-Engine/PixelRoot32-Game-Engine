# PixelRoot32 — Examples

**Five small, single-idea projects.** Each one exists to show one capability of
the engine clearly enough to copy, and stops there.

> **Looking for a game?** These are not games. Complete games, and per-topic
> demos for audio, UI, input, gameplay systems and performance, live in
> [**PixelRoot32-Demo-Projects**](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main). That repository is where a project
> grows; this one is where a capability is explained. If you came here for
> something bigger than a single feature, go there — and see
> [Where the bigger projects went](#where-the-bigger-projects-went) if you are
> following an old link.

Each folder is a self-contained **[PlatformIO](https://platformio.org/)** project
that builds on **PC (SDL2)** and on **ESP32-class boards**, with its own
**`platformio.ini`**, **`src/`** entry point, and **`README.md`** covering build
flags, supported environments and documentation links.

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
| [animated_tilemap](animated_tilemap/) | Tile animation, palettes, static tilemap framebuffer cache (reference depth) | `native`, `esp32dev`, `esp32cyd` |


## Suggested learning order

1. **sprites** — graphics and colour models (single, dual and dual-inverted palette).  
2. **camera** — scrolling, parallax, camera effects and tweens.  
3. **animated_tilemap** — tile animation, the static tilemap cache and dirty regions.  
4. **physics** — bodies, sensors, touch, area queries.  
5. **mono_oled** — the other end of the hardware range: a 1-bit OLED on an ESP32-C3. Read it when you are choosing a display, not when you are learning the engine — it is mostly about which renderer paths a monochrome panel takes away.

## Where the bigger projects went

This catalogue used to list thirteen projects, most of them complete games. They
were **not deleted** — they moved to
[**PixelRoot32-Demo-Projects**](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main), which is built for projects that keep
growing. If you followed a link here, this table is where it now points:

| Was | Now |
| --- | --- |
| `examples/2048` | [`games/2048`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/games/2048) |
| `examples/bomberbot` | [`games/bomberbot`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/games/bomberbot) |
| `examples/flappy_bird` | [`games/flappy_bird`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/games/flappy_bird) |
| `examples/legend_of_clone` | [`games/legend_of_clone`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/games/legend_of_clone) |
| `examples/midway_clone` | [`games/midway_clone`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/games/midway_clone) |
| `examples/metroidvania` | [`gameplay/metroidvania`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/gameplay/metroidvania) |
| `examples/iso_dungeon` | [`graphics/iso_dungeon`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/graphics/iso_dungeon) |
| `examples/music-demo` | [`audio/music_sequencer`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/audio/music_sequencer) |

`flappy_bird` and `music-demo` were also the only `esp32c3` examples here. That
coverage is now [mono_oled](mono_oled/), written for it deliberately rather than
inherited from a game.

> **Display drivers.** `PIXELROOT32_USE_U8G2` selects `U8G2_Drawer` for
> monochrome panels; without it, ESP32 builds default to TFT_eSPI. Only
> [mono_oled](mono_oled/) sets it, and it is also the only `esp32c3` example. This flag is not part of the
> `PIXELROOT32_ENABLE_*` family below.

## Where each opt-in capability is demonstrated

These five examples cover the rendering, camera, physics and display side of
the engine. Each row names the examples that actually define the flag in their
`platformio.ini` (or their `lib/platformio.ini` template):

| Capability | Flag | Example |
|---|---|---|
| Sprite bit depths | `2BPP_SPRITES`, `4BPP_SPRITES` | [sprites](sprites/), [animated_tilemap](animated_tilemap/) |
| Tile animation | `TILE_ANIMATIONS` | [animated_tilemap](animated_tilemap/) |
| Static tilemap framebuffer cache | `STATIC_TILEMAP_FB_CACHE` | [animated_tilemap](animated_tilemap/) |
| Dirty regions (selective clear) | `DIRTY_REGIONS` | [animated_tilemap](animated_tilemap/) |
| Camera effects | `CAMERA_EFFECTS` | [camera](camera/) |
| Camera tweens | `CAMERA_TWEEN` | [camera](camera/) |
| Spatial queries | `SPATIAL_QUERY` | [physics](physics/) |
| Particles | `PARTICLES` | [physics](physics/) |
| Touch input | `TOUCH` | [physics](physics/) |
| Scene arena | `SCENE_ARENA` | [animated_tilemap](animated_tilemap/), [camera](camera/), [physics](physics/) |
| Monochrome display driver | `PIXELROOT32_USE_U8G2` | [mono_oled](mono_oled/) |

Every flag above except the last takes the `PIXELROOT32_ENABLE_` prefix.

### The gameplay framework lives in Demo-Projects

None of the `PIXELROOT32_ENABLE_GAMEPLAY_*` capabilities is demonstrated here
any more. Each has a demo of its own in
[**PixelRoot32-Demo-Projects**](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main), which is where complete projects and
per-system demos belong:

| Capability | Flag | Demo |
|---|---|---|
| Grid space / motion | `GAMEPLAY_GRID_SPACE` | [`games/snake`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/games/snake), [`games/bomberbot`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/games/bomberbot) |
| State machine | `GAMEPLAY_STATE_MACHINE` | [`gameplay/state_machine`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/gameplay/state_machine) |
| Object pool | `GAMEPLAY_OBJECT_POOL` | [`gameplay/object_pool`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/gameplay/object_pool) |
| Room graph | `GAMEPLAY_ROOM` | [`gameplay/room_screen`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/gameplay/room_screen) |
| Gameplay event bus | `GAMEPLAY_EVENTS` | [`gameplay/metroidvania`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/gameplay/metroidvania) |
| Interaction triggers | `INTERACTION_TRIGGERS` | [`gameplay/metroidvania`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/gameplay/metroidvania) |
| Depth sorting | `DEPTH_SORT` | [`graphics/depth_sort`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/graphics/depth_sort) |
| Cell-to-screen projection | `PROJECTION`, `TILEMAP_PROJECTION` | [`graphics/iso_dungeon`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/graphics/iso_dungeon), [`graphics/iso_tilemap_export`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/graphics/iso_tilemap_export) |
| Static layer snapshot | `STATIC_LAYER_SNAPSHOT` | [`graphics/iso_dungeon`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/graphics/iso_dungeon) |
| Per-pixel tile collision | *(always on)* | [`gameplay/room_screen`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/gameplay/room_screen) |
| Scene transitions | `SCENE_TRANSITIONS` | [`games/legend_of_clone`](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/games/legend_of_clone) |

**Not demonstrated anywhere.** `PIXELROOT32_TFT_12BIT_COLOR` is not part of the
`PIXELROOT32_ENABLE_*` family; it is a `TFT_eSPI_Drawer` option that defaults to
`0` in [`PlatformDefaults.h`](../include/platforms/PlatformDefaults.h). Nothing
in this repository or in Demo-Projects switches it on.

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
