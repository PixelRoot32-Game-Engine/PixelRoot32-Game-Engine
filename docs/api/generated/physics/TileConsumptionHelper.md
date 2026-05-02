# TileConsumptionHelper

<Badge type="info" text="Class" />

**Source:** `TileConsumptionHelper.h`

## Description

Helper class for consuming tiles (removing bodies and updating visuals).

This class implements Phase 7 of the tile attribute system:
1. Remove tile body from Scene (CollisionSystem no longer considers it)
2. Update TileMapGeneric::runtimeMask to hide consumed tiles
3. Reuse existing runtimeMask instead of creating separate consumedMask

Usage:
```cpp
TileConsumptionHelper helper(scene, tilemap, config);
bool consumed = helper.consumeTile(tileActor, tileX, tileY);
```

## Methods

### `bool isTileConsumed(uint16_t tileX, uint16_t tileY) const`

**Description:**

Check if a tile has been consumed (hidden in tilemap).

**Parameters:**

- `tileX`: Tile X coordinate
- `tileY`: Tile Y coordinate

**Returns:** true if tile is consumed (inactive), false if still visible

### `bool restoreTile(uint16_t tileX, uint16_t tileY)`

**Description:**

Restore a consumed tile (for debugging or special game mechanics).

**Parameters:**

- `tileX`: Tile X coordinate
- `tileY`: Tile Y coordinate

**Returns:** true if tile was restored, false if tile was not consumed

### `void extractTilemapDimensions()`

**Description:**

Extract tilemap dimensions from the tilemap pointer.

### `void updateTilemapRuntimeMask(uint16_t tileX, uint16_t tileY, bool active)`

**Description:**

Template method to update tilemap runtimeMask.

### `bool checkTilemapRuntimeMask(uint16_t tileX, uint16_t tileY) const`

**Description:**

Template method to check tilemap runtimeMask state.

### `bool validateCoordinates(uint16_t tileX, uint16_t tileY) const`

**Description:**

Validate tile coordinates against tilemap dimensions.

### `TileConsumptionHelper helper(scene, tilemap, config)`

**Description:**

Convenience function for consuming tiles from collision callbacks.

**Parameters:**

- `tileActor`: Pointer to the tile physics actor
- `packedUserData`: Packed userData from tileActor
- `scene`: Reference to the scene
- `tilemap`: Pointer to the tilemap (TileMapGeneric*)
- `config`: Optional consumption configuration

**Returns:** true if tile was consumed, false otherwise
