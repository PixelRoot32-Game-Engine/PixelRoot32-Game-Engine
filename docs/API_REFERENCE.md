# API Reference

This document provides a complete reference for the PixelRoot32 Game Engine public API.

> **Note:** For the most up-to-date and comprehensive API documentation with examples and cross-references, visit the [official documentation](https://docs.pixelroot32.org/api_reference/).

## Table of Contents

The API documentation has been split into modular files for easier maintenance. Click on a topic to jump to the detailed documentation.

### Core Reference

| Topic | Description |
|-------|-------------|
| [Configuration](api/API_CONFIG.md) | Build flags, modular compilation, constants |
| [Math Module](api/API_MATH.md) | Scalar, Vector2, MathUtil, PRNG |
| [Core Module](api/API_CORE.md) | Engine, Entity, Actor, Scene, SceneManager |
| [Physics Module](api/API_PHYSICS.md) | CollisionSystem, PhysicsActor, RigidActor, collision helpers |
| [Graphics Module](api/API_GRAPHICS.md) | Renderer, sprites, tilemaps, particles, Camera2D |
| [UI Module](api/API_UI.md) | UI system, touch widgets, layouts |
| [Audio Module](api/API_AUDIO.md) | AudioEngine, MusicPlayer, music tracks |
| [Input Module](api/API_INPUT.md) | InputManager, TouchManager, touch calibration |
| [Platform Abstractions](api/API_PLATFORM.md) | Logging, PlatformMemory, hardware capabilities |

---

## Quick Reference by Feature

### Basic Setup

```cpp
// Include the engine
#include "core/Engine.h"

int main() {
    // Configure display
    DisplayConfig config;
    config.type = DisplayType::ST7789;
    config.physicalWidth = 240;
    config.physicalHeight = 240;
    config.logicalWidth = 240;
    config.logicalHeight = 240;

    // Create engine
    Engine engine(std::move(config));
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

    void draw(Renderer& renderer) override {
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

- [Architecture](ARCHITECTURE.md) (includes [ESP32 rendering and tilemap caching](ARCHITECTURE.md#esp32-rendering-pipeline-and-tilemap-caching))
- [Platform Compatibility Guide](PLATFORM_COMPATIBILITY.md)
- [Getting Started Guide](GETTING_STARTED.md)
- [Extending PixelRoot32](EXTENDING_PIXELROOT32.md)
- [Touch Input Architecture](architecture/ARCH_TOUCH_INPUT.md)
- [Migration Guides](docs/MIGRATION_*.md)