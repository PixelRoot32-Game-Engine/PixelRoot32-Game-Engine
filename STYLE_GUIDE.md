# PixelRoot32 Game Engine

PixelRoot32 is a lightweight 2D game engine designed for ESP32-based systems.  
It focuses on simplicity, deterministic behavior, and low memory usage, making it suitable for embedded environments and small-scale games.

---

## 📐 Coding Style Guide

PixelRoot32 follows a strict set of conventions to ensure consistency, readability, and long-term maintainability of the engine.

### Language

- C++17
- Avoid RTTI and exceptions in performance-critical runtime code
- Prefer deterministic and explicit control flow

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

Example usage in a game project:

class BallActor : public pixelroot32::core::Actor {
    ...
};

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

- Public headers must not use `using namespace`
- Public headers must always reference fully-qualified names
- In internal implementation files (`.cpp`), namespace aliases are preferred

Recommended internal alias:

namespace pr32 = pixelroot32;

The use of `using namespace pixelroot32::...` is discouraged even internally, except in very small, localized implementation files.

---

## 📦 Library Usage Expectations

- Users are expected to include headers only from `include/`
- Users should reference engine types via fully-qualified namespaces
- The engine does not pollute the global namespace

---

## 📚 Additional Documentation

The following documents are recommended as part of the project:

- [API_REFERENCE.md](API_REFERENCE.md) — Engine API reference (Godot-style)
- [CONTRIBUTING.md](CONTRIBUTING.md) — Contribution guidelines

---

## 🚀 Best Practices & Optimization

These guidelines are derived from practical implementation in `examples/GeometryJump`, `examples/BrickBreaker`, and `examples/Pong`.

### 💾 Memory & Resources

- **Object Pooling**: Pre-allocate all game objects (obstacles, particles, enemies) during `init()`.
  - *Pattern*: Use fixed-size arrays (e.g., `Particle particles[50]`) and flags (`isActive`) instead of `std::vector` with `push_back`/`erase`.
- **Zero Runtime Allocation**: Never use `new` or `malloc` inside the game loop (`update` or `draw`).
- **String Handling**: Avoid `std::string` in `update()`/`draw()`. Use `snprintf` with stack-allocated `char` buffers for UI text.

### ⚡ Performance (ESP32 Focus)

- **Inlining**:
  - Define trivial accessors (e.g., `getHitBox`, `getX`) in the header (`.h`) to allow compiler inlining.
  - Keep heavy implementation logic in `.cpp`.
- **Fast Randomness**: `std::rand()` is slow and uses division. Use lightweight PRNGs (like Xorshift) for visual effects (particles).
- **Collision Detection**: Use simple AABB (Axis-Aligned Bounding Box) checks first. Use Collision Layers (`GameLayers.h`) to avoid checking unnecessary pairs.

### 🏗️ Code Architecture

- **Tuning Constants**: Extract gameplay values (gravity, speed, dimensions) into a dedicated `GameConstants.h`. This allows designers to tweak the game without touching logic code.
- **State Management**: Implement a `reset()` method for Actors to reuse them after "Game Over", rather than destroying and recreating the scene.
- **Component Pattern**: Inherit from `PhysicsActor` for moving objects and `Actor` for static ones.

### 🎮 Game Feel & Logic

- **Frame-Rate Independence**: Always multiply movement by `deltaTime`.
  - *Example*: `x += speed * (deltaTime * 0.001f);`
- **Logic/Visual Decoupling**: For infinite runners, keep logic progression (obstacle spacing) constant in time, even if visual speed increases.
- **Snappy Controls**: For fast-paced games, prefer higher gravity and jump forces to reduce "floatiness".

---

### 🎨 Sprite & Graphics Guidelines

- **1bpp Sprites**: Define sprite bitmaps as `static const uint16_t` arrays, one row per element. Use bit `0` as the leftmost pixel and bit (`width - 1`) as the rightmost pixel.
- **Sprite Descriptors**: Wrap raw bitmaps in `pixelroot32::graphics::Sprite` or `MultiSprite` descriptors and pass them to `Renderer::drawSprite` / `Renderer::drawMultiSprite`.
- **No Bit Logic in Actors**: Actors should never iterate bits or draw individual pixels. They only select the appropriate sprite (or layered sprite) and call the renderer.
- **Layered Sprites**: For multi-color sprites, compose multiple 1bpp `SpriteLayer` entries instead of introducing new bitmap formats. Keep layer data `static const` to allow storage in flash.
- **Integer-Only Rendering**: Sprite rendering must remain integer-only and avoid dynamic allocations to stay friendly to ESP32 constraints.

---

PixelRoot32 Game Engine aims to remain simple, explicit, and predictable, prioritizing clarity over abstraction and control over convenience.
