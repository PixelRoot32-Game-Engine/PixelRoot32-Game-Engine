---
title: "Advanced Guide"
description: "Tile animations, license, multi-palette export, tile attributes, flag rules, and project optimization"
---

# Tilemap Editor - Advanced Guide

**Level**: ⭐⭐ Intermediate | ⭐⭐⭐ Advanced

---

> **Quick Index**
>
> - [Tile Animations](#tile-animations)
> - [License System](#license-system)
> - [Multi-Palette Export](#multi-palette-export)
> - [Tile Attributes](#tile-attributes)
> - [Tile Flag Rules](#tile-flag-rules)
> - [Project Optimization](#project-optimization)

---

## Tile Animations

### ⭐⭐ What are Tile Animations?

**Tile animations** create dynamic, moving tiles that auto-animate in game. System is synchronized with ESP32 engine.

**Features**:

- **Global Sync**: All instances animate simultaneously
- **Engine-Optimized**: Designed for ESP32 memory
- **Real-time Preview**: See animations on canvas
- **Easy Integration**: Auto C++ export

### ⭐⭐ Animation Panel

**Access**: The **Animations** panel is always docked at the bottom of the editor window (part of the default layout). No menu toggles it.

**Components**:

- **Animation List**: All project animations
- **Add/Remove**: Create/delete animations
- **Properties Panel**: Configure settings
- **Preview Panel**: Real-time playback

### ⭐⭐⭐ Creating Animations

**Step 1**: New animation

1. Click **Add** in the Animations panel
2. "Animation N" appears in the list
3. Select to configure

**Step 2**: Configure properties

| Property | Description | Recommended |
|----------|--------------|-------------|
| **Name** | Descriptive name | "Water", "Fire" |
| **Base Tile** | First tile index (assigned via Animation tool, see Step 3) | - |
| **Frame Count** | Number of frames | 2-8 |
| **Frame Duration** | Speed in engine cycles | 8-16 |

**Example**: Water with 4 frames

```
Base Tile: 16
Frame Count: 4 (uses tiles 16,17,18,19)
Frame Duration: 8 ticks
```

**Step 3**: Assign tiles

Select the **Animation** tool from the toolbar, then click the tile on the canvas that should be the animation's base tile. The clicked tile becomes the base tile of the currently selected animation.

### ⭐⭐⭐ Visual Indicators

- Animated tiles show ▶ icon on the canvas
- Indicators visible only on the active layer
- Hover for details

### ⭐⭐⭐ Live Preview

**Toggle**:

1. Click the **▶ Live Preview** button in the toolbar
2. Animations play in real-time on the canvas

**Features**:

- **Synchronized**: Same timer as the engine
- **Optimized**: Only renders visible
- **Frame Accurate**: Exact game frames
- **Multi-layer**: Works across layers

**Controls**:

- **Play / Pause / Stop**: Toggle playback
- Enabled at zoom levels above the Live Preview threshold

### ⭐⭐⭐ Validation

The editor auto-validates:

**Checks**:

- **Tile Bounds**: base_tile + frame_count ≤ tileset
- **Frame Duration**: Valid values (1-255)
- **Memory Constraints**: ESP32 limits (64 animations, 256 frames per scene)

> Animations are capped at 64 per scene; attempting to add more silently stops adding new ones.

### ⭐⭐⭐ Export to C++

Animations auto-export with C++ export — embedded in the scene header alongside the layer definitions (no separate animation file):

```cpp
// scene_name.h (excerpt)
static const pixelroot32::graphics::TileAnimation FOREGROUND_TILE_ANIMATIONS[] = {
    { 16, 4, 8, 0 },  // Water
    { 32, 2, 12, 0 }, // Fire
};
static const uint8_t FOREGROUND_TILE_ANIMATION_COUNT = 2;
```

The initializer order is: `{ baseTile, frameCount, frameDuration, 0 }` (the trailing `0` is a reserved flag field).

**Engine Integration**:

```cpp
#include "level1.h"

void game_loop() {
    level1::init();
    while (game_running) {
        level1::getForegroundAnimManager().step();
        render_tilemap();
    }
}
```

> Each animated layer gets a `<LayerName>AnimManager()` accessor. A legacy alias `getAnimManager()` maps to the Details layer when present.

### ⭐⭐⭐ ESP32 Limits

| Limit | Value |
|--------|-------|
| **Max Animations** | 64 per scene |
| **Max Total Frames** | 256 per scene |
| **Animation Memory** | 4 bytes each |
| **Lookup Table** | 1 byte per tile |

---

## License System

> ⚠️ **Full documentation**: See **[License & Activation](/tools/tilemap-editor/license-and-activation)** for the complete licensing model (key format, fingerprint binding, storage, troubleshooting).

### 🔒 Export License

C++ export **requires a valid license**. Without:

- The **Upgrade Required** dialog appears when attempting to export ("This feature requires an active license")
- Other features work normally

### ⭐⭐ Verify License

**Help → License Info** (or the launcher footer **License Info** button) shows:

- License status (active / not active)
- Masked key (e.g. `PR32-3X-1-****-XXXXXXXX`)
- Product version
- Activation date

### ⭐⭐ Activate License

1. **Help → License Info** or the trial dialog on first launch
2. Enter license key
3. Click **Activate License**

The key is validated against an Ed25519 v3 signature and bound to your machine fingerprint. See the [full license documentation](/tools/tilemap-editor/license-and-activation) for details on fingerprint tolerance and key rotation.

---

## Multi-Palette Export

### 🎨 Concept

**Multi-Palette Mode** allows up to 8 palettes (slots P0-P7):

- Layer-specific color conversion
- Memory optimization
- Artistic flexibility

### ⭐⭐⭐ Assigning Slots

1. Select layer in **LAYERS** panel
2. Use **Palette Slot** control (0-7)
3. List shows: `Background [P0]`, `Platforms [P1]`

**Recommended assignment**:

- **P0**: Backgrounds, shared
- **P1-P3**: Main elements
- **P4-P7**: Secondary

### ⭐⭐⭐ Auto-Detect Mode

| Mode | Trigger |
|------|---------|
| **Single Palette** | All layers use P0 |
| **Multi-Palette** | Any layer uses P1-P7 |

### ⭐⭐⭐ Export

**Single Palette** (all P0):

- Shared palette
- Single tile pool

**Multi-Palette** (P1-P7):

- One palette per slot
- One tile pool per layer
- Auto-generates `setBackgroundCustomPaletteSlot()`

---

## Tile Attributes

### 🏷️ Concept

**Tile attributes** attach key-value metadata for game logic:

- Collision (solid, sensor, oneway)
- Interactions (interactable, locked)
- Gameplay (damage, collectible, trigger)
- Custom properties

### ⭐⭐ Two-Level System

**Tileset Defaults** (per tile type):

- Defined once, apply to all instances
- Example: All "wall" tiles have `solid=true`

**Instance Attributes** (per placement):

- Override defaults for specific tiles
- Example: One door has `locked=true`

### ⭐⭐⭐ Using Attribute Tool

1. Select **Attribute Tool** (**A**)
2. Click tile on canvas
3. Dialog shows:
   - Tile preview
   - Default attributes ("(default)")
   - Instance attributes
4. Add/edit/remove
5. Click **Save**

**Visual**: Green triangle indicator on tiles with instance attributes.

> ⚠️ Indicators only visible on active layer.

### ⭐⭐⭐ Inheritance

Query attributes:

1. Get tileset defaults
2. Get instance overrides
3. Instance overwrites same keys
4. Result: merged dictionary

**Example**:

- Default: `type=door, solid=false`
- Instance: `locked=true`
- Result: `type=door, solid=false, locked=true`

### ⭐⭐⭐ Common Patterns

**Collision**:

```
solid = true/false
sensor = true/false
oneway = true/false
```

**Interactions**:

```
interactable = true/false
locked = true/false
type = door/chest/switch
```

**Gameplay**:

```
damage = 10
collectible = true/false
trigger = true/false
```

---

## Tile Flag Rules

### 🚩 Concept

**Tile Flag Rules** define how attributes convert to bit flags in exported C++:

```json
{
  "rules": [
    {
      "key": "solid",
      "value": true,
      "flags": ["TILE_SOLID", "COLLISION"]
    },
    {
      "key": "type",
      "value": ["door", "chest"],
      "flags": ["INTERACTABLE"]
    }
  ]
}
```

### ⭐⭐⭐ Resolution Hierarchy

Rules resolve in order:

1. **Project rules** (`project_dir/tile_flag_rules.json`)
2. **Editor defaults** (`assets/tilemap/tile_flag_rules.json`)
3. **Legacy fallback** (hardcoded)

### ⭐⭐⭐ Managing Project Rules

**Access**: Click the **Settings** (gear) button in the toolbar → "Tile Flag Rules" section

**Indicator**:

- "Using: Editor Defaults" (gray) - No custom rules
- "Using: Project Rules" (blue) - Custom rules exist

**Actions**:

**Create**:

1. Click **Create Project Rules**
2. Template created
3. Status: "Created tile_flag_rules.json"

**Reset**:

1. Click **Reset to Defaults**
2. Confirm
3. Status: "Reset to editor default rules"

### ⭐⭐⭐⭐ Available Flags

| Flag | Function |
|------|----------|
| `TILE_NONE` | Default / no flag (0) |
| `TILE_SOLID` | Collision |
| `TILE_SENSOR` | Trigger without blocking |
| `TILE_DAMAGE` | Hurts player |
| `TILE_COLLECTIBLE` | Can be collected |
| `TILE_ONEWAY` | One-way platform |
| `TILE_TRIGGER` | Activates events |

### ⭐⭐⭐⭐ Export

Generates:

- **Behavior Layer**: `TileFlags` array
- **Query Functions**: Runtime access methods
- **ESP32 Optimization**: Compacted in flash

```cpp
extern const TileFlags BEHAVIOR_LAYER[] = {
    0x01, 0x02, 0x04, 0x01, 0x08, ...
};

// Runtime query
const char* type = level1::get_tile_attribute(0, x, y, "type");
if (type && strcmp(type, "door") == 0) {
    // Handle door
}
```

---

## Project Optimization

### 🚀 Large Project Optimization

**Memory**:

1. **Binary format** (`.bin`) for >1MB projects
2. **History Compression** in preferences
3. **Close unused scenes**

**Export size**:

1. **Limit palette** to 16 colors
2. **Remove duplicate tiles**
3. **Use appropriate BPP** (1/2/4)

> There is no separate "Project Statistics" dialog. The export status output reports tile counts and estimated size.

---

## Related Guides

- [Quick Start](/tools/tilemap-editor/quick-start) - 5 minute guide
- [Usage Guide](/tools/tilemap-editor/usage-guide) - Essential features
- [Technical Reference](/tools/tilemap-editor/technical-reference) - Technical specs
- [Isometric Guide](/tools/tilemap-editor/isometric-guide) - Projection, cell stride, isometric export
