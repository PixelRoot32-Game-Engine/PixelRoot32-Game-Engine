# Roadmap

Full roadmap for PixelRoot32 Game Engine — planned, in-progress and completed work.

## Planned

- 💾 **Persistence (Save/Load)**: Abstract key-value storage (NVS on ESP32).
- 📡 **ESP-NOW Networking Module**: Optional peer-to-peer communication layer for local multiplayer and device synchronization. Provides packet abstraction, Scene event integration, optional reliability (ACK/retry), and deterministic state sync. Designed for router-free ESP32 communication.
- 🔊 **Audio Coprocessor Module**: Optional dual-ESP32 architecture that offloads audio synthesis to a dedicated ESP32-C3 via SPI, improving game performance while remaining fully backward compatible.

## Completed Features ✅

- ✅ **Gameplay Framework**: Grid space, state machines, object pools, event bus, interaction triggers, room graphs, camera tweens, depth sorting and spatial queries — every capability opt-in behind its own `PIXELROOT32_ENABLE_*` flag. Shipped in [1.9.0](../CHANGELOG.md).
- ✅ **Spatial Partitioning (Uniform Grid)**: Optional collision optimization system that divides the world into fixed-size grid cells to reduce collision checks.
- ✅ **Advanced Physics System (Flat Solver)**: Godot-like Kinematic/Rigid actors, stable stacking, and iterative collision resolution.
- ✅ **Moving Platform Support**: Kinematic floor velocity inheritance and platform-aware character movement.
- ✅ **Custom Hitboxes**: Independent hitbox sizing and offsets for gameplay collision tuning.
- ✅ **Scene Transition System**: Fade, Iris, and Diagonal Wipe transitions with configurable animation behavior.
- ✅ **Camera Effects System**: Deterministic shake, punch, and offset effects optimized for ESP32.
- ✅ **Dual Numeric Backend (Float / Fixed-Point)**: Support for ESP32 variants without FPU (C3, C2, C6).
- ✅ **u8g2 Support**: Support for monochrome OLEDs (SSD1306, SH1106).
- ✅ **Native Bitmap Font System**: Font system based on 1bpp sprites.
- ✅ **UI Layout System**: Automatic layouts (Vertical, Horizontal, Grid, Panel, Anchor, Padding).
- ✅ **Tile Animation System**: O(1) frame resolution for tile-based animations with zero-allocation policy.
- ✅ **Multi-Palette Graphics**: Per-cell palette indexing for tilemaps and sprites.
- ✅ **One-Way Platform Collision**: Jump-through platforms with spatial crossing detection.
- ✅ **Modular Compilation**: `PIXELROOT32_ENABLE_*` flags for conditional subsystem inclusion.
- ✅ **Unified Logging System**: Cross-platform `log()` abstraction with `PIXELROOT32_DEBUG_MODE`.
- ✅ **Touch Screen Support**: `UITouchButton`, `UITouchCheckbox`, and `UITouchSlider`.
- ✅ **4+4 Audio Voice Partition**: Melodic sequencer tracks (slots 0–3) isolated from percussion/SFX (slots 4–7) with subpool-limited voice stealing.
- ✅ **Advanced SFX Synthesis**: Looping SFX, noise period sweep, Linear/Exponential curves, duty/pitch breakpoint envelopes, and header-only `playSfxBank` (additive `AudioEvent` ABI).
- ✅ **Shared APU Library**: The synthesis core lives in [PixelRoot32-APU](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-APU), shared with the PixelRoot32 Tool Suite — one ABI source, byte-identical preview/export audio.
- ✅ **TileMap Editor**: Specialized tool to design environments with C++ export. [PixelRoot32 Tool Suite](https://pixelroot32.com).

## Navigation

- [Home](../README.md)
- [Guide](./guide/) - User guides and how-to documentation
