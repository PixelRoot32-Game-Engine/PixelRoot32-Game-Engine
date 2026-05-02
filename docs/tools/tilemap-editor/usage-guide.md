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

> ⚠️ **Correction**: Previous docs said 4 layers. Actual limit is **8 layers**.

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
1. Double-click layer name
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

💡 **Tip**: Use "Fit Map to Hardware Limit" to auto-fit to ESP32 (320×240).

### ⭐ Project Settings

1. Click **Settings** (gear icon)
2. Or **File → Project Settings**
3. Modify values
4. Click **OK**

> ⚠️ **Important**: Changing tile size affects all existing tilesets.

### ⭐ Save & Load

| Format | Extension | Advantages |
|--------|----------|------------|
| **JSON** | `.pr32scene` | Human-readable, git-friendly |
| **Binary** | `.pr32scene.bin` | Up to 335× smaller, 10× faster |

**Switch to binary**: **File → Preferences → Use Binary Format**

---

## Tilesets

### ⭐ Importing a Tileset

**Method 1** - TILESET panel:
1. Click **Import tileset**
2. Select PNG/JPG/BMP
3. Auto-copied to `assets/tilesets/`

**Method 2** - Menu:
1. **File → Import Tileset**
2. Select image

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
- Increments: 0.5× (min 1×, max 10×)

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
1. Right-click scene → **Rename**
2. Type new name
3. Press Enter

**Duplicate**:
1. Right-click scene → **Duplicate**
2. Exact copy with "(Copy)"

**Delete**:
1. Right-click scene → **Delete**
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

### ⭐⭐ Pipette (P)

1. Select Pipette (**P**)
2. Click tile on canvas
3. Auto-selected in TILESET

### ⭐⭐ Attribute Tool (A)

1. Select Attribute (**A**)
2. Click tile to assign/edit
3. Configure properties

> 💡 **See**: [Advanced Guide](/tools/tilemap-editor/advanced-guide) for attributes

### ⭐⭐ Animation Eyedropper (I)

1. Select Animation Eyedropper (**I**)
2. Click tile on canvas
3. Auto-linked to current animation

> 💡 **See**: [Advanced Guide](/tools/tilemap-editor/advanced-guide) for animations

### ⭐⭐ Live Preview (L)

1. Click **Live Preview** button
2. Or press **L**
3. Animations play in real-time on canvas

### ⭐ Pan (Space)

1. Hold **Space**
2. Drag to move view
3. Release to return to tool

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
1. Click 🧅 icon next to scene
2. Scene appears translucent

**Global**:
1. Activate **"Show Onion Skin"** checkbox
2. Shows/hides all onion-enabled scenes

### ⭐ Adjusting Opacity

- Use **"Opacity"** slider
- Recommended: 0.3-0.5 (30-50%)
- Default: 0.4 (40%)

### ⭐⭐ Example: Aligning Exit

1. Enable onion on previous level scene
2. Adjust opacity to see both
3. Place exit aligned with entrance
4. Disable when done

> ⚠️ Onion scenes are visual-only (not editable).

---

## Preferences

### ⭐ Access

**File → Preferences**

**Grid Settings**:
- **Canvas Grid Intensity**: Grid opacity (0-255)
- **Tileset Grid Intensity**: Grid opacity in tileset
- **Attribute Indicator Opacity**: Marker opacity (0.0-1.0)

**Auto-save**:
- **Enabled**: On/off
- **Interval**: Minutes (1-60)

**Optimization**:
- **History Compression**: Compresses consecutive operations
- **Use Binary Format**: Default .bin format

### ⭐⭐ Memory & Lazy Loading

Editor implements **lazy loading** for memory optimization:

- Inactive scenes stay unloaded
- Load on demand when switching
- Configurable in preferences

---

## Keyboard Shortcuts

### Tools

| Key | Action |
|-----|--------|
| **B** | Brush |
| **E** | Eraser |
| **R** | Rectangle |
| **P** | Pipette |
| **A** | Attribute |
| **I** | Animation Eyedropper |
| **L** | Live Preview |
| **Space** | Pan (hold) |

### Navigation

| Shortcut | Action |
|----------|--------|
| **Ctrl+Wheel** | Zoom |
| **Ctrl++** | Zoom in |
| **Ctrl+-** | Zoom out |
| **Ctrl+0** | Reset zoom |
| **Ctrl+F** | Fit to screen |

### Editing

| Shortcut | Action |
|----------|--------|
| **Ctrl+Z** | Undo |
| **Ctrl+Y** | Redo |
| **Ctrl+S** | Save |
| **Ctrl+E** | Export C++ |
| **Esc** | Close panels |

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