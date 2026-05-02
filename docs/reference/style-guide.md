# Documentation Index - PixelRoot32

> Modular guides for coding, gameplay, graphics, memory, and performance.

---

## 🎨 Coding & Style

| Document | Purpose | Lines |
|----------|---------|-------|
| [engine-philosophy.md](../philosophy/engine-philosophy.md) | Why this engine exists | ~40 |
| [coding-style.md](./coding-style.md) | C++ conventions, namespaces, naming | ~120 |

---

## 🎮 Development

| Document | Purpose | Lines |
|----------|---------|-------|
| [gameplay-guidelines.md](../guide/gameplay-guidelines.md) | Game feel, deltaTime, slopes, anti-patterns | ~100 |
| [ui-guidelines.md](./ui-guidelines.md) | Layouts, panels, navigation, HUDs | ~100 |
| [graphics-guidelines.md](./graphics-guidelines.md) | Sprites, tilemaps, palettes, animation | ~150 |
| [memory-guidelines.md](./memory-guidelines.md) | Pools, zero allocation, strings | ~80 |

---

## ⚡ Performance & Architecture

| Document | Purpose | Lines |
|----------|---------|-------|
| [performance/esp32-performance.md](./performance/esp32-performance.md) | Hot paths, build profiles, optimization | ~150 |
| [ARCHITECTURE.md](../architecture/overview.md) | Layer overview, navigation hub | ~170 |
| [architecture/memory-system.md](../architecture/memory-system.md) | C++17 memory deep dive | ~690 |
| [platform-compatibility.md](./platform-compatibility.md) | Hardware matrix, ESP32 variants | ~490 |

---

## 📚 API Reference

| Document | Content |
|----------|---------|
| [API Reference](../api/index.md) | Complete API (Godot-style) |
| [api/*.md](../api/) | Per-module API docs |

---

## 🚀 Quick Start

**New to the engine?** Read in this order:

1. [engine-philosophy.md](../philosophy/engine-philosophy.md) - Understand the "why"
2. [coding-style.md](./coding-style.md) - Write consistent code
3. [gameplay-guidelines.md](../guide/gameplay-guidelines.md) - Avoid common mistakes
4. [memory-guidelines.md](./memory-guidelines.md) - Embedded constraints
5. [performance/esp32-performance.md](./performance/esp32-performance.md) - Optimize

---

*PixelRoot32: Clarity over abstraction, control over convenience.*
