---
title: "Quick Start"
description: "Create your first PixelRoot32 tilemap in 5 minutes - step-by-step guide"
---

# Tilemap Editor - Quick Start Guide

**Time**: 5 minutes | **Level**: ⭐ Beginner

---

> **Quick Index**
> - [Step 1: Open Editor](#⏱️-step-1-open-the-editor-30-seconds)
> - [Step 2: Create Project](#⏱️-step-2-create-project-30-seconds)
> - [Step 3: Import Tileset](#⏱️-step-3-import-tileset-1-minute)
> - [Step 4: Paint](#⏱️-step-4-paint-your-first-map-2-minutes)
> - [Step 5: Save/Export](#⏱️-step-5-save-and-export-1-minute)
> - [Keyboard Shortcuts](#keyboard-shortcuts)

---

## Your First Map in 5 Minutes

### ⏱️ Step 1: Open the Editor (30 seconds)

1. Launch **PixelRoot32 Tool Suite** from your applications menu
2. Click **Launch** on the **Tilemap Editor** card

When starting without a project, you see the toolbar with:
- **File → New Project** (`Ctrl+N`): Create a new project from scratch
- **File → Open** (`Ctrl+O`): Open `.pr32scene.bin` (or legacy `.pr32scene`)

---

### ⏱️ Step 2: Create Project (30 seconds)

1. Click **File → New Project** or press **Ctrl+N**
2. Configure parameters:

| Field | Default | Notes |
|-------|---------|-------|
| **Name** | "New Scene" | Sanitized to lowercase alphanumeric + `_` |
| **Tile Size** | 8 px | Validated 1-32 against engine limits |
| **Map Width** | 40 tiles | Max 255 |
| **Map Height** | 30 tiles | Max 255 |
| **Orientation** | Landscape | Portrait also available |

3. Click **Create**
4. Select an empty folder

💡 **Tip**: Use **"Fit Map to Hardware Limit"** to auto-fit to ESP32 screen (320×240).

---

### ⏱️ Step 3: Import Tileset (1 minute)

1. In the **TILESET** panel, click **Import tileset**
2. Select a PNG, JPG, or BMP image
3. The image is auto-copied to `assets/tilesets/`

📝 **Recommended**: PNG, multiples of tile size, up to 16 colors for 4bpp.

---

### ⏱️ Step 4: Paint Your First Map (2 minutes)

1. **Select Brush** - Press **B** or click Brush tool
2. **Pick a tile** - Click any tile in the TILESET panel (cyan border for selected tile)
3. **Paint** - Click on canvas to place, drag to paint continuously
4. **Undo** - Use **Ctrl+Z** if you make a mistake

🛠️ **Basic Tools**:

| Key | Tool | Use |
|-----|------|-----|
| **B** | Brush | Paint tiles |
| **E** | Eraser | Erase tiles |
| **R** | Rectangle | Draw filled rectangles |
| **G** | Pan | Pan the canvas view |
| **I** | Pipette | Pick a tile from the canvas |
| **A** | Attribute | Open tile attribute editor |
| **Space** | Pan (hold) | Temporarily pan while held |

🛠️ **Additional tools** (via toolbar): Animation — assign tiles to animations from the panel.

---

### ⏱️ Step 5: Save and Export (1 minute)

1. **Save project**: **Ctrl+S** or **File → Save**
2. **Export to C++**: Click the **Export** button in the toolbar, or **File → Export**

⚙️ **Export options**: Namespace, color depth (auto-detected), "Store in Flash (ESP32)" for PROGMEM, and Legacy format for compatibility without Flash attributes.

🔒 **Note**: C++ export requires a valid license. Without license:
- Export button shows 🔒
- Upgrade dialog appears
- Other features remain available

> See **[License & Activation](/tools/tilemap-editor/license-and-activation)** for details on activating your license.

---

## Next Steps

Once you've completed your first map:

1. **Add more layers** - Click **+** in the LAYERS panel (max 8 per scene)
2. **Create multiple scenes** - Click **+** in the SCENE panel
3. **Explore advanced features** - See [Advanced Guide](/tools/tilemap-editor/advanced-guide)

---

## Keyboard Shortcuts

### File
| Shortcut | Action |
|----------|--------|
| **Ctrl+N** | New project |
| **Ctrl+O** | Open project |
| **Ctrl+S** | Save |
| **Ctrl+Shift+S** | Save As |
| **Ctrl+W** | Close project |

### Edit
| Shortcut | Action |
|----------|--------|
| **Ctrl+Z** | Undo |
| **Ctrl+Y** / **Ctrl+Shift+Z** | Redo |

### Tools
| Key | Tool |
|-----|------|
| **B** | Brush |
| **E** | Eraser |
| **R** | Rectangle |
| **G** | Pan |
| **I** | Pipette |
| **A** | Attribute |
| **Space** | Pan (hold while pressed) |

### View
| Shortcut | Action |
|----------|--------|
| **Ctrl++** | Zoom in |
| **Ctrl+-** | Zoom out |
| **Ctrl+0** | Zoom reset |
| **Ctrl+F** | Fit to window |
| **F1** | Keyboard shortcuts panel |

---

## Related Guides

- [Usage Guide](/tools/tilemap-editor/usage-guide) - Essential features
- [Advanced Guide](/tools/tilemap-editor/advanced-guide) - Advanced features
- [Technical Reference](/tools/tilemap-editor/technical-reference) - Technical specs
