<p align="center">
  <img src="assets/pr32_logo.png" alt="PixelRoot32 Logo" width="300"/>
</p>

# PixelRoot32 Game Engine

[![Support me on Ko-fi](https://img.shields.io/badge/Support%20me%20on%20Ko--fi-29ABE0?style=for-the-badge&logo=ko-fi&logoColor=ffffff)](https://ko-fi.com/gperez88)

PixelRoot32 is a lightweight, modular 2D game engine written in C++ and designed specifically for **ESP32 microcontrollers**, with a native simulation layer for **PC (SDL2)**.

> **Status:** PixelRoot32 is under active development. APIs may change and some subsystems are still experimental. Expect occasional issues or breaking changes, especially on less-tested configurations; feedback and bug reports are welcome.

The engine adopts a simple scene-based architecture inspired by **Godot Engine**, making it intuitive for developers familiar with modern game development workflows.

## 📑 Table of Contents

- [Origin and Inspirations](#origin-and-inspirations)
- [Demo in Action](#demo-in-action)
- [Key Features](#key-features)
- [Target Platforms](#target-platforms)
- [Documentation](#documentation)
- [Asset Tools](#asset-tools)
- [Project Structure](#project-structure)
- [Roadmap](#roadmap)
- [Changelog](#changelog)
- [Getting Started](#getting-started)
- [License](#license)
- [Credits](#credits)

---

## Origin and Inspirations

PixelRoot32 is an evolution of [ESP32-Game-Engine](https://github.com/nbourre/ESP32-Game-Engine) by **nbourre**, extended with architectural concepts from **Godot Engine**.

Special thanks to **nbourre** for open-sourcing the original engine and inspiring this project. Without that work, PixelRoot32 would not exist.

## Demo in Action

Watch PixelRoot32 running on ESP32 with example games:

[![PixelRoot32 Demo](https://img.youtube.com/vi/55_Jwkx-gPs/0.jpg)](https://www.youtube.com/shorts/55_Jwkx-gPs)

> Click the image to watch the full demo on YouTube.  

## Key Features

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
- **UI System**: Lightweight UI controls (Label, Button) with automatic layout management. Includes `UIVerticalLayout`, `UIHorizontalLayout`, `UIGridLayout`, `UIPaddingContainer`, `UIPanel`, and `UIAnchorLayout` for organizing elements, adding spacing, creating visual containers, and positioning HUD elements, eliminating manual position calculations.
- **Native Bitmap Font System**: Platform-agnostic 1bpp sprite-based text renderer with built-in 5x7 font, ensuring pixel-perfect consistency between PC and ESP32.

## Target Platforms

1. **ESP32**: Currently supports **TFT_eSPI** library for hardware-accelerated rendering.
2. **Desktop (Native)**: Uses **SDL2** for rapid development, debugging, and testing.

> **Note:** Future support for **u8g2** library on embedded platforms is planned.

## Documentation

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

The engine supports two modes:
- **Legacy mode (default)**: Single global palette for all rendering
- **Dual palette mode**: Separate palettes for backgrounds and sprites

Developers can select **one active palette** at a time in legacy mode, or use **two separate palettes** in dual palette mode.

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

> **Note:** In legacy mode, only one palette is active globally. When switching scenes, it is good practice to explicitly set the desired palette in `init()`.

### Dual Palette Mode

The engine supports **dual palette mode**, allowing you to use different palettes for backgrounds and sprites simultaneously. This is useful for creating visual contrast between game layers.

**How to use dual palette mode:**

```cpp
#include <graphics/Color.h>

void MyScene::init() {
    // Enable dual palette mode
    pr32::graphics::enableDualPaletteMode(true);
    
    // Set different palettes for backgrounds and sprites
    pr32::graphics::setBackgroundPalette(pr32::graphics::PaletteType::NES);
    pr32::graphics::setSpritePalette(pr32::graphics::PaletteType::GB);
    
    // Or use the convenience function:
    // pr32::graphics::setDualPalette(
    //     pr32::graphics::PaletteType::NES,  // Background palette
    //     pr32::graphics::PaletteType::GB    // Sprite palette
    // );
}
```

**How it works:**
- **Layer 0 (Background)**: Automatically uses the background palette for all drawing operations
- **Layer 1+ (Sprites/Gameplay)**: Automatically uses the sprite palette for all drawing operations
- **Tilemaps**: Always use the background palette
- **Sprites**: Always use the sprite palette

**Custom palettes in dual mode:**

```cpp
static const uint16_t BG_PALETTE[16] = { /* ... */ };
static const uint16_t SPRITE_PALETTE[16] = { /* ... */ };

void MyScene::init() {
    pr32::graphics::enableDualPaletteMode(true);
    pr32::graphics::setBackgroundCustomPalette(BG_PALETTE);
    pr32::graphics::setSpriteCustomPalette(SPRITE_PALETTE);
    
    // Or use the convenience function:
    // pr32::graphics::setDualCustomPalette(BG_PALETTE, SPRITE_PALETTE);
}
```

> **Note:** Dual palette mode is opt-in. Legacy code using `setPalette()` continues to work unchanged, maintaining 100% backward compatibility.

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
2. **Pointer-Based Switching**: The engine maintains global pointers that point to the active arrays. Calling palette functions merely updates these pointers, making the operation instantaneous (**O(1)**).
3. **Dynamic Resolution**: The `resolveColor(Color c)` and `resolveColor(Color c, PaletteContext ctx)` functions use the enum value as a direct index into the appropriate palette array. This ensures that all rendering calls automatically use the correct colors without needing to redraw or reload assets.
4. **Dual Palette Mode**: When enabled, the engine maintains separate pointers for background and sprite palettes. The renderer automatically selects the correct palette based on the current render layer (0 = background, 1+ = sprite).
5. **Performance**: Dual palette mode adds minimal overhead (< 0.1%): only 9 bytes of RAM (2 pointers + 1 bool) and a single branch check that is highly predictable by the CPU.

Sprites are defined as compact 1bpp bitmaps by default:

- One `uint16_t` per row, each bit representing a pixel (`0` = transparent, `1` = on).
- Bit 0 is the leftmost pixel, bit (`width - 1`) the rightmost pixel.
- `Renderer::drawSprite` draws a single-color sprite using any palette `Color`.
- `Renderer::drawMultiSprite` composes multiple 1bpp layers (each with its own color) to build multi-color, NES/GameBoy-style sprites without changing the underlying format.
  > **Performance Note:** While `MultiSprite` allows many layers, it is recommended to keep the layer count between **2 and 4** for optimal performance on ESP32, as each layer incurs a separate drawing pass.
- Optional 2bpp/4bpp packed formats can be enabled at compile time for higher fidelity assets while keeping 1bpp as the default path for ESP32-friendly games.

## Asset Tools

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

## Project Structure

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

## Roadmap

The following features are planned to enhance the engine's capabilities, focusing on workflow efficiency and ESP32 optimization.

### Planned Features

#### 1. 📟 Driver: u8g2 Support

- **Goal**: Add support for the **u8g2** graphics library.
- **Why**: Expands hardware support to monochrome OLEDs (SSD1306, SH1106) and other displays not covered by TFT_eSPI, making the engine more versatile for low-power devices.

#### 2. 🎵 Tooling: Music Compiler (`pr32-music-compiler`)

- **Goal**: Convert standard tracker formats (FTM/MML/MIDI) into `MusicNote` C++ structures.
- **Why**: Manual music coding is inefficient. Enables complex chiptune soundtracks stored in Flash.

#### 3. 🗺️ Tooling: Tilemap Compiler (`pr32-map-compiler`)

- **Goal**: Import Tiled (.tmx) or JSON maps into compressed `TileMap` structures.
- **Why**: Reduces RAM usage compared to Entity-based levels and streamlines level design.

#### 4. 🔊 Engine: SFX Manager

- **Goal**: "Fire-and-forget" sound effect system with channel management (priorities, virtual channels).
- **Why**: Automates hardware channel allocation (Pulse/Triangle/Noise) for game events.

#### 5. 💾 Core: Persistence (Save/Load)

- **Goal**: Abstract Key-Value storage (NVS on ESP32, File on PC).
- **Why**: Standardizes saving high scores and progress across platforms.

#### 6. ⚡ Engine: Spatial Partitioning

- **Goal**: Implement a Uniform Grid for collision detection.
- **Why**: Optimizes collision checks from O(N²) to O(N), allowing more active entities on ESP32 (240MHz).

---

### Completed Features

#### ✅ 🅰️ Engine: Native Bitmap Font System

- **Goal**: Implement a platform-agnostic 1bpp sprite-based text renderer.
- **Why**: Ensures pixel-perfect consistency between PC (SDL2) and ESP32, removing dependency on external font libraries.
- **Status**: ✅ **Implemented and integrated**. The engine now uses a native bitmap font system with a built-in 5x7 font. All text rendering is handled through `Renderer::drawText()` using sprite-based glyphs, ensuring pixel-perfect consistency across platforms.

#### ✅ 📐 Engine: UI Layout System (Partial)

- **Goal**: Implement automatic layout management for UI elements (Vertical Layout, Horizontal Layout, Grid Layout).
- **Why**: Simplifies UI creation by eliminating manual position calculations and enables handling of long lists that exceed screen size.
- **Status**: ✅ **All basic layouts implemented**. `UIVerticalLayout` organizes elements vertically with automatic scroll support (NES-style instant scroll). `UIHorizontalLayout` organizes elements horizontally with the same features. `UIGridLayout` organizes elements in a matrix with 4-direction navigation (UP/DOWN/LEFT/RIGHT). All layouts support viewport culling, optimized rendering, and automatic navigation.

## Changelog

### v0.2.0-dev

- **Documentation Overhaul**: Added comprehensive Table of Contents, step-by-step SDL2 installation guide for Windows (MSYS2), and critical PlatformIO installation notes.
- **Architecture**: Moved `DrawSurface` implementation handling to the engine core. This removes the need for manual developer implementation and facilitates the integration of future display drivers.
- **Driver Support**: Clarified driver support status (TFT_eSPI & SDL2) and roadmap.
- **Native Bitmap Font System**: Implemented platform-agnostic 1bpp sprite-based text rendering system. Added `Font`, `FontManager`, and built-in `FONT_5X7` (5x7 pixel font with 95 ASCII characters). All text rendering now uses the native font system, ensuring pixel-perfect consistency between PC (SDL2) and ESP32. The system is fully integrated with `Renderer::drawText()` and `Renderer::drawTextCentered()`, maintaining 100% backward compatibility with existing code.
- **UI Layout System**: Implemented `UIVerticalLayout`, `UIHorizontalLayout`, and `UIGridLayout` for automatic organization of UI elements. `UIVerticalLayout` organizes elements vertically (UP/DOWN navigation) with scroll support. `UIHorizontalLayout` organizes elements horizontally (LEFT/RIGHT navigation) with scroll support. `UIGridLayout` organizes elements in a matrix (4-direction navigation with wrapping). All layouts feature NES-style instant navigation, automatic viewport culling for performance, optimized rendering, selection management, and automatic button styling.

### v0.1.0-dev (Release)

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

## Getting Started

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
2. **Install the library**:

   > **⚠️ CRITICAL INSTALLATION NOTE**
   >
   > When installing via **PlatformIO (`lib_deps`)**, you **must** use the exact version string below.
   > **Do NOT use the caret (`^`) symbol** or fuzzy versioning, as it may pull an incorrect or incompatible version.
   >
   > ```ini
   > lib_deps =
   >     gperez88/PixelRoot32-Game-Engine@0.2.0-dev
   > ```

   Alternatively, you can manually copy the `PixelRoot32-Game-Engine` folder into your project's `lib/` directory (or use a Git submodule).

3. **Configure TFT_eSPI (ESP32 only)**:

   If you are using the ESP32 platform, you need to configure the `TFT_eSPI` driver in your `platformio.ini` file. Add the corresponding build flags to your `[env:esp32dev]` section to define your display pins and settings.

   Here are two **tested configurations** that are known to work well:

   **Option A: ST7789 (240x240)**

   ```ini
   build_flags =
       -D ST7789_DRIVER
       -D TFT_WIDTH=240
       -D TFT_HEIGHT=240
       -D TFT_MOSI=23
       -D TFT_SCLK=18
       -D TFT_DC=2
       -D TFT_RST=4
       -D TFT_CS=-1
       -D LOAD_GLCD
       -D LOAD_FONT2
       -D LOAD_FONT4
       -D LOAD_FONT6
       -D LOAD_FONT7
       -D LOAD_FONT8
       -D LOAD_GFXFF
       -D SMOOTH_FONT
       -D SPI_FREQUENCY=40000000
       -D SPI_READ_FREQUENCY=20000000
   ```

   **Option B: ST7735 (128x128)**

   ```ini
   build_flags =
       -D ST7735_DRIVER
       -D ST7735_GREENTAB3
       -D TFT_WIDTH=128
       -D TFT_HEIGHT=128
       -D TFT_MOSI=23
       -D TFT_SCLK=18
       -D TFT_DC=2
       -D TFT_RST=4
       -D TFT_CS=-1
       -D LOAD_GLCD
       -D LOAD_FONT2
       -D LOAD_FONT4
       -D LOAD_FONT6
       -D LOAD_FONT7
       -D LOAD_FONT8
       -D LOAD_GFXFF
       -D SMOOTH_FONT
       -D SPI_FREQUENCY=27000000
       -D SPI_READ_FREQUENCY=20000000
   ```

4. In your `src/main.cpp` (ESP32) or `src/main_native.cpp` (Native), include the engine and configure the drivers. The `DisplayConfig` now handles the driver instantiation automatically.

   **ESP32 Example (`src/main.cpp`):**

   ```cpp
   #include <Arduino.h>
   #include <core/Engine.h>
   #include <drivers/esp32/ESP32_DAC_AudioBackend.h> // Or ESP32_I2S_AudioBackend.h

   namespace pr32 = pixelroot32;

   // 1. Audio Setup (DAC Example)
   const int DAC_PIN = 25;
   pr32::drivers::esp32::ESP32_DAC_AudioBackend audioBackend(DAC_PIN, 11025);

   // 2. Display Setup (TFT_eSPI is automatically selected by the driver enum)
   // 0 = Rotation, 240x240 = Resolution
   pr32::graphics::DisplayConfig displayConfig(pr32::graphics::DisplayDriver::TFT_eSPI, 0, 240, 240);

   // 3. Input Setup
   // count, UP, DOWN, LEFT, RIGHT, A, B
   pr32::input::InputConfig inputConfig(6, 32, 27, 33, 14, 13, 12);

   // 4. Engine Initialization
   pr32::audio::AudioConfig audioConfig(&audioBackend, audioBackend.getSampleRate());
   pr32::core::Engine engine(displayConfig, inputConfig, audioConfig);

   void setup() {
       engine.init();
       // engine.setScene(&yourScene);
   }

   void loop() {
       engine.run();
   }
   ```

   **Native PC Example (`src/main_native.cpp`):**

   ```cpp
   #define SDL_MAIN_HANDLED
   #include <SDL2/SDL.h>
   #include <core/Engine.h>
   #include <drivers/native/SDL2_AudioBackend.h>

   namespace pr32 = pixelroot32;

   // 1. Audio Setup
   pr32::drivers::native::SDL2_AudioBackend audioBackend(22050, 1024);

   // 2. Display Setup (NONE type defaults to SDL2 on Native platform)
   pr32::graphics::DisplayConfig displayConfig(pr32::graphics::DisplayType::NONE, 0, 240, 240);

   // 3. Input Setup (SDL Scancodes)
   pr32::input::InputConfig inputConfig(6, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE, SDL_SCANCODE_RETURN);

   // 4. Engine Initialization
   pr32::audio::AudioConfig audioConfig(&audioBackend, 22050);
   pr32::core::Engine engine(displayConfig, inputConfig, audioConfig);

   int main(int argc, char* argv[]) {
       engine.init();
       // engine.setScene(&yourScene);
       engine.run();
       return 0;
   }
   ```

5. Create your own scenes by inheriting from `pixelroot32::core::Scene` and
    actors by inheriting from `pixelroot32::core::Actor` or `PhysicsActor`, and
    assign them with `engine.setScene(...)` in `setup()` or `main()`.

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
