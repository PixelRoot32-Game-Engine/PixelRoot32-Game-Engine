---
title: "Usage Guide"
description: "Day-to-day PixelRoot32 Tilemap Editor reference - layers, projects, tilesets, tools, and keyboard shortcuts"
---

# Tilemap Editor - Usage Guide

**Level**: ⭐ Beginner | ⭐⭐ Intermediate

---

> **Quick Index**
> - [Layers](#layers)
> - [Projects](#projects)
> - [Tilesets](#tilesets)
> - [Scenes](#scenes)
> - [Editing Tools](#editing-tools)
> - [Onion Skinning](#onion-skinning)
> - [Preferences](#preferences)
> - [Glossary](#glossary)

---

## Layers

### ⭐ Layers: Basic Concept

**Layers** organize map elements at different depth levels:

- **Top layer**: Rendered above others
- **Bottom layer**: Rendered below others
- **Maximum**: **8 layers** per scene

### ⭐⭐ Managing Layers

**Add layer**:
1. Click **+** in **LAYERS** panel
2. New layer inserted above selected

**Delete layer**:
1. Click 🗑️ icon on layer
2. Confirm deletion
3. Cannot delete last layer

**Duplicate layer**:
1. Click 📄 icon on layer
2. Copy created with "(Copy)"

**Reorder**:
- Drag and drop in LAYERS panel
- Or use ordering commands

**Rename**:
1. Click the ✏️ rename button on the layer row
2. Type new name
3. Press Enter

**Visibility**:
- Click 👁️ to show/hide
- Hidden layers not exported

---

## Projects

### ⭐ Creating a New Project

1. **Create New Project** on Welcome Screen
2. Configure:

| Field | Description | Default |
|-------|-------------|----------|
| **Name** | Project name | "New Scene" |
| **Description** | Optional | - |
| **Tile Size** | Pixels | 8 |
| **Map Width** | Tiles | 40 |
| **Map Height** | Tiles | 30 |
| **Orientation** | Landscape/Portrait | Landscape |

3. Click **Create Project**
4. Select empty folder

> Project names are sanitized to lowercase alphanumeric + `_`. Dots `..` and `.` are rejected. See [Tile Flag Rules](/tools/tilemap-editor/advanced-guide#tile-flag-rules) for per-project rule files.

💡 **Tip**: Use "Fit Map to Hardware Limit" to auto-fit to ESP32 (320×240).

### ⭐ Project Settings

1. Click **Settings** (gear icon)
2. Or **File → Project Settings**
3. Modify values
4. Click **OK**

> ⚠️ **Important**: Changing tile size affects all existing tilesets.

### ⭐ Save & Load

| Extension | Content |
|-----------|---------|
| `.pr32scene.bin` | Binary v6 format (default) |
| `.pr32scene` | Legacy extension; still binary v6 content |

**File → Preferences** opens a modal dialog. The **"Use Binary Format"** checkbox inside only changes the saved **extension** (`.pr32scene.bin` vs `.pr32scene`). The on-disk content is always the binary v6 container — there is no JSON project format.

### ⭐ Toolbar Indicators

The top toolbar shows status indicators while a project is open:

| Indicator | Meaning | Interactive |
|-----------|---------|:---:|
| `HIST:n` | Number of undo entries in history (max **100**) | ❌ |
| `BIN` / `JSON` | Current save format preference | ✅ Click to toggle |
| `Export` | Shortcut to export dialog | ✅ Click to open |

> The undo history is limited to **100 operations per session**. Enabling **History Compression** in Preferences merges consecutive similar edits to stay within the cap longer.

---

## Tilesets

### ⭐ Importing a Tileset

1. In the **TILESET** panel, click **Import tileset**
2. Select PNG/JPG/BMP
3. Auto-copied to `assets/tilesets/`

📝 **Format**: PNG recommended, multiples of tile size, up to 16 colors.

### ⭐ Selecting Tiles

**Single selection**:
- Click tile (cyan border highlight)

**Rectangular selection**:
- Click start tile
- Drag to end tile
- Release to confirm

### ⭐⭐ Zoom in Tileset Panel

- **Zoom In**: Mouse wheel up
- **Zoom Out**: Mouse wheel down
- Increments: 0.25× (min 1×, max 4×)

### ⭐ Multiple Tilesets

1. Import first tileset normally
2. Repeat for additional tilesets
3. Displayed one after another
4. **Global tile index** (accumulated)

```
Tileset A: 10 tiles (indices 0-9)
Tileset B: 8 tiles (indices 10-17)
```

---

## Scenes

### ⭐⭐ Scenes: Concept

**Scenes** are independent levels/rooms in a project:

- Own dimensions
- Own layers
- Access to shared project tilesets

### ⭐ Creating a New Scene

1. Click **+** in **SCENE** panel
2. Created with same dimensions as active
3. "Background" layer auto-added

### ⭐ Switching Between Scenes

- Click scene name in SCENE panel
- Canvas updates automatically
- Layers panel shows scene's layers

### ⭐⭐ Managing Scenes

**Rename**:
1. Click the ✏️ rename button on the scene row
2. Type new name
3. Press Enter

**Duplicate**:
1. Click the 📄 duplicate button on the scene row
2. Exact copy with "(Copy)"

**Delete**:
1. Click 🗑️ delete button on the scene row
2. Confirm
> ⚠️ Cannot delete last scene

---

## Editing Tools

### ⭐ Brush (B)

1. Select Brush (**B**)
2. Pick tile from TILESET
3. Click/drag on canvas

**Rectangular patterns**:
1. Select rectangle in tileset
2. Paint - full pattern applied

### ⭐ Rectangle (R)

1. Select Rectangle (**R**)
2. Click and drag on canvas
3. Release to fill

### ⭐ Eraser (E)

**Method 1**:
1. Select Eraser (**E**)
2. Click/drag to erase

**Method 2** (universal):
- Right-click with any tool

### ⭐⭐ Pipette (I)

1. Select Pipette (**I**)
2. Click tile on canvas to pick it as the active tile
3. The picked tile is auto-selected in the TILESET panel
4. To assign a tile to an animation: pick the tile with Pipette, open the **Animations** panel, select a target animation, and click **Apply**

> 💡 **See**: [Advanced Guide](/tools/tilemap-editor/advanced-guide) for animations and attributes

### ⭐⭐ Attribute Tool (A)

### ⭐⭐ Live Preview

1. Click the **▶ Live Preview** button in the toolbar
2. Animations play in real-time on the canvas
3. Uses the **Play/Pause** button, speed controls (1× / 2× / 0.5×), and frame-stepping from the toolbar

### ⭐⭐ Pan

**Persistent Pan (G)**:
- Press **G** to switch to Pan tool permanently
- Use another tool key (B, E, R, I, A) to switch back

**Temporary Pan (Space)**:
- Hold **Space** to temporarily pan
- Release to return to your previous tool

### ⭐ Zoom Controls

| Action | Method |
|--------|-------|
| **Zoom In** | Ctrl++ or Ctrl+wheel up |
| **Zoom Out** | Ctrl+- or Ctrl+wheel down |
| **Reset Zoom** | Ctrl+0 |
| **Fit to Screen** | Ctrl+F |

### ⭐ Tool Preview

Mouse over canvas:
- Dotted rectangle shows paint position
- Preview tiles 50% opacity
- Precise positioning before clicking

---

## Onion Skinning

### ⭐⭐ Concept

**Onion skinning** shows translucent scenes over active scene:

- Align exits between levels
- Check platform consistency
- Compare designs between scenes

### ⭐ Activating

**Per scene**:
1. Check the 🧅 checkbox next to the scene
2. Scene appears translucent

**Global**:
1. Activate **"Show Onion Skin"** checkbox
2. Shows/hides all onion-enabled scenes

### ⭐ Adjusting Opacity

- Use **"Opacity"** slider (0.0-1.0)
- Recommended: 0.3-0.5 (30-50%)

### ⭐⭐ Example: Aligning Exit

1. Enable onion on previous level scene
2. Adjust opacity to see both
3. Place exit aligned with entrance
4. Disable when done

> ⚠️ Onion scenes are visual-only (not editable).

---

## Export

### ⭐⭐ Export to C++

1. Click the **Export** button in the toolbar, or **File → Export**
2. Configure options:

| Option | Description |
|--------|-------------|
| **C++ Namespace** | Namespace for generated code |
| **Color Depth** | Auto-detected BPP (1/2/4) |
| **Store in Flash (ESP32)** | Save tile data to PROGMEM |
| **Legacy Format** | Without Flash attributes (compatibility) |

3. Select output directory
4. Generated files: `scene_name.h`, `scene_name.cpp`, optional `scene_name_animations.h/.cpp`, and `shared_palette.h`

🔒 **License required**: Exporting to C++ requires a valid license. See [License & Activation](/tools/tilemap-editor/license-and-activation).

---

## Preferences

### ⭐ Access

**File → Preferences**

**Grid Settings**:
- **Grid Background Color**: Color of the canvas background behind the grid
- **Canvas Grid Intensity**: Grid opacity on the canvas (0-255, default 40)
- **Tileset Grid Intensity**: Grid opacity in the tileset panel (0-255, default 120)
- **Attribute Indicator Opacity**: Marker opacity for tile attributes (0-255, default 200)
- **Animation Indicator Opacity**: Marker opacity for animated tiles (0-255, default 180)

**Auto-save**:
- **Enabled**: On/off (enabled by default)
- **Interval**: Minutes (1-30, default 5 min)

**Optimization**:
- **History Compression**: Compresses consecutive undo operations
- **Use Binary Format**: Sets the saved file extension

---

## Keyboard Shortcuts

### Tools

| Key | Action |
|-----|--------|
| **B** | Brush |
| **E** | Eraser |
| **R** | Rectangle |
| **G** | Pan (persistent) |
| **I** | Pipette |
| **A** | Attribute |
| **Space** | Pan (temporary hold) |

### Navigation

| Shortcut | Action |
|----------|--------|
| **Ctrl+Wheel** | Zoom |
| **Ctrl++** | Zoom in |
| **Ctrl+-** | Zoom out |
| **Ctrl+0** | Reset zoom |
| **Ctrl+F** | Fit to screen |

### File

| Shortcut | Action |
|----------|--------|
| **Ctrl+N** | New project |
| **Ctrl+O** | Open project |
| **Ctrl+S** | Save |
| **Ctrl+Shift+S** | Save As |
| **Ctrl+W** | Close project |

### Editing

| Shortcut | Action |
|----------|--------|
| **Ctrl+Z** | Undo |
| **Ctrl+Y** / **Ctrl+Shift+Z** | Redo |
| **F1** | Keyboard shortcuts panel |

### Mouse

| Action | Result |
|--------|--------|
| **Left click** | Paint/Select |
| **Right click** | Erase |
| **Wheel** | Zoom in tileset |
| **Ctrl+Wheel** | Zoom in canvas |

---

## Glossary

| Term | Definition |
|------|------------|
| **Tile** | Basic graphic unit (8×8, 16×16, etc.) |
| **Tileset** | Image containing multiple tiles |
| **Layer** | Depth level in map |
| **Scene** | Independent level/room |
| **Canvas** | Drawing area |
| **ESP32** | Target hardware |
| **BPP** | Bits per pixel (1/2/4) |
| **RGB565** | Color format (5R+6G+5B) |

---

## Related Guides

- [Quick Start](/tools/tilemap-editor/quick-start) - 5 minute guide
- [Advanced Guide](/tools/tilemap-editor/advanced-guide) - Advanced features
- [Technical Reference](/tools/tilemap-editor/technical-reference) - Technical specs