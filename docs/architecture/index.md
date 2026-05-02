# Architecture

System architecture and design documentation for PixelRoot32 Game Engine.

## Overview

- [Architecture Overview](./overview.md) - Main architecture navigation hub
- [Layer Overview](./layers-overview.md) - Executive summary and design philosophy

## Layer Architecture

- [Layer 0 - Hardware](./layer-hardware.md) - ESP32, displays, audio hardware, PC simulation
- [Layer 1 - Drivers](./layer-drivers.md) - TFT_eSPI, U8G2, SDL2, AudioBackends
- [Layer 2 - Abstraction](./layer-abstraction.md) - DrawSurface, PlatformMemory, Logging, Math
- [Layer 3 - Systems](./layer-systems.md) - Renderer, Audio, Physics, UI subsystems
- [Layer 4 - Scene](./layer-scene.md) - Engine, SceneManager, Entity, Actor hierarchy

## Subsystem Deep Dives

- [Audio Subsystem](./audio-subsystem.md) - 4-channel NES-style audio
- [Physics Subsystem](./physics-subsystem.md) - Flat Solver, collisions, CCD
- [Memory System](./memory-system.md) - Smart pointers, RAII, ESP32 DRAM
- [Resolution Scaling](./resolution-scaling.md) - Logical vs physical resolution
- [Tile Animation](./tile-animation.md) - Lookup tables, O(1) resolve
- [Touch Input](./touch-input.md) - Pipeline, XPT2046, calibration

## Navigation

- [Guide](../guide/) - User guides and how-to documentation
- [API Reference](../api/) - Complete API documentation
- [Reference](../reference/) - Coding standards and guidelines
