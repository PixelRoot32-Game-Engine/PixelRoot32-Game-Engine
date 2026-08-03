# Tools

PixelRoot32 ships optional **workflow tools** alongside the open-source engine. They are not required to build games, but they speed up asset prep (sprites, tilemaps).

> **Note:** The [PixelRoot32 Game Engine](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine) remains **free and open source**. The tools below are separate products intended to streamline content creation.

## Sprite Compiler (free)

Converts **PNG** images into **C headers** with sprite data (layered 1bpp, 2bpp, 4bpp, grid selection).

**Two ways to use it:**

- **Tool Suite (recommended)**: Integrated as a native GUI module inside the Tool Suite. The actual compilation uses an external `pr32-sprite-compiler` CLI (bundled with the Tool Suite or available on `PATH`).
- **Standalone CLI**: Open-source Python repo (`python main.py …`) — ideal for CI scripts and build-automation without the full Tool Suite.

- [Overview](/tools/sprite-compiler/overview)
- [Installation](/tools/sprite-compiler/installation)
- [Usage guide](/tools/sprite-compiler/usage-guide)
- [Advanced features](/tools/sprite-compiler/advanced-features)

---

## Tool Suite (premium)

::: tip Premium Tool Suite

The **Tool Suite** adds advanced editor modules. Licensing and downloads are handled on the [PixelRoot32](https://pixelroot32.com) site.

:::

### Tilemap Editor (module 1)

Available - visual editor for multi-layer tilemaps, tilesets, animations, attributes, and **C++ export** aligned with the engine. Ships as a native desktop app (C++17 / SDL2 / ImGui) inside the Tool Suite.

- [Overview](/tools/tilemap-editor/overview)
- [Quick start](/tools/tilemap-editor/quick-start)
- [Installation](/tools/tilemap-editor/installation)
- [Usage guide](/tools/tilemap-editor/usage-guide)
- [Advanced guide](/tools/tilemap-editor/advanced-guide)
- [Technical reference](/tools/tilemap-editor/technical-reference)

### Music Editor (module 2)

Upcoming - pattern-based tracker with multi-channel support for PixelRoot32 APU (planned).

---

## Summary

| Tool | Type | Status |
|------|------|--------|
| Sprite Compiler | Free / standalone | Available |
| Tilemap Editor | Premium (suite) | Available |
| Music Editor | Premium (suite) | Upcoming |

**Engine docs:** [Graphics Techniques](../guide/graphics-techniques) · [Rendering](../guide/rendering) · [Tile animation (architecture)](../architecture/tile-animation)
