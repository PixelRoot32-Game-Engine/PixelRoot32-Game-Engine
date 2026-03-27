# Project Overview: PixelRoot32 Game Engine

## Summary

PixelRoot32 is a lightweight, modular 2D game engine written in C++17, designed primarily for ESP32 microcontrollers, with a native simulation layer for PC (SDL2) that enables rapid development without hardware. The engine follows a scene-based architecture inspired by Godot Engine.

## Key Characteristics

| Property | Value |
|----------|-------|
| **Target Platforms** | ESP32/ESP32-S3 (primary), PC/Native with SDL2 (simulation) |
| **Language** | C++17 |
| **Architecture** | Layered (Hardware → Driver → Abstraction → System → Scene → Game) |
| **Design Philosophy** | Modularity, Selective Compilation, Portability, Performance, Extensibility |
| **Version** | v1.1.0 |

## Core Features

| Feature | Description |
|---------|-------------|
| **Rendering** | Logical resolution independent of physical; 1bpp/2bpp/4bpp sprites; tilemaps with viewport culling; multi-palette support |
| **Audio** | NES-style 4-channel audio (Pulse, Triangle, Noise); MusicPlayer for sequencing |
| **Physics** | "Flat Solver" with AABB and Circle collision; Static/Kinematic/Rigid actors; sensors; one-way platforms |
| **Input** | Button debouncing; states (Pressed, Released, Down, Clicked) |
| **UI System** | Automatic layouts (Horizontal, Vertical, Grid, Anchor); Labels, Buttons, Checkboxes, Panels |
| **Particles** | ParticleEmitter with configurable presets |
| **Tile Animation** | Frame-based animations (water, lava); zero dynamic allocations; O(1) lookup |

## Directory Structure

```
PixelRoot32-Game-Engine/
├── include/                    # Header files
│   ├── core/                   # Engine, SceneManager, Scene, Entity, Actor
│   ├── graphics/               # Renderer, DrawSurface, Camera2D, UI, Particles
│   ├── audio/                  # AudioEngine, MusicPlayer, AudioScheduler
│   ├── physics/                # CollisionSystem, Actors (Static, Kinematic, Rigid)
│   ├── input/                  # InputManager, InputConfig
│   ├── math/                   # Scalar, Fixed16, Vector2, MathUtil
│   ├── drivers/                # Platform drivers
│   │   ├── esp32/              # TFT_eSPI, U8G2, ESP32 DAC/I2S
│   │   └── native/              # SDL2
│   └── platforms/              # Platform abstractions, config
├── src/                        # Source implementation
├── docs/                       # Documentation
├── platformio.ini              # Build configuration
└── CMakeLists.txt              # Native build
```

## Architecture Layers

### 1. Hardware Layer
- **ESP32/ESP32-S3**: Main microcontrollers
- **Displays**: ST7789, ST7735 (TFT), SSD1306, SH1106 (OLED)
- **Audio**: Internal DAC, I2S with PAM8302A
- **Input**: Physical buttons on GPIOs

### 2. Driver Layer
- Platform-specific implementations
- TFT_eSPI_Drawer, U8G2_Drawer (ESP32)
- SDL2_Drawer, SDL2_AudioBackend (Native)

### 3. Abstraction Layer
- DrawSurface (Bridge Pattern)
- AudioScheduler (Strategy Pattern)
- Scalar/Fixed16 (Math abstraction)

### 4. System Layer
- Renderer, InputManager, AudioEngine, CollisionSystem, UI System, Particles, TileAnimation

### 5. Scene Layer
- Engine → SceneManager → Scene → Entity → Actor hierarchy

### 6. Game Layer
- User-defined game code

## Class Hierarchy

```
Engine
├── SceneManager
│   └── Scene
│       ├── Entity
│       │   ├── Actor
│       │   │   └── PhysicsActor
│       │   │       ├── StaticActor (walls, floors)
│       │   │       ├── SensorActor (triggers)
│       │   │       ├── KinematicActor (player movement)
│       │   │       └── RigidActor (physics objects)
│       │   └── UIElement
│       │       ├── UILabel, UIButton, UICheckbox
│       │       └── UIPanel → UILayout
│       └── CollisionSystem
├── Renderer
│   └── DrawSurface (TFT_eSPI, U8G2, SDL2)
├── InputManager
├── AudioEngine
│   ├── AudioScheduler
│   └── MusicPlayer
└── Camera2D
```

## Conditional Compilation Flags

### Modular Subsystems
| Flag | Default | Description |
|------|---------|-------------|
| `PIXELROOT32_ENABLE_AUDIO` | 1 | Audio subsystem |
| `PIXELROOT32_ENABLE_PHYSICS` | 1 | Physics/Collision system |
| `PIXELROOT32_ENABLE_UI_SYSTEM` | 1 | UI elements and layouts |
| `PIXELROOT32_ENABLE_PARTICLES` | 1 | Particle system |
| `PIXELROOT32_ENABLE_TILE_ANIMATIONS` | 1 | Tile animation system |
| `PIXELROOT32_ENABLE_2BPP_SPRITES` | 0 | 2bpp sprite support |
| `PIXELROOT32_ENABLE_4BPP_SPRITES` | 0 | 4bpp sprite support |
| `PIXELROOT32_ENABLE_DEBUG_OVERLAY` | 0 | FPS/RAM debug overlay |
| `PIXELROOT32_ENABLE_SCENE_ARENA` | 0 | Custom memory allocator |
| `PIXELROOT32_ENABLE_PROFILING` | 0 | Physics profiling hooks |

### Platform Macros
| Flag | Description |
|------|-------------|
| `PLATFORM_ESP32` | Target is ESP32 |
| `PLATFORM_NATIVE` | Target is PC/Native |
| `PIXELROOT32_DEBUG_MODE` | Enable debug logging |

### Memory Savings (when disabled)
| Subsystem | RAM Savings | Flash Savings |
|-----------|-------------|---------------|
| Audio | ~8 KB | ~15 KB |
| Physics | ~12 KB | ~25 KB |
| UI System | ~4 KB | ~20 KB |
| Particles | ~6 KB | ~10 KB |

## Physics System (Flat Solver)

The Flat Solver is a deterministic physics pipeline:

```
1. Detect Collisions    → Broadphase (Spatial Grid) + Narrowphase (AABB/Circle)
2. Solve Velocity       → Impulse-based response (2 iterations)
3. Integrate Positions  → p = p + v * dt
4. Solve Penetration    → Baumgarte stabilization
5. Trigger Callbacks     → onCollision notifications
```

### Actor Types
- **StaticActor**: Immovable walls/floors (static grid layer)
- **SensorActor**: Triggers (setSensor(true)), collision callbacks without physics response
- **KinematicActor**: Player characters, move_and_slide movement
- **RigidActor**: Full physics simulation with gravity

### Physics Constants
- `FIXED_DT = 1/60s`: Fixed timestep
- `SLOP = 0.02`: Ignore penetration < 2cm
- `BIAS = 0.2`: Position correction per frame
- `VELOCITY_ITERATIONS = 2`: Impulse solver passes

## Math System

### Scalar Abstraction
- **FPU platforms (ESP32, S3)**: Uses `float`
- **Non-FPU platforms (C3, C6)**: Uses Fixed16 (16.16 fixed-point)

### Fixed16 (non-FPU)
- 32-bit signed integer (16 bits integer, 16 bits fractional)
- Use `_fp` suffix for literals: `Scalar gravity = 9.8_fp;`

## Memory Limits

| Limit | Default | Configurable |
|-------|---------|---------------|
| Max Entities | 32 | ✅ `MAX_ENTITIES` |
| Max Render Layers | 3 | ✅ `MAX_LAYERS` |
| Max Physics Contacts | 128 | ✅ `PHYSICS_MAX_CONTACTS` |
| Spatial Grid Cell Size | 32px | ✅ `SPATIAL_GRID_CELL_SIZE` |

## Build Commands

| Command | Purpose |
|---------|---------|
| `pio run -e esp32dev` | Build for ESP32 |
| `pio run -e native` | Build for PC (native) |
| `pio test` | Run tests |
| `pio device monitor` | Serial monitor for ESP32 |

## Performance Targets

- **FPS**: 30-60 on ESP32
- **Audio Latency**: < 50ms
- **Memory**: < 100KB RAM for complete engine
- **Sprite Capacity**: 100+ sprites @ 60fps (128x128 logical resolution)

## Key Patterns

### Bridge Pattern
`DrawSurface` abstracts rendering from specific drivers (TFT_eSPI, U8G2, SDL2)

### Strategy Pattern
`AudioScheduler` allows different scheduling implementations (ESP32 vs Native)

### Scene-Entity System
Godot-inspired hierarchy: Engine → SceneManager → Scene → Entity → Actor

## Documentation Reference

| Document | Purpose |
|----------|---------|
| `docs/ARCHITECTURE.md` | Detailed architecture and design decisions |
| `docs/API_REFERENCE.md` | Complete API reference |
| `docs/PHYSICS_SYSTEM_REFERENCE.md` | Physics system details |
| `docs/AUDIO_NES_SUBSYSTEM_REFERENCE.md` | Audio subsystem |
| `docs/MEMORY_MANAGEMENT_GUIDE.md` | Memory management |
| `docs/STYLE_GUIDE.md` | Code style conventions |
| `docs/EXTENDING_PIXELROOT32.md` | Extension guide |
| `docs/MIGRATION_v1.1.0.md` | Migration from v1.0.0 |
| `docs/TESTING_GUIDE.md` | Testing guidelines |
| `docs/RESOLUTION_SCALING.md` | Resolution scaling |
| `docs/PLATFORM_COMPATIBILITY.md` | Platform compatibility |

## Version

- **Current**: v1.1.0
