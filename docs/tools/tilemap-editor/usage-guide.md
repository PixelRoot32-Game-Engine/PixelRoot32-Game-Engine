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
> - [Player Spawn](#player-spawn)
> - [Rooms](#rooms)
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
| **Tile Size** | Pixels; the default new tilesets and scenes inherit | 8 |
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

> ⚠️ **Important**: The project's **Tile Size** is a *default*, not a global.
> Each imported tileset carries its own `tileWidth`/`tileHeight`, so a project
> can pair a 32×16 floor sheet with a 32×40 wall sheet — which is what makes
> isometric art possible. Changing the project default does not rewrite sheets
> you have already imported.

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

## Player Spawn

### ⭐ Player Spawn: Concept

The **player spawn** is the tile where the game places the player when the scene starts.

- **Per scene** and **optional** - a scene without a spawn exports exactly as it did before
- **One** spawn per scene
- Coordinates are in **tiles**, not pixels
- **Y is the tile row where the player's feet rest**

### ⭐ Setting the Spawn

1. In the **SCENE** panel, click the 🏃 (person running) icon on the scene row - that scene becomes active and the **Player Spawn** dialog opens
2. Tick **Enable Player Spawn** (the X/Y fields stay disabled while it is unticked)
3. Type **X** and **Y** in tiles
4. Click **Apply Spawn**

| Button | Effect |
|--------|--------|
| **Apply Spawn** | Writes the spawn and closes the dialog |
| **Clear Spawn** | Removes the spawn from the scene and closes |
| **Cancel Spawn Edit** | Closes without touching the scene |

The dialog header shows **Configured** (green) or **Not set**.

### ⭐ Bounds

The spawn must sit inside the scene, in tiles:

| Axis | Valid range |
|------|-------------|
| **X** | `0` to `scene width - 1` |
| **Y** | `0` to `scene height - 1` |

Out of range, the dialog stays open, writes nothing, and shows in red:
`Spawn must be within [0,width) x [0,height)`.

> If you switch the active scene while the dialog is open, **Apply Spawn** is refused: the draft is reloaded from the new active scene so the value can never land on the wrong one. Review the reloaded values and apply again.

### ⭐ Seeing the Spawn

| Where | Indicator |
|-------|-----------|
| **Scene row** | The 🏃 icon turns green; its tooltip becomes "Player Spawn Configured" |
| **Canvas** | A green 🏃 marker on the spawn tile of the **active** scene |
| **Canvas tooltip** | Hover the marker for "Player Spawn Configured" plus the tile coordinates |

> The canvas marker is hidden when you zoom out far enough that a tile is drawn under 4 px - it would be unreadable at that size.

### ⭐⭐ Undo, Save and Export

- **Apply Spawn** and **Clear Spawn** are each **one undo step** (**Ctrl+Z**)
- Undo/redo shortcuts stand down while the dialog is open; close it first, then undo
- Saving re-validates every scene's spawn. An out-of-range spawn aborts the save with the same message and leaves the file on disk untouched
- Exporting a scene with a valid spawn emits `SPAWN_TILE_X` and `SPAWN_TILE_Y` in the generated header
- The spawn is stored per scene in the binary v6 project file - see [File Formats](/tools/tilemap-editor/technical-reference#file-formats)

---

## Rooms

### ⭐⭐ Rooms: Concept

A scene can carry an optional **room layer**: a set of rectangles, in tiles, plus the connections between them. The engine turns it into a room graph that drives camera bounds and room transitions - see [Room Layer](/tools/tilemap-editor/technical-reference#room-layer) for the exported structures.

- **Per scene** and **optional** - a scene with no rooms exports exactly as it did before
- Each room is a rect in tiles: **origin col/row** plus **width/height**
- Each room has **four direction slots** (Up, Down, Left, Right); each holds **one target room or None**
- Rooms are identified by **index** (Room 0, Room 1, ...). Deleting a room renumbers the ones above it and rewrites the other rooms' targets to match

### ⭐ Opening the Rooms Dialog

1. In the **SCENE** panel, click the 🚪 (open door) icon on the scene row
2. That scene becomes active and the **Rooms** dialog opens

The icon is green with the tooltip "Rooms Configured" when the scene already has at least one room.

| Area of the dialog | Contents |
|--------------------|----------|
| **Room list** | One row per room: `Room i  (col,row) WxH`, a ⚠️ marker when the room has a note, and a 🗑️ delete button |
| **Properties** | Origin Col/Row, Width/Height, the four direction combos, the reciprocal checkbox, and **Apply** - all for the selected room |
| **Footer** | **Add Room**, **Auto-connect adjacent**, the auto-connect report, and the **Notes** box |

Selecting a row loads that room into the properties block and highlights it on the canvas.

### ⭐ Adding a Room

1. Click **+ Add Room**
2. The new room is sized to **one game screen in tiles** (`screen width / tile size` × `screen height / tile size`), clamped to the scene
3. It is placed in the **first free slot** of that grid, scanning right and then down

Because of the slot order, two rooms added one after the other come out **adjacent** and are ready to connect.

> If no slot is free, the room is placed at `(0,0)` on top of an existing one and a note says so. Overlapping rooms are legal - move or resize the new room if the overlap was not intentional.

### ⭐⭐ Editing a Room's Rect

1. Select the room in the list
2. Edit **Origin Col** / **Origin Row** and **Width** / **Height** (all in tiles)
3. Click **Apply**

The rect and every connection you changed are committed as **one undo step**, so they always restore together.

### ⭐⭐ Connections

Connections are **per room and per direction**. Each of the four slots holds a target room index or **None**, and each is **directed**: a two-way door needs both halves (`A.Right = B` **and** `B.Left = A`).

1. Select the room
2. Pick a target in the **Up**, **Down**, **Left** or **Right** combo
3. Leave **Also write the reciprocal connection** ticked (it is on by default)
4. Click **Apply**

The reciprocal write fills the **opposite slot on the target room** - but only when that slot is **empty**. It never overwrites a connection you set by hand: if the target already points somewhere else, an informational note names the room it points at and tells you to change it from that room instead. The reciprocal write is part of the **same undo step**.

### ⭐⭐ Adjacency: the Rule to Learn First

**The row axis grows downward.** Row 0 is the top row, so **Up means a smaller `originRow`** and **Down means a larger one**. This trips up almost everyone the first time.

Two rooms are adjacent in a direction only when their edges **touch exactly** *and* they **overlap by at least one tile on the perpendicular axis**:

| Direction | Edge condition | Also requires |
|-----------|----------------|---------------|
| `A` **Right** `B` | `A.originCol + A.cols == B.originCol` | ≥ 1 tile of shared rows |
| `A` **Left** `B` | `B.originCol + B.cols == A.originCol` | ≥ 1 tile of shared rows |
| `A` **Down** `B` | `A.originRow + A.rows == B.originRow` | ≥ 1 tile of shared columns |
| `A` **Up** `B` | `B.originRow + B.rows == A.originRow` | ≥ 1 tile of shared columns |

**Worked example - a correct pairing that is still not adjacent**:

| Room | Origin (col, row) | Size |
|------|-------------------|------|
| Room 0 | (10, 10) | 1×1 |
| Room 1 | (10, 19) | 1×1 |

Paired `Room 0.Down = Room 1` and `Room 1.Up = Room 0`. Both halves exist and the directions are the right way round, so the **pairing is correct**. The rooms are still **not adjacent**: Room 0's bottom edge is row `10 + 1 = 11`, Room 1 starts at row 19, and the two are 8 tiles apart. `Down` from Room 0 requires Room 1 at `originRow = 11`.

> This does not block anything. A non-adjacent connection is legal and exports as a **teleport**.

### ⭐⭐ Adjacent or Teleport

Every candidate in the four combos is annotated with what **Apply** would produce, using the values **currently in the Origin/Size fields** - not the values already saved:

| Annotation | Meaning |
|------------|---------|
| `(adjacent)` | The rects already touch along this direction |
| `(teleport - needs originRow = N)` | One origin value on the target would make them touch, and it fits inside the scene |
| `(teleport - not fixable here; move or resize Room N instead)` | The value that would fix it falls outside the scene, so the target cannot take it - move the room you are editing |
| `(teleport - no single move makes Room A and Room B adjacent)` | The perpendicular spans do not overlap at all, so no origin value alone can make them touch |

### ⭐⭐ Auto-connect Adjacent

Click **Auto-connect adjacent** to fill connections from geometry:

- Fills **only empty** direction slots - it never overwrites a connection you set
- Resolves ties to the **lowest room index** when several rooms are adjacent in the same direction
- Reports what it did: `Auto-connect: N filled, N already set, N ambiguous`
- The whole pass is **one undo step**; if nothing was filled, nothing is committed

### ⭐⭐ Notes vs Errors

This distinction is the one users get wrong most often.

**Notes** are informational. They **never** block Apply, save or export:

| Note | Raised when |
|------|-------------|
| **W1** one-way | A connects to B, but B does not connect back |
| **W2** self-link | A room connects to itself (a runtime no-op) |
| **W3** non-adjacent | The two rects are not adjacent - the connection exports as a teleport |

W3 also states the rule and names the `originRow`/`originCol` value that would make the pair touch, or says plainly that no single move can. Rooms carrying a note get a ⚠️ marker in the list; the full text is in the **Notes** box.

**Errors** block **Apply** *and* **save**. They appear in red under the properties block:

| Error | Cause |
|-------|-------|
| **E1** | A room has zero size - `cols` and `rows` must both be ≥ 1 |
| **E2** | The rect extends past the scene bounds `[0,width) × [0,height)` |
| **E3** | The room's far edge exceeds the engine limit of **32767** world units |
| **E4** | A connection targets a room index that does not exist |
| **E5** | The room count exceeds the format limit of **65535** |

> A project with only notes saves and exports normally. A project with an error is refused at save time, and the file on disk is left untouched.

### ⭐ Deleting a Room

1. Click 🗑️ on the room row
2. Confirm in the **Delete Room** popup

The delete is undoable. Remember that it renumbers every room above it.

### ⭐ Rooms on the Canvas

While a scene has rooms, the canvas outlines each of them:

- The room **selected in the dialog** is drawn bright and thick; the others stay faint
- Each outline carries its **room index** in the top-left corner
- Hovering a room shows its index, origin and size
- Rooms fully outside the visible viewport are skipped

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
4. Generated files: `scene_name.h`, `scene_name.cpp`, and the shared palette header `{namespace}_tilemap_palette.h`

> Tile animations are embedded in the scene header. There are no separate `*_animations.h/.cpp` files.

🔒 **License required**: Exporting to C++ requires a valid license. See [License & Activation](/tools/tilemap-editor/license-and-activation).

---

## Scene Projection

### ⭐⭐ Orthogonal or isometric

Projection is a property of the **scene**, so one project can hold an isometric
dungeon and an orthogonal menu.

1. In the **SCENES** panel, click the **cubes** icon on the scene's row
2. Choose `Orthogonal` or `Isometric`
3. Set the **Cell** — the *stride*, not your tile bitmap size
4. **Apply**

The icon shows green on any scene that is isometric.

> ⚠️ **Important**: Changing a painted scene's projection or cell
> **reinterprets every tile in it**. The indices survive; what they mean in
> space does not. The dialog requires an explicit confirmation and there is no
> automatic migration — set this before you paint.

📖 **Full walkthrough**: [Isometric Guide](/tools/tilemap-editor/isometric-guide)

---

## Preferences

### ⭐ Access

**File → Preferences**

**Grid Settings**:
- **Grid Background Color**: Color of the canvas background behind the grid
- **Canvas Grid Intensity**: Grid opacity on the canvas (0-255, default 40)
- **Tileset Grid Intensity**: Grid opacity in the tileset panel (0-255, default 120)
- **Show device screen overlay**: Draws the project's screen on the canvas (on by default). On a map larger than one screen it is a reference for how much is visible at once, not a limit — the editor exports the whole map and the game scrolls a camera over it
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
| **Player Spawn** | Optional per-scene tile where the player starts (Y = feet row) |
| **Room** | Rect in tiles inside a scene, with four direction slots |
| **Room Layer** | The optional set of rooms and connections of one scene |
| **Connection** | Directed link from one room's direction slot to a target room |
| **Adjacent** | Two rooms whose edges touch exactly and share ≥ 1 tile perpendicular |
| **Teleport** | A connection between rooms that are not adjacent (legal) |
| **ESP32** | Target hardware |
| **BPP** | Bits per pixel (1/2/4) |
| **RGB565** | Color format (5R+6G+5B) |

---

## Related Guides

- [Quick Start](/tools/tilemap-editor/quick-start) - 5 minute guide
- [Advanced Guide](/tools/tilemap-editor/advanced-guide) - Advanced features
- [Isometric Guide](/tools/tilemap-editor/isometric-guide) - Projection, cell stride, isometric export
- [Technical Reference](/tools/tilemap-editor/technical-reference) - Technical specs