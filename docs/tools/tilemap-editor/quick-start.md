# Tilemap Editor - Quick Start Guide

**Time**: 5 minutes | **Level**: ⭐ Beginner

---

> **Quick Index**
> - [Step 1: Open Editor](#-step-1-open-the-editor-30-seconds)
> - [Step 2: Create Project](#-step-2-create-project-30-seconds)
> - [Step 3: Import Tileset](#-step-3-import-tileset-1-minute)
> - [Step 4: Paint](#-step-4-paint-your-first-map-2-minutes)
> - [Step 5: Save/Export](#-step-5-save-and-export-1-minute)
> - [Keyboard Shortcuts](#keyboard-shortcuts)

---

## Your First Map in 5 Minutes

### ⏱️ Step 1: Open the Editor (30 seconds)

1. Launch **PixelRoot32 Tool Suite** from your applications menu
2. Click **Launch** on the **Tilemap Editor** card

When starting without a project, you see the **Welcome Screen** with:
- **Create New Project**: Create a new project from scratch
- **Open Existing Project**: Open `.pr32scene` or `.pr32scene.bin`

---

### ⏱️ Step 2: Create Project (30 seconds)

1. Click **Create New Project**
2. Configure parameters:

| Field | Default |
|-------|--------|
| **Name** | "New Scene" |
| **Tile Size** | 8 px |
| **Map Width** | 40 tiles |
| **Map Height** | 30 tiles |
| **Orientation** | Landscape |

3. Click **Create Project**
4. Select an empty folder

💡 **Tip**: Use **"Fit Map to Hardware Limit"** to auto-fit to ESP32 screen (320×240).

---

### ⏱️ Step 3: Import Tileset (1 minute)

**Method 1** - TILESET panel:
1. In **TILESET** panel (left sidebar), click **Import tileset**
2. Select PNG, JPG, or BMP image
3. Auto-copied to `assets/tilesets/`

**Method 2** - File menu:
1. **File → Import Tileset**
2. Select image

📝 **Recommended**: PNG, multiples of tile size, up to 16 colors for 4bpp.

---

### ⏱️ Step 4: Paint Your First Map (2 minutes)

1. **Select Brush** - Press **B** or click Brush tool
2. **Pick a tile** - Click any tile in TILESET panel (cyan border)
3. **Paint** - Click on canvas to place, drag to paint continuously
4. **Undo** - Use **Ctrl+Z** if you make a mistake

🛠️ **Basic Tools**:

| Key | Tool | Use |
|-----|------|-----|
| **B** | Brush | Paint tiles |
| **E** | Eraser | Erase tiles |
| **R** | Rectangle | Draw rectangles |
| **P** | Pipette | Copy tile from canvas |
| **A** | Attribute | Assign attributes |

---

### ⏱️ Step 5: Save and Export (1 minute)

1. **Save project**: **Ctrl+S** or **File → Save**
2. **Export to C++**: **Ctrl+E** or **File → Export to C++**

🔒 **Note**: C++ export requires a valid license. Without license:
- Export button shows 🔒
- Upgrade dialog appears
- Other features remain available

---

## Next Steps

Once you've completed your first map:

1. **Add more layers** - Click **+** in LAYERS panel
2. **Create multiple scenes** - Click **+** in SCENE panel
3. **Explore advanced features** - See [Advanced Guide](/tools/tilemap-editor/advanced-guide)

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| **Ctrl+Z** | Undo |
| **Ctrl+Y** | Redo |
| **Ctrl+S** | Save |
| **Ctrl+E** | Export C++ |
| **Space+Drag** | Pan view |
| **Ctrl+Wheel** | Zoom |
| **L** | Toggle animation |

---

## Related Guides

- [Usage Guide](/tools/tilemap-editor/usage-guide) - Essential features
- [Advanced Guide](/tools/tilemap-editor/advanced-guide) - Advanced features
- [Technical Reference](/tools/tilemap-editor technical-reference) - Technical specs