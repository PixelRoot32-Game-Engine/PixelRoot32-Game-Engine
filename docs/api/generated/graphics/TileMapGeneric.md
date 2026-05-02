# TileMapGeneric

<Badge type="info" text="Struct" />

**Source:** `Renderer.h`

## Description

Generic tilemap structure supporting 1bpp, 2bpp, or 4bpp tile graphics.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `runtimeMask` | `uint8_t*` | Bitmask for runtime tile activation (1 bit per tile, nullptr = all active) |
| `animManager` | `TileAnimationManager*` | Optional animation manager for tile animations |

## Methods

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
