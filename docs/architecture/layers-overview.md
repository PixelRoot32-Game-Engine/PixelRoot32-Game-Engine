# Architecture Overview - PixelRoot32 Game Engine

## Executive Summary

PixelRoot32 is a lightweight, modular 2D game engine written in C++17, designed primarily for ESP32 microcontrollers, with a native simulation layer for PC (SDL2) that enables rapid development without hardware.

The engine follows a scene-based architecture inspired by Godot Engine, making it intuitive for developers familiar with modern game development workflows.

**[Hub de arquitectura](./overview.md)** — tablas de navegación, diagrama de jerarquía de clases (`Entity` / UI), matriz `PIXELROOT32_ENABLE_*`, y pipeline ESP32 / `StaticTilemapLayerCache`.

> **Note:** For detailed architecture documentation with diagrams and examples, visit the [official documentation](https://docs.pixelroot32.org/manual/engine_architecture/).

---

## Design Philosophy

- **Modularity**: Each subsystem can be used independently and compiled conditionally
- **Selective Compilation**: Subsystems can be excluded at compile time to reduce firmware size and RAM usage
- **Portability**: Same code for ESP32 and PC (SDL2)
- **Performance**: Optimized for resource-constrained hardware with aggressive dead code elimination
- **Extensibility**: Plugin architecture for drivers and backends
- **Modern C++**: Leverages C++17 features (smart pointers, string_view) for safety and efficiency

### What Does "Modularity" Mean in PixelRoot32?

**Modularity** means that each main subsystem has **low coupling** and can be instantiated, tested, and used in isolation, without depending on other subsystems. This allows:

- **Independent testing**: Each module can be unit tested
- **Selective usage**: Use only the modules you need
- **Easy replacement**: Change implementations without affecting the rest of the code
- **Conditional compilation**: Exclude entire subsystems at compile time to save firmware size and RAM

**Concrete examples of independence:**

```cpp
// 1. AudioEngine works without Renderer or SceneManager (if enabled)
#if PIXELROOT32_ENABLE_AUDIO
AudioConfig audioConfig;
AudioEngine audio(audioConfig);
audio.init();
audio.playEvent({WaveType::PULSE, 440.0f, 0.5f, 0.8f});
#endif

// 2. Renderer can be used without Audio or Input
DisplayConfig displayConfig;
Renderer renderer(displayConfig);
renderer.init();
renderer.beginFrame();
renderer.drawSprite(sprite, 10, 10, Color::White);
renderer.endFrame();

// 3. InputManager is autonomous
InputConfig inputConfig;
InputManager input(inputConfig);
input.init();
input.update(deltaTime);
if (input.isButtonPressed(0)) { /* ... */ }

// 4. CollisionSystem is optional per scene (if enabled)
#if PIXELROOT32_ENABLE_PHYSICS
Scene scene;
// You can update physics only if you need it
scene.collisionSystem.update();
#endif

// 5. Interchangeable drivers without changing game code
// Same code works with TFT_eSPI_Drawer, U8G2_Drawer, or SDL2_Drawer
```

**Note**: `Engine` is the only component with tight coupling (orchestrates everything), but each subsystem can exist and function independently. The modular compilation system uses `PIXELROOT32_ENABLE_*` flags to conditionally compile subsystems, dramatically reducing firmware size and RAM usage on embedded targets.

---

## Main Architectural Features

- Stack-based Scene-Entity system
- Rendering with logical resolution independent of physical resolution
- NES-style 4-channel audio subsystem (conditionally compiled)
- UI system with automatic layouts (conditionally compiled)
- "Flat Solver" physics with specialized Actor types (conditionally compiled)
- Circular and AABB collision support
- Multi-platform support through driver abstraction
- **Modular compilation** for selective subsystem inclusion

---

## Layer Hierarchy

The engine is organized into 5 architectural layers:

| Layer | Name | Description | Document |
|-------|------|-------------|----------|
| Layer 0 | Hardware | Physical hardware (ESP32, displays, audio) | [Hardware layer](./layer-hardware.md) |
| Layer 1 | Drivers | Platform-specific drivers (TFT_eSPI, U8G2, SDL2) | [Driver layer](./layer-drivers.md) |
| Layer 2 | Abstraction | Abstract interfaces (DrawSurface, PlatformMemory) | [Abstraction layer](./layer-abstraction.md) |
| Layer 3 | Systems | High-level subsystems (Renderer, Audio, Physics, UI) | [Systems layer](./layer-systems.md) |
| Layer 4 | Scene | Scene and entity management | [Scene layer](./layer-scene.md) |
| Layer 5 | Game | User game code | (Implemented by user) |

---

## Subsystem Deep Dives

For detailed documentation on specific subsystems, see:

| Subsystem | Document |
|-----------|----------|
| Audio NES | [Audio subsystem](./audio-subsystem.md) |
| Physics | [Physics subsystem](./physics-subsystem.md) |
| Memory Management | [Memory system](./memory-system.md) |
| Resolution Scaling | [Resolution scaling](./resolution-scaling.md) |
| Tile Animation | [Tile animation](./tile-animation.md) |
| Touch Input | [Touch input](./touch-input.md) |
| Extending | [Extending PixelRoot32](../guide/extending-pixelroot32.md) |

---

## Quick Reference: Module Dependencies

```
Engine
├── SceneManager
│   └── Scene
│       ├── Entity
│       │   ├── Actor
│       │   └── UIElement
│       └── CollisionSystem
├── Renderer
│   ├── DrawSurface (abstract)
│   │   ├── TFT_eSPI_Drawer
│   │   ├── U8G2_Drawer
│   │   └── SDL2_Drawer
│   ├── Font (abstract)
│   │   └── Font5x7
│   └── Camera2D
├── InputManager
│   └── InputConfig
├── AudioEngine
│   ├── AudioScheduler (abstract)
│   │   ├── ESP32AudioScheduler
│   │   └── NativeAudioScheduler
│   └── MusicPlayer
└── PlatformCapabilities
```

---

## Performance Optimizations

### Implemented Strategies

1. **Logical vs Physical Resolution**: Rendering at low resolution (e.g., 128x128) with high-performance scaling to physical display (e.g., 240x240).

2. **Scaling Pipeline (v1.0.0)**:
   - **Fast-Path Switching**: Specialized routines for 1:1 and 2x integer scaling
   - **Bit-Expansion LUTs**: OLED horizontal expansion via lookup tables
   - **32-bit Register Writes**: TFT vertical duplication via optimized memcpy

3. **Multi-Core Audio (ESP32)**:
   - Core 0: Audio scheduling and generation
   - Core 1: Main game loop

4. **Mixer LUT**: Lookup tables for mixing without FPU

5. **DMA Pipelining (TFT)**: Double buffering with large block sizes

6. **IRAM-Cached Rendering**: Critical functions in internal RAM

7. **Viewport Culling**: Only render visible entities

### Performance Metrics

- **FPS Target**: 30-60 FPS on ESP32
- **Audio Latency**: < 50ms
- **Memory Footprint**: < 100KB RAM for complete engine
- **Sprite Capacity**: 100+ sprites @ 60fps (logical resolution 128x128)

---

## Configuration and Compilation

### Key Configuration Files

| File | Description |
|------|-------------|
| `platforms/EngineConfig.h` | Global engine configuration |
| `platforms/PlatformDefaults.h` | Platform-specific defaults |
| `platforms/PlatformCapabilities.h` | Hardware detection |
| `graphics/DisplayConfig.h` | Display configuration |
| `input/InputConfig.h` | Input configuration |
| `audio/AudioConfig.h` | Audio configuration |

### Common Compilation Flags

| Flag | Description |
|------|-------------|
| `PLATFORM_ESP32` | Compilation for ESP32 |
| `PLATFORM_NATIVE` | Compilation for PC |
| `PIXELROOT32_ENABLE_DEBUG_OVERLAY` | Enable debug overlay |
| `PIXELROOT32_ENABLE_2BPP_SPRITES` | 2bpp sprite support |
| `PIXELROOT32_ENABLE_4BPP_SPRITES` | 4bpp sprite support |
| `PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE` | Static 4bpp tilemap layer framebuffer snapshot (`StaticTilemapLayerCache`) |

---

## Conclusion

PixelRoot32 implements a well-defined layered architecture that enables:

1. **Portability**: 100% portable game code between ESP32 and PC
2. **Modularity**: Independent and replaceable subsystems
3. **Performance**: Specific optimizations for embedded hardware
4. **Extensibility**: Easy addition of new drivers and features
5. **Simplicity**: Intuitive API inspired by Godot Engine

The Scene-Entity architecture provides a familiar programming model for game developers, while the driver abstraction layer enables multi-platform support without sacrificing performance.
