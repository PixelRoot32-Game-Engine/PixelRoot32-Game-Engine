---
title: "Isometric Guide"
description: "Isometric scenes in the Tilemap Editor: projection, cell stride, fitting the cell to your art, canvas overlays, what the export adds, and one map per room"
---

# Tilemap Editor - Isometric Guide

**Level**: ⭐⭐⭐ Advanced

> **Quick Index**
>
> - [What changes](#what-changes-in-an-isometric-scene)
> - [Turning a scene isometric](#turning-a-scene-isometric)
> - [The cell stride](#the-cell-stride)
> - [Fit to art](#fit-to-art)
> - [Reading the canvas](#reading-the-canvas)
> - [Painting](#painting)
> - [Export](#export)
> - [Rooms](#rooms)
> - [Re-exporting](#re-exporting)
> - [Troubleshooting](#troubleshooting)

::: warning Read this page in order

Changing a scene's projection or its cell **reinterprets every tile already
painted in it**. The tile indices survive; what they mean in space does not.
The editor makes you confirm it, and there is no automatic migration.

Set the projection and the cell **before** you paint, and this page never
costs you a map.

:::

---

## What changes in an isometric scene

Projection is a property of the **scene**, not of the project. One project can
hold an isometric dungeon and an orthogonal menu, and each scene carries its
own.

| | Orthogonal | Isometric |
|---|---|---|
| A cell on screen | rectangle | diamond |
| Cell anchor | top-left corner | diamond **centre** |
| `+1` on X | steps right | steps right **and down** |
| `+1` on Y | steps down | steps left **and down** |
| Art vs cell | art fills its cell | art is often **taller** than its cell |
| Map outline | rectangle | diamond |

The two axes both descend, which is the thing that catches every reader once.
The canvas draws an **axis gizmo** in its lower-left corner for exactly that
reason — read it whenever the status bar's cell coordinates stop making sense.

---

## Turning a scene isometric

1. In the **SCENES** panel, click the **cubes** icon on the scene's row
2. Set **Projection** to `Isometric`
3. Check the **Cell** values (see below)
4. **Apply**

The icon turns green on any scene whose projection is isometric, so the panel
tells you at a glance which scenes are which.

### The reinterpretation warning

If the scene already has tiles in it, the dialog says so and requires an
explicit tick before **Apply** enables. That is not a formality: every index
stays where it is in the layer array, but the array is now read as a diamond
grid instead of a rectangular one, so the map you painted becomes a different
map. Editing the draft after ticking clears the tick — you agreed to one
change, not to whatever the draft became.

---

## The cell stride

The **Cell** is the *stride*: how far the next cell sits, in pixels. It is
**not** the size of your tile bitmaps.

When a scene first turns isometric the editor seeds a **2:1 diamond** — the
width rounded down to a multiple of 4, and the height half of that. A 1:1
diamond is the isometric nobody draws; it was only ever the default because the
cell inherited the project's square tile size.

::: tip Why a multiple of 4

The isometric basis halves **both** axes, so an odd cell truncates toward a
different basis than the one the export emits. The editor refuses odd cells
outright. A width that is a multiple of 4 is what keeps both halves whole.

Rounded **down** rather than up because a cell wider than the sprite spaces
tiles apart and leaves gaps, while a narrower one overlaps them — and isometric
art is drawn to overlap.

:::

The seed only applies to a cell you have **not** set yourself. A number you
typed is never overwritten.

### It is a starting point, not an answer

The stride and the sprite size are independent, and no default derived from the
tileset can know your art. A pack of 18×18 sprites can have a 16×8 base diamond
because it carries a pixel of padding on each side — so neither `18×18` nor
`18×9` is the stride.

That is what the next section is for.

---

## Fit to art

In the **TILE SET** panel header, the **square** icon measures the selected
tile's base diamond and **proposes** it as the scene's cell.

1. Select a tile that **shows its base** — a box, a floor, anything whose
   bottom is a clean diamond
2. Click **Fit cell to art**
3. The projection dialog opens with the measured `W × H` already filled in
4. Confirm with **Apply**, or cancel

It measures by walking the tile's alpha silhouette up from its bottom row while
the width keeps growing, taking the slope of that taper. Both axes are rounded
to even for the same reason the cell must be.

### It proposes; it never applies

The measurement leaves through the **same dialog** an authored cell does, so it
passes the same validation and raises the same reinterpretation warning. There
is no second path that writes the scene directly.

### When it refuses

It is a heuristic over art, and it is built to fail loudly rather than answer
confidently:

| Message | Cause | What to do |
|---------|-------|------------|
| no taper | The tile does not widen toward its base — a box-cropped sprite, orthogonal art | Pick a tile whose base is exposed |
| empty silhouette | The tile is fully transparent | Pick another tile |
| inconsistent slope | The taper is irregular — usually a soft shadow | Pick a tile with a hard edge |

A tile whose body runs straight to the sprite's last row has **no base diamond
in the picture** and is refused. Its top face is the same diamond and is
deliberately *not* used as a fallback: a top silhouette equals the base only for
a box, so falling back to it would answer confidently about a tree or a wall
corner.

::: tip Verify the measurement

With an isometric scene active, the tileset preview draws a **cyan diamond** and
an **amber foot line** over every tile. After applying a cell, the diamond
should sit on the base of your boxes. If it cuts through them, the cell is
wrong — and you can see it before painting anything.

:::

Importing a tileset never triggers this. In a mixed pack, every import would
otherwise change the scene's geometry.

---

## Reading the canvas

### The grid

The isometric grid is **dashed**, and it draws only over **empty** cells. Placed
art hides it not by covering it but by the line never being drawn there — which
is what keeps a painted map readable. A continuous line at 45° over pixel art
reads as a sprite border; a dashed one does not.

It has three levels, chosen by the cell's **smaller** axis on screen:

| Cell's shorter axis | What you get |
|---|---|
| ≥ 12 px | outline per empty cell |
| 4–12 px | long lattice, dimmer |
| < 4 px | nothing |

The smaller axis decides because a 16×8 diamond at 0.4× is 6.4 px wide but only
3.2 px tall — the vertical axis runs out first.

On a very large **empty** map the per-cell mode falls back to the long lattice
rather than emitting tens of thousands of outlines per frame. A painted map has
few empty cells left, so you will rarely see it switch.

The **orthogonal** grid is unchanged: solid lines, drawn over everything. There
a straight line lands on the tile's own border, which is where it belongs.

### The overlays

| Overlay | What it means |
|---------|---------------|
| **SCENE** outline, green | The area you are working in. A diamond in isometric. This is the primary border |
| **VIEWPORT** / **ONE SCREEN**, cyan dashed | The project's screen. It reads `ONE SCREEN` once the scene is bigger than one, because at that point it is a reference for how much is visible at once, not a limit |
| **MAX CANVAS**, red | The hard 255-cell paint ceiling. Drawn **only** when you get near it |

The scene outline moves when you paint past the scene's edge, because the scene
grows. That is information, not noise: the editor exports the **whole** map and
your game scrolls a camera over it, so the scene's border is the number you
work with.

The screen overlay can be switched off in **File → Preferences → Show device
screen overlay**. It ships on.

### The axis gizmo

Lower-left corner, pinned to the canvas rather than to the world so it survives
any pan or zoom. Red is `+X`, blue is `+Y`, and **both point down**.

---

## Painting

The tools are the orthogonal ones — brush, eraser, rectangle, pipette, layers,
animations, attributes. Three things behave differently:

- **The cell highlight is a diamond**, and so is the rectangle tool's preview.
  Both show the shape the write will actually produce rather than a
  screen-aligned box.
- **Tall art overhangs its cell.** A 32×40 wall on a 32×16 diamond reaches
  24 px above its own anchor. The tile you are placing is drawn where it will
  land, foot and all.
- **Hold `Alt`** to dim the sprites standing in front of the cell under the
  cursor, so you can see what you are painting underneath them.

Row-major order is already back-to-front under the isometric basis, so tiles
later in the layer paint over earlier ones automatically. There is no depth
sort and none is needed.

---

## Export

An isometric scene exports the same `.h`/`.cpp` pair as an orthogonal one, in
a different shape. Full field-by-field detail is in the
[Technical Reference](/tools/tilemap-editor/technical-reference#isometric-projection).

The header is split in two. The geometry gameplay needs is always compiled;
the tile data sits behind the engine's switch for its colour depth:

```cpp
// level1.h (abridged)
#include <math/Projection.h>

namespace level1 {
    inline constexpr uint8_t TILE_WIDTH  = 32;   // the CELL stride,
    inline constexpr uint8_t TILE_HEIGHT = 16;   // not the bitmap size
    inline constexpr uint8_t MAP_WIDTH   = 21;
    inline constexpr uint8_t MAP_HEIGHT  = 7;

    inline constexpr pixelroot32::math::ProjectionSpec ISO_PROJECTION{
        120, 88, 16, 8, -16, 8};
    // + three static_asserts on ISO_PROJECTION

    inline constexpr uint8_t TILESET_MAX_TILE_HEIGHT = 40;  // tallest tile
    // + SPAWN_TILE_X/Y, room metadata, behaviour layers (when the scene has them)
}

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES      // _2BPP_ at 2bpp; none at 1bpp
namespace level1 {
    inline constexpr uint16_t TILESET_TILE_COUNT = 7;
    extern const pixelroot32::graphics::Sprite4bpp TILESET_SPRITES[TILESET_TILE_COUNT];
    extern const uint8_t TILESET_FOOT_Y[TILESET_TILE_COUNT];
    extern pixelroot32::graphics::TileMap4bpp tiles;   // one per layer
    void init();
}
#endif
```

- **`ISO_PROJECTION`** — the six integers that map a cell to the screen
- **`TILESET_FOOT_Y`** — which bitmap row of each tile sits on its cell centre
- **`TILE_WIDTH` / `TILE_HEIGHT`** — the **cell stride**, which for a tall tile
  is smaller than its bitmap
- **`TILESET_MAX_TILE_HEIGHT`** — the tallest tile in pixels; a tile overhangs
  its cell by up to this minus `TILE_HEIGHT`. Size cull margins and depth
  bands from it instead of hard-coding a number.
- **`TILESET_SPRITES`** — one descriptor per tile, with **external linkage**:
  every translation unit that includes the header sees the same array in flash

The `.cpp` wraps its whole body in the same `#ifdef`. A build without the
depth switch still compiles everything that places actors on cells; it is the
draw that needs the flag. All the scalar constants are `inline constexpr`, so
your own `constexpr` values and `static_assert`s can be built from them.

The export's `init()` wires the foot table onto every layer, so a game calls
`init()`, draws through `ISO_PROJECTION`, and is done:

```cpp
#include "level1.h"

level1::init();
renderer.drawTileMap(level1::tiles, 0, 0,
                     pixelroot32::graphics::LayerType::Static,
                     level1::ISO_PROJECTION);
```

If your scene gives each layer its own palette slot, each layer gets its own
tileset and therefore its own table — `BACKGROUND_TILESET_FOOT_Y`,
`LAYER_1_TILESET_FOOT_Y`, and so on.

### What else the isometric export does for you

- **Slot 0 is an empty, zero-extent tile** — `{ nullptr, PALETTE, 0, 0, … }`,
  with no bytes in the data pool and a foot of 0. Index 0 means "no tile" and
  `drawTileMap` skips it, but the projected path still reads `tiles[0]` when it
  sizes its cull window, so the slot must exist; zero extent keeps it from
  widening that window. An orthogonal export keeps its full-size blank tile.
- **Span tables are precomputed in flash** (4bpp and 2bpp). Each descriptor
  points at `TILESET_SPAN_MIN_X` / `TILESET_SPAN_MAX_X`, the opaque run of every
  row, exactly what the engine's `computeSpanTable` would produce. The renderer
  skips transparent padding without a RAM copy of the tileset and without a
  `computeSpanTable` pass at startup — draw straight from `TILESET_SPRITES`.
- **The header states its own invariants** with three `static_assert`s on
  `ISO_PROJECTION`. They fire at **your** compile, which is deliberate: a
  projection the engine cannot use should stop the build that would ship it.

An **orthogonal** export is unchanged — no projection, no foot table, no span
tables, one tile size, no depth guard split.

### Installing the palette

**Install palette in init()**, under *Export options* in the export dialog, picks
which palette bank `init()` fills with the scene's palette:

| Option | `init()` calls | Use it when |
|--------|----------------|-------------|
| **None** (default) | nothing | your game installs palettes itself |
| **Background** | `setBackgroundCustomPalette(...)` | sprites use a palette of their own |
| **Dual (background + sprites)** | `setDualCustomPalette(...)` with the scene palette in both banks | tiles and sprites share one palette |

`drawTileMap` reads the **background** bank and `drawSprite` the **sprite**
bank, so with *None* the tiles draw with whatever background palette happens to
be installed. The option works in orthogonal scenes too. In multi-palette mode
it installs the lowest slot's palette before the per-slot calls, which still
win. At 1bpp there is no RGB565 palette, so every option behaves as *None*.

### Width limit at 1bpp

An engine 1bpp `Sprite` stores one `uint16_t` per row, so a 1bpp tile can be
at most **16 px** wide; bit `width - 1` is the leftmost pixel, and transparent
pixels export as off. Isometric art is rarely that narrow: when a 1bpp
isometric scene uses a tile wider than 16 px, the export reports a warning
naming the widest tile. The files are still written, but those tiles will not
draw as authored — add a colour so the scene exports at 2bpp, or use narrower
tiles.

### The projection origin

By default the exported origin is the map's own inset, so the map starts at
`(0,0)` in its own pixel space and your game scrolls a camera over it. Tick
**Custom screen origin** in the projection dialog only for a fixed, one-screen
room — the `iso_dungeon` demo's project sets `(120, 88)` to place each 7×7
room on its 240×240 screen.

---

## Rooms

Rooms are authored the same way in every scene — see
[Rooms](/tools/tilemap-editor/usage-guide#rooms) in the Usage Guide. In an
isometric scene the export adds **one map per room**, so a game can draw one
room without drawing, or even storing a reference to, the rest of the scene.

For a scene file `iso_dungeon_rooms.h` with one layer called *Tiles* and three
7×7 rooms, the header gains:

```cpp
#if PIXELROOT32_ENABLE_GAMEPLAY_ROOM
inline constexpr pixelroot32::gameplay::RoomData ISO_DUNGEON_ROOMS_ROOMS[] = {
    // originCol, originRow, cols, rows, { Up, Down, Left, Right }
    {  0, 0, 7, 7, {      1, 0xFFFF,      2, 0xFFFF } },
    {  7, 0, 7, 7, { 0xFFFF, 0xFFFF,      0, 0xFFFF } },
    { 14, 0, 7, 7, {      0, 0xFFFF, 0xFFFF, 0xFFFF } },
};
inline constexpr pixelroot32::gameplay::RoomLayer ISO_DUNGEON_ROOMS_ROOM_LAYER = {
    ISO_DUNGEON_ROOMS_ROOMS, 3, 32, 16   // rooms, roomCount, CELL stride
};
#endif

// inside the depth guard
extern pixelroot32::graphics::TileMap4bpp tiles_room0;   // <layer>_room<r>
extern pixelroot32::graphics::TileMap4bpp tiles_room1;
extern pixelroot32::graphics::TileMap4bpp tiles_room2;
inline constexpr uint8_t NUM_ROOM_LAYERS = 3;            // one map per room
extern pixelroot32::graphics::TileMap4bpp* const ROOM_LAYERS[NUM_ROOM_LAYERS];
```

- Each room map is cropped to its room: **local cells from (0, 0)**, the same
  shared tileset, foot table and span tables as the whole-scene map. `init()`
  wires them all.
- `ROOMS` keeps each room's origin in **scene** cells, and `ROOM_LAYER` carries
  the **cell stride** (32×16), not the tile bitmap size.
- The whole-scene map (`tiles`) is still exported, for code that draws the
  scene in one call.
- With several layers, `ROOM_LAYERS` holds the bottom layer's maps, and the
  header adds `LAYERS_PER_ROOM` and one `<LAYER>_ROOM_LAYERS[]` per layer:
  drawing room `r` is one `drawTileMap` per layer, bottom to top.
- Behaviour and attribute layers are **not** cropped. They stay in scene cells;
  add the room's `originCol`/`originRow` when you query them.

An orthogonal scene with rooms exports its room metadata as before, with no
room maps.

### Drawing a room

**One screen per room** (what `iso_dungeon` does): draw the current room's map
through the scene's own `ISO_PROJECTION`. Every room lands at the same place on
screen, and no camera is needed:

```cpp
using namespace iso_dungeon;
rooms::init();

pixelroot32::gameplay::RoomGraph<rooms::NUM_ROOM_LAYERS> graph;
pixelroot32::gameplay::buildRoomGraph(rooms::ISO_DUNGEON_ROOMS_ROOM_LAYER, graph);
graph.enterRoom(0, /*camera=*/nullptr);

// draw()
renderer.drawTileMap(*rooms::ROOM_LAYERS[graph.currentRoomIndex()], 0, 0,
                     pixelroot32::graphics::LayerType::Static,
                     rooms::ISO_PROJECTION);
```

**Rooms placed in a scrolling scene**: give `buildRoomGraph` the projection, so
each room's camera rect is the screen box of its cell diamonds (exactly what a
floor-only room draws, 32×16 floor tiles anchored at foot row 8), and draw each
room through `roomProjection`, which moves the origin to the room's first cell
so the room lands where it sits in the scene:

```cpp
namespace gp = pixelroot32::gameplay;
using namespace iso_dungeon;

if (gp::buildRoomGraph(rooms::ISO_DUNGEON_ROOMS_ROOM_LAYER, graph,
                       rooms::ISO_PROJECTION) > 0) {
    setRoomGraph(&graph);
    graph.enterRoom(0, &camera);
}

// draw()
const uint16_t r = graph.currentRoomIndex();
renderer.drawTileMap(*rooms::ROOM_LAYERS[r], 0, 0,
                     pixelroot32::graphics::LayerType::Static,
                     gp::roomProjection(rooms::ISO_PROJECTION,
                                        rooms::ISO_DUNGEON_ROOMS_ROOMS[r]));
```

`buildRoomGraph` takes an optional fourth argument, one
`graphics::ScreenBounds` per room (typically `expandProjectedMapBounds` over
that room's map), to widen each camera rect by how far tall art overhangs the
cells. Both need `PIXELROOT32_ENABLE_GAMEPLAY_ROOM` and
`PIXELROOT32_ENABLE_TILEMAP_PROJECTION`.

---

## Re-exporting

The exported files are output, not source: the editor overwrites them on every
export, so keep game code in your own files and change the map in the editor.

1. **File → Open...** the `.pr32scene.bin`, edit the scene, **File → Save**
2. **File → Export...** (Ctrl+E). The project file does **not** store the
   export settings, so set them again every time: **C++ Namespace**,
   **Store data in Flash (ESP32)**, **Legacy format**, **Install palette in
   init()**. Leave **Color Depth** auto-detected.
3. **Export Now** and pick the same folder. The editor overwrites
   `<namespace>_<scene>.h`, `<namespace>_<scene>.cpp` and
   `<namespace>_tilemap_palette.h`; the scene's code lives in namespace
   `<namespace>::<scene>`.
4. Rebuild. The `static_assert`s and your own `static_assert`s against the
   exported constants catch a scene that no longer fits your game.

The [`iso_dungeon` and `iso_tilemap_export` demos](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/graphics)
keep their editor projects next to the code and list their exact export
settings in their READMEs.

---

## Troubleshooting

**The tiles do not sit in the grid.** The cell is not your art's base diamond.
Use **Fit to art** on a tile that shows its base, and check the cyan diamond in
the tileset preview.

**"An isometric cell must be even on both axes."** The projection halves both
axes and an odd number truncates. Round to even — and prefer a width that is a
multiple of 4, so the height stays even too.

**Fit to art refuses every tile.** They are probably flat-bottomed: their body
runs to the sprite's last row, so the base diamond is not in the picture. Pick a
free-standing box or a floor tile.

**A red diamond crosses the whole canvas.** You are within 32 cells of the
255-cell paint ceiling. It only appears when it is about to matter.

**Sprites overlap wrongly after changing the cell.** Changing the cell
reinterprets the scene. There is no automatic migration; undo, or set the cell
before painting.

**My game does not compile after export.** Read the `static_assert` message —
it names which invariant the projection broke. This is the intended failure
mode; the alternative is a map that draws wrong at runtime.

**`TILESET_SPRITES`, the maps or `init()` are "not declared".** The tile data
sits behind `#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES` (or `_2BPP_` at 2bpp).
Add the flag to your build; the geometry above the guard compiles either way.

**The export warns about 1bpp tile widths.** The scene came out 1bpp and uses
tiles wider than 16 px, which an engine 1bpp sprite cannot hold. See
[Width limit at 1bpp](#width-limit-at-1bpp).

**The tiles draw in the wrong colours.** `drawTileMap` reads the background
palette bank. Export with **Install palette in init()** set to *Background* or
*Dual*, or install the palette yourself before the first draw.

**`roomProjection` or the four-argument `buildRoomGraph` does not exist.**
Both exist since engine 1.15.0: a project on an older engine should upgrade,
or use the one-screen-per-room pattern in
[Drawing a room](#drawing-a-room) meanwhile.

---

## Related Guides

- [Usage Guide](/tools/tilemap-editor/usage-guide) - scenes, layers, tilesets, export
- [Advanced Guide](/tools/tilemap-editor/advanced-guide) - animations, attributes, multi-palette
- [Technical Reference](/tools/tilemap-editor/technical-reference) - formats, limits, generated code
