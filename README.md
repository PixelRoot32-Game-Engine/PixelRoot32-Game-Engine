<p align="center">
  <img src="assets/pr32_logo.png" alt="PixelRoot32 Logo" width="300"/>
</p>

<h1 align="center">PixelRoot32 Game Engine</h1>

<p align="center">
  <strong>A lightweight, modular 2D game engine for ESP32 and PC</strong>
</p>

<p align="center">
  <a href="https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License"></a>
  <a href="https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine"><img src="https://img.shields.io/github/stars/Gperez88/PixelRoot32-Game-Engine?style=social" alt="GitHub stars"></a>
  <a href="https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine/issues"><img src="https://img.shields.io/github/issues/Gperez88/PixelRoot32-Game-Engine" alt="GitHub issues"></a>
  <a href="https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine/pulls"><img src="https://img.shields.io/github/issues-pr/Gperez88/PixelRoot32-Game-Engine" alt="GitHub pull requests"></a>
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

> **✅ Project Status**: PixelRoot32 v1.1.0 is stable release.
The core rendering and physics systems are production-ready.
New features and optimizations will continue to evolve in future minor versions.

---

## Demo in Action

Watch PixelRoot32 running on ESP32 with example games:

[![PixelRoot32 Demo](https://img.youtube.com/vi/55_Jwkx-gPs/0.jpg)](https://www.youtube.com/shorts/55_Jwkx-gPs)

> Click the image to watch the full demo on YouTube.  

## ✨ Key Features

- **Cross-Platform**: Develop on PC (Windows/Linux/macOS) and deploy on ESP32.
- **Scene-Entity System**: Intuitive management of Scenes, Entities, and Actors.
- **High Performance**: Optimized for ESP32 with DMA transfers and IRAM-cached rendering.
- **Sprite System**: Support for 1bpp/2bpp/4bpp sprites with multi-palette selection, flipping, rotation, and animation.
- **Tilemap Support**: Optimized rendering with viewport culling, multi-palette, and tile animations.
- **Tile Animation System**: Frame-based animations (water, lava) with O(1) frame resolution and zero-allocation policy.
- **Independent Resolution Scaling**: Render at low logical resolutions (e.g., 128x128) and scale to physical displays (e.g., 240x240).
- **NES-Style Audio**: Built-in 4-channel audio subsystem (Pulse, Triangle, Noise).
- **Lightweight UI**: Label, Button, and Checkbox with automatic layouts.
- **AABB Physics**: Godot-style physics with Kinematic/Rigid actors, sensors, and one-way platforms.
- **Indexed Color Palettes**: Optimized palettes (PR32, NES, GameBoy, PICO-8) with multi-palette support.
- **Modular Architecture**: Compile only needed subsystems via `PIXELROOT32_ENABLE_*` flags to reduce firmware size.

> 💡 **Detailed info:** Check out the [Full Feature List](https://docs.pixelroot32.org/getting_started/what_is_pixelroot32/).

---

## ⚠️ Known Issues

### DMA + ESP32-S3 + Arduino Core > 2.0.14

**Problem**: When using ESP32-S3 with Arduino Core versions newer than 2.0.14, DMA-based transfers may freeze after the first frame. This is a known issue affecting the ESP32-S3 GDMA subsystem in ESP-IDF 4.4.7+ (used by Arduino Core 2.0.15+).

**Symptoms**:

- Display freezes after rendering the first frame
- DMA transfer not completing
- Random crashes during display initialization

**Workaround**: Use Arduino Core 2.0.14 (the last stable version before the GDMA changes).

In PlatformIO, this is configured via the `platform_packages` directive:

```ini
[env:esp32s3]
platform_packages =
    framework-arduinoespressif32 @ https://github.com/espressif/arduino-esp32#2.0.14
```

> **Note**: This workaround is already configured in the `hello_world` example's `platformio.ini` for ESP32-S3. If you create new projects, ensure this is set when targeting ESP32-S3.

**Related Issues**:

- [espressif/arduino-esp32 #9618](https://github.com/espressif/arduino-esp32/issues/9618) - Original report: ESP32-S3 DMA issues with Core > 2.0.14
- [TFT_eSPI #3329](https://github.com/Bodmer/TFT_eSPI/issues/3329)
- [TFT_eSPI #3367](https://github.com/Bodmer/TFT_eSPI/issues/3367)
- [ESP32-HUB75-MatrixPanel-DMA #775](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/issues/775)

---

### Framework Cache Corruption (pins_arduino.h missing)

**Problem**: When Arduino Core packages become corrupted (especially after changing versions like the DMA workaround), you may encounter:

```
fatal error: pins_arduino.h: No such file or directory
```

**Symptoms**:

- Build fails with `pins_arduino.h` not found
- Previously working projects suddenly stop compiling
- Happens after changing Arduino Core versions (e.g., applying the DMA workaround)

**Solution**:

1. Clean the build cache:

   ```bash
   pio run --target clean
   ```

2. Remove the corrupted framework package:

   ```bash
   rmdir /s /q %USERPROFILE%\.platformio\packages\framework-arduinoespressif32
   ```

3. Rebuild - PlatformIO will reinstall the framework:

   ```bash
   pio run
   ```

**Prevention**: After changing `platform_packages` for Arduino Core versions, always run a clean build to ensure the framework is properly reinstalled.

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

### Fast Setup

1. **Clone this repository** and open an example under [`examples/`](examples/):

   ```bash
   git clone https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine.git
   cd PixelRoot32-Game-Engine/examples/hello_world
   ```

   Each folder (`hello_world`, `animated_tilemap`, `snake`, `flappy_bird`, `metroidvanina`, `tic_tac_toe`, `physics`, `camera`, `dual_palette`, `sprites`) is a **standalone PlatformIO project** with its own `platformio.ini`.

2. **Open that example folder in VS Code** (File → Open Folder) and select your environment (`env:esp32dev`, `env:esp32cyd`, `env:esp32c3`, or `env:native`).
3. **Build and Upload** using PlatformIO.

> 📚 **More information:** See the [Getting Started Guide](https://docs.pixelroot32.org/getting_started/what_is_pixelroot32/).

---

## 🛠️ Best Practices

To ensure high performance on ESP32, PixelRoot32 enforces strict development patterns:

1. **Fixed-Point Math**: Always use `Scalar` instead of `float`. Use `math::toScalar()` for literals.
2. **Zero Allocation**: Avoid `new`/`malloc` during the game loop. Use **Object Pooling** and `std::unique_ptr`.
3. **Render Layers**: Organize entities by `renderLayer` (0=Bg, 1=Game, 2=UI) to optimize drawing order.
4. **Platform Memory**: Use `PIXELROOT32_FLASH_ATTR` and `PIXELROOT32_READ_*_P` macros for cross-platform Flash/RAM access.
5. **Centralized Logging**: Use `log()` from `core/Log.h` instead of `Serial.print` or `printf`.

> 📘 **Essential Reading**: Check the **[Style & Best Practices Guide](docs/STYLE_GUIDE.md)** for detailed rules on memory management, performance optimization, and coding style.

---

## 📚 Documentation

### Online Resources

- **[📖 Full Documentation](https://docs.pixelroot32.org)**: Guides, API reference, and tutorials.
- **[📦 Examples](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine/tree/main/examples)**: Runnable demos in-repo (`examples/*`, each with its own `platformio.ini`).
- **[🛠️ Asset Tools](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler)**: Sprite compiler and development tools.

### Local Reference

- **[Examples](examples/)**: Local path to the same demos (open a subfolder in PlatformIO).
- **[API Reference](docs/API_REFERENCE.md)**: Class reference and usage.
- **[Architecture](docs/ARCHITECTURE.md)**: System design and layer hierarchy (includes [ESP32 tilemap static cache](docs/ARCHITECTURE.md#esp32-rendering-pipeline-and-tilemap-caching)).
- **[Animated tilemap example](examples/animated_tilemap/README.md)**: **Read this** if you use `AnimatedTilemapScene` or **`StaticTilemapLayerCache`**—documents engine snapshot API, **`invalidateStaticLayerCache()`**, and **static vs dynamic** layer groups (performance-critical on ESP32).
- **[Physics System](docs/architecture/ARCH_PHYSICS_SUBSYSTEM.md)**: Flat Solver documentation.
- **[Audio Subsystem](docs/architecture/ARCH_AUDIO_SUBSYSTEM.md)**: Sound engine details.
- **[Migration v1.1.0](docs/MIGRATION_v1.1.0.md)**: Guide for upgrading from v1.0.0.
- **[Contributing](CONTRIBUTING.md)** | **[Style Guide](docs/STYLE_GUIDE.md)**

---

## 🗺️ Roadmap

- 🗺️ **TileMap Editor**: Specialized tool to design environments with C++ export.
- 🎵 **Music Editor**: Mini DAW for SFX and music creation.
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

---

## 🕒 Changelog

## 1.1.0

### 🎨 Graphics & Animations

- **Multi-Palette Support**: Multi-palette tilemaps and sprites with per-cell palette indexing.
- **Tile Animation System**: O(1) frame resolution animations with zero-allocation policy.

### 🎮 Physics

- **One-Way Platforms**: Jump-through platforms with spatial crossing detection.
- **TileCollisionBuilder**: New builder for generating physics bodies from tile layers.

### ⚡ Architecture

- **Modular Compilation**: `PIXELROOT32_ENABLE_*` flags for conditional subsystem inclusion.
- **Unified Logging**: Cross-platform `log()` with `PIXELROOT32_DEBUG_MODE` flag.

> **Migration guide v1.0.0 → v1.1.0**: [MIGRATION_v1.1.0](docs/MIGRATION_v1.1.0.md)

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
