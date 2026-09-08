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
| [music-demo](music-demo/) | **`MusicPlayer`** **multi-track** (main + sub-tracks), **tick / BPM** timing, **`InstrumentPreset`** melodies + **percussion** presets; UI-based sound testing | `native`, `esp32c3`, `esp32dev` |
| [physics](physics/) | `RigidActor` / `KinematicActor` / `StaticActor`, touch, optional touch UI (CYD), **layer-aware radius query** overlay, **collision-driven particle burst** | `native`, `esp32dev`, `esp32cyd` |
| [metroidvania](metroidvania/) | 4bpp tilemaps, `StaticTilemapLayerCache`, dirty regions, platformer player with gravity + climbing, **interaction triggers** + **gameplay event bus** on sensor pickups | `native`, `esp32dev` |
| [animated_tilemap](animated_tilemap/) | Tile animation, palettes, static tilemap framebuffer cache (reference depth) | `native`, `esp32dev`, `esp32cyd` |
| [2048](2048/) | 2048 puzzle game: grid rendering, touch swipes, D-pad controls, score tracking, **AI auto-play** (expectimax algorithm), audio SFX | `native`, `esp32cyd` |
| [flappy_bird](flappy_bird/) | Physics flappy clone, U8g2 OLED, ESP32-C3 (**no audio** in this sample) | `native`, `esp32c3` |
| [bomberbot](bomberbot/) | Original **bomberman-style** game (all CC0 art): interpolated grid movement, deterministic seeded board generation, bounded chain-reaction explosions, PRNG enemy AI, **Y-axis depth sorting**, HUD + text overlays, `AudioEngine` | `native`, `esp32dev` |
| [iso_dungeon](iso_dungeon/) | **Isometric dungeon** built on a generic `ProjectionSpec`: exact tile-to-tile movement via `GridMotion` + the projection overload of `interpolatedWorld`, **projection-aware depth sorting** (`compareByDepthKey`) so the hero passes behind and in front of the altar, row-major painter's order with no sort, **three rooms connected by a `RoomGraph`** whose catalog validates itself with `static_assert`, programmatically generated 4bpp iso art | `native`, `esp32dev` |
| [midway_clone](midway_clone/) | **Clone of Midway** — vertically scrolling shooter: a camera driven **every frame**, `ObjectPool` bullets/enemies/explosions, camera-keyed wave table, sprite-vs-sprite AABB with physics off, and a measured look at what a moving camera costs `StaticTilemapLayerCache` (spoiler: less than the unconditional full-frame SPI push) | `native`, `esp32dev` |
| [legend_of_clone](legend_of_clone/) | **The Legend of Clone** — 8-bit-style **screen-by-screen overworld and dungeon**: two scenes over a shared room-grid base, scrolling room transitions with input lockout, `triggerTransition` fade between scenes, exported flash-resident 4bpp tilemaps + `StaticTilemapLayerCache`, dual palette mode, and **selectable tile collision** — whole-tile, per-pixel, or per-pixel with erosion via `isTilePixelSolid` | `native`, `esp32dev` |


## Suggested learning order

1. **sprites** — graphics and colour models (single, dual and dual-inverted palette).  
2. **camera** — scrolling, parallax, camera effects and tweens.  
3. **animated_tilemap** or **metroidvania** — tilemaps and caching (read **animated_tilemap** for the fullest tilemap write-up).  
4. **physics** — bodies, sensors, touch, area queries.  
5. **music-demo** / **2048** / **bomberbot** — **audio** (SFX events, or the **multi-track** reference in **music-demo**). **flappy_bird** — physics + OLED on a 72x40 logical screen, no audio subsystem.  
6. **midway_clone** — where the frame budget actually goes on an ESP32. Read it after step 3: it is the counter-example to the tilemap cache, and it shows how to measure rather than guess.

## Where each opt-in capability is demonstrated

Each row names the examples that actually define the flag in their
`platformio.ini` (or their `lib/platformio.ini` template). Start here when you
want to see a capability in use:

| Capability | Flag | Example |
|---|---|---|
| Grid space / motion | `GAMEPLAY_GRID_SPACE` | [2048](2048/), [bomberbot](bomberbot/), [iso_dungeon](iso_dungeon/) |
| State machine | `GAMEPLAY_STATE_MACHINE` | [flappy_bird](flappy_bird/), [metroidvania](metroidvania/) |
| Object pool | `GAMEPLAY_OBJECT_POOL` | [midway_clone](midway_clone/) |
| Room graph | `GAMEPLAY_ROOM` | [iso_dungeon](iso_dungeon/), [legend_of_clone](legend_of_clone/) |
| Per-pixel tile collision | *(always on)* | [legend_of_clone](legend_of_clone/) |
| Gameplay event bus | `GAMEPLAY_EVENTS` | [metroidvania](metroidvania/) |
| Interaction triggers | `INTERACTION_TRIGGERS` | [metroidvania](metroidvania/) |
| Spatial queries | `SPATIAL_QUERY` | [physics](physics/) |
| Depth sorting | `DEPTH_SORT` | [bomberbot](bomberbot/), [iso_dungeon](iso_dungeon/) |
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
  to `0` in [`PlatformDefaults.h`](../include/platforms/PlatformDefaults.h). The
  only reference to it in this tree is a **commented-out** line in
  [midway_clone](midway_clone/)'s `platformio.ini`, so no example builds with it
  on.

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
