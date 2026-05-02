# Coding Style Guide - PixelRoot32

**Scope:** C++ coding conventions and style rules.  
**For other topics:** See [Memory Guidelines](./MEMORY_GUIDELINES.md), [Gameplay Guidelines](../guide/GAMEPLAY_GUIDELINES.md), [UI Guidelines](./UI_GUIDELINES.md), [Graphics Guidelines](./GRAPHICS_GUIDELINES.md), [ESP32 Performance](./performance/ESP32_PERFORMANCE.md).

---

## 🎯 Design Principles

These principles guide all implementation decisions:

- **Deterministic over convenient**: Explicit control flow, no hidden allocations
- **Zero allocation at runtime**: Pre-allocate during init, pool objects
- **Data-oriented where possible**: Prefer contiguous arrays, cache-friendly layouts
- **No exceptions, no RTTI**: Error handling via return codes
- **Embedded-first**: Optimize for ESP32, portable to native

---

## 📐 Language & Standards

- **C++17** minimum
- **No exceptions** - compile with `-fno-exceptions`
- **No RTTI** - compile with `-fno-rtti`
- Prefer deterministic and explicit control flow

---

## 🔧 Modern C++ Features (C++17)

| Feature | Usage | Avoid In |
|---------|-------|----------|
| `std::unique_ptr` | Init-time ownership | Hot paths |
| `std::string_view` | Non-owning strings | - |
| `std::optional` | May-not-exist values | Hot paths |
| `[[nodiscard]]` | Must-check returns | - |
| `constexpr` | Compile-time constants | - |

**Example:**
```cpp
// ✅ Smart pointer for ownership
auto player = std::make_unique<PlayerActor>(x, y);
scene.addEntity(player.get());  // Non-owning access

// ✅ String view for parameters
void setName(std::string_view name);  // No copy

// ⚠️ Optional OK for init, avoid in update()
std::optional<Config> loadConfig(const char* path);
```

---

## 📁 File Structure

| Extension | Purpose |
|-----------|---------|
| `.h` | Interfaces, public types, trivial inline code |
| `.cpp` | Implementations, heavy logic |

### Include Rules

```
src/        → Can include anything
include/    → Can include from src/ (implementation details)
            → Must NOT expose internal headers
```

**Critical rule:** Source files in `src/` must **never** include headers from `include/`.

---

## 🏷️ Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes/Structs | PascalCase | `PhysicsActor`, `SpriteLayer` |
| Methods/Functions | camelCase | `update()`, `drawSprite()` |
| Variables/Members | camelCase | `position`, `velocity` |
| Constants | UPPER_SNAKE | `MAX_ENTITIES` |
| Namespaces | lowercase | `pixelroot32::graphics` |

### Member Prefix (Optional)

Use `m_` or `this->` when parameter shadows member:

```cpp
// ✅ Option A: m_ prefix
void setX(int x) { m_x = x; }

// ✅ Option B: this-> explicit
void setX(int x) { this->x = x; }

// ❌ Avoid: shadowing bug
void setX(int x) { x = x; }  // Bug!
```

**No Hungarian notation** (e.g., `strName`, `nCount`).

---

## 🧩 Namespace Design

### Root Namespace

All symbols live under `pixelroot32`.

### Public API Namespaces

```cpp
pixelroot32::core        // Engine, Scene, Actor
pixelroot32::graphics    // Renderer, sprites
pixelroot32::input       // Input handling
pixelroot32::physics     // Collision system
pixelroot32::math        // Scalar, Vector2
```

### Internal Namespaces (Non-API)

```cpp
pixelroot32::platform    // Hardware abstraction
pixelroot32::internal    // Engine internals
pixelroot32::detail      // Implementation details
```

**Rules for internal:**
- May change without notice
- Must not be included by user projects
- Must not be exposed through `include/` headers

---

## 📦 Namespace Usage Rules

**Golden Rule:** *Never in headers. Rarely in .cpp. Only in small scopes.*

### Headers (`.h`)

```cpp
// ✅ Fully qualified
pixelroot32::graphics::Renderer renderer;

// ✅ Alias inside function only
void MyClass::draw() {
    namespace gfx = pixelroot32::graphics;
    gfx::Renderer renderer;
}

// ❌ NEVER in headers
using namespace pixelroot32::graphics;
```

### Implementation (`.cpp`)

| Approach | Use Case | Example |
|----------|----------|---------|
| Subsystem alias | Large modules | `namespace gfx = pixelroot32::graphics;` |
| Root alias | Multiple refs | `namespace pr32 = pixelroot32;` |
| Selective using | Specific symbols | `using pixelroot32::graphics::Renderer;` |
| `using namespace` | Tests only, small scopes (<20 lines) | Inside functions |

**Acceptable only in:**
- Unit tests (`test_*.cpp`)
- Function scopes (not file/namespace scope)
- Prototypes (<50 lines)

**Prohibited in:**
- Engine core `.cpp` files
- Namespace scope
- Public headers

---

## 📚 Class Layout

Order inside classes:

```cpp
class Example {
public:      // 1. Public interface first
    void publicMethod();
    int publicMember;

protected:   // 2. Protected for inheritance
    void protectedMethod();

private:     // 3. Implementation last
    void privateMethod();
    int privateMember;
};
```

---

## 🔗 Related Documentation

| Document | Topic |
|----------|-------|
| [MEMORY_GUIDELINES.md](MEMORY_GUIDELINES.md) | Object pooling, zero allocation |
| [GAMEPLAY_GUIDELINES.md](GAMEPLAY_GUIDELINES.md) | Game feel, deltaTime, slopes |
| [UI_GUIDELINES.md](UI_GUIDELINES.md) | Layouts, panels, navigation |
| [GRAPHICS_GUIDELINES.md](GRAPHICS_GUIDELINES.md) | Sprites, tilemaps, palettes |
| [performance/ESP32_PERFORMANCE.md](performance/ESP32_PERFORMANCE.md) | Hot paths, optimization |
| [Memory system](../architecture/memory-system.md) | C++17 memory deep dive |

---

*PixelRoot32 aims for clarity over abstraction, control over convenience.*
