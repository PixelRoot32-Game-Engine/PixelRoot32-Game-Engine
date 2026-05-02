# API Reference

This document provides a high-level conceptual reference for the PixelRoot32 Game Engine public API.

> **Source of Truth:** 
> The Markdown files in this directory are **high-level conceptual guides**. For detailed, method-level documentation (signatures, parameter descriptions, return values), the **C++ header files (`include/**/*.h`)** serve as the single source of truth.

> **Note:** For the most up-to-date and comprehensive API documentation with examples and cross-references, visit the [official documentation](https://docs.pixelroot32.org/api_reference/).

## Table of Contents

The API documentation has been split into modular conceptual guides for easier maintenance. Click on a topic to jump to the detailed documentation.

### Core Reference

| Topic | Description |
|-------|-------------|
| [Configuration](config.md) | Build flags, modular compilation, constants |
| [Math Module](math.md) | Scalar, Vector2, MathUtil, PRNG |
| [Core Module](core.md) | Engine, Entity, Scene, SceneManager |
| [Physics Module](physics.md) | CollisionSystem, PhysicsActor, RigidActor, collision helpers |
| [Graphics Module](graphics.md) | Renderer, sprites, tilemaps, particles, Camera2D |
| [UI Module](ui.md) | UI system, touch widgets, layouts |
| [Audio Module](audio.md) | AudioEngine, MusicPlayer, music tracks |
| [Input Module](input.md) | InputManager, TouchManager, touch calibration |
| [Platform Abstractions](platform.md) | Logging, PlatformMemory, hardware capabilities |

---

## Quick Reference by Feature

### Basic Setup

```cpp
// Include the engine
#include "core/Engine.h"

int main() {
    // Configure display
    pixelroot32::graphics::DisplayConfig config;
    config.type = pixelroot32::graphics::DisplayType::ST7789;
    config.physicalWidth = 240;
    config.physicalHeight = 240;
    config.logicalWidth = 240;
    config.logicalHeight = 240;

    // Create engine
    pixelroot32::core::Engine engine(std::move(config));
    engine.init();

    // Run game loop
    engine.run();

    return 0;
}
```

### Scene Creation

```cpp
#include "core/Scene.h"

class MyScene : public pixelroot32::core::Scene {
public:
    void init() override {
        // Initialize scene
    }

    void update(unsigned long deltaTime) override {
        // Update game logic
    }

    void draw(pixelroot32::graphics::Renderer& renderer) override {
        renderer.beginFrame();
        // Draw everything
        renderer.endFrame();
    }
};
```

---

## Module Availability

Some modules are optional and can be disabled to save memory:

| Module | Macro | Default |
|--------|-------|---------|
| Audio | `PIXELROOT32_ENABLE_AUDIO` | Enabled |
| Physics | `PIXELROOT32_ENABLE_PHYSICS` | Enabled |
| UI System | `PIXELROOT32_ENABLE_UI_SYSTEM` | Enabled |
| Particles | `PIXELROOT32_ENABLE_PARTICLES` | Enabled |
| Touch Input | `PIXELROOT32_ENABLE_TOUCH` | Disabled |
| 2bpp Sprites | `PIXELROOT32_ENABLE_2BPP_SPRITES` | Disabled |
| 4bpp Sprites | `PIXELROOT32_ENABLE_4BPP_SPRITES` | Disabled |
| Tile Animations | `PIXELROOT32_ENABLE_TILE_ANIMATIONS` | Enabled |
| Static tilemap FB cache (4bpp) | `PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE` | Enabled (`PlatformDefaults.h`) |
| Debug Overlay | `PIXELROOT32_ENABLE_DEBUG_OVERLAY` | Disabled |

---

## Related Documentation

- [Architecture](../architecture/architecture-index.md) (includes [ESP32 rendering and tilemap caching](../architecture/architecture-index.md#esp32-rendering-pipeline-and-tilemap-caching))
- [Platform Compatibility Guide](../guide/platform-compatibility.md)
- [Getting Started Guide](../../README.md)
- [Extending PixelRoot32](../guide/extending-pixelroot32.md)
- [Touch Input Architecture](../architecture/touch-input.md)