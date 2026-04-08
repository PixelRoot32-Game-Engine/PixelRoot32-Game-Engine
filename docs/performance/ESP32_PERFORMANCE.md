# ESP32 Performance Guide - PixelRoot32

## 🔥 Hot Path Rules

**Hot paths** = `update()`, `draw()`, collision detection loops, audio callbacks.

**Prohibited in hot paths:**

| Feature | Why | Alternative |
|---------|-----|-------------|
| `std::optional` | Extra branching/code size | Raw pointers or sentinel values |
| Virtual calls | Vtable indirection | Templates or function pointers |
| `new` / `malloc` | Heap fragmentation | Pre-allocated pools |
| `std::vector::push_back` | Potential reallocation | Fixed-size arrays with flags |
| Logging / `log()` | String formatting overhead | Debug-only counters or periodic logging |
| `std::rand()` | Slow, uses division | `math::randomScalar()` (Xorshift) |

---

## ⚡ Performance (ESP32 Focus)

### Inlining

- Define trivial accessors (e.g., `getHitBox`, `getX`) in the header (`.h`) to allow compiler inlining.
- Keep heavy implementation logic in `.cpp`.

### Fast Randomness

`std::rand()` is slow and uses division. Use `math::randomScalar()` or `math::randomRange()` (which use optimized Xorshift algorithms compatible with `Fixed16`) for visual effects.

### Collision Detection

- Use simple AABB (Axis-Aligned Bounding Box) checks first. Use Collision Layers (`GameLayers.h`) to avoid checking unnecessary pairs.
- For very fast projectiles (bullets, lasers), prefer lightweight sweep tests:
  - Represent the projectile as a small `physics::Circle` and call `physics::sweepCircleVsRect(startCircle, endCircle, targetRect, tHit)` against potential targets.
  - Use sweep tests only for the few entities that need them; keep everything else on basic AABB to avoid unnecessary CPU cost.

---

## 💾 Memory & Resources

**📖 For comprehensive C++17 memory management guide, see [Memory Management Guide](../architecture/ARCH_MEMORY_SYSTEM.md)**

### Smart Pointers (C++17)

Use `std::unique_ptr` for **init-time ownership** (Scenes, Actors, UI elements) to automate memory management and document ownership.

- Use `std::make_unique<T>(...)` to create objects during initialization only.
- Pass raw pointers (via `.get()`) to functions that do *not* take ownership (like `addEntity`).
- Use `std::move` only when transferring ownership explicitly.
- ⚠️ **Do not use in hot paths**: `unique_ptr` is for init-time, not runtime game loop.

### Object Pooling

Pre-allocate all game objects (obstacles, particles, enemies) during `init()`.

- Pools are for **runtime** zero-allocation recycling; `unique_ptr` is for **init-time** ownership semantics.
- Pattern: Use fixed-size arrays (e.g., `Particle particles[50]`) and flags (`isActive`) instead of `std::vector` with `push_back`/`erase`.
- Trade-off: Eliminates runtime allocations and fragmentation at the cost of a slightly higher fixed RAM footprint; dimension pools to realistic worst-case usage.

### Zero Runtime Allocation

Never use `new` or `malloc` inside the game loop (`update` or `draw`).

### String Handling

Avoid `std::string` copies. Use `std::string_view` for passing strings. For formatting, use `snprintf` with stack-allocated `char` buffers.

### Scene Arenas (`PIXELROOT32_ENABLE_SCENE_ARENA`)

Use a single pre-allocated buffer per scene for temporary entities or scratch data when you need strict zero-allocation guarantees.

- Trade-off: Very cache-friendly and fragmentation-proof, but the buffer cannot grow at runtime; oversizing wastes RAM, undersizing returns `nullptr` and requires graceful fallback logic.

---

## 🏗️ Build Profiles

PixelRoot32 supports two build profiles for different use cases:

### Embedded Profile (Default)

For ESP32 and resource-constrained hardware:

- **Zero allocation** at runtime
- **No exceptions**, `-fno-exceptions` flag
- **Deterministic** behavior prioritized
- **Modular compilation** to reduce binary size
- **All Hot Path Rules** enforced

### Native Profile (Optional)

For PC simulation and development:

- **Relaxed constraints** for faster iteration
- **Exceptions permitted** if needed for tooling
- **Debug-friendly** features enabled
- **All subsystems** can be compiled in

Use `PLATFORM_NATIVE` flag to switch profiles.

---

## 📊 Recommended Build Profiles

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

---

## 📚 Related Documentation

| Document | Description |
|----------|-------------|
| [Memory Management Guide](../architecture/ARCH_MEMORY_SYSTEM.md) | Complete C++17 memory guide with smart pointers |
| [Platform Compatibility](../PLATFORM_COMPATIBILITY.md) | Hardware matrix and feature support |
| [Architecture Overview](../ARCHITECTURE.md) | Layer architecture and subsystem navigation |
