# TileMapGeneric

<Badge type="info" text="Struct" />

**Source:** `Renderer.h`

## Description

Generic tilemap structure supporting 1bpp, 2bpp, or 4bpp tile graphics.

T The sprite type used for tiles (Sprite, Sprite2bpp, or Sprite4bpp).

## Properties

| Name | Type | Description |
|------|------|-------------|
| `indices` | `uint8_t*` | Pointer to tile indices array (size = width * height). |
| `width` | `uint8_t` | Map width in tiles. |
| `height` | `uint8_t` | Map height in tiles. |
| `T` | `const` | Pointer to tileset array. |
| `tileWidth` | `uint8_t` | Width of each tile in pixels. |
| `tileHeight` | `uint8_t` | Height of each tile in pixels. |
| `tileCount` | `uint16_t` | Number of unique tiles in the tileset. |
| `runtimeMask` | `uint8_t*` | Bitmask for runtime tile activation (1 bit per tile, nullptr = all active) |
| `animManager` | `TileAnimationManager*` | Optional animation manager for tile animations |

## Methods

### `inline uint8_t footYFor(uint16_t index) const`

**Description:**

Resolve the foot-anchor row for a tile index.

**Parameters:**

- `index`: Tile index into `tiles[]`.

**Returns:** `tileFootY[index]` when a table is present and `index` is in
        range; 0 otherwise (current top-left behaviour).

::: tip
`index >= tileCount` returns 0. `drawTileMap` already guards
      `index >= map.tileCount` (src/graphics/Renderer.cpp:892), so this
      is defence in depth against a caller that does not, not a
      load-bearing branch.
:::

::: tip
`footYFor(0)` reads the table with no special case, even though
      index 0 is the empty-tile sentinel `drawTileMap` skips. The table
      is parallel to `tiles[]`, so slot 0 exists; special-casing it
      would bind the asset format's meaning to a renderer policy that
      is free to change.
:::

### `inline void initRuntimeMask()`

**Description:**

Initialize runtime mask buffer for tile activation control.

::: tip
Must be called before using isTileActive() or setTileActive()
:::

::: tip
Existing mask is freed if already allocated
:::

### `inline bool isTileActive(int x, int y) const`

**Description:**

Check if a tile is currently active (visible).

**Parameters:**

- `x`: Tile X coordinate
- `y`: Tile Y coordinate

**Returns:** true if tile is active, false if inactive

::: tip
Returns true for out-of-bounds coordinates or when no mask is initialized
:::

### `inline void setTileActive(int x, int y, bool active)`

**Description:**

Set tile activation state.

**Parameters:**

- `x`: Tile X coordinate
- `y`: Tile Y coordinate
- `active`: true to activate tile (visible), false to deactivate (hidden)

::: tip
Out-of-bounds coordinates are ignored
:::

### `inline uint8_t* getRuntimeMask() const`

**Description:**

Get pointer to runtime mask buffer.

**Returns:** Pointer to runtime mask array, or nullptr if not initialized

### `inline void cleanupRuntimeMask()`

**Description:**

Destructor cleanup for runtime mask.
