# StaticLayerSnapshot

<Badge type="info" text="Class" />

**Source:** `StaticLayerSnapshot.h`

## Description

Framebuffer cache for static layers a game draws ITSELF.

`StaticTilemapLayerCache` solves the same problem for games whose background
is a `TileMap4bpp`: it owns the tilemaps, redraws them when the cache goes
cold, and snapshots the result. That ownership is exactly what an isometric
or oblique room cannot satisfy -- `drawTileMap()` assumes axis-aligned cells
and cannot express a diamond, so such a room is drawn sprite-per-cell by game
code and there is no `TileMap4bpp` to hand over.

This class inverts the relationship: it never draws anything. The game says
"the framebuffer now holds my static layers" (capture()) and, on later
frames, "put them back" (restore()). What those layers are, and how they were
drawn, is none of its business -- which is the whole point, and why this is
projection-agnostic where the tilemap cache is not.

### What it costs, stated plainly

One full logical framebuffer of heap: 57,600 B at 240x240. That is the entire
trade. In exchange a static layer of N sprites costs zero draw calls per
frame instead of N, and with dirty regions enabled the restore touches only
the cells last frame's movers dirtied rather than the whole buffer.

### Restore has two speeds, and the fast one needs dirty regions

With `PIXELROOT32_ENABLE_DIRTY_REGIONS`, restore() repaints only the
previously-dirtied cells -- for a room with one moving actor that is a few
hundred bytes. Without it, there is no record of what moved, so restore()
falls back to copying the whole buffer. Both are correct; the selective path
is roughly two orders of magnitude cheaper. See restore().

### Usage


```cpp
// Scene::init() -- allocate off the game loop (see ARCH_MEMORY_SYSTEM.md).
if (!snapshot_.allocateForRenderer(engine.getRenderer())) {
    // Out of memory: every call below degrades to "draw normally".
}

// Scene::adviseFramebufferBeforeBeginFrame() -- lets beginFrame skip work
// restore() is about to redo anyway.
snapshot_.adviseFramebufferBeforeBeginFrame(renderer);

// In the static layer's draw(), before anything dynamic:
if (!snapshot_.restore(renderer)) {
    drawMyStaticLayers(renderer);   // cache cold, or unavailable
    snapshot_.capture(renderer);
}
```


Call invalidate() whenever the static layers would draw differently -- a
palette swap, a camera move, a door opening. The class cannot detect that on
its own and deliberately does not try: it has no idea what it is holding.

## Methods

### `void clear()`

### `bool allocateForLogicalSize(int width, int height)`

**Description:**

Pre-allocates a buffer of width * height bytes.

**Returns:** false if the dimensions are invalid or the allocation failed. A
        false return is not fatal: every other method then reports
        "unavailable" and the caller draws normally.

### `bool allocateForRenderer(const Renderer& renderer)`

**Description:**

Same as allocateForLogicalSize(renderer.getLogicalWidth/Height()).

### `void invalidate()`

### `bool isValid() const`

### `bool capture(Renderer& renderer)`

**Description:**

Copies the current framebuffer into the snapshot.

**Returns:** false when there is no buffer or the driver exposes no 8bpp
        framebuffer; the snapshot stays invalid and restore() keeps
        reporting cold.

### `bool restore(Renderer& renderer)`

**Description:**

Puts the static layers back onto the framebuffer.

**Returns:** false when the snapshot is cold or unavailable, which is the
        caller's signal to draw its static layers and capture() them.

### `void adviseFramebufferBeforeBeginFrame(Renderer& renderer) const`

**Description:**

Tells the renderer it may skip its own clear this frame.
