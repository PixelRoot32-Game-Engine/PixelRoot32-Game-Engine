# PixelRoot32 Game Engine

PixelRoot32 Game Engine is a lightweight, modular 2D game engine written in C++ and designed specifically for ESP32 microcontrollers.

The engine adopts a node- and scene-based architecture inspired by Godot Engine, and provides a hardware abstraction layer (HAL) that enables native simulation on PC using SDL2. This makes cross‑platform development and debugging much easier: you can iterate quickly on desktop and then deploy the same code to the ESP32.

---

## Table of Contents

- [Origin and Inspiration](#origin-and-inspiration)
- [Coding Standards & Architecture](#-coding-standards--architecture)
- [Project Structure](#project-structure)
- [Main Components](#main-components)
- [Color Palette](#color-palette)
- [High-Performance Optimizations](#high-performance-optimizations)
  - [Particle System (Pooled Memory)](#particle-system-pooled-memory)
  - [Asynchronous Rendering via DMA](#asynchronous-rendering-via-dma)
- [Custom DrawSurface Implementations](#custom-drawsurface-implementations)
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

Credits: this project explicitly acknowledges and thanks the original work by **nbourre**, on which PixelRoot32 is built and evolved.

---

## 📐 Coding Standards & Architecture

PixelRoot32 follows a well-defined set of coding conventions and architectural rules to ensure consistency, maintainability, and long-term scalability of the engine.

Before contributing or extending the engine, please review the following documents:

- [STYLE_GUIDE.md](STYLE_GUIDE.md) — Official coding style, naming conventions, and file structure rules
- [API_REFERENCE.md](API_REFERENCE.md) — Public engine API reference, following a Godot-inspired style
- [CONTRIBUTING.md](CONTRIBUTING.md) — Guidelines for contributing to the project

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

### Color Palette

PixelRoot32 uses a fixed indexed color palette optimized for embedded hardware.

- Colors are represented as 8-bit indices
- Internally resolved to RGB565
- Improves performance and memory usage
- Ensures visual consistency across games

The engine provides a built-in palette of 32 colors via the `graphics::Color` enum.

Direct RGB565 usage is supported but discouraged for regular gameplay rendering.

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

## Physics System

The engine provides `PhysicsActor`, a specialized actor that handles 2D physics behavior similar to a `RigidBody2D` in Godot. It simplifies the implementation of moving objects by managing velocity, acceleration, and world boundary collisions automatically.

### Key Features

- **Velocity & Movement**: Managed via `vx` (horizontal) and `vy` (vertical) properties. The `update` loop automatically integrates these values to update position.
- **World & Custom Bounds**: 
  - Uses `worldWidth` and `worldHeight` to define the default play area.
  - Supports custom limits via `LimitRect` (top, bottom, left, right) to constrain actors to specific zones (e.g. `setLimits({0, 0, 320, 240})`).
- **Collision Resolution**: The `resolveWorldBounds()` function automatically detects collisions with the defined limits and applies a bounce response based on the `restitution` (bounciness) coefficient.
- **Properties**:
  - `restitution`: Controls energy conservation (1.0 = full bounce, < 1.0 = dampening).
  - `friction`: Applies drag to movement.

### Example: BallActor

Here is how you can use `PhysicsActor` to create a bouncing ball, like in Pong:

```cpp
class BallActor : public pixelroot32::core::PhysicsActor {
public:
    BallActor(float x, float y) : PhysicsActor(x, y, 10, 10) {
        // 1. Initialize velocity
        vx = 200.0f;
        vy = -150.0f;
        
        // 2. Set physical properties
        setRestitution(1.0f); // Perfect bounce (no energy loss)
        setFriction(0.0f);    // No friction
    }
    
    // 3. Customize behavior on impact (optional)
    void onWorldCollision() override {
        // Logic when hitting walls (e.g. play sound)
    }
};

// In your scene:
// Create the ball and set the world boundaries for collision
auto* ball = new BallActor(160, 120);
ball->setWorldSize(320, 240); 
addEntity(ball);
```

---

## Custom DrawSurface Implementations

PixelRoot32 uses the `DrawSurface` interface as a thin abstraction layer between the engine and the underlying graphics backend. The high-level `Renderer` only depends on `DrawSurface`, so you can plug in your own implementation without modifying engine code.

### Responsibilities of DrawSurface

A concrete `DrawSurface` implementation is responsible for:

- Initializing the graphics backend (SPI display, SDL window, etc.).
- Managing an internal framebuffer (or equivalent).
- Implementing basic drawing primitives:
  - Text rendering (`drawText`, `drawTextCentered`).
  - Shapes (`drawFilledCircle`, `drawCircle`, `drawRectangle`, `drawFilledRectangle`, `drawLine`).
  - Bitmaps (`drawBitmap`).
  - Single pixels (`drawPixel`).
- Handling display state:
  - Rotation (`setRotation`).
  - Logical size (`setDisplaySize`).
  - Contrast/brightness (`setContrast`).
- Presenting frames:
  - Flushing the buffer to the display (`sendBuffer`).
  - Swapping buffers or updating the window (`present`).
  - Optionally processing platform events (`processEvents`).

Reference implementations:

- ESP32: [`TFT_eSPI_Drawer`](lib/Engine/include/drivers/esp32/TFT_eSPI_Drawer.h).
- Native (PC): [`SDL2_Drawer`](lib/Engine/include/drivers/native/SDL2_Drawer.h).

### Basic Steps to Implement Your Own DrawSurface

1. Create a new class that derives from `pixelroot32::graphics::DrawSurface` inside your own namespace. For example:

```cpp
namespace mygame::drivers {

class MyDisplayDriver : public pixelroot32::graphics::DrawSurface {
public:
    void init() override;
    void setRotation(uint8_t rotation) override;
    void clearBuffer() override;
    void sendBuffer() override;

    void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) override;
    void drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size) override;
    void drawFilledCircle(int x, int y, int radius, uint16_t color) override;
    void drawCircle(int x, int y, int radius, uint16_t color) override;
    void drawRectangle(int x, int y, int width, int height, uint16_t color) override;
    void drawFilledRectangle(int x, int y, int width, int height, uint16_t color) override;
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color) override;
    void drawBitmap(int x, int y, int width, int height, const uint8_t* bitmap, uint16_t color) override;
    void drawPixel(int x, int y, uint16_t color) override;

    void setContrast(uint8_t level) override;
    void setTextColor(uint16_t color) override;
    void setTextSize(uint8_t size) override;
    void setCursor(int16_t x, int16_t y) override;

    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) override;
    void setDisplaySize(int w, int h) override;

    bool processEvents() override;
    void present() override;
};

}
```

You can use `TFT_eSPI_Drawer` and `SDL2_Drawer` as concrete references for how to implement each method.

2. Instantiate your driver and wire it into a `DisplayConfig`:

```cpp
mygame::drivers::MyDisplayDriver myDriver;

pixelroot32::graphics::DisplayConfig config(
    &myDriver,
    0,
    SCREEN_WIDTH,
    SCREEN_HEIGHT
);
```

The `Renderer` will use this `DrawSurface` instance internally and will not need to know anything about the underlying hardware or window system.

3. Pass the `DisplayConfig` to the `Engine`:

```cpp
pixelroot32::core::Engine engine(config, inputConfig);
```

From this point on, all scenes and entities can draw using the standard `Renderer` API, while your custom `DrawSurface` takes care of the low-level details.

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

The `native` PlatformIO environment is intended for running and debugging your game
logic directly on a desktop PC (using SDL2) without flashing the firmware to an
ESP32 on every iteration. This allows fast feedback loops while keeping the same
engine API and game code for both targets.

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
   - A display driver library compatible with your hardware that you plug in via a custom `DrawSurface` implementation. The engine ships with a reference driver based on **TFT_eSPI**, but you can use alternatives such as **U8g2** or your own driver as long as they implement the `DrawSurface` interface.

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
