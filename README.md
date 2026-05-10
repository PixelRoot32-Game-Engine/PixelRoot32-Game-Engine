<p align="center">
  <img src="assets/pr32_logo.png" alt="PixelRoot32 Logo" width="300"/>
</p>

<h1 align="center">PixelRoot32 Game Engine</h1>

<p align="center">
  <strong>A lightweight, modular 2D game engine for ESP32 and PC</strong>
</p>

<p align="center">
  <a href="https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License"></a>
  <a href="https://registry.platformio.org/libraries/gperez88/PixelRoot32-Game-Engine"><img src="https://badges.registry.platformio.org/packages/gperez88/library/PixelRoot32-Game-Engine.svg" alt="PlatformIO Registry" /></a>
  <a href="https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine"><img src="https://img.shields.io/github/stars/Gperez88/PixelRoot32-Game-Engine?style=social" alt="GitHub stars"></a>
  <a href="https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine/issues"><img src="https://img.shields.io/github/issues/Gperez88/PixelRoot32-Game-Engine" alt="GitHub issues"></a>
  <a href="https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine/pulls"><img src="https://img.shields.io/github/issues-pr/Gperez88/PixelRoot32-Game-Engine" alt="GitHub pull requests"></a>
</p>

<p align="center">
  <a href="https://ko-fi.com/gperez88"><img src="https://img.shields.io/badge/Support%20me%20on%20Ko--fi-29ABE0?style=flat&logo=ko-fi&logoColor=ffffff" alt="Support on Ko-fi"></a>
  <a href="https://www.paypal.com/ncp/payment/THC3PDSRQKZW6"><img src="https://img.shields.io/badge/Support%20me%20on%20PayPal-0070BA?style=flat&logo=paypal&logoColor=ffffff" alt="Support on PayPal"></a>
</p>

<p align="center">
  <a href="#-overview">Overview</a> •
  <a href="#-key-features">Features</a> •
  <a href="#-quick-start">Quick Start</a> •
  <a href="#-best-practices">Best Practices</a> •
  <a href="#-documentation">Documentation</a> •
  <a href="#-roadmap">Roadmap</a> •
  <a href="#-changelog">Changelog</a> •
  <a href="#-contributing">Contributing</a> •
  <a href="#-license">License</a> •
  <a href="#-credits">Credits</a>
</p>

---

## 📖 Overview

**PixelRoot32** is a lightweight, modular 2D game engine written in **C++17**, designed primarily for **ESP32 microcontrollers**, with a native simulation layer for **PC (SDL2)** to enable rapid development without hardware.

The engine follows a scene-based architecture inspired by **Godot Engine**, making it intuitive for developers familiar with modern game development workflows.

## 🧠 Engine Philosophy

PixelRoot32 is not the product of a traditional electronics expert or a large engineering team.

It was born from curiosity, experimentation, and a deep love for retro games from the 90s.

Coming from a mobile development background with limited experience in C++, this project represents a leap into embedded systems. This was made possible by how accessible knowledge has become today—especially with AI-assisted tools lowering the barrier to building complex systems.

PixelRoot32 is a reflection of that shift: exploring new domains driven by curiosity rather than specialization.

At its core, the engine is built around a few simple ideas:

- Determinism over convenience  
- No hidden costs (no runtime allocations, no magic)  
- Simplicity and explicit control  
- Performance as a core constraint  
- Retro-inspired design with modern practices  

👉 Learn more: [Engine Philosophy](docs/philosophy/engine-philosophy.md)

---

## Demo in Action

Watch PixelRoot32 running on ESP32 with example games:

[![PixelRoot32 Demo](https://img.youtube.com/vi/55_Jwkx-gPs/0.jpg)](https://www.youtube.com/shorts/55_Jwkx-gPs)

> Click the image to watch the full demo on YouTube.  

## ✨ Key Features

- **Cross-Platform**: Develop on PC (Windows/Linux/macOS) and deploy on ESP32.
- **Scene-Entity System**: Intuitive management of Scenes, Entities, and Actors.
- **High Performance**: Optimized for ESP32 with DMA transfers, IRAM-cached rendering, and a Dirty Regions pipeline.
- **Sprite System**: Support for 1bpp/2bpp/4bpp sprites with multi-palette selection, flipping, rotation, and animation.
- **Tilemap Support**: Optimized rendering with viewport culling, static layer caching, multi-palette, and tile animations.
- **Tile Animation System**: Frame-based animations (water, lava) with O(1) frame resolution and zero-allocation policy.
- **Independent Resolution Scaling**: Render at low logical resolutions (e.g., 128x128) and scale to physical displays (e.g., 240x240).
- **NES-Style Audio**: Built-in dynamic 8-voice audio subsystem with fixed-point No-FPU optimizations (Pulse, Triangle, Noise, Sine, Saw).
- **Lightweight UI**: Label, Button, and Checkbox with automatic layouts.
- **AABB Physics**: Godot-style physics with Kinematic/Rigid actors, sensors, and one-way platforms.
- **Indexed Color Palettes**: Optimized palettes (PR32, NES, GameBoy, PICO-8) with multi-palette support.
- **Modular Architecture**: Compile only needed subsystems via `PIXELROOT32_ENABLE_*` flags to reduce firmware size.

> 💡 **Detailed info:** Check out the [Full Feature List](https://docs.pixelroot32.org/#getting-started).

---

## Quick Start

### ⚠️ Configuration Requirement

To compile PixelRoot32, you **must** configure your `platformio.ini` to use C++17 and disable exceptions:

```ini
build_unflags = -std=gnu++11
build_flags =
    -std=gnu++17
    -fno-exceptions
```

### Prerequisites

- **VS Code + PlatformIO**
- **ESP32 DevKit** or **SDL2** (for PC simulation)

### 📦 Installation (via PlatformIO)

To use PixelRoot32 in your own project, add the following to the `lib_deps` option of your `platformio.ini`:

```ini
lib_deps =
    gperez88/PixelRoot32-Game-Engine@^1.5.0
```

PlatformIO will automatically download and install the library and its dependencies during the next build.

### Fast Setup

1. **Clone this repository** and open an example under [`examples/`](examples/):

   ```bash
   git clone https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine.git
   cd PixelRoot32-Game-Engine/examples/hello_world
   ```

   Each folder (`hello_world`, `animated_tilemap`, `snake`, `flappy_bird`, `metroidvanina`, `tic_tac_toe`, `space_invaders`, `brick_breaker`, `physics`, `camera`, `dual_palette`, `sprites`) is a **standalone PlatformIO project** with its own `platformio.ini`.

2. **Open that example folder in VS Code** (File → Open Folder) and select your environment (`env:esp32dev`, `env:esp32cyd`, `env:esp32c3`, or `env:native`).
3. **Build and Upload** using PlatformIO.

> 📚 **More information:** See the [Getting Started Guide](https://docs.pixelroot32.org/).

---

## 🛠️ Best Practices

To ensure high performance on ESP32, PixelRoot32 enforces strict development patterns:

1. **Fixed-Point Math**: Always use `Scalar` instead of `float`. Use `math::toScalar()` for literals.
2. **Zero Allocation**: Avoid `new`/`malloc` during the game loop. Use **Object Pooling** and `std::unique_ptr`.
3. **Render Layers**: Organize entities by `renderLayer` (0=Bg, 1=Game, 2=UI) to optimize drawing order.
4. **Platform Memory**: Use `PIXELROOT32_FLASH_ATTR` and `PIXELROOT32_READ_*_P` macros for cross-platform Flash/RAM access.
5. **Centralized Logging**: Use `log()` from `core/Log.h` instead of `Serial.print` or `printf`.

> 📘 **Essential Reading**: Check the **[Style & Best Practices Guide](docs/guide/index.md#-standards-&-compatibility)** for detailed rules on memory management, performance optimization, and coding style.

---

## 📚 Documentation

### Online Resources

- **[📖 Full Documentation](https://docs.pixelroot32.org)**: Guides, API reference, and tutorials.
- **[🛠️ Asset Tools](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler)**: Sprite compiler and development tools.

### Local Reference

- **[Examples](examples/)**: Local path to the same demos (open a subfolder in PlatformIO).
- **[API Reference](docs/api/index.md)**: Class reference and usage.
- **[Architecture](docs/architecture/overview.md)**: System design and layer hierarchy.
- **[Physics System](docs/architecture/physics-subsystem.md)**: Flat Solver documentation.
- **[Audio Subsystem](docs/architecture/audio-subsystem.md)**: Sound engine details.
- **[Contributing](CONTRIBUTING.md)** | **[Style Guide](docs/guide/style-guide.md)**

---

## 🗺️ Roadmap

- 🗺️ **TileMap Editor**: Specialized tool to design environments with C++ export.
- 🎵 **Music Editor**: Mini DAW for SFX and music creation.
- 💾 **Persistence (Save/Load)**: Abstract key-value storage (NVS on ESP32).
- 📡 **ESP-NOW Networking Module**: Optional peer-to-peer communication layer for local multiplayer and device synchronization. Provides packet abstraction, Scene event integration, optional reliability (ACK/retry), and deterministic state sync. Designed for router-free ESP32 communication.

### Completed Features ✅

- ✅ **Spatial Partitioning (Uniform Grid)**: Optional collision optimization system that divides the world into fixed-size grid cells to reduce collision checks.
- ✅ **Advanced Physics System (Flat Solver)**: Godot-like Kinematic/Rigid actors, stable stacking, and iterative collision resolution.
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

---

## 🕒 Changelog

## 1.5.0

### 🚀 Rendering Performance & Graphics

- **Dirty Regions Pipeline**: Implemented `DirtyGrid` optimization with a double dirty grid to eliminate mandatory full-frame redraws, drastically reducing memory bandwidth on ESP32.
- **Static Tilemap Integration**: Integrated Dirty Regions with `StaticTilemapLayerCache` using fast-path `memcpy` background restores and intelligent dynamic-only dirty marking.

### 🔊 Audio System

- **No-FPU Optimizations**: Added Q15 fixed-point LFO (triangle waves, tremolo, vibrato) and High-Pass Filter (HPF) to eliminate slow soft-float operations on platforms like ESP32-C3.
- **Performance**: Replaced conditional branching with a static function pointer array dispatch in wave generation to reduce branch mispredictions.
- **Configurable Block Size**: Added `blockSize` parameter to `AudioScheduler` for platform-specific tuning.

### 🎮 Examples

- **2048 Puzzle Game**: Added a complete 2048 clone showcasing UI layouts, grid mechanics, NES-style audio, and dual-platform support (Native/ESP32-CYD).

### 🧪 Testing & QA

- **Collision System Tests**: Added comprehensive physics unit tests for sensor contacts, velocity integration, and penetration correction.
- **Expanded Coverage**: Improved assertions and expanded test coverage across audio, scene, input, graphics, and UI modules.

### ⚡ Memory Optimization

- **`InputConfig` API Change**: The `count` parameter is no longer required. Use `InputConfig(PIN1, PIN2, ...)` instead of `InputConfig(count, PIN1, PIN2, ...)`. See [migration guide](../docs/migration/migration-v1-5-0.md).


Full changelog: [CHANGELOG.md](CHANGELOG.md)

---

## 🤝 Contribute

Contributions are welcome! Read our [Contributing Guide](CONTRIBUTING.md) to get started.

---

## 📄 License

PixelRoot32 is an **open-source** project licensed under the **MIT License**.
Based on [ESP32-Game-Engine](https://github.com/nbourre/ESP32-Game-Engine) by nbourre.

See the [LICENSE](LICENSE) file for the full text.

> ⚠️ The PixelRoot32 Game Engine name and logo are subject to the trademark policy. See [TRADEMARK.md](./TRADEMARK.md).
---

## 👏 Credits

Developed by **Gabriel Perez** as a modular game engine for embedded systems.

Special thanks to **nbourre** for the original ESP32-Game-Engine.

---

<p align="center">
  <em>Built with ❤️ for the retro-dev community.</em>
</p>
