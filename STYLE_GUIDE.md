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

PixelRoot32 Game Engine aims to remain simple, explicit, and predictable, prioritizing clarity over abstraction and control over convenience.
