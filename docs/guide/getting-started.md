# Getting Started

**PixelRoot32** is a lightweight, modular 2D game engine written in **C++17**, designed primarily for **ESP32 microcontrollers**, with a native simulation layer for **PC (SDL2)** to enable rapid development without hardware.

## Overview

PixelRoot32 follows a **scene-based architecture inspired by Godot Engine**, making it intuitive for developers familiar with modern game development workflows.

**Key features**

- **Cross-platform** — Develop on PC (Windows/Linux/macOS) and deploy on ESP32
- **Scene–entity system** — Scenes, entities, and actors
- **High performance** — DMA transfers and IRAM-friendly paths on ESP32
- **Sprites** — 1bpp/2bpp/4bpp, palettes, animation
- **Tilemaps** — Viewport culling, multi-palette, tile animations
- **NES-style audio** — 4-channel subsystem
- **AABB physics** — Kinematic / rigid / static / sensor actors
- **UI** — Labels, buttons, layouts; optional touch widgets
- **Modular builds** — `PIXELROOT32_ENABLE_*` compile-time flags

## Prerequisites

- **VS Code** with **PlatformIO IDE**
- **ESP32** board or PC for `native` builds
- **USB cable** for flashing (ESP32)
- For PC: **SDL2** dev libraries (see [Platform compatibility](./platform-compatibility.md))

## Installation

### Option 1: PlatformIO Registry

```ini
lib_deps =
    gperez88/PixelRoot32-Game-Engine@^1.2.1
```

### Option 2: Clone the repository

```bash
git clone https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine.git
cd PixelRoot32-Game-Engine
```

### Open an example

Each example is a self-contained PlatformIO project under [`examples/`](../../examples/README.md).

```bash
cd examples/hello_world
```

## Configure PlatformIO

> **Required** — C++17 and no exceptions:

```ini
build_unflags = -std=gnu++11
build_flags =
    -std=gnu++17
    -fno-exceptions
```

Typical environments: `native`, `esp32dev`, `esp32cyd`, `esp32c3`, `esp32s3` (see each example’s `platformio.ini`).

## Minimal main file

```cpp
#include <Arduino.h>
#include <Engine.h>
#include <Scene.h>

using namespace pixelroot32;

class GameScene : public core::Scene {
public:
    void init() override {}
    void update(unsigned long deltaTime) override { (void)deltaTime; }
    void draw(graphics::Renderer& renderer) override {
        renderer.drawText("Hello World!", 10, 10, graphics::Color::WHITE, 2);
    }
};

core::Engine* engine;
GameScene scene;

void setup() {
    graphics::DisplayConfig displayConfig(240, 240);
    input::InputConfig inputConfig;
    inputConfig.addButton(input::ButtonName::A, 0);
    engine = new core::Engine(std::move(displayConfig), inputConfig);
    engine->setScene(&scene);
    engine->init();
    engine->run();
}

void loop() {}
```

## Understanding the game loop

PixelRoot32 follows a classic loop:

```mermaid
flowchart LR
    A[Initialize] --> B[Input]
    B --> C[Update]
    C --> D[Draw]
    D --> E[Present]
    E --> B
```

| Phase | Description |
|-------|-------------|
| **Input** | Buttons, touch, etc. |
| **Update** | Logic, physics, AI |
| **Draw** | Framebuffer |
| **Present** | DMA / display output |

See [Game loop](./game-loop.md) for detail.

## Best practices (ESP32)

1. Use **`math::Scalar`** / fixed-point patterns from the style guide; avoid raw `float` literals where the project uses fixed math.
2. **No heap churn** in `update()`/`draw()` — pool or pre-allocate in `init()`.
3. **Render layers** — background / world / UI ordering.
4. Use **`log()`** from `core/Log.h` instead of ad-hoc `Serial` spam.

See [Coding style](./coding-style.md) and [Memory system](../architecture/memory-system.md).

## Next steps

- [Core diagrams & scenes](../architecture/layer-scene.md) — entities and scene layer
- [Game loop](./game-loop.md)
- [Entities tutorial](./entities-scene-tutorial.md) — didactic `Entity` patterns
- [API index](../api/index.md)
- [Changelog](../../CHANGELOG.md)
