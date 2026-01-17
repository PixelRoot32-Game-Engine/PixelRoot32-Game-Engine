# PixelRoot32 Game Engine

PixelRoot32 is a lightweight, modular 2D game engine written in C++ and designed specifically for **ESP32 microcontrollers**, with a native simulation layer for **PC (SDL2)**.

The engine adopts a simple scene-based architecture inspired by **Godot Engine**, making it intuitive for developers familiar with modern game development workflows.

---

## 💡Origin and Inspirations

PixelRoot32 is an evolution of [ESP32-Game-Engine](https://github.com/nbourre/ESP32-Game-Engine) by **nbourre**, extended with architectural concepts from **Godot Engine**.

Special thanks to **nbourre** for open-sourcing the original engine and inspiring this project. Without that work, PixelRoot32 would not exist.

## 🚀 Key Features

- **Scene & Entity System**: Scenes managing Entities, Actors, PhysicsActors and UI elements.
- **Cross-Platform**: Develop on PC (Windows/Linux via SDL2) and deploy to ESP32 (ST7735/ILI9341 via SPI/DMA).
- **NES-Style Audio**: Integrated audio subsystem with 2 Pulse, 1 Triangle, and 1 Noise channels.
- **Color Palette**: Fixed indexed palette (32 colors) using RGB565 for fast rendering.
- **Sprite System**: 1bpp monochrome sprites with support for layered, multi-color sprites built from multiple 1bpp layers.
- **Physics & Collision**: AABB collision detection, gravity, and basic kinematics.
- **Particle System**: High-performance, memory-pooled particle effects.
- **UI System**: Lightweight UI controls (Label, Button).

## 🛠 Target Platforms

1.  **ESP32**: Optimized for embedded constraints (limited RAM, DMA transfer).
2.  **Desktop (Native)**: Uses SDL2 for rapid development, debugging, and testing.

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

The engine provides a built-in palette of 32 colors via the
`pixelroot32::graphics::Color` enum.

Sprites are defined as compact 1bpp bitmaps:

- One `uint16_t` per row, each bit representing a pixel (`0` = transparent, `1` = on).
- Bit 0 is the leftmost pixel, bit (`width - 1`) the rightmost pixel.
- `Renderer::drawSprite` draws a single-color sprite using any palette `Color`.
- `Renderer::drawMultiSprite` composes multiple 1bpp layers (each with its own color) to build multi-color, NES/GameBoy-style sprites without changing the underlying format.

## 📁 Project Structure

Main structure of the `PixelRoot32-Game-Engine` library:

```txt
PixelRoot32-Game-Engine/
├── assets/                 # Icons and logos
├── examples/               # Example games
│   ├── Pong/
│   ├── GeometryJump/
│   ├── BrickBreaker/
│   └── TicTacToe/
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
├── library.json
└── library.properties
```

## 📦 Getting Started

### Using this example repository

1.  Clone this repository.
2.  Open it in **PlatformIO** (VS Code).
3.  Select the environment (`esp32` or `native`).
4.  Build and run the **GeometryJump** example to see the engine in action.

### Create your own project using PixelRoot32 as a library

1.  Create a new PlatformIO project for your ESP32.
2.  Copy the `PixelRoot32-Game-Engine` folder into your project's `lib/` directory  
    (or add it as a Git submodule in `lib/PixelRoot32-Game-Engine`).
3.  Create a `src/drivers` folder in your project and add your `DrawSurface`
    implementations there, for example:
    - `src/drivers/esp32/TFT_eSPI_Drawer.cpp` for TFT_eSPI displays.
    - `src/drivers/native/SDL2_Drawer.cpp` for the native PC mode.
4.  In your `src/main.cpp`, include the engine and configure the drivers, similar to:

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

5.  Create your own scenes by inheriting from `pixelroot32::core::Scene` and
    actors by inheriting from `pixelroot32::core::Actor` or `PhysicsActor`, and
    assign them with `engine.setScene(...)` in `setup()`.

Reference project: [PixelRoot32-Game-Engine-Samples](https://github.com/Gperez88/PixelRoot32-Game-Engine-Samples)

---
*Built with ❤️ for the retro-dev community.*
