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
  <a href="#-key-features">Features</a> •
  <a href="#-quick-start">Quick Start</a> •
  <a href="#-documentation">Documentation</a> •
  <a href="#-examples">Examples</a> •
  <a href="#-build">Build</a> •
  <a href="#-contribute">Contribute</a> •
  <a href="#-license">License</a>
</p>

---

## 📖 Overview

**PixelRoot32** is a lightweight, modular 2D game engine written in C++17, designed primarily for **ESP32 microcontrollers**, with a native simulation layer for **PC (SDL2)** to enable rapid development without hardware.

The engine follows a scene-based architecture inspired by **Godot Engine**, making it intuitive for developers familiar with modern game development workflows.

> **⚠️ Project Status:** PixelRoot32 is under active development. APIs may change and some subsystems are still experimental. Occasional issues or breaking changes are expected, especially in less-tested configurations; feedback and bug reports are welcome.

---

## ✨ Key Features

### 🎮 Engine Core
- **Scene and Entity System**: Scene management with Entities, Actors, PhysicsActors, and UI elements
- **Cross-Platform**: Develop on PC (Windows/Linux via **SDL2**) and deploy on ESP32 using **TFT_eSPI** (ST7735/ILI9341 via SPI/DMA)
- **Deterministic Game Loop**: Precise delta-time control and frame updates

### 🎨 Graphics
- **Sprite System**: Monochrome 1bpp sprites with support for multi-layer sprites, plus optional 2bpp/4bpp for richer assets
- **Sprite Animation**: Lightweight, step-based animation system compatible with simple sprites and `MultiSprite`
- **Color Palettes**: Fixed indexed palette (24 visible colors + Transparent) using RGB565 for fast rendering
- **Render Layers and Tilemaps**: Simple logical layers (background, gameplay, UI) and a compact 1bpp tilemap helper
- **2D Camera and Scrolling**: Camera with dead-zone (`Camera2D`) that follows a target horizontally (and optionally vertically)
- **Particle System**: High-performance particles with memory pooling

### 🔊 Audio
- **NES-Style Audio**: Built-in audio subsystem with 2 Pulse channels, 1 Triangle, and 1 Noise
- **Music Player**: Lightweight background music system based on notes

### 🎯 Physics and Collisions
- **AABB Collision Detection**: Axis-aligned bounding box collision system
- **Basic Gravity and Kinematics**: Suitable for arcade and simple platformer games

### 🖥️ User Interface
- **Lightweight UI System**: UI controls (Label, Button) with automatic layout management
- **Automatic Layouts**: `UIVerticalLayout`, `UIHorizontalLayout`, `UIGridLayout`, `UIPaddingContainer`, `UIPanel`, and `UIAnchorLayout` for organizing elements without manual calculations
- **Native Bitmap Font**: Text renderer based on 1bpp sprites with an integrated 5x7 font, ensuring pixel-perfect consistency across PC and ESP32

---

## 🚀 Quick Start

### Prerequisites

- **VS Code + PlatformIO extension**
- **C++17 toolchain**
- **ESP32 DevKit** (for hardware) or **SDL2** (for native development)

### Fast Setup

1. **Clone the samples repository:**
   ```bash
   git clone https://github.com/Gperez88/PixelRoot32-Game-Engine-Samples.git
   cd PixelRoot32-Game-Engine-Samples
   ```

2. **Open the project in VS Code** and let PlatformIO initialize

3. **Select the environment:**
   - `env:esp32dev` for ESP32
   - `env:native` for PC (requires SDL2)

4. **Build and run** from PlatformIO

### Minimal Example

```cpp
#include <core/Engine.h>
#include <drivers/esp32/ESP32_DAC_AudioBackend.h>

namespace pr32 = pixelroot32;

pr32::drivers::esp32::ESP32_DAC_AudioBackend audioBackend(25, 11025);
pr32::graphics::DisplayConfig displayConfig(
    pr32::graphics::DisplayDriver::TFT_eSPI, 0, 240, 240
);
pr32::input::InputConfig inputConfig(6, 32, 27, 33, 14, 13, 12);
pr32::audio::AudioConfig audioConfig(&audioBackend, 11025);

pr32::core::Engine engine(displayConfig, inputConfig, audioConfig);

void setup() {
    engine.init();
    // engine.setScene(&yourScene);
}

void loop() {
    engine.run();
}
```

> 📚 **More information:** See the [Full Documentation](#-documentation) for detailed guides.

---

## 📚 Documentation

### Main Documentation

- **[API Reference](API_REFERENCE.md)**: Complete class reference and usage examples
- **[Audio Subsystem](AUDIO_NES_SUBSYSTEM_REFERENCE.md)**: NES-style sound engine architecture
- **[Style Guide](STYLE_GUIDE.md)**: Coding conventions and best practices
- **[Contributing Guide](CONTRIBUTING.md)**: How to contribute to the project

### Quick Guides

- **[Getting Started](#-quick-start)**: Initial setup and first project
- **[Color Palettes](#-color-palettes)**: How to use and customize palettes
- **[Sprite System](#-sprite-system)**: Creating and using sprites
- **[UI System](#-ui-system)**: Building user interfaces

### Color Palettes

PixelRoot32 uses a fixed indexed palette optimized for embedded hardware:

| Palette | Description | Preview |
| :--- | :--- | :--- |
| `PR32` (Default) | PixelRoot32 standard palette | <img src="assets/palette_PR32.png" width="150"/> |
| `NES` | Nintendo Entertainment System style | <img src="assets/palette_NES.png" width="150"/> |
| `GB` | GameBoy style (Gray/Green) | <img src="assets/palette_GB.png" width="150"/> |
| `GBC` | GameBoy Color style | <img src="assets/palette_GBC.png" width="150"/> |
| `PICO8` | PICO-8 fantasy console style | <img src="assets/palette_PICO8.png" width="150"/> |

**Basic usage:**
```cpp
#include <graphics/Color.h>

void MyScene::init() {
    pr32::graphics::setPalette(pr32::graphics::PaletteType::NES);
}
```

**Dual-palette mode:**
```cpp
void MyScene::init() {
    pr32::graphics::enableDualPaletteMode(true);
    pr32::graphics::setDualPalette(
        pr32::graphics::PaletteType::NES,  // Background
        pr32::graphics::PaletteType::GB    // Sprites
    );
}
```

### Sprite System

Sprites are defined as compact 1bpp bitmaps by default:

- One `uint16_t` per row, each bit represents a pixel (`0` = transparent, `1` = active)
- `Renderer::drawSprite` draws a monochrome sprite using any palette color
- `Renderer::drawMultiSprite` composes multiple 1bpp layers to create multicolor sprites NES/GameBoy-style

**Example:**
```cpp
// Simple 1bpp sprite (16x16)
static const uint16_t PLAYER_SPRITE[16] = {
    0b0000111111000000,
    0b0011111111110000,
    // ... more rows
};

// Render
renderer.drawSprite(PLAYER_SPRITE, 16, 16, x, y, Color::WHITE);
```

### UI System

The UI system enables building interfaces without manual position calculations:

```cpp
// Create vertical layout
auto* layout = new UIVerticalLayout(10, 10, 220, 200);
layout->setSpacing(5);
layout->setPadding(10);

// Add elements
auto* title = new UILabel("MAIN MENU", Color::WHITE);
auto* btn1 = new UIButton("Play", []() { /* callback */ });
auto* btn2 = new UIButton("Options", []() { /* callback */ });

layout->addElement(title);
layout->addElement(btn1);
layout->addElement(btn2);

scene->addEntity(layout);
```

---

## 🎮 Examples

The repository [PixelRoot32-Game-Engine-Samples](https://github.com/Gperez88/PixelRoot32-Game-Engine-Samples) includes several complete examples:

### 🎯 Space Invaders
**Primary reference for the standard 1bpp sprite system**
- Full entity system (Player, Aliens, Bunkers, Projectiles)
- Swept-circle vs rectangle collisions
- Audio with SFX and music synchronized to tempo
- **Paleta:** NES

### 📷 CameraDemo
**Reference for camera, parallax, and platforms**
- Horizontal scrolling with a camera that follows the player
- Parallax effects with multiple layers
- Platform system with collisions
- **Paleta:** PR32 (Default)

### 🐍 Snake
**Reference for entity pooling and discrete game loop**
- Entity pooling with no dynamic allocations
- Grid-based movement with discrete timing
- Scoring and game over system
- **Paleta:** GB

### ⭕ Tic-Tac-Toe
**Reference for custom palette and basic AI**
- Uses custom palette (Neon/Cyberpunk)
- Basic AI with heuristics
- **Paleta:** Custom Neon

### 🎨 SpritesDemo
**Reference for 2bpp/4bpp sprites (EXPERIMENTAL)**
- Demonstrates 2bpp (4 colors) and 4bpp (16 colors) sprites
- Animations with sprite groups
- **Paleta:** GBC
- ⚠️ Requires experimental build flags

---

## 🔧 Build

### Native Environment (SDL2)

#### Windows (Recommended: MSYS2)

1. **Install MSYS2**: Download from [msys2.org](https://www.msys2.org/)
2. **Update package database:**
   ```bash
   pacman -Syu
   ```
3. **Install GCC and SDL2:**
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2
   ```
4. **Add to PATH**: Ensure `C:\msys64\mingw64\bin` is in your Windows PATH

#### Linux (Debian/Ubuntu)
```bash
sudo apt-get install libsdl2-dev
```

#### macOS (Homebrew)
```bash
brew install sdl2
```

### ESP32 Configuration

Configure `TFT_eSPI` in your `platformio.ini`:

**Example for ST7789 (240x240):**
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
build_flags =
    -D ST7789_DRIVER
    -D TFT_WIDTH=240
    -D TFT_HEIGHT=240
    -D TFT_MOSI=23
    -D TFT_SCLK=18
    -D TFT_DC=2
    -D TFT_RST=4
    -D TFT_CS=-1
    -D SPI_FREQUENCY=40000000
lib_deps =
    bodmer/TFT_eSPI@^2.5.43
```

### Experimental Build Flags

```ini
build_flags =
    -D PIXELROOT32_ENABLE_2BPP_SPRITES    # 2bpp sprites (4 colors)
    -D PIXELROOT32_ENABLE_4BPP_SPRITES    # 4bpp sprites (16 colors)
    -D PIXELROOT32_ENABLE_SCENE_ARENA     # Scene memory arena
```

> ⚠️ **Note:** These features are experimental and may cause performance issues or compilation errors on specific hardware.

---

## 🛠️ Asset Tools

### Sprite Compiler (`pr32-sprite-compiler`)

Python tool to convert standard PNG sprite sheets into C headers compatible with PixelRoot32.

**Repository:** [PixelRoot32-Sprite-Sheet-Compiler](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler)

**Features:**
- Automatic color detection and 1bpp layer generation
- Grid-based extraction
- CLI and GUI available

**Usage:**
```bash
python pr32-sprite-compiler.py my_sprites.png --grid 16x16 --out sprites.h
```

---

## 🗺️ Roadmap

### Planned Features

- 📟 **Driver: u8g2 Support** - Support for monochrome OLEDs (SSD1306, SH1106)
- 🎵 **Music Compiler** - Convert tracker formats (FTM/MML/MIDI) to C++ structures
- 🗺️ **Tilemap Compiler** - Import Tiled maps (.tmx) or JSON
- 🔊 **SFX Manager** - Fire-and-forget system with channel management
- 💾 **Persistence (Save/Load)** - Abstract key-value storage (NVS on ESP32)
- ⚡ **Spatial Partitioning** - Uniform Grid for collision optimization

### Completed Features ✅

- ✅ **Native Bitmap Font System** - Font system based on 1bpp sprites
- ✅ **UI Layout System** - Automatic layouts (Vertical, Horizontal, Grid, Panel, Anchor, Padding)

---

## 🤝 Contribute

Contributions are welcome! Please:

1. Read [CONTRIBUTING.md](CONTRIBUTING.md) and [STYLE_GUIDE.md](STYLE_GUIDE.md)
2. Fork the repository
3. Create a branch for your feature (`git checkout -b feature/AmazingFeature`)
4. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
5. Push the branch (`git push origin feature/AmazingFeature`)
6. Open a Pull Request

### Report Bugs

Use the [issue tracker](https://github.com/Gperez88/PixelRoot32-Game-Engine/issues) to report bugs. Include:
- Clear description of the problem
- Steps to reproduce
- Expected vs. observed behavior
- Platform information (ESP32/PC, version, etc.)

---

## 📄 License

PixelRoot32 is an **open-source** project licensed under the **MIT License**.

This project is based on and derived from [ESP32-Game-Engine](https://github.com/Gperez88/ESP32-Game-Engine), which is also licensed under the MIT License.

See the [LICENSE](LICENSE) file for the full license text.

---

## 👏 Credits

Developed by **Gabriel Perez** as a modular game engine for embedded systems.

Special thanks to **nbourre** for the original ESP32-Game-Engine.

---

## 🔗 Useful Links

- **Main Repository**: [PixelRoot32-Game-Engine](https://github.com/Gperez88/PixelRoot32-Game-Engine)
- **Examples and Samples**: [PixelRoot32-Game-Engine-Samples](https://github.com/Gperez88/PixelRoot32-Game-Engine-Samples)
- **Sprite Compiler**: [PixelRoot32-Sprite-Sheet-Compiler](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler)
- **Support**: [Ko-fi](https://ko-fi.com/gperez88) • [PayPal](https://www.paypal.com/ncp/payment/THC3PDSRQKZW6)

---

<p align="center">
  <em>Built with ❤️ for the retro-dev community.</em>
</p>
