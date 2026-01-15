# PixelRoot32 Game Engine

PixelRoot32 Game Engine is a lightweight, modular 2D game engine written in C++ and designed specifically for ESP32 microcontrollers.

The engine adopts a node- and scene-based architecture inspired by Godot Engine, and provides a hardware abstraction layer (HAL) that enables native simulation on PC using SDL2. This makes cross‑platform development and debugging much easier: you can iterate quickly on desktop and then deploy the same code to the ESP32.

---

## Table of Contents

- [Origin and Inspiration](#origin-and-inspiration)
- [Coding Standards & Architecture](#-coding-standards--architecture)
- [Project Structure](#project-structure)
- [Main Components](#main-components)
- [High-Performance Optimizations](#high-performance-optimizations)
  - [Particle System (Pooled Memory)](#particle-system-pooled-memory)
  - [Asynchronous Rendering via DMA](#asynchronous-rendering-via-dma)
- [User Interface (UI) System](#user-interface-ui-system)
  - [Class Hierarchy](#class-hierarchy)
- [Example Usage in a Scene](#example-usage-in-a-scene)
  - [Technical Notes for the Implementation](#technical-notes-for-the-implementation)
- [Platform Configuration](#-platform-configuration)
- [Requirements](#-requirements)
- [Philosophy](#philosophy)

---

## Origin and Inspiration

PixelRoot32 is a direct evolution of the project:

ESP32-Game-Engine by nbourre  
<https://github.com/nbourre/ESP32-Game-Engine>

On top of this solid base, PixelRoot32 extends the original concept by incorporating ideas inspired by Godot, such as:

- Hierarchical organization using scenes and nodes
- Clear separation between logic, rendering, and input
- Reusable and decoupled components
- Structured update flow (`update` / `draw`)

Credits: this project explicitly acknowledges and thanks the original work by nbourre, on which PixelRoot32 is built and evolved.

---

## 📐 Coding Standards & Architecture

PixelRoot32 follows a well-defined set of coding conventions and architectural rules to ensure consistency, maintainability, and long-term scalability of the engine.

Before contributing or extending the engine, please review the following documents:

- [STYLE_GUIDE.md](lib/Engine/STYLE_GUIDE.md) — Official coding style, naming conventions, and file structure rules
- [API_REFERENCE.md](lib/Engine/API_REFERENCE.md) — Public engine API reference, following a Godot-inspired style
- [CONTRIBUTING.md](lib/Engine/CONTRIBUTING.md) — Guidelines for contributing to the project

These documents define:

- How code should be written and organized
- Which namespaces are public versus internal
- What is considered stable API and what may change internally

Adhering to these guidelines is required for all engine development.

---

## Project Structure

The engine architecture separates high-level logic from the hardware layer (HAL), enabling efficient development both on ESP32 and in a desktop environment.

```txt
Engine/
├── include/
│   ├── core/
│   │   ├── Actor.h
│   │   ├── Engine.h
│   │   ├── Scene.h
│   │   ├── Entity.h
│   │   ├── SceneManager.h
│   ├── graphics/
│   │   ├── ui/
│   │   │   ├── UIElement.h
│   │   │   ├── UILabel.h
│   │   │   └── UIButton.h
│   │   ├── Renderer.h
│   │   └── ...
│   ├── input/
│   └── physics/
├── src/
│   ├── core/
│   │   ├── Actor.cpp
│   │   ├── Engine.cpp
│   │   ├── Scene.cpp
│   │   ├── Entity.cpp
│   │   ├── SceneManager.cpp
│   ├── graphics/
│   │   ├── ui/
│   │   │   ├── UILabel.cpp
│   │   │   └── UIButton.cpp
│   │   └── Renderer.cpp
│   └── ...
```

---

## Main Components

Core  
Controls the `SceneManager`, the node tree, and the main execution loop (`update` / `draw`).

Renderer  
Unified rendering API. On ESP32 it uses `TFT_eSprite` with double buffering to remove visible flicker.

InputManager  
Abstracts physical buttons (GPIO) and PC keyboard keys into logical commands (`UP`, `DOWN`, `A`, `B`).

CollisionSystem  
Provides AABB collision detection and supports grid-based movement.

---

## High-Performance Optimizations

### Particle System (Pooled Memory)

- Uses static arrays to reuse particles and avoid memory fragmentation.
- Pre-calculated trigonometry to minimize cost in the update loop.
- Auto-clipping of entities that are off-screen.

### Asynchronous Rendering via DMA

- Non-blocking transfers using `pushImageDMA`.
- Real parallelism between game logic and SPI transfer.
- No tearing thanks to synchronization via `dmaWait`.

---

## User Interface (UI) System

The UI system is hierarchical and integrates with the normal scene flow, inspired by Godot’s node approach.

### Class Hierarchy

UIElement  
Base class with visibility and state control.

UILabel  
Efficient text rendering with dynamic alignment helpers (for example, centering).

UIButton  
Interactive element connected to `InputManager` (in active development).

---

## Example Usage in a Scene

```cpp
#include "graphics/ui/UILabel.h"

class GameScene : public Scene {
    UI::UILabel* lblStart;

    void init() override {
        lblStart = new UI::UILabel("PRESS A TO START", 0, 150, COLOR_WHITE, 1);
        lblStart->centerX(SCREEN_WIDTH);
        addEntity(lblStart);
    }

    void update(unsigned long deltaTime) override {
        if (gameStarted) {
            lblStart->setVisible(false);
        }
        Scene::update(deltaTime);
    }
};
```

### Technical Notes for the Implementation

1. Drawing Optimization: To avoid the "overlapping text" (ghosting) effect, `UILabel` implements an internal visibility check:

```c++
void UILabel::draw(Renderer& renderer) {
    if (!isVisible) return; // Avoid drawing elements that are hidden
    renderer.drawText(text.c_str(), x, y, color, size);
}
```

1. Size Calculation: The element width (`width`) is automatically calculated in the constructor by multiplying the number of characters by the font width (`size * 6`). This allows collision and centering systems to operate with precise bounds.

---

## 🛠️ Platform Configuration

The engine uses preprocessor directives to switch between hardware and simulator:

| Feature     | ESP32 (Production)        | Native (PC Development)     |
|------------|---------------------------|-----------------------------|
| Graphics   | TFT_eSPI (SPI bus)        | SDL2 (window manager)       |
| Input      | Physical buttons (GPIO)   | Keyboard (WASD / Arrows)    |
| Time       | `millis()` (Arduino)      | `MockArduino` / `SDL_GetTicks` |
| Debug      | Serial Monitor            | Standard console (`stdout`) |

📝 Example Implementation

```c++
#include "Scene.h"

class MainMenu : public Scene {
    void update(unsigned long deltaTime) override {
        if (engine.getInputManager().wasPressed(Input::BUTTON_A)) {
            // Change scene or start game
        }
    }
    
    void draw(Renderer& renderer) override {
        renderer.drawTextCentered("PRESS START", 120, COLOR_WHITE);
    }
};
```

---

## ⚙️ Requirements

1. ESP32 Environment:
   - Arduino framework for ESP32.
   - TFT_eSPI library (configure `User_Setup.h`, including MISO pin definition for DMA support).

2. Native Environment:
   - C++ compiler (GCC/Clang).
   - SDL2 library installed on the system.

Developed to be efficient, fast, and easy to extend.

---

## Philosophy

PixelRoot32 aims to provide:

- Clear and extensible architecture
- Real performance on constrained hardware
- A modern workflow inspired by high-level engines
- Full hardware control with a simple API

---
