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
| [hello_world](hello_world/) | Minimal `Scene`, `UILabel`, button input, background color cycle | `native`, `esp32dev` |
| [camera](camera/) | `Camera2D`, parallax, tile platforms, `KinematicActor` | `native`, `esp32dev` |
| [dual_palette](dual_palette/) | Dual palette mode (background vs sprite color tables) | `native`, `esp32dev` |
| [sprites](sprites/) | 2bpp / 4bpp sprites and animation | `native`, `esp32dev` |
| [snake](snake/) | Grid game, segment pool, `AudioEngine` + platform audio backends | `native`, `esp32dev` |
| [physics](physics/) | `RigidActor` / `KinematicActor` / `StaticActor`, touch, optional touch UI (CYD) | `native`, `esp32dev`, `esp32cyd` |
| [metroidvania](metroidvania/) | 4bpp tilemaps, `StaticTilemapLayerCache`, platformer player | `native`, `esp32dev` |
| [animated_tilemap](animated_tilemap/) | Tile animation, palettes, static tilemap framebuffer cache (reference depth) | `native`, `esp32dev`, `esp32cyd` |
| [tic_tac_toe](tic_tac_toe/) | UI, GPIO vs touch, minimax AI, vector-drawn board | `native`, `esp32dev`, `esp32cyd` |
| [flappy_bird](flappy_bird/) | Physics-based flappy clone, U8g2 OLED, ESP32-C3 preset | `native`, `esp32c3` |

## Suggested learning order

1. **hello_world** — engine init, one scene, text and input.  
2. **sprites** or **dual_palette** — graphics and color models.  
3. **camera** or **metroidvania** / **animated_tilemap** — scrolling, tilemaps, caching (read **animated_tilemap** for the fullest tilemap write-up).  
4. **physics** — bodies, sensors, touch.  
5. **snake** / **flappy_bird** / **tic_tac_toe** — small full games and audio or UI patterns.

## Engine documentation

- [API reference index](../docs/API_REFERENCE.md)  
- [Architecture](../docs/ARCHITECTURE.md)  
- Module docs under [`docs/api/`](../docs/api/) (Graphics, Physics, UI, Input, Audio, Core, …)

## Format reference for per-example READMEs

The **[animated_tilemap](animated_tilemap/README.md)** example is the template for depth: opening summary, **Requirements (build flags)**, optional technical subsection, **Documentation links**, **Features**, and **Build** commands. Scene intent is also described in each `src/*Scene.h` file.
