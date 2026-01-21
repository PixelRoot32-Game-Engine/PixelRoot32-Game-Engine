<p align="center">
  <img src="assets/pr32_logo.png" alt="PixelRoot32 Logo" width="300"/>
</p>

# PixelRoot32 Game Engine
[![Support me on Ko-fi](https://img.shields.io/badge/Support%20me%20on%20Ko--fi-29ABE0?style=for-the-badge&logo=ko-fi&logoColor=ffffff)](https://ko-fi.com/gperez88)
[![Support me on PayPal](https://img.shields.io/badge/Support%20me%20on%20PayPal-003087?style=for-the-badge&logo=paypal&logoColor=white)](https://www.paypal.com/ncp/payment/THC3PDSRQKZW6)



PixelRoot32 is a lightweight, modular 2D game engine written in C++ and designed specifically for **ESP32 microcontrollers**, with a native simulation layer for **PC (SDL2)**.

> **Status:** PixelRoot32 is under active development. APIs may change and some subsystems are still experimental. Expect occasional issues or breaking changes, especially on less-tested configurations; feedback and bug reports are welcome.

The engine adopts a simple scene-based architecture inspired by **Godot Engine**, making it intuitive for developers familiar with modern game development workflows.

---

## 💡Origin and Inspirations

PixelRoot32 is an evolution of [ESP32-Game-Engine](https://github.com/nbourre/ESP32-Game-Engine) by **nbourre**, extended with architectural concepts from **Godot Engine**.

Special thanks to **nbourre** for open-sourcing the original engine and inspiring this project. Without that work, PixelRoot32 would not exist.

## 🎬 Demo in Action

Watch PixelRoot32 running on ESP32 with example games:

[![PixelRoot32 Demo](https://img.youtube.com/vi/55_Jwkx-gPs/0.jpg)](https://www.youtube.com/shorts/55_Jwkx-gPs)

> Click the image to watch the full demo on YouTube.  

## 🚀 Key Features

- **Scene & Entity System**: Scenes managing Entities, Actors, PhysicsActors and UI elements.
- **Cross-Platform**: Develop on PC (Windows/Linux via **SDL2**) and deploy to ESP32 using **TFT_eSPI** (ST7735/ILI9341 via SPI/DMA).
- **NES-Style Audio**: Integrated audio subsystem with 2 Pulse, 1 Triangle, and 1 Noise channels.
- **Color Palette**: Fixed indexed palette (24 visible colors + Transparent) using RGB565 for fast rendering.
- **Sprite System**: 1bpp monochrome sprites with support for layered, multi-color sprites built from multiple 1bpp layers, plus optional 2bpp/4bpp packed sprites for richer visuals.
- **Sprite Animation**: Lightweight, step-based sprite animation that works with both simple sprites and layered `MultiSprite`, without coupling animation logic to rendering.
- **Render Layers & Tilemaps**: Simple logical render layers (background, gameplay, UI) and a compact 1bpp tilemap helper for backgrounds and side-scrolling levels, designed to stay friendly to ESP32 RAM/CPU limits.
- **2D Camera & Scrolling**: Dead-zone camera (`Camera2D`) that follows a target horizontally (and optionally vertically) by driving `Renderer::setDisplayOffset`, enabling parallax backgrounds and long platformer levels.
- **Physics & Collision**: AABB collision detection, gravity, and basic kinematics suitable for arcade games and simple platformers.
- **Particle & Object Pooling**: High-performance, memory-pooled particles and reusable gameplay entities (projectiles, snake segments, etc.) designed to avoid allocations inside the game loop on ESP32.
- **UI System**: Lightweight UI controls (Label, Button).

## 🛠 Target Platforms

1. **ESP32**: Currently supports **TFT_eSPI** library for hardware-accelerated rendering.
2. **Desktop (Native)**: Uses **SDL2** for rapid development, debugging, and testing.

> **Note:** Future support for **u8g2** library on embedded platforms is planned.

## 📚 Documentation

Detailed documentation for engine subsystems and coding standards:

- **[API Reference](API_REFERENCE.md)**: Core classes and usage examples.
- **[Audio Subsystem](AUDIO_NES_SUBSYSTEM_REFERENCE.md)**: Architecture of the NES-like sound engine.
- **[Style Guide](STYLE_GUIDE.md)**: Coding conventions and best practices.

### Color Palette & Sprites

PixelRoot32 uses a fixed indexed color palette optimized for embedded hardware:

- Colors are represented as 8-bit indices.
- Internally resolved to RGB565.
- Improves performance and memory usage.
- Ensures visual consistency across games.

The engine provides a built-in palette of 16 colors (plus transparent) via the
`pixelroot32::graphics::Color` enum.

### Color Palette Selection

The engine supports multiple pre-defined color palettes. Developers can select **one active palette** at a time for their game.

**Available Palettes:**

| Palette | Description | Preview |
| :--- | :--- | :--- |
| `PR32` (Default) | The standard PixelRoot32 palette | <img src="assets/palette_PR32.png" width="150"/> |
| `NES` | Nintendo Entertainment System style | <img src="assets/palette_NES.png" width="150"/> |
| `GB` | GameBoy (Greyscale/Green) style | <img src="assets/palette_GB.png" width="150"/> |
| `GBC` | GameBoy Color style | <img src="assets/palette_GBC.png" width="150"/> |
| `PICO8` | PICO-8 fantasy console style | <img src="assets/palette_PICO8.png" width="150"/> |

> **Note:** You can import these images into your pixel art editor (Aseprite, Photoshop, etc.) to ensure your assets use the correct colors.

**How to Select a Palette:**

Call `pr32::graphics::setPalette(...)` in your scene's `init()` method.

```cpp
#include <graphics/Color.h>

void MyScene::init() {
    // Select the GameBoy palette
    pr32::graphics::setPalette(pr32::graphics::PaletteType::GB);
    
    // ... rest of initialization
}
```

> **Note:** Only one palette can be active globally. When switching scenes, it is good practice to explicitly set the desired palette in `init()`.

### Custom Color Palette

You can also define your own custom palette. This is useful for giving your game a unique look while respecting the 16-color limit.

**How to define and use a custom palette:**

1. Define a `static const` array of 16 `uint16_t` values (RGB565 format).
2. Pass it to `pr32::graphics::setCustomPalette(...)`.

```cpp
#include <graphics/Color.h>

// Define your custom 16-color palette (RGB565)
// Values must be ordered to match the Color enum indices (Black=0, White=1, etc.)
// if you want to keep compatibility with standard Color names.
static const uint16_t MY_SEPIA_PALETTE[16] = {
    0x0000, // 0: Black
    0xE79C, // 1: White (Sepia tone)
    0x3186, // 2: Navy
    0x52AA, // 3: Blue
    // ... define all 16 colors ...
    0xCE79  // 15: Gray
};

void MyScene::init() {
    // Apply the custom palette
    pr32::graphics::setCustomPalette(MY_SEPIA_PALETTE);
}
```

> **Warning:** The array passed to `setCustomPalette` **must remain valid** while it is active. Always use `static const` arrays or global variables. Do not pass a local stack array.

### Technical Implementation

The palette system is designed for **zero-overhead switching**, critical for the limited resources of the ESP32:

1. **Flash Storage**: All palettes are stored as `static constexpr` arrays in flash memory (RODATA), consuming no dynamic RAM.
2. **Pointer-Based Switching**: The engine maintains a single global pointer `currentPalette` that points to the active array. Calling `setPalette()` merely updates this pointer, making the operation instantaneous (**O(1)**).
3. **Dynamic Resolution**: The `resolveColor(Color c)` function uses the enum value as a direct index into the array referenced by `currentPalette`. This ensures that all rendering calls automatically use the new colors without needing to redraw or reload assets.

Sprites are defined as compact 1bpp bitmaps by default:

- One `uint16_t` per row, each bit representing a pixel (`0` = transparent, `1` = on).
- Bit 0 is the leftmost pixel, bit (`width - 1`) the rightmost pixel.
- `Renderer::drawSprite` draws a single-color sprite using any palette `Color`.
- `Renderer::drawMultiSprite` composes multiple 1bpp layers (each with its own color) to build multi-color, NES/GameBoy-style sprites without changing the underlying format.
  > **Performance Note:** While `MultiSprite` allows many layers, it is recommended to keep the layer count between **2 and 4** for optimal performance on ESP32, as each layer incurs a separate drawing pass.
- Optional 2bpp/4bpp packed formats can be enabled at compile time for higher fidelity assets while keeping 1bpp as the default path for ESP32-friendly games.

## 🎨 Asset Tools

### Sprite Compiler (`pr32-sprite-compiler`)

A Python tool is available to convert standard PNG sprite sheets into PixelRoot32-compatible C headers (`.h`). The source code and detailed usage instructions are available in the [PixelRoot32-Sprite-Sheet-Compiler](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler) repository.

- **Input**: PNG sprite sheet (RGB/RGBA).
- **Output**: C header with `uint16_t` arrays for each color layer (1bpp per layer).
- **Features**:
  - Automatically detects colors and generates a 1bpp layer for each distinct color.
  - Supports grid-based sprite extraction.
  - Includes both a **Command Line Interface (CLI)** and a **GUI** for ease of use.

**Usage Example:**

```bash
# Convert a sprite sheet with 16x16 sprites
python pr32-sprite-compiler.py my_sprites.png --grid 16x16 --out sprites.h
```

This tool simplifies the process of creating multi-layer 1bpp sprites from modern image editors.

## 📁 Project Structure

Main structure of the `PixelRoot32-Game-Engine` library:

```txt
PixelRoot32-Game-Engine/
├── assets/                 # Icons and logos
├── include/                # Public engine headers
│   ├── audio/
│   ├── core/
│   ├── drivers/
│   │   ├── esp32/
│   │   └── native/
│   ├── graphics/
│   │   ├── particles/
│   │   └── ui/
│   ├── input/
│   ├── math/
│   └── physics/
├── src/                    # Engine implementations
│   ├── audio/
│   ├── core/
│   ├── drivers/
│   │   ├── esp32/
│   │   └── native/
│   ├── graphics/
│   │   ├── particles/
│   │   └── ui/
│   ├── input/
│   ├── physics/
│   └── platforms/
│       └── mock/
├── test/
├── API_REFERENCE.md
├── AUDIO_NES_SUBSYSTEM_REFERENCE.md
├── STYLE_GUIDE.md
├── library.json
└── library.properties
```

## 🗺️ Roadmap

The following features are planned to enhance the engine's capabilities, focusing on workflow efficiency and ESP32 optimization.

### 1. 📟 Driver: u8g2 Support

- **Goal**: Add support for the **u8g2** graphics library.
- **Why**: Expands hardware support to monochrome OLEDs (SSD1306, SH1106) and other displays not covered by TFT_eSPI, making the engine more versatile for low-power devices.

### 2. 🎵 Tooling: Music Compiler (`pr32-music-compiler`)

- **Goal**: Convert standard tracker formats (FTM/MML/MIDI) into `MusicNote` C++ structures.
- **Why**: Manual music coding is inefficient. Enables complex chiptune soundtracks stored in Flash.

### 3. 🗺️ Tooling: Tilemap Compiler (`pr32-map-compiler`)

- **Goal**: Import Tiled (.tmx) or JSON maps into compressed `TileMap` structures.
- **Why**: Reduces RAM usage compared to Entity-based levels and streamlines level design.

### 4. 🅰️ Engine: Native Bitmap Font System

- **Goal**: Implement a platform-agnostic 1bpp sprite-based text renderer.
- **Why**: Ensures pixel-perfect consistency between PC (SDL2) and ESP32, removing dependency on external font libraries.

### 5. 🔊 Engine: SFX Manager

- **Goal**: "Fire-and-forget" sound effect system with channel management (priorities, virtual channels).
- **Why**: Automates hardware channel allocation (Pulse/Triangle/Noise) for game events.

### 6. 💾 Core: Persistence (Save/Load)

- **Goal**: Abstract Key-Value storage (NVS on ESP32, File on PC).
- **Why**: Standardizes saving high scores and progress across platforms.

### 7. ⚡ Engine: Spatial Partitioning

- **Goal**: Implement a Uniform Grid for collision detection.
- **Why**: Optimizes collision checks from O(N²) to O(N), allowing more active entities on ESP32 (240MHz).

## 📜 Changelog

### v0.1.0-dev (Pre-release)

- **Initial Public Preview.**
- **Core Architecture**: Scene, Entity, Actor, and PhysicsActor system.
- **Rendering**: 1bpp Sprites, MultiSprite (layered colors), and Tilemap support.
- **Audio**: NES-style sound engine (Pulse, Triangle, Noise channels).
- **Physics**: AABB Collision detection and basic kinematics.
- **Platform Support**: ESP32 (SPI/DMA) and PC (SDL2) targets.
- **Tools**: Added Sprite Compiler python tool.
- **Experimental Build Flags**:
  - `PIXELROOT32_ENABLE_2BPP_SPRITES`: Enables support for 2bpp (4-color) packed sprites.
  - `PIXELROOT32_ENABLE_4BPP_SPRITES`: Enables support for 4bpp (up to 16-color) packed sprites, intended for high-fidelity UI elements or special effects where more colors per sprite are needed.
  - `PIXELROOT32_ENABLE_SCENE_ARENA`: Enables dedicated memory arena for scene management.

## 📦 Getting Started

### Setting up Native Environment (SDL2)

To run the engine on your PC (Native mode), you need **SDL2** installed.

> **Note:** The officially tested native platform is **Windows**. Linux and macOS should work in theory but are currently experimental.

#### 🖥️ Windows (Recommended: MSYS2)

We strongly recommend using **MSYS2** for a stable and easy setup.

1. **Install MSYS2**: Download and install from [msys2.org](https://www.msys2.org/).
2. **Update Package Database**: Open the MSYS2 terminal (UCRT64 or MINGW64) and run:
   ```bash
   pacman -Syu
   ```
3. **Install GCC and SDL2**:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2
   ```
4. **Add to PATH**: Ensure your MSYS2 `bin` folder (e.g., `C:\msys64\mingw64\bin`) is in your Windows System PATH.

#### 🐧 Linux (Debian/Ubuntu)

```bash
sudo apt-get install libsdl2-dev
```

#### 🍎 macOS (Homebrew)

```bash
brew install sdl2
```

### Using this example repository

1. Clone this repository [PixelRoot32-Game-Engine-Samples](https://github.com/Gperez88/PixelRoot32-Game-Engine-Samples).
2. Open it in **PlatformIO** (VS Code).
3. Select the environment (`esp32` or `native`).
4. Build and run the **GeometryJump** example to see the engine in action.

### Create your own project using PixelRoot32 as a library

1. Create a new PlatformIO project for your ESP32.
2. Copy the `PixelRoot32-Game-Engine` folder into your project's `lib/` directory  
    (or add it as a Git submodule in `lib/PixelRoot32-Game-Engine`).
3. Create a `src/drivers` folder in your project and add your `DrawSurface`
    implementations there, for example:
    - `src/drivers/esp32/TFT_eSPI_Drawer.cpp` for TFT_eSPI displays.
    - `src/drivers/native/SDL2_Drawer.cpp` for the native PC mode.
4. In your `src/main.cpp`, include the engine and configure the drivers, similar to:

```cpp
#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <drivers/esp32/ESP32_I2S_AudioBackend.h>
#include <core/Engine.h>

namespace pr32 = pixelroot32;

pr32::drivers::esp32::TFT_eSPI_Drawer drawer;
pr32::drivers::esp32::ESP32_I2S_AudioBackend audioBackend(26, 25, 22, 22050);
pr32::graphics::DisplayConfig displayConfig(&drawer, 0, 240, 240);
pr32::input::InputConfig inputConfig(5, 13, 12, 14, 32, 33);
pr32::audio::AudioConfig audioConfig(&audioBackend, 22050);
pr32::core::Engine engine(displayConfig, inputConfig, audioConfig);

void setup() {
    engine.init();
    // engine.setScene(&yourScene);
}

void loop() {
    engine.run();
}
```

1. Create your own scenes by inheriting from `pixelroot32::core::Scene` and
    actors by inheriting from `pixelroot32::core::Actor` or `PhysicsActor`, and
    assign them with `engine.setScene(...)` in `setup()`.

---

## License

PixelRoot32 is an **open-source** project.

- Source files derived from *ESP32-Game-Engine* are licensed under the **MIT License**.
- Modified versions of those files remain under the **MIT License**.
- New source files authored by **Gabriel Perez (2026)** are licensed under the **GNU GPL v3**.

See individual source files for license details.

---

## Credits

Developed by **Gabriel Perez** as a modular **game engine for embedded systems**.  
Special thanks to **nbourre** for the original ESP32-Game-Engine.

---
*Built with ❤️ for the retro-dev community.*
