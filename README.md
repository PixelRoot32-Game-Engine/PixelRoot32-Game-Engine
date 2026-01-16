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

### Color Palette

PixelRoot32 uses a fixed indexed color palette optimized for embedded hardware:

- Colors are represented as 8-bit indices.
- Internally resolved to RGB565.
- Improves performance and memory usage.
- Ensures visual consistency across games.

The engine provides a built-in palette of 32 colors via the
`pixelroot32::graphics::Color` enum.

## 📁 Project Structure

Estructura principal de la librería `PixelRoot32-Game-Engine`:

```txt
PixelRoot32-Game-Engine/
├── assets/                 # Iconos y logos
├── examples/               # Juegos de ejemplo
│   ├── Pong/
│   ├── GeometryJump/
│   ├── BrickBreaker/
│   └── TicTacToe/
├── include/                # Headers públicos del engine
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
├── src/                    # Implementaciones del engine
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

### Usar este repositorio de ejemplo

1.  Clona este repositorio.
2.  Ábrelo en **PlatformIO** (VS Code).
3.  Selecciona el entorno (`esp32` o `native`).
4.  Compila y ejecuta el ejemplo **GeometryJump** para ver el engine en acción.

### Crear tu propio proyecto con PixelRoot32 como librería

1.  Crea un nuevo proyecto en PlatformIO para tu ESP32.
2.  Copia la carpeta `PixelRoot32-Game-Engine` dentro de la carpeta `lib/` de tu proyecto  
    (o añádela como submódulo Git en `lib/PixelRoot32-Game-Engine`).
3.  Crea una carpeta `src/drivers` en tu proyecto y añade allí tus implementaciones
    de `DrawSurface`, por ejemplo:
    - `src/drivers/esp32/TFT_eSPI_Drawer.cpp` para pantallas TFT_eSPI.
    - `src/drivers/native/SDL2_Drawer.cpp` para el modo PC nativo.
4.  En tu `src/main.cpp`, incluye el engine y configura los drivers, similar a:

```cpp
#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <drivers/esp32/ESP32_AudioBackend.h>
#include <core/Engine.h>

namespace pr32 = pixelroot32;

pr32::drivers::esp32::TFT_eSPI_Drawer drawer;
pr32::drivers::esp32::ESP32_AudioBackend audioBackend(26, 25, 22, 22050);
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

5.  Crea tus propias escenas heredando de `pixelroot32::core::Scene` y actores
    heredando de `pixelroot32::core::Actor` o `PhysicsActor`, y asígnalos con
    `engine.setScene(...)` en `setup()`.

---
*Built with ❤️ for the retro-dev community.*
