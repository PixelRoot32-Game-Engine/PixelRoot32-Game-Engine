---
title: "Isometric Guide"
description: "Isometric scenes in the Tilemap Editor: projection, cell stride, fitting the cell to your art, canvas overlays, and what the export adds"
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

Everything works as it does orthogonally — brush, eraser, rectangle, pipette,
layers, animations, attributes. Three things behave differently:

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

An isometric scene's export adds three things to the header. Full field-by-field
detail is in the
[Technical Reference](/tools/tilemap-editor/technical-reference#isometric-projection).

```cpp
#include <math/Projection.h>

inline constexpr uint8_t TILE_WIDTH  = 32;   // the CELL stride
inline constexpr uint8_t TILE_HEIGHT = 16;   // not the bitmap size

inline constexpr pixelroot32::math::ProjectionSpec ISO_PROJECTION{ /* six ints */ };

extern const uint8_t TILESET_FOOT_Y[TILESET_TILE_COUNT];
```

- **`ISO_PROJECTION`** — the six integers that map a cell to the screen
- **`TILESET_FOOT_Y`** — which bitmap row of each tile sits on its cell centre
- **`TILE_WIDTH` / `TILE_HEIGHT`** — the **cell stride**, which for a tall tile
  is smaller than its bitmap

The header states its own invariants with `static_assert`. They fire at **your**
compile, which is deliberate: a projection the engine cannot use should stop the
build that would ship it.

An **orthogonal** export is unchanged — no projection, no foot table, one tile
size.

### The projection origin

By default the exported origin is the map's own inset, so the map starts at
`(0,0)` in its own pixel space and your game scrolls a camera over it. Tick
**Custom screen origin** in the projection dialog only for a fixed, one-screen
room — the shipped `iso_dungeon` uses `(120, 88)`, placed by hand.

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

---

## Related Guides

- [Usage Guide](/tools/tilemap-editor/usage-guide) - scenes, layers, tilesets, export
- [Advanced Guide](/tools/tilemap-editor/advanced-guide) - animations, attributes, multi-palette
- [Technical Reference](/tools/tilemap-editor/technical-reference) - formats, limits, generated code
