---
title: "Technical Reference"
description: "Engine limits, binary format v9, project structure, C++ export, data formats, and compatibility"
---

# Tilemap Editor - Technical Reference

**Level**: ⭐⭐⭐ Advanced

---

> **Quick Index**
> - [Engine Limits](#engine-limits)
> - [File Formats](#file-formats)
> - [Project Structure](#project-structure)
> - [Application Architecture](#application-architecture)
> - [C++ Export](#c-export)
> - [Data Formats](#data-formats)
> - [Compatibility](#compatibility)
> - [Glossary](#glossary)

---

## Engine Limits

### ⚠️ Limits Table

| Parameter | Limit | Description |
|-----------|--------|-------------|
| **MAX_TILE_WIDTH** | 32 px | Maximum tile size |
| **MAX_TILE_HEIGHT** | 32 px | Maximum tile size |
| **MAX_MAP_DIM** | 255 tiles | Maximum map dimension |
| **MAX_UNIQUE_TILES** | 256 | Unique tiles per project |
| **MAX_LAYERS** | 8 | Layers per scene |
| **MAX_SCREEN_WIDTH** | 320 px | Screen width |
| **MAX_SCREEN_HEIGHT** | 240 px | Screen height |
| **MAX_ANIMATIONS** | 64 | Animations per scene |
| **MAX_ANIMATION_FRAMES** | 256 | Total frames |
| **MIN_FRAME_COUNT** | 1 | Minimum frames |
| **MAX_FRAME_COUNT** | 255 | Maximum frames |
| **MIN_FRAME_DURATION** | 1 | Minimum duration (ticks) |
| **MAX_FRAME_DURATION** | 255 | Maximum duration (ticks) |

> **Enforcement**: Tile size, layer count, and animation limits are validated at edit time (project creation, add-layer, add-animation). At save, animations and player spawn positions are re-validated.

### Screen Resolutions

**Landscape**:
- Maximum: 320×240 px
- Aspect ratio: 4:3

**Portrait**:
- Maximum: 240×320 px
- Aspect ratio: 3:4

---

## File Formats

### Project File

The editor stores projects in a **single binary format** (`.pr32scene.bin`, version **9**). There is **no human-readable JSON writer**; the JSON object used inside the serializer only carries project metadata and per-tile attributes embedded in the binary container.

The **"Use Binary Format"** preference in **File → Preferences** only affects the **file extension** written to disk (`.pr32scene.bin` vs `.pr32scene`). The on-disk content is the same binary v9 container either way.

| Extension | Content |
|-----------|---------|
| `.pr32scene.bin` | Binary v9 container (default) |
| `.pr32scene` | Legacy extension accepted for open; still binary v9 when saved |

### Binary Format

The `.pr32scene.bin` format is a **big-endian** binary container (current version **9**). Layout:

| Field | Size | Notes |
|-------|------|-------|
| **MAGIC** | 4 B | `PR32` (big-endian `0x50523332`) |
| **VERSION** | u16 | Current: **9** |
| **FLAGS** | u16 | Bit 0: `COMPRESSION_ZLIB` |
| **tileSize** | u8 | Project default tile size in px |
| **reserved** | 3 B | Padding |
| **Metadata** | u32 len + JSON | Project metadata (JSON embedded) |
| **Tilesets** | u16 count + entries | Per-tileset data, including each sheet's own `tileWidth`/`tileHeight` |
| **Scenes** | u16 count + entries | Per-scene data; optional zlib-compressed layers |

The serializer targets byte-for-byte compatibility with the engine's binary project format (big-endian layout). Layer payloads can be zlib-compressed (flag bit 0).

#### Scene record, by version

Every block below sits **inside** a scene entry, after `width`/`height` and
**before** `layerCount`. Each version appends to the previous one, so the order
is the order they were added:

| Version | Block | Size |
|---------|-------|------|
| **v6+** | `hasSpawn` u8 + optional player spawn `x`/`y` u16 | 1 or 5 B |
| **v7+** | `roomCount` u16 + `roomCount` × 16-byte room records | 2 + 16·n B |
| **v8+** | `projection` u8 + `cellWidth` u16 + `cellHeight` u16 | 5 B, fixed |
| **v9+** | `originAuthored` u8 + `originX` i16 + `originY` i16 | 5 B, fixed |

A room record is `originCol`, `originRow`, `cols`, `rows` (u16 each) followed by
four u16 connection targets in `Up, Down, Left, Right` order, where `0xFFFF`
means "no connection".

::: warning Reading these blocks is not optional

A reader that skips an unknown block **desynchronizes the stream**. `layerCount`
and the compressed layer payload that follow would then be parsed from the wrong
offset, which does not reliably fail — it can produce a plausible, corrupt
project. The editor refuses the whole file instead. Any third-party parser
should do the same.

:::

#### Why projection is per scene

`projection`, `cellWidth` and `cellHeight` live in the **scene** record rather
than in the header, even though the header still has three spare reserved bytes
that would have held them for less. One project can pair an isometric dungeon
with an orthogonal menu, and a header field would have made that impossible.

`cellWidth` and `cellHeight` of `0` mean **inherit the project's `tileSize`**,
which is what every pre-v8 file meant implicitly. That is why legacy files load
correctly: their defaults — orthogonal, both extents inheriting — are exactly
what they always were.

An unrecognized `projection` ordinal comes from a file written by a **newer**
editor. It falls back to orthogonal, which keeps the scene loadable and visibly
wrong rather than rejecting a file whose tiles are all still readable.

### Size & Performance

The binary format keeps projects compact and fast to load (single-format; there is no alternative JSON serialization to compare against).

---

## Project Structure

### File Structure

```
my_project/
├── my_project.pr32scene.bin  # Project file (binary v9, default)
├── tile_flag_rules.json   # Custom rules (optional)
└── assets/
    └── tilesets/
        ├── tileset1.png
        └── tileset2.png
```

> The file is stored as `.pr32scene.bin` by default. With **"Use Binary Format"** disabled it is written as `.pr32scene` instead (still binary v9 content). Only the extension changes.

### Exported Files

```
output/
├── my_scene.h              # Declarations + animations + palettes (multi-palette)
├── my_scene.cpp            # Data (palettes, tiles, indices)
└── {namespace}_tilemap_palette.h  # Shared palette (single palette mode)
```

> Animations are embedded in `my_scene.h` alongside their layer (`<LAYER>_TILE_ANIMATIONS[]`). There are no separate `*_animations.h/.cpp` files.

---

## Application Architecture

The Tilemap Editor is a **native desktop application** built with **C++17** on **SDL2 + ImGui** + **OpenGL 3.3**. There is no Python/Tkinter runtime. The editor is distributed as a compiled binary within the Tool Suite.

### Core Services

The editor's logic is organized into the following internal services (C++):

| Service | File (source) | Responsibility |
|---------|---------------|----------------|
| **ProjectService** | `tools/tilemap_module/project_service.{h,cpp}` | Create / load / save projects, validate project name, manage tile flag rules |
| **BinarySerializer** | `tools/tilemap_module/binary_serializer.{h,cpp}` | Read/write `.pr32scene.bin` (v9, big-endian, optional zlib compression) |
| **HistoryManager** | `tools/tilemap_module/history_manager.{h,cpp}` | Bounded (100 entries) undo/redo stack with optional compression |
| **AnimationValidator** | `tools/tilemap_module/core/animation_validator.{h,cpp}` | Validate animations against engine limits (bounds, overlap, count, duration) |
| **ExporterService** | `tools/tilemap_module/exporter_service.{h,cpp}` | Gate C++ export behind a valid license; coordinate the native export pipeline |
| **ExportOrchestrator** | `tools/tilemap_module/native_export/export_orchestrator.{h,cpp}` | Multi-palette detection, image processing, palette analysis, tile dedup, C++ code generation |
| **AutosaveService** | `tools/tilemap_module/autosave_service.{h,cpp}` | Interval-based autosave |
| **ToolManager** | `tools/tilemap_module/tool_manager.{h,cpp}` | Active tool registry (Brush, Eraser, Rectangle, Pan, Pipette, Attribute, Anim) |

> **Note:** These services are internal C++ modules compiled into the Tool Suite binary. They are **not** a public Python API and cannot be imported by external scripts. Automation should use the file formats described below or the external `pr32-sprite-compiler` CLI (Sprite Compiler module).

### Runtime Environment

| Item | Detail |
|------|--------|
| **Language / standard** | C++17 |
| **GUI framework** | Dear ImGui (v1.92.8) |
| **Windowing / GPU** | SDL2 + OpenGL 3.3 (single window, DockSpace layout) |
| **JSON** | nlohmann/json v3.11.3 |
| **SVG rasterization** | lunasvg v3.1.0 |
| **Native file pickers** | portable-file-dialogs |
| **Compression** | zlib (`.pr32scene.bin` layer payloads) |
| **Crypto** | OpenSSL (SHA-256 checksum + AES-256-CBC) |
| **Build system** | CMake ≥ 3.20 |

---

## C++ Export

### Requirements

⚠️ **Important**: C++ export **requires a valid license** (Ed25519 v3 key, machine-bound). See **[License & Activation](/tools/tilemap-editor/license-and-activation)** for full details.

- Without license, the **Upgrade Required** dialog appears when attempting to export
- Other features work without license

### Export Options

| Option | Description | Recommended |
|--------|-------------|-------------|
| **C++ Namespace** | Namespace for code | Project name |
| **Color Depth (BPP)** | Read-only; auto-detected (1/2/4) | Auto-detect |
| **Store in Flash (ESP32)** | Save to PROGMEM | ✅ Always |
| **Legacy Format** | Without Flash attributes | Compatibility only |

### Export Mode

| Mode | Trigger | Generated |
|------|---------|-----------|
| **Single Palette** | All layers use P0 | Shared palette |
| **Multi-Palette** | Any layer P1-P7 | Per-slot palettes |

### Generated Files

#### Single Palette

```cpp
// level1.h
static const uint16_t TILEMAP_PALETTE_DATA[] = { /* RGB565 */ };
extern pixelroot32::graphics::TileMap4bpp layer_foreground;

// level1.cpp
static const pixelroot32::graphics::Sprite4bpp TILESET_SPRITES[] = { /* tiles */ };
static const uint8_t LAYER_FOREGROUND_INDICES[] = { /* indices */ };
```

> The palette array is declared in the header; tiles and index data live in the `.cpp`. The layer struct is `TileMap`, `TileMap2bpp`, or `TileMap4bpp` depending on the auto-detected BPP.

#### Multi-Palette

```cpp
// level1.h
static const uint16_t PLATFORMS_PALETTE[16] = { /* RGB565 */ };
static const uint16_t STAIRS_PALETTE[16] = { /* RGB565 */ };

// level1.cpp
void init() {
    setBackgroundCustomPaletteSlot(1, PLATFORMS_PALETTE);
    setBackgroundCustomPaletteSlot(2, STAIRS_PALETTE);
}
```

#### Isometric Projection

Emitted only when the scene's projection is isometric. An orthogonal scene's
projection is the identity, so naming one would be dead weight in flash on
every existing target — an orthogonal export is byte-for-byte what it always
was.

```cpp
// level1.h
#include <math/Projection.h>

inline constexpr uint8_t TILE_WIDTH  = 32;
inline constexpr uint8_t TILE_HEIGHT = 16;

inline constexpr pixelroot32::math::ProjectionSpec ISO_PROJECTION{
    120, 88,    // where cell (0,0)'s diamond CENTRE lands
     16,  8,    // +1 cellX steps right and down
    -16,  8};   // +1 cellY steps left and down

extern const uint8_t TILESET_FOOT_Y[TILESET_TILE_COUNT];
```

::: warning TILE_WIDTH and TILE_HEIGHT are the cell STRIDE

They are not the tileset's bitmap extents. An isometric tile is usually
**taller** than its cell — a 32×40 wall standing on a 32×16 diamond — and the
difference is exactly what `TILESET_FOOT_Y` records.

In an orthogonal export the two happen to coincide, which is why the
distinction never came up before.

:::

`TILESET_FOOT_Y[i]` is the bitmap row of tile `i` that lands on its cell's
diamond centre. It is parallel to `TILESET_SPRITES`, and it is what lets the
renderer stand a tall sprite on a short cell. The orthogonal export has no
analogue: its art fills its cell, so there is no anchor to record.

##### The four static_asserts

The header states its own invariants rather than trusting the exporter, because
a plain aggregate cannot assert its own initializers under `-fno-exceptions`:

| Assert | What breaks if it fails |
|--------|-------------------------|
| `projectionSpecIsValid(ISO_PROJECTION, MAP_WIDTH, MAP_HEIGHT)` | The basis exceeds `Scalar`'s fixed-point range for this map size |
| `projectionDet(ISO_PROJECTION) == N` | The determinant stopped being a power of two, so `screenToCell` loses its strength reduction to a shift |
| `rowMajorIsPainterOrder(ISO_PROJECTION)` | Row-major iteration is no longer back-to-front, and the projected `drawTileMap` has **no depth sort** — walls would draw in front of what stands behind them |

These fire at **your** compile, not at export time. That is deliberate: a
projection the engine cannot use should stop the build that would ship it.

#### Room Metadata

Optional. Emitted only when the project defines rooms; a project without them
exports exactly as before, so this is additive to the format, not a new version.

```cpp
// level1.h
static const pixelroot32::gameplay::RoomData LEVEL1_ROOMS[] = {
    // originCol, originRow, cols, rows, { Up, Down, Left, Right }
    {  0, 0, 20, 15, { 0xFFFF, 0xFFFF, 0xFFFF,      1 } },
    { 20, 0, 20, 15, { 0xFFFF, 0xFFFF,      0, 0xFFFF } },
};

static const pixelroot32::gameplay::RoomLayer LEVEL1_ROOM_LAYER = {
    LEVEL1_ROOMS, 2, 16, 16   // rooms, roomCount, tileWidth, tileHeight
};
```

> Both arrays are `static const` so the linker parks them in flash — a room
> layer costs **0 bytes of SRAM**. The structs are declared in
> `include/gameplay/RoomLayout.h`, behind `PIXELROOT32_ENABLE_GAMEPLAY_ROOM`,
> so the generated header must guard the block with that flag.

### Engine Integration

**Single Palette**:
```cpp
#include "level1.h"

level1::init();
renderer.drawTileMap(level1::layer_foreground, x, y);
```

**Multi-Palette**:
```cpp
#include "level1.h"

level1::init();  // Registers palettes
renderer.drawTileMap(level1::background, 0, 0);
renderer.drawTileMap(level1::platforms,  0, 0);
```

**Attributes/Flags**:
```cpp
// Query attributes
const char* type = level1::get_tile_attribute(0, x, y, "type");

// Query flags
uint8_t flags = level1::getTileFlags(0, x, y);  // layer index, x, y
if (flags & TILE_SOLID) { /* collision */ }
```

**Animations**:
```cpp
level1::getForegroundAnimManager().step();
renderer.drawTileMap(level1::layer_foreground, x, y);
```

**Rooms**:
```cpp
#include "level1.h"

// In Scene::init() — rects and connections both come from the export.
pixelroot32::gameplay::RoomGraph<8> rooms;
if (pixelroot32::gameplay::buildRoomGraph(level1::LEVEL1_ROOM_LAYER, rooms) > 0) {
    setRoomGraph(&rooms);
    rooms.enterRoom(0, &camera);
}
```

> `buildRoomGraph()` returns how many rooms it added. Fewer than `roomCount`
> means the graph's capacity `N` truncated the layer; `0` means the layer was
> rejected (see [Room Layer](#room-layer)). Check it — `enterRoom()` on an
> empty graph is a silent no-op, so a rejected layer otherwise leaves the scene
> running with no camera bounds and no current room.

> Each animated layer exposes a `<LayerName>AnimManager()`. A legacy `getAnimManager()` alias maps to the Details layer when present.

---

## Data Formats

### Palette

- **Format**: RGB565
- **Size**: 16 colors max
- **Index 0**: Transparent (multi-bpp)

### Tiles

| BPP | Per Row | Colors |
|-----|--------|--------|
| 1 bpp | 1 byte | 2 |
| 2 bpp | 2 bytes | 4 |
| 4 bpp | 4 bytes | 16 |

### Index Map

- 1 byte per cell (`uint8_t`)
- Value -1 (editor) = Index 0 (export) = Empty

### Room Layer

Optional metadata describing a graph of connected rooms inside a single scene.
Coordinates are in **tiles**, not world units — the engine multiplies by the
layer's tile size when it builds the runtime graph.

**`RoomData`** — one room, 16 bytes on every target:

| Field | Type | Meaning |
|-------|------|---------|
| `originCol` / `originRow` | `uint16_t` | Tile coordinates of the room's top-left corner |
| `cols` / `rows` | `uint16_t` | Room size in tiles |
| `connections[4]` | `uint16_t` | Target room index per direction, indexed `0=Up, 1=Down, 2=Left, 3=Right`. `0xFFFF` = wall |

**`RoomLayer`** — the array header:

| Field | Type | Meaning |
|-------|------|---------|
| `rooms` | `const RoomData*` | Pointer to the exported array |
| `roomCount` | `uint16_t` | Number of entries |
| `tileWidth` / `tileHeight` | `uint8_t` | Tile size in world units, both `>= 1` |

**Emitter rules**:

- Connection targets are indices **into this same array**, so a room may
  reference one declared after it — the engine resolves connections in a
  second pass.
- Connections are directed. A two-way door needs both halves
  (`A.Right = B` **and** `B.Left = A`).
- A room's far edge (`(originCol + cols) * tileWidth`, likewise for rows) must
  stay at or below **32767**. `Scalar` is `Fixed16` on FPU-less targets and
  would wrap past that. The engine rejects the whole layer if any room
  exceeds it, rather than building a partially-correct map — including rooms
  the consuming graph's capacity would have truncated away. A broken export
  fails closed.
- Emit at most 4 connections per room. There is one slot per cardinal
  direction; diagonal or multi-door connections are out of scope for v1.

### BPP Auto-Detection

| Real Colors (excl. transparency) | BPP | Maximum |
|------------|-----|----------|
| ≤ 1 | 1 bpp | 2 |
| 2-4 total slots (incl. optional transparency) | 2 bpp | 4 |
| otherwise | 4 bpp | 16 |

> `totalSlots = realColors + (hasTransparency ? 1 : 0)`. 1bpp requires ≤1 real color; 2bpp when total slots ≤ 4; otherwise 4bpp.

---

## Compatibility

### Runtime

The Tilemap Editor is distributed as a **pre-built native binary** (part of the Tool Suite). No Python, Tkinter, or Pillow installation is required.

| Requirement | Detail |
|-------------|--------|
| **Operating system** | Windows, Linux, macOS (per release) |
| **GPU / Windowing** | OpenGL 3.3 capable GPU; SDL2-based window |
| **Storage** | ~tens of MB for the app + generated assets |
| **License** | A valid Tool Suite license is required for **C++ export** |

### Target Hardware

- **ESP32** (PixelRoot32 engine)
- Flash: 4MB minimum recommended
- RAM: 520KB minimum

---

## Glossary

| Term | Definition |
|------|------------|
| **ENGINE_LIMITS** | Engine limit constants |
| **ProjectModel** | Project class |
| **SceneModel** | Scene model |
| **LayerModel** | Layer model |
| **TileAnimation** | Tile animation model |
| **RGB565** | Color format (5+6+5 bits) |
| **BPP** | Bits per pixel |
| **PROGMEM** | ESP32 flash storage |
| **Sprite4bpp** | 4bpp sprite |
| **TileMap** | Exported tilemap structure |

---

## Related Guides

- [Quick Start](/tools/tilemap-editor/quick-start) - 5 minute guide
- [Usage Guide](/tools/tilemap-editor/usage-guide) - Essential features
- [Advanced Guide](/tools/tilemap-editor/advanced-guide) - Advanced features
- [Isometric Guide](/tools/tilemap-editor/isometric-guide) - Projection, cell stride, isometric export