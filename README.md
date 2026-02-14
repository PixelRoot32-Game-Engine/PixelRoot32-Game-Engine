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
  <a href="#-documentation">Documentation</a> •
  <a href="#-roadmap">Roadmap</a> •
  <a href="#-changelog">Changelog</a> •
  <a href="#-contributing">Contributing</a> •
  <a href="#-license">License</a> •
  <a href="#-credits">Credits</a>
</p>

---

## 📖 Overview

**PixelRoot32** is a lightweight, modular 2D game engine written in C++, designed primarily for **ESP32 microcontrollers**, with a native simulation layer for **PC (SDL2)** to enable rapid development without hardware.

The engine follows a scene-based architecture inspired by **Godot Engine**, making it intuitive for developers familiar with modern game development workflows.

> **⚠️ Project Status:** PixelRoot32 is under active development. APIs may change and some subsystems are still experimental.

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
- ⚡ **Spatial Partitioning**: Uniform Grid for collision optimization.

### Completed Features ✅

- ✅ **u8g2 Support**: Support for monochrome OLEDs (SSD1306, SH1106).
- ✅ **Native Bitmap Font System**: Font system based on 1bpp sprites.
- ✅ **UI Layout System**: Automatic layouts (Vertical, Horizontal, Grid, Panel, Anchor, Padding).

---

## 🕒 Changelog

---

### v0.7.0-dev

- **Decoupled Multi-Core Audio**: New architecture running on Core 0 (ESP32) for sample-accurate timing and improved performance.
- **Advanced Audio Mixing**: Non-linear mixer with soft clipping and high-performance LUT-based mixing for no-FPU hardware (ESP32-C3).
- **Internal DAC Enhancements**: Optimized software-mode driver with 0.7x scaling for PAM8302A amplifiers and improved stability.
- **Unified Platform Configuration**: Consolidated settings in `include/platforms/` with new `PlatformDefaults.h` for better hardware support (ESP32-S3, etc.).
- **Graphics Extensibility & U8g2**: Introduced `BaseDrawSurface` and native support for monochromatic OLED displays via the U8G2 library.

### v0.6.0-dev

- **Independent Resolution Scaling**: Decoupled logical/physical resolution to improve performance.
- **Comprehensive Debug Overlay**: Real-time metrics for FPS, RAM, and CPU load.
- **Fixed Position UI**: Support for HUD elements that ignore camera scrolling.

> 📝 **Full History:** See the complete [CHANGELOG.md](CHANGELOG.md) for previous versions.

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
