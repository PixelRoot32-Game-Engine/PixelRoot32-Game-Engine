# Guide

User guides and how-to documentation for PixelRoot32 Game Engine.

## Getting Started

- [Getting Started](./getting-started.md) — Install, first project, game loop overview
- [Game Loop](./game-loop.md) — Phases, delta time, lifecycle diagrams
- [Music Player Guide](./music-player-guide.md) - Complete guide for the NES-style audio system
- [Extending PixelRoot32](./extending-pixelroot32.md) - Creating custom drivers and extending the engine
- [Gameplay Guidelines](./gameplay-guidelines.md) - Best practices for game development
- [Entities & scene tutorial](./entities-scene-tutorial.md) - Didactic `Entity` walkthrough (not a bundled `examples/` folder)

## Game development topics

- [Core concepts](./core-concepts.md) — Terminology and mental model
- [Scenes](./scenes.md) — Scene lifecycle, stacking, transitions
- [Entities & actors](./entities-actors.md) — Patterns for gameplay objects
- [Rendering](./rendering.md) — Sprites, layers, camera-oriented drawing
- [Input](./input.md) — `InputManager`, touch pipeline, calibration
- [Physics](./physics.md) — Collisions, actors, tile helpers
- [Audio](./audio.md) — Channels, SFX, and music overview
- [UI system](./ui-system.md) — Layouts, widgets, HUDs

## Advanced

- [Memory](./memory.md) — Pools, arenas, embedded constraints
- [Performance](./performance/esp32-performance.md) — Hot paths, memory, build profiles, resolution scaling
- [Graphics Techniques](./graphics-techniques.md) — Tilemaps, palettes, indexed color
- [Platform Compatibility](./platform-compatibility.md) — `platformio.ini`, feature flags

## Contributing & tooling

- [Testing](./testing.md) — Unity, `native_test`, coverage, CI

## Standards & compatibility

- [Coding style](./coding-style.md) — C++ conventions, namespaces, naming
- [Coding Style](./coding-style.md) — C++ conventions, namespaces, naming
- [Graphics guidelines](./graphics-guidelines.md) — Sprites, tilemaps, palettes (pipeline: [Rendering](./rendering.md))
- [UI guidelines](./ui-guidelines.md) — Layout patterns (architecture: [UI system](./ui-system.md))
- [Platform compatibility](./platform-compatibility.md) — Hardware matrix, ESP32 variants
- [Performance](./performance/) — ESP32 hot paths, build profiles

## Tools & samples

- [Tools (sprite compiler, tilemap editor docs)](../tools/index.md) - Workflow documentation shipped with the engine
- [Example projects catalog](../../examples/README.md) - PlatformIO samples under `examples/`

## Navigation

- [Architecture](../architecture/) - System architecture and design
- [API Reference](../api/) - Complete API documentation
- [Workflow tools](../tools/index.md) - Sprite compiler and tilemap editor documentation
- [Migration](../migration/) - Version upgrade guides
- [Philosophy](../philosophy/) - Engine design philosophy
