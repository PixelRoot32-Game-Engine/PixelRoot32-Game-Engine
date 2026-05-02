# TileBehaviorLayer

<Badge type="info" text="Struct" />

**Source:** `TileAttributes.h`

## Description

Runtime representation of exported behavior layer for O(1) flag lookup.

This structure matches the format exported by the Tilemap Editor
and provides efficient access to tile behavior flags without runtime strings.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `uint8_t` | `const` | Pointer to dense uint8_t array (1 byte per tile) |
| `width` | `uint16_t` | Layer width in tiles |
| `height` | `uint16_t` | Layer height in tiles |

## Methods

### `inline uint8_t getTileFlags(const TileBehaviorLayer& layer, int x, int y)`

**Description:**

Get TileFlags for a specific tile position in a behavior layer.

**Parameters:**

- `layer`: Behavior layer structure containing the data
- `x`: X coordinate in tiles
- `y`: Y coordinate in tiles

**Returns:** TileFlags combination (0 = TILE_NONE if out of bounds)
