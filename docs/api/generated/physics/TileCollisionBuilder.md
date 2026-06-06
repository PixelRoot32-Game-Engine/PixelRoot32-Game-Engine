# TileCollisionBuilder

<Badge type="info" text="Class" />

**Source:** `TileCollisionBuilder.h`

## Description

Helper class for creating physics bodies from exported behavior layers.

This builder follows the exact plan specification:
- Iterates behavior layers looking for non-TILE_NONE flags
- Creates StaticActor or SensorActor based on TileFlags
- Configures sensor/one-way properties from flags
- Packs tile coordinates and flags into userData for gameplay callbacks
- Registers bodies with the scene's entity system and collision system

Usage:
```cpp
TileBehaviorLayer layer = { behaviorData, 32, 32 };
TileCollisionBuilder builder(scene, config);
int entitiesCreated = builder.buildFromBehaviorLayer(layer, 0);
```

## Methods

### `int buildFromBehaviorLayer(const TileBehaviorLayer& layer, uint8_t layerIndex = 0)`

**Description:**

Creates physics bodies from a behavior layer.

**Parameters:**

- `layer`: Behavior layer containing tile flags
- `layerIndex`: Index of the layer (for debugging/logging)

**Returns:** Number of entities created, or -1 if entity limit was exceeded

### `int getEntitiesCreated() const`

**Description:**

Gets the number of entities created by this builder.

**Returns:** Entity count

### `void reset()`

**Description:**

Resets the entity counter.

### `pixelroot32::core::PhysicsActor* createTileBody(uint16_t x, uint16_t y, TileFlags flags)`

**Description:**

Creates a single tile physics body.

**Parameters:**

- `x`: Tile X coordinate
- `y`: Tile Y coordinate
- `flags`: TileFlags for this tile

**Returns:** Pointer to created actor, or nullptr if entity limit reached

### `void configureTileBody(pixelroot32::core::PhysicsActor* body, TileFlags flags)`

**Description:**

Configures a tile body based on its flags.

**Parameters:**

- `body`: The physics actor to configure
- `flags`: TileFlags for configuration

### `pixelroot32::math::Vector2 tileToWorldPosition(uint16_t x, uint16_t y) const`

**Description:**

Converts tile coordinates to world position.

**Parameters:**

- `x`: Tile X coordinate
- `y`: Tile Y coordinate

**Returns:** World position vector

### `inline int buildTileCollisions( pixelroot32::core::Scene& scene, const TileBehaviorLayer& layer, uint8_t tileWidth = 16, uint8_t tileHeight = 16, uint8_t layerIndex = 0 )`

**Description:**

Convenience function for building collision bodies from behavior layers.

**Parameters:**

- `scene`: Scene to add entities to
- `layer`: Behavior layer to process
- `tileWidth`: Width of tiles in world units
- `tileHeight`: Height of tiles in world units
- `layerIndex`: Layer index for debugging

**Returns:** Number of entities created, or -1 if entity limit exceeded
