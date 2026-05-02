# Tilemap Editor - Advanced Guide

**Level**: ⭐⭐ Intermediate | ⭐⭐⭐ Advanced

---

> **Quick Index**
>
> - [Tile Animations](#tile-animations)
> - [License System](#license-system)
> - [Memory & Lazy Loading](#memory--lazy-loading)
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

**Access**: **View → Animation Panel** or click animation icon

**Components**:

- **Animation List**: All project animations
- **Add/Remove**: Create/delete animations
- **Properties Panel**: Configure settings
- **Preview Panel**: Real-time playback

### ⭐⭐⭐ Creating Animations

**Step 1**: New animation

1. Click **Add** in Animation Panel
2. "New Animation" appears in list
3. Select to configure

**Step 2**: Configure properties

| Property | Description | Recommended |
|----------|--------------|-------------|
| **Name** | Descriptive name | "Water", "Fire" |
| **Base Tile** | First tile index | - |
| **Frame Count** | Number of frames | 2-8 |
| **Frame Duration** | Speed in engine ticks | 8-16 |

**Example**: Water with 4 frames

```
Base Tile: 16
Frame Count: 4 (uses tiles 16,17,18,19)
Frame Duration: 8 ticks
```

**Step 3**: Assign tiles

**Method 1** - Animation Eyedropper:

1. Select **Animation Eyedropper** (**I**)
2. Click tiles on canvas
3. Auto-linked

**Method 2** - Manual:

1. Select animation
2. Enter base tile
3. Click **Apply**

### ⭐⭐⭐ Visual Indicators

- Animated tiles show ▶ icon
- Icon color indicates status
- Hover for details

### ⭐⭐⭐ Live Preview

**Toggle**:

1. Click **Live Preview** button
2. Or press **L**
3. Animations play in real-time

**Features**:

- **Synchronized**: Same timer as ESP32
- **Optimized**: Only renders visible
- **Frame Accurate**: Exact game frames
- **Multi-layer**: Works across layers

**Controls**:

- **Play/Pause**: Toggle playback
- **Speed Control**: 1×, 2×, 0.5×
- **Frame Step**: Debug frame by frame

### ⭐⭐⭐ Validation

The editor auto-validates:

**Checks**:

- **Tile Bounds**: base_tile + frame_count ≤ tileset
- **Overlap Detection**: No overlapping ranges
- **Memory Constraints**: ESP32 limits (64 animations, 256 frames)
- **Frame Duration**: Valid values (1-255 ticks)

**Error Messages**:

- "Animation exceeds tileset bounds"
- "Animations overlap"
- "Too many animations"

### ⭐⭐⭐ Export to C++

Animations auto-export with C++ export:

```cpp
// scene_name_animations.h
extern const pixelroot32::graphics::TileAnimation scene_name_animations[];
constexpr size_t SCENE_NAME_ANIMATION_COUNT = 2;

// scene_name_animations.cpp
static const pixelroot32::graphics::TileAnimation scene_name_animations[] = {
    { 16, 4, 8, 0 },  // Water
    { 32, 2, 12, 0 }  // Fire
};
```

**Engine Integration**:

```cpp
#include "level1_animations.h"

void game_loop() {
    level1::init();
    while (game_running) {
        level1::get_animation_manager().step();
        render_tilemap();
    }
}
```

### ⭐⭐⭐ ESP32 Limits

| Limit | Value |
|--------|-------|
| **Max Animations** | 64 per scene |
| **Max Total Frames** | 256 per scene |
| **Animation Memory** | 4 bytes each |
| **Lookup Table** | 1 byte per tile |

---

## License System

### 🔒 Export License

C++ export **requires a valid license**. Without:

- Export button shows 🔒
- Upgrade dialog appears
- Other features work normally

### ⭐⭐⭐ Verify License

**File → License Status** shows:

- License status (active/expired)
- Expiration date
- Available features

### ⭐⭐⭐ Activate License

1. **File → Activate License**
2. Enter license key
3. Click **Activate**

---

## Memory & Lazy Loading

### 💾 Lazy Loading

Editor implements **lazy loading**:

- Inactive scenes stay unloaded
- Load on demand when switching
- Configurable in preferences

### ⭐⭐⭐ Memory Optimization

**For large projects**:

1. **Use binary format**: `.pr32scene.bin` (up to 335× smaller)
2. **Enable History Compression**: In preferences
3. **Close unused scenes**: Free memory
4. **Limit preload**: Reduce RAM usage

**Check**: **File → Project Statistics** shows estimated memory.

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

**Visual**: Orange triangle on tiles with instance attributes.

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
2. **Editor defaults** (`modules/tilemap_editor/assets/tile_flag_rules.json`)
3. **Legacy fallback** (hardcoded)

### ⭐⭐⭐ Managing Project Rules

**Access**: **File → Project Settings** → "Tile Flag Rules"

**Indicator**:

- "Using: Editor Defaults" (gray) - No custom rules
- "Using: Project Rules" (blue) - Custom rules exist

**Actions**:

**Create**:

1. Click **Create Project Rules**
2. Template created
3. Status: "✓ Created template rules file"

**Reset**:

1. Click **Reset to Defaults**
2. Confirm
3. Status: "✓ Reset to editor defaults"

### ⭐⭐⭐⭐ Available Flags

| Flag | Function |
|------|----------|
| `TILE_NONE` | No flag |
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
4. **Limit preload**

**Export size**:

1. **Limit palette** to 16 colors
2. **Remove duplicate tiles**
3. **Use appropriate BPP** (1/2/4)

### ⭐⭐⭐ Statistics

**File → Project Statistics**:

- Unique tiles used
- Layers and scenes
- Estimated memory
- Animation limits

---

## Related Guides

- [Quick Start](/tools/tilemap-editor/quick-start) - 5 minute guide
- [Usage Guide](/tools/tilemap-editor/usage-guide) - Essential features
- [Technical Reference](/tools/tilemap-editor/technical-reference) - Technical specs
