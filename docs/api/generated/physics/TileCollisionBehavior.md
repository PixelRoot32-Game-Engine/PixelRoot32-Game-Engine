# TileCollisionBehavior

<Badge type="info" text="Enum" />

**Source:** `TileAttributes.h`

## Description

Defines how a tile collider behaves in the physics system.

Use TileFlags for new implementations. Kept for backward compatibility.

## Methods

### `inline uintptr_t packTileData(uint16_t x, uint16_t y, TileFlags flags)`

**Description:**

Pack tile coordinates and TileFlags into a single value for userData.
Encoding: bits [0-9]=x, [10-19]=y, [20-27]=flags. Max 1024x1024 tiles.

**Parameters:**

- `x`: Tile X (0..1023).
- `y`: Tile Y (0..1023).
- `flags`: TileFlags combination.

**Returns:** Packed value to store via setUserData(reinterpret_cast<void*>(packed)).

### `inline void unpackTileData(uintptr_t packed, uint16_t& x, uint16_t& y, TileFlags& flags)`

**Description:**

Unpack tile data from userData with TileFlags.

**Parameters:**

- `packed`: Packed value from userData.
- `x`: Output tile X coordinate.
- `y`: Output tile Y coordinate.
- `flags`: Output TileFlags combination.

### `inline uintptr_t packTileData(uint16_t x, uint16_t y, TileCollisionBehavior behavior)`

**Description:**

Pack tile coordinates and TileCollisionBehavior into a single value for userData.
Encoding: bits [0-9]=x, [10-19]=y, [20-23]=behavior. Max 1024x1024 tiles.

**Parameters:**

- `x`: Tile X (0..1023).
- `y`: Tile Y (0..1023).
- `behavior`: TileCollisionBehavior.

**Returns:** Packed value to store via setUserData(reinterpret_cast<void*>(packed)).

### `inline void unpackTileData(uintptr_t packed, uint16_t& x, uint16_t& y, TileCollisionBehavior& behavior)`

**Description:**

Unpack tile data from userData with TileCollisionBehavior.

**Parameters:**

- `packed`: Packed value from userData.
- `x`: Output tile X coordinate.
- `y`: Output tile Y coordinate.
- `behavior`: Output TileCollisionBehavior.

### `inline bool isSensorTile(TileFlags flags)`

**Description:**

Helper to derive sensor flag from TileFlags combination.

**Parameters:**

- `flags`: TileFlags combination.

**Returns:** true if tile should be configured as sensor.

### `inline bool isOneWayTile(TileFlags flags)`

**Description:**

Helper to derive one-way flag from TileFlags combination.

**Parameters:**

- `flags`: TileFlags combination.

**Returns:** true if tile should be configured as one-way platform.

### `inline bool isSolidTile(TileFlags flags)`

**Description:**

Helper to derive solid flag from TileFlags combination.

**Parameters:**

- `flags`: TileFlags combination.

**Returns:** true if tile should be configured as solid body.

### `inline uintptr_t packCoord(uint16_t x, uint16_t y)`

**Description:**

Legacy: pack only coordinates (16+16 bits). Compatible with existing userData usage.
Use when TileCollisionBehavior is not needed (max 65535x65535 tiles).

### `inline void unpackCoord(uintptr_t packed, uint16_t& x, uint16_t& y)`

**Description:**

Legacy: unpack coordinates from legacy encoding.
