# PixelRoot32 Game Engine

PixelRoot32 is a lightweight 2D game engine designed for ESP32-based systems.  
It focuses on simplicity, deterministic behavior, and low memory usage, making it suitable for embedded environments and small-scale games.

---

## 📐 Coding Style Guide

PixelRoot32 follows a strict set of conventions to ensure consistency, readability, and long-term maintainability of the engine.

### Language

- C++17
- Avoid RTTI and exceptions (use `-fno-exceptions`)
- Prefer deterministic and explicit control flow

### Modern C++ Features (C++17)

PixelRoot32 embraces C++17 to write safer and more expressive code without sacrificing performance.

- **Smart Pointers**: `std::unique_ptr` for exclusive ownership.
- **String Views**: `std::string_view` for non-owning string references (avoid `std::string` copies).
- **Optional**: `std::optional` for values that may or may not exist (cleaner than pointer checks or magic values).
- **Attributes**: Use `[[nodiscard]]` for functions where the return value must not be ignored (e.g., error codes).
- **Constexpr**: Use `constexpr` for compile-time constants and `if constexpr` for compile-time branching.

### Files

- `.h` files define interfaces and public types
- `.cpp` files contain implementations
- Public headers must not contain heavy logic (only trivial inline code if needed)

### Includes

- User code must include headers only from `include/`
- Headers in `include/` may include headers from `src/`
- Source files in `src/` must never include headers from `include/`
- Internal headers that are not part of the public API must not be exposed via `include/`

### Naming Conventions

- Classes and structs: PascalCase
- Methods and functions: camelCase
- Variables and members: camelCase
- No Hungarian notation
- No `m_` or `_` prefixes for members

### Order inside classes

- Public members first
- Protected members second
- Private members last

---

## 🧩 Namespace Design

PixelRoot32 uses namespaces to clearly separate public API from internal implementation details.

### Root Namespace

All engine symbols live under the root namespace:

pixelroot32

---

### Public Namespaces (API)

These namespaces are considered part of the stable public API and may be used directly by game projects:

- pixelroot32::core
- pixelroot32::graphics
- pixelroot32::graphics::ui
- pixelroot32::input
- pixelroot32::physics
- pixelroot32::math
- pixelroot32::drivers

**Modular Compilation Note:** Some namespaces may be conditionally available:

- `pixelroot32::audio`: Only available when `PIXELROOT32_ENABLE_AUDIO=1`
- `pixelroot32::graphics::ui`: Only available when `PIXELROOT32_ENABLE_UI_SYSTEM=1`
- `pixelroot32::physics`: Only available when `PIXELROOT32_ENABLE_PHYSICS=1`

Example usage in a game project:

```cpp
class BallActor : public pixelroot32::core::Actor {
    ...
};

#if PIXELROOT32_ENABLE_AUDIO
class AudioManager {
    pixelroot32::audio::AudioEngine* audio;
    // Audio management code
};
#endif

#if PIXELROOT32_ENABLE_UI_SYSTEM
class GameUI {
    pixelroot32::graphics::ui::UIButton button;
    // UI management code
};
#endif
```

---

### Internal Namespaces (Non-API)

The following namespaces are intended for internal engine use only and are not part of the stable public API:

- pixelroot32::platform
- pixelroot32::platform::mock
- pixelroot32::platform::esp32
- pixelroot32::internal
- pixelroot32::detail

Rules for internal namespaces:

- They may change without notice
- They must not be included directly by user projects
- They must not be exposed through headers in `include/`

---

### Namespace Usage Rules

#### Public Headers (`.h`)

- **Prohibited:** `using namespace` in any form
- **Required:** Fully qualified names or local aliases within limited scopes

```cpp
// ✅ Correct: Fully qualified
pixelroot32::graphics::Renderer renderer;

// ✅ Correct: Subsystem alias (inside function or method)
void MyClass::draw() {
    namespace gfx = pixelroot32::graphics;
    gfx::Renderer renderer;
}

// ❌ Prohibited: using namespace in header
using namespace pixelroot32::graphics;
```

#### Implementation Files (`.cpp`)

| Approach | Recommended Use | Example |
|----------|-----------------|---------|
| **Subsystem alias** | Daily use in large modules (rendering, physics) | `namespace gfx = pixelroot32::graphics;` |
| **Root alias** | Multiple references to `pixelroot32::*` | `namespace pr32 = pixelroot32;` |
| **Selective using** | Specific frequently-used symbols | `using pixelroot32::graphics::Renderer;` |
| **Using namespace** | Only in: tests, small scopes (<20 lines), prototypes | `using namespace pixelroot32::math;` // inside function |

**Recommended subsystem aliases:**

```cpp
// graphics
namespace gfx = pixelroot32::graphics;

// ui (inside graphics)
namespace ui = pixelroot32::graphics::ui;

// physics
namespace phy = pixelroot32::physics;

// math (when used intensively)
namespace math = pixelroot32::math;

// input
namespace input = pixelroot32::input;

// core
namespace core = pixelroot32::core;
```

**Restrictions for `using namespace`:**

✅ **Acceptable in:**

- Unit tests (files `test_*.cpp`)
- Function scopes (not file or namespace scope)
- Internal prototype/tool files (<50 lines)
- Very local initialization functions

❌ **Prohibited in:**

- Engine core files (`.cpp` in `src/core/`, `src/physics/`, etc.)
- Namespace scope (affects entire file)
- Public headers (`.h`)
- Production code on devices

**Example of selective using (preferred over using namespace):**

```cpp
// ✅ Correct: Import only specific symbols
using pixelroot32::graphics::Renderer;
using pixelroot32::graphics::Sprite;
using pixelroot32::math::Vector2;

Renderer r;      // Clearly graphics::Renderer
Sprite s;        // Clearly graphics::Sprite
Vector2 pos;     // Clearly math::Vector2

// ❌ Incorrect: Pollutes entire namespace
using namespace pixelroot32::graphics;  // Renderer? Sprite? From where?
```

**Example of limited scope:**

```cpp
void processPhysics() {
    // ✅ Correct: Alias inside function
    namespace phy = pixelroot32::physics;
    
    phy::CollisionSystem cs;
    phy::KinematicActor actor;
    
    // Does not affect other functions in the file
}

void processInput() {
    // ✅ Different subsystem, same file, no conflicts
    namespace input = pixelroot32::input;
    
    input::TouchEvent event;
    // phy:: is not available here (intentional)
}
```

---

## 📦 Library Usage Expectations

- Users are expected to include headers only from `include/`
- Users should reference engine types via fully-qualified namespaces
- The engine does not pollute the global namespace

---

## 📚 Additional Documentation

The following documents are recommended as part of the project:

- [API_REFERENCE.md](API_REFERENCE.md) — Engine API reference (Godot-style)
- [CONTRIBUTING.md](../CONTRIBUTING.md) — Contribution guidelines

---

### 🚀 Best Practices & Optimization

These guidelines are derived from practical implementation in `examples/GeometryJump`, `examples/BrickBreaker`, `examples/Pong`, and the side-scrolling platformer prototype used in the camera demo.

### 💾 Memory & Resources

**📖 For comprehensive C++17 memory management guide, see [Memory Management Guide](MEMORY_MANAGEMENT_GUIDE.md)**

- **Smart Pointers (C++17)**: Prefer `std::unique_ptr` for owning objects (like Scenes, Actors, UI elements) to automate memory management and document ownership.
  - Use `std::make_unique<T>(...)` to create objects.
  - Pass raw pointers (via `.get()`) to functions that do *not* take ownership (like `addEntity`).
  - Use `std::move` only when transferring ownership explicitly.
- **Object Pooling**: Pre-allocate all game objects (obstacles, particles, enemies) during `init()`.
  - *Pattern*: Use fixed-size arrays (e.g., `Particle particles[50]`) and flags (`isActive`) instead of `std::vector` with `push_back`/`erase`.
  - *Trade-off*: Eliminates runtime allocations and fragmentation at the cost of a slightly higher fixed RAM footprint; dimension pools to realistic worst-case usage.
- **Zero Runtime Allocation**: Never use `new` or `malloc` inside the game loop (`update` or `draw`).
- **String Handling**: Avoid `std::string` copies. Use `std::string_view` for passing strings. For formatting, use `snprintf` with stack-allocated `char` buffers.

- **Scene Arenas** (`PIXELROOT32_ENABLE_SCENE_ARENA`):
  - Use a single pre-allocated buffer per scene for temporary entities or scratch data when you need strict zero-allocation guarantees.
  - *Trade-off*: Very cache-friendly and fragmentation-proof, but the buffer cannot grow at runtime; oversizing wastes RAM, undersizing returns `nullptr` and requires graceful fallback logic.

#### Modular Compilation Memory Optimization

The modular compilation system provides powerful memory optimization capabilities:

- **Selective Inclusion**: Only compile subsystems you actually use
- **Memory Savings**: Each disabled subsystem eliminates its static buffers and runtime allocations:
  - Audio disabled: ~8KB RAM saved
  - Physics disabled: ~12KB RAM saved
  - UI System disabled: ~4KB RAM saved
  - Particles disabled: ~6KB RAM saved
- **Firmware Size**: Disabled subsystems are completely excluded from the final binary, reducing flash usage

#### Recommended Pooling Patterns (ESP32)

- **High-rotation entities** (bullets, snake segments, particles):
  - Create all instances once in `init()` or in an initial `resetGame()`.
  - Keep a usage flag (for example `isActive`) or a separate container that represents the active subset.
  - Reactivate entities with a `reset(...)` method that configures position/state without allocating memory again.
  - Avoid calling `delete` inside the game loop; deactivate and recycle entities instead.
- **Engine examples**:
  - Space Invaders projectiles: fixed-size bullet pool reused via `reset(...)`.
  - Snake segments: segment pool reused for growth without `new` during gameplay.

### ⚡ Performance (ESP32 Focus)

- **Inlining**:
  - Define trivial accessors (e.g., `getHitBox`, `getX`) in the header (`.h`) to allow compiler inlining.
  - Keep heavy implementation logic in `.cpp`.
- **Fast Randomness**: `std::rand()` is slow and uses division. Use `math::randomScalar()` or `math::randomRange()` (which use optimized Xorshift algorithms compatible with `Fixed16`) for visual effects.
- **Collision Detection**:
  - Use simple AABB (Axis-Aligned Bounding Box) checks first. Use Collision Layers (`GameLayers.h`) to avoid checking unnecessary pairs.
  - For very fast projectiles (bullets, lasers), prefer lightweight sweep tests:
    - Represent the projectile as a small `physics::Circle` and call `physics::sweepCircleVsRect(startCircle, endCircle, targetRect, tHit)` against potential targets.
    - Use sweep tests only for the few entities that need them; keep everything else on basic AABB to avoid unnecessary CPU cost.

### 🏗️ Code Architecture

- **Tuning Constants**: Extract gameplay values (gravity, speed, dimensions) into a dedicated `GameConstants.h`. This allows designers to tweak the game without touching logic code.
- **State Management**: Implement a `reset()` method for Actors to reuse them after "Game Over", rather than destroying and recreating the scene.
- **Component Pattern**: Inherit from `PhysicsActor` for moving objects and `Actor` for static ones.

### 🎮 Game Feel & Logic

- **Frame-Rate Independence**: Always multiply movement by `deltaTime`.
  - *Example*: `x += speed * math::toScalar(deltaTime * 0.001f);`
- **Logic/Visual Decoupling**: For infinite runners, keep logic progression (obstacle spacing) constant in time, even if visual speed increases.
- **Snappy Controls**: For fast-paced games, prefer higher gravity and jump forces to reduce "floatiness".
- **Slopes & Ramps on Tilemaps**: When implementing ramps on a tilemap, treat contiguous ramp tiles as a single logical slope and compute the surface height using linear interpolation over world X instead of resolving per tile. Keep gravity and jump parameters identical between flat ground and ramps so jump timing remains consistent.

#### Modular Compilation Considerations

- **Conditional Logic**: Use `#if PIXELROOT32_ENABLE_*` guards around subsystem-specific code
- **Fallback Systems**: When a subsystem is disabled, provide simplified alternatives:

  ```cpp
  #if PIXELROOT32_ENABLE_PHYSICS
      // Full physics collision
      scene.collisionSystem.update();
  #else
      // Simple AABB collision check
      checkSimpleCollisions();
  #endif
  ```

#### Recommended Build Profiles

Choose a profile based on your game type to optimize memory usage:

| Game Type | Profile | Enabled | Disabled |
|-----------|---------|---------|----------|
| Arcade shooters/platformers | `arcade` | Audio, Physics, Particles | UI System |
| Puzzle/casual games | `puzzle` | Audio, UI System | Physics, Particles |
| Retro/minimal | `retro` | None | All |
| Educational/tools | `puzzle` or custom | Audio, UI System | Physics, Particles |

**Example platformio.ini configuration:**

```ini
[env:esp32_arcade]
extends = base_esp32, profile_arcade
build_flags =
    ${base_esp32.build_flags}
    ${profile_arcade.build_flags}

[env:esp32_puzzle]
extends = base_esp32, profile_puzzle
build_flags =
    ${base_esp32.build_flags}
    ${profile_puzzle.build_flags}

[env:native_retro]
build_flags =
    -DPLATFORM_NATIVE=1
    -DPIXELROOT32_ENABLE_AUDIO=0
    -DPIXELROOT32_ENABLE_PHYSICS=0
    -DPIXELROOT32_ENABLE_PARTICLES=0
    -DPIXELROOT32_ENABLE_UI_SYSTEM=0
```

### Logging Best Practices

The unified logging system (`pixelroot32::core::logging`) provides conditional logging that is completely eliminated from production builds.

#### When to Use Each LogLevel

| Level | Use Case | Example |
|-------|----------|---------|
| `Info` | General debug messages, state changes | `"Player position: %d, %d"`, `"Level loaded: %s"` |
| `Profiling` | Performance timing markers | `"Frame time: %d ms"`, `"Physics: %d contacts"` |
| `Warning` | Non-critical issues, graceful degradation | `"Low memory: %d bytes"`, `"Audio buffer underrun"` |
| `Error` | Critical failures, unrecoverable | `"Failed to load sprite: %s"`, `"Display init failed"` |

#### Performance Considerations

- **Zero overhead in production**: When `PIXELROOT32_DEBUG_MODE` is not defined, all `log()` calls become no-ops at compile time
- **Enable only in development**: Add to `platformio.ini` for debug builds only:

  ```ini
  [env:debug]
  build_flags = -DPIXELROOT32_DEBUG_MODE
  
  [env:release]
  build_flags =  ; No debug mode
  ```

#### Platform Differences

- **ESP32**: Output routed to `Serial` (115200 baud recommended)
- **Native/PC**: Output routed to `stdout` with auto-flush

#### Common Patterns

```cpp
#include "core/Log.h"
using namespace pixelroot32::core::logging;

// Initialization logging
log("Engine initialized: %dx%d", width, height);

// State transitions
log("Scene changed: %s", sceneName);

// Performance monitoring
log(LogLevel::Profiling, "Update: %lu ms", updateTime);
log(LogLevel::Profiling, "Draw: %lu ms", drawTime);

// Error handling
if (!texture.load(filename)) {
    log(LogLevel::Error, "Failed to load texture: %s", filename);
    return false;
}
```

#### Avoid in Production

```cpp
// BAD: Logging in hot paths (game loop)
void update() {
    log("Velocity: %f", velocity.x);  // Expensive string formatting
}

// GOOD: Conditional logging with frame skipping
void update() {
    if (frameCount % 60 == 0) {  // Log every 60 frames
        log("FPS: %d", calculateFPS());
    }
}
```

### 🧮 Math & Fixed-Point Guidelines

The engine uses a **Math Policy Layer** to support both FPU (Float) and non-FPU (Fixed-Point) hardware seamlessly.

1. **Use `Scalar` everywhere**: Never use `float` or `double` explicitly in game logic, physics, or positioning. Use `pixelroot32::math::Scalar`.
2. **Literals**: Use `math::toScalar(0.5f)` for floating-point literals. This ensures they are correctly converted to `Fixed16` on integer-only platforms.
    - *Bad*: `Scalar speed = 2.5;` (Implicit double conversion, slow/error-prone on Fixed16)
    - *Good*: `Scalar speed = math::toScalar(2.5f);`
3. **Renderer Conversion**: The `Renderer` works with pixels (`int`). Keep positions as `Scalar` logic-side and convert to `int` **only** when calling draw methods.
    - *Example*: `renderer.drawSprite(spr, static_cast<int>(x), static_cast<int>(y), ...)`
4. **Audio Independence**: The audio subsystem is optimized separately and does **not** use `Scalar`. It continues to use its own internal formats (integer mixing).

---

### 🎨 Sprite & Graphics Guidelines

- **1bpp Sprites**: Define sprite bitmaps as `static const uint16_t` arrays, one row per element. Use bit `0` as the leftmost pixel and bit (`width - 1`) as the rightmost pixel.

### 🎨 UI Layout Guidelines

- **Use Layouts for Automatic Organization**: Prefer `UIVerticalLayout` (for vertical lists), `UIHorizontalLayout` (for horizontal menus/bars), or `UIGridLayout` (for matrix layouts like inventories) over manual position calculations when organizing multiple UI elements. This simplifies code and enables automatic navigation.
- **Use Padding Container for Spacing**: Use `UIPaddingContainer` to add padding around individual elements or to nest layouts with custom spacing. This is more efficient than manually calculating positions and allows for flexible UI composition.
- **Use Panel for Visual Containers**: Use `UIPanel` to create retro-style windows, dialogs, and menus with background and border. Panels typically contain layouts (Vertical, Horizontal, or Grid) which then contain buttons and labels. Ideal for Game & Watch style interfaces.
- **Use Anchor Layout for HUDs**: Use `UIAnchorLayout` to position HUD elements (score, lives, health bars) at fixed screen positions without manual calculations. Supports 9 anchor points (corners, center, edges). Very efficient on ESP32 as it has no reflow - positions are calculated once or when screen size changes.
- **Performance on ESP32**: Layouts use viewport culling and optimized clearing (only when scroll changes) to minimize rendering overhead. The layout system is designed to be efficient on embedded hardware.
- **Scroll Behavior**: Vertical and horizontal layouts use NES-style instant scroll on selection change for responsive navigation. Smooth scrolling is available for manual scrolling scenarios.
- **Navigation**: `UIVerticalLayout` handles UP/DOWN navigation, `UIHorizontalLayout` handles LEFT/RIGHT navigation, and `UIGridLayout` handles 4-direction navigation (UP/DOWN/LEFT/RIGHT) with wrapping. All layouts support automatic selection management and button styling.
- **Grid Layout**: `UIGridLayout` automatically calculates cell dimensions based on layout size, padding, and spacing. Elements are centered within cells if they're smaller than the cell size. Ideal for inventories, level selection screens, and item galleries.
- **Sprite Descriptors**: Wrap raw bitmaps in `pixelroot32::graphics::Sprite` or `MultiSprite` descriptors and pass them to `Renderer::drawSprite` / `Renderer::drawMultiSprite`.
- **No Bit Logic in Actors**: Actors should never iterate bits or draw individual pixels. They only select the appropriate sprite (or layered sprite) and call the renderer.
- **Layered Sprites First**: Prefer composing multi-color sprites from multiple 1bpp `SpriteLayer` entries. Keep layer data `static const` to allow storage in flash and preserve the 1bpp-friendly pipeline.
- **Optional 2bpp/4bpp Sprites**: For higher fidelity assets, you can enable packed 2bpp/4bpp formats via compile-time flags (for example `PIXELROOT32_ENABLE_2BPP_SPRITES` / `PIXELROOT32_ENABLE_4BPP_SPRITES`). Treat these as advanced options: they improve visual richness (better shading, logos, UI) at the cost of 2x/4x sprite memory and higher fill-rate. Use them sparingly on ESP32 and keep gameplay-critical sprites on the 1bpp path.
- **Integer-Only Rendering**: Sprite rendering must remain integer-only and avoid dynamic allocations to stay friendly to ESP32 constraints.

#### Modular Compilation UI Impact

- **Conditional UI**: UI system is only compiled when `PIXELROOT32_ENABLE_UI_SYSTEM=1`
- **Memory Savings**: Disabling UI saves ~4KB RAM and reduces firmware size by 8-15%
- **Alternative Approaches**: When UI is disabled, use simple text rendering or custom bitmap-based interfaces

### 🧱 Render Layers & Tilemaps

- **Render Layers**:
  - Use `Entity::renderLayer` to separate concerns:
    - `0` – background (tilemaps, solid fills, court outlines).
    - `1` – gameplay actors (player, enemies, bullets, snake segments, ball/paddles).
    - `2` – UI (labels, menus, score text).
  - Scenes draw entities by iterating these layers in ascending order. Higher layers naturally appear on top.
- **Background Entities**:
  - Prefer lightweight background entities in layer `0` (for example, starfields or playfield outlines) instead of redrawing background logic inside every scene `draw()`.
- **Tilemaps**:
  - For grid-like backgrounds, use the `TileMap` helper with 1bpp `Sprite` tiles and `Renderer::drawTileMap`.
  - Keep tile indices in a compact `uint8_t` array and reuse tiles across the map to minimize RAM and flash usage on ESP32.
  - *Trade-off*: Greatly reduces background RAM compared to full bitmaps, but adds a predictable per-tile draw cost; avoid unnecessarily large maps or resolutions on ESP32.
  - For side-scrolling platformers, combine tilemaps with `Camera2D` and `Renderer::setDisplayOffset` instead of manually offsetting individual actors. Keep camera logic centralized (for example in a `Scene`-level camera object) and use different parallax factors per layer to achieve multi-layer scrolling without additional allocations.

### Tile Animation Usage

The tile animation system provides frame-based animations for tilemaps (water, lava, conveyors) with O(1) frame resolution and zero dynamic allocations.

#### Memory Budget

| Tileset Size | RAM Usage | % ESP32 DRAM |
|--------------|-----------|--------------|
| 64 tiles | 73 bytes | 0.02% |
| 128 tiles | 137 bytes | 0.04% |
| 256 tiles | 265 bytes | 0.08% |

**Recommendation**: Start with 64 or 128 tile tilesets. Increase only if needed.

#### Initialization Pattern

```cpp
// In scene header (PROGMEM)
PIXELROOT32_SCENE_FLASH_ATTR const TileAnimation animations[] = {
    { 2, 4, 8, 0 },  // Water: tiles 2-5, 4 frames, 8 ticks/frame
    { 6, 2, 6, 0 },  // Lava: tiles 6-7, 2 frames, 6 ticks/frame
};

// Global manager instance
TileAnimationManager animManager(animations, 2, 64);

// Link to tilemap
TileMap2bpp backgroundLayer = {
    // ... other fields ...
    nullptr,              // runtimeMask
    nullptr,              // paletteIndices
    &animManager          // animManager - enables animations
};
```

#### Game Loop Integration

```cpp
void MyScene::update(unsigned long dt) {
    // Advance animations once per frame
    animManager.step();
    
    // ... other update logic ...
    Scene::update(dt);
}

void MyScene::draw(Renderer& r) {
    // Renderer automatically calls resolveFrame() for each visible tile
    r.drawTileMap(backgroundLayer, 0, 0);
    Scene::draw(r);
}
```

#### Animation Speed Control

```cpp
void MyScene::update(unsigned long dt) {
    // Half speed: advance every 2 frames
    if (frameCount % 2 == 0) {
        animManager.step();
    }
    
    // Pause when game is paused
    if (!isPaused) {
        animManager.step();
    }
    
    // Speed up specific animations
    animManager.step();  // Normal speed
    animManager.step();  // Extra step = double speed
}
```

#### Multiple Animation Managers

For layered tilemaps with independent animation timing:

```cpp
TileAnimationManager bgAnim(bgAnims, 2, 64);   // Background water
TileAnimationManager fgAnim(fgAnims, 4, 64);   // Foreground lava

backgroundLayer.animManager = &bgAnim;
foregroundLayer.animManager = &fgAnim;

void MyScene::update(unsigned long dt) {
    bgAnim.step();
    fgAnim.step();
}
```

#### Common Pitfalls

1. **Sequential frames only**: Animation frames must be contiguous in the tileset (tiles 2,3,4,5). Non-sequential animations require multiple animation definitions or pre-sorted tile indices.

2. **Shared animation state**: All instances of a tile share the same animation frame. For independent animations, use different base tile indices.

3. **Global timing**: All animations advance together. For desynchronized animations (e.g., two water pools with different phases), create separate animation managers with different frame offsets, or use conditional `step()` calls.

---

### 🎨 Multi-Palette Systems (Sprites & Backgrounds)

The engine supports multiple palettes for both sprites and backgrounds through palette slot banks. These systems follow consistent naming and usage patterns.

#### Naming Conventions

**Palette Slot Functions:**

```cpp
// Background palette slots (for tilemaps)
initBackgroundPaletteSlots()           // Initialize all slots
setBackgroundPaletteSlot(slot, type)   // Set slot by PaletteType
setBackgroundCustomPaletteSlot(slot, palette) // Set slot by custom palette
getBackgroundPaletteSlot(slot)         // Get palette pointer (never nullptr)

// Sprite palette slots (for sprites)
initSpritePaletteSlots()               // Initialize all slots  
setSpritePaletteSlot(slot, type)       // Set slot by PaletteType
setSpriteCustomPaletteSlot(slot, palette) // Set slot by custom palette
getSpritePaletteSlot(slot)              // Get palette pointer (never nullptr)
```

**Context Functions:**

```cpp
// Sprite palette slot context (for batch rendering)
setSpritePaletteSlotContext(slot)       // Set global context slot
getSpritePaletteSlotContext()           // Get current context slot
```

**Constants:**

```cpp
MAX_BACKGROUND_PALETTE_SLOTS   // Default: 8, configurable
MAX_SPRITE_PALETTE_SLOTS       // Default: 8, configurable
```

#### Usage Patterns

**1. Game Initialization - Setup Palette Slots**

```cpp
class MyGameScene : public pixelroot32::core::Scene {
public:
    void init() override {
        // Enable dual palette mode for separate sprite/background palettes
        pixelroot32::graphics::Color::enableDualPaletteMode(true);
        
        // Initialize palette slot banks
        pixelroot32::graphics::initBackgroundPaletteSlots();
        pixelroot32::graphics::initSpritePaletteSlots();
        
        // Setup background palette slots for different tilemap areas
        pixelroot32::graphics::setBackgroundPaletteSlot(0, pixelroot32::graphics::PaletteType::PR32);  // Default ground
        pixelroot32::graphics::setBackgroundPaletteSlot(1, pixelroot32::graphics::PaletteType::NES);  // Water areas  
        pixelroot32::graphics::setBackgroundPaletteSlot(2, pixelroot32::graphics::PaletteType::GB);   // Underground
        
        // Setup sprite palette slots for different enemy types
        pixelroot32::graphics::setSpritePaletteSlot(0, pixelroot32::graphics::PaletteType::PR32);  // Player
        pixelroot32::graphics::setSpritePaletteSlot(1, pixelroot32::graphics::PaletteType::NES);  // Fire enemies
        pixelroot32::graphics::setSpritePaletteSlot(2, pixelroot32::graphics::PaletteType::GBC);  // Ice enemies
        pixelroot32::graphics::setSpritePaletteSlot(3, pixelroot32::graphics::PaletteType::PICO8); // Boss enemies
    }
};
```

**2. Custom Palettes - Define and Apply**

```cpp
// Define custom palettes (typically as static const for flash storage)
static const uint16_t CUSTOM_PLAYER_PALETTE[] = {
    0x0000, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFE0, 0xF81F, 0x07FF,
    0x8410, 0x0410, 0x0010, 0x8000, 0x8400, 0x8010, 0x841F, 0x0000
};

static const uint16_t CUSTOM_FIRE_PALETTE[] = {
    0x0000, 0xFFFF, 0xF800, 0xFC00, 0xFA00, 0xF800, 0xF600, 0xF400,
    0xF200, 0xF000, 0xE800, 0xE000, 0xC000, 0x8000, 0x4000, 0x0000
};

class EnemyManager {
public:
    void loadCustomPalettes() {
        // Apply custom palettes to specific slots
        pixelroot32::graphics::setSpriteCustomPaletteSlot(4, CUSTOM_PLAYER_PALETTE);
        pixelroot32::graphics::setSpriteCustomPaletteSlot(5, CUSTOM_FIRE_PALETTE);
    }
};
```

**3. Rendering - Use Palette Slots**

```cpp
class PlayerActor : public pixelroot32::core::Actor {
public:
    void draw(pixelroot32::graphics::Renderer& renderer) override {
        // Draw player using sprite palette slot 0 (default)
        renderer.drawSprite(playerSprite, static_cast<int>(x), static_cast<int>(y), 0, facingLeft);
    }
};

class EnemyActor : public pixelroot32::core::Actor {
private:
    uint8_t enemyType;  // 0=normal, 1=fire, 2=ice, 3=boss
    
public:
    void draw(pixelroot32::graphics::Renderer& renderer) override {
        // Draw enemy using appropriate palette slot
        uint8_t paletteSlot = 1 + enemyType;  // Slots 1-4 for different enemy types
        renderer.drawSprite(enemySprite, static_cast<int>(x), static_cast<int>(y), paletteSlot, facingLeft);
    }
};

class TilemapRenderer {
public:
    void drawBackground(pixelroot32::graphics::Renderer& renderer, const TileMap2bpp& map) {
        // Tilemap automatically uses paletteIndices array if present
        // Each cell can select background palette slot 0-7
        renderer.drawTileMap(map, originX, originY);
    }
};
```

**4. Batch Rendering - Use Context for Performance**

```cpp
class BulletManager {
public:
    void drawAllBullets(pixelroot32::graphics::Renderer& renderer) {
        // Set context once for all bullets (same palette slot)
        renderer.setSpritePaletteSlotContext(1);  // Use fire palette for all bullets
        
        for (auto& bullet : bullets) {
            renderer.drawSprite(bulletSprite, bullet.x, bullet.y, 0, false);  // paletteSlot ignored
        }
        
        // Reset context (optional, good practice)
        renderer.setSpritePaletteSlotContext(0xFF);  // 0xFF = inactive
    }
};
```

**5. Comments and Documentation**

```cpp
// Background palette slot assignments:
// Slot 0: Default ground tiles (PR32 palette)
// Slot 1: Water tiles (NES palette - blue tones)
// Slot 2: Underground tiles (GB palette - green tones)
// Slot 3: Lava tiles (custom red palette)
// Slots 4-7: Reserved for future use

// Sprite palette slot assignments:
// Slot 0: Player character (PR32 palette)
// Slot 1: Fire enemies (NES palette)
// Slot 2: Ice enemies (GBC palette)  
// Slot 3: Boss enemies (PICO8 palette)
// Slot 4: Custom player palette (power-up form)
// Slot 5: Custom fire palette (enhanced fire enemies)
// Slots 6-7: Reserved for future use

class PaletteManager {
private:
    bool isInitialized = false;
    
public:
    void initializeGamePalettes() {
        if (isInitialized) return;  // Prevent double initialization
        
        // Initialize slot banks
        pixelroot32::graphics::initBackgroundPaletteSlots();
        pixelroot32::graphics::initSpritePaletteSlots();
        
        // Setup standard palette assignments
        setupBackgroundPalettes();
        setupSpritePalettes();
        
        isInitialized = true;
    }
    
private:
    void setupBackgroundPalettes() {
        // Configure background palette slots for different level areas
        pixelroot32::graphics::setBackgroundPaletteSlot(0, pixelroot32::graphics::PaletteType::PR32);
        pixelroot32::graphics::setBackgroundPaletteSlot(1, pixelroot32::graphics::PaletteType::NES);
        pixelroot32::graphics::setBackgroundPaletteSlot(2, pixelroot32::graphics::PaletteType::GB);
    }
    
    void setupSpritePalettes() {
        // Configure sprite palette slots for different character types
        pixelroot32::graphics::setSpritePaletteSlot(0, pixelroot32::graphics::PaletteType::PR32);
        pixelroot32::graphics::setSpritePaletteSlot(1, pixelroot32::graphics::PaletteType::NES);
        pixelroot32::graphics::setSpritePaletteSlot(2, pixelroot32::graphics::PaletteType::GBC);
    }
};
```

#### Best Practices

1. **Document Slot Assignments**: Always comment which slots are used for what purpose
2. **Initialize Early**: Call `init*PaletteSlots()` during scene initialization
3. **Use Context for Batching**: Set context once when drawing many sprites with same palette
4. **Fallback Handling**: `get*PaletteSlot()` never returns `nullptr` - always falls back to slot 0
5. **Custom Palettes**: Store custom palettes as `static const` arrays for flash storage
6. **Backward Compatibility**: Legacy `drawSprite(sprite, x, y, flipX)` calls use slot 0 automatically

---

PixelRoot32 Game Engine aims to remain simple, explicit, and predictable, prioritizing clarity over abstraction and control over convenience.
