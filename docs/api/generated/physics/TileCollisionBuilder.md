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
