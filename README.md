<p align="center">
  <img src="assets/pr32_logo.png" alt="PixelRoot32 Logo" width="300"/>
</p>

<h1 align="center">PixelRoot32 Game Engine</h1>

<p align="center">
  <strong>A lightweight, modular 2D game engine for ESP32 and PC</strong>
</p>

<p align="center">
  <a href="https://github.com/Gperez88/PixelRoot32-Game-Engine/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License"></a>
  <a href="https://github.com/Gperez88/PixelRoot32-Game-Engine"><img src="https://img.shields.io/github/stars/Gperez88/PixelRoot32-Game-Engine?style=social" alt="GitHub stars"></a>
  <a href="https://github.com/Gperez88/PixelRoot32-Game-Engine/issues"><img src="https://img.shields.io/github/issues/Gperez88/PixelRoot32-Game-Engine" alt="GitHub issues"></a>
  <a href="https://github.com/Gperez88/PixelRoot32-Game-Engine/pulls"><img src="https://img.shields.io/github/issues-pr/Gperez88/PixelRoot32-Game-Engine" alt="GitHub pull requests"></a>
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

> **⚠️ Project Status**: PixelRoot32 is under active development. APIs may change without notice, and some subsystems are experimental or incomplete.

---

## Demo in Action

Watch PixelRoot32 running on ESP32 with example games:

[![PixelRoot32 Demo](https://img.youtube.com/vi/55_Jwkx-gPs/0.jpg)](https://www.youtube.com/shorts/55_Jwkx-gPs)

> Click the image to watch the full demo on YouTube.  

## ✨ Key Features

- **Cross-Platform**: Develop on PC (Windows/Linux/macOS) and deploy on ESP32.
- **Scene-Entity System**: Intuitive management of Scenes, Entities, and Actors.
- **High Performance**: Optimized for ESP32 with DMA transfers and IRAM-cached rendering.
- **Sprite System**: Support for 1bpp/2bpp/4bpp sprites with flipping, rotation, and animation.
- **Tilemap Support**: Optimized rendering of large maps with viewport culling and multiple layers.
- **Independent Resolution Scaling**: Render at low logical resolutions (e.g., 128x128) and scale to physical displays (e.g., 240x240).
- **NES-Style Audio**: Built-in 4-channel audio subsystem (Pulse, Triangle, Noise).
- **Lightweight UI**: Label, Button, and Checkbox with automatic layouts.
- **AABB Physics**: Simple collision detection and kinematics.
- **Indexed Color Palettes**: Optimized palettes (PR32, NES, GameBoy, PICO-8) with dual-palette support.

> 💡 **Detailed info:** Check out the [Full Feature List](https://docs.pixelroot32.org/getting_started/what_is_pixelroot32/).

---

## 🚀 Quick Start

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

1. **Clone the samples repository:**

   ```bash
   git clone https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Samples.git
   cd PixelRoot32-Game-Engine-Samples
   ```

2. **Open in VS Code** and select your environment (`env:esp32dev`, `env:esp32s3`, or `env:native`).
3. **Build and Upload** using PlatformIO.

> 📚 **More information:** See the [Getting Started Guide](https://docs.pixelroot32.org/getting_started/what_is_pixelroot32/).

---

## 🛠️ Best Practices

To ensure high performance on ESP32, PixelRoot32 enforces strict development patterns:

1. **Fixed-Point Math**: Always use `Scalar` instead of `float`. Use `math::toScalar()` for literals.
2. **Zero Allocation**: Avoid `new`/`malloc` during the game loop. Use **Object Pooling** and `std::unique_ptr`.
3. **Render Layers**: Organize entities by `renderLayer` (0=Bg, 1=Game, 2=UI) to optimize drawing order.

> 📘 **Essential Reading**: Check the **[Style & Best Practices Guide](docs/STYLE_GUIDE.md)** for detailed rules on memory management, performance optimization, and coding style.

---

## 📚 Documentation

### Online Resources

- **[📖 Full Documentation](https://docs.pixelroot32.org)**: Guides, API reference, and tutorials.
- **[🎮 Game Samples](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Samples)**: Complete examples to start building.
- **[🛠️ Asset Tools](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler)**: Sprite compiler and development tools.

### Local Reference

- **[API Reference](docs/API_REFERENCE.md)**: Class reference and usage.
- **[Audio Subsystem](docs/AUDIO_NES_SUBSYSTEM_REFERENCE.md)**: Sound engine details.
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

---

## 🕒 Changelog

## 1.0.0 (Stable)

First stable release. Complete performance overhaul and API stabilization.

### 🚀 Rendering Performance

- **TFT DMA Pipelining**: Double-buffered pipeline for `TFT_eSPI_Drawer` — CPU processes next block while DMA transmits current one. **~43 FPS** stable on 240×240 displays @ 40MHz (up from ~14 FPS).
- **Fast-Path Kernels**: OLED 2x bit-expansion LUT (U8G2); TFT row duplication with 32-bit native access and `memcpy` for vertical scaling.
- **I2C 1MHz**: Official support in `DisplayConfig` for sustained **60 FPS** on OLED (SSD1306/SH1106).

### 🎮 Physics (Flat Solver 1.0)

- **Broadphase**: Uniform grid (32px cells) with static shared buffers to reduce DRAM usage.
- **KinematicActor**: Rewrote `moveAndSlide` and `moveAndCollide` with binary search, wall sliding, and accurate collision normal detection.
- **Stable stacking**: Baumgarte correction, iterative position relaxation, fixed timestep 1/60s.
- **Godot-style API**: `KinematicCollision`, actor types `Static`/`Kinematic`/`Rigid`. Renamed `PHYSICS_RELAXATION_ITERATIONS` → `VELOCITY_ITERATIONS`.

### 🔢 Math System (Scalar / Fixed-Point)

- Numeric abstraction layer: `Scalar` = `float` on ESP32-S3 (FPU) or `Fixed16` (Q16.16) on C3/S2/C6.
- `Vector2`, `Rect`, and physics unified under `Scalar`. ~30% FPS gain on C3/S2 by eliminating software float emulation.
- `MathUtil`: `fixed_sqrt`, `fixed_sin`, `fixed_cos`, `toScalar()`.

### 🛠️ Other

- **Memory**: Explicit `MALLOC_CAP_DMA` support in drivers; broadphase buffer reuse across frames.
- **C++17**: Migrated from C++11.

> **Migration guide v0.8.1-dev → v1.0.0**: [MIGRATION_v1.0.0](docs/MIGRATION_v1.0.0.md)

### 0.8.1-dev

- **Render Loop Fix**: Resolved critical double-buffer send issue on ESP32, saving ~23ms per frame.
- **Stutter-Free**: Removed periodic Serial logs (Heartbeat/DMA) for smoother gameplay.

### 0.8.0-dev

- **Extreme Display Performance**: Implemented Parallel DMA Pipeline and aggressive optimizations for `TFT_eSPI`, significantly boosting FPS.
- **Fast 1:1 Rendering**: Added a dedicated 32-bit fast path for non-scaled rendering, bypassing all scaling overhead.
- **Native XBM Blitting**: Refactored `U8G2` driver to use row-aligned buffers and native XBM calls, eliminating per-pixel draw overhead.
- **Latency Reduction**: Replaced blocking delays with `yield()` in the engine loop to maximize CPU utilization.

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
