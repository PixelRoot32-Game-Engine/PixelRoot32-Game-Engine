# SpatialGrid

<Badge type="info" text="Class" />

**Source:** `SpatialGrid.h`

## Description

Optimized spatial partitioning with separate static/dynamic layers.

Static layer: built once per level (or when entities change), not cleared each frame.
Dynamic layer: cleared and refilled every frame (RIGID, KINEMATIC).
Reduces per-frame cost when many static tiles are present.

## Methods

### `void clearDynamic()`

**Description:**

Clears all dynamic entities from the grid.

### `void clear()`

**Description:**

Clears all entities (static and dynamic) from the grid.

### `void markStaticDirty()`

**Description:**

Marks the static layer as dirty, requiring a rebuild.

### `void rebuildStaticIfNeeded(pixelroot32::core::Entity* const* entities, uint16_t entityCount)`

**Description:**

Rebuilds the static layer if marked dirty.

**Parameters:**

- `entities`: Pointer to array of entities.
- `entityCount`: Total number of entities.

### `void insertDynamic(pixelroot32::core::Actor* actor)`

**Description:**

Inserts a dynamic actor into the grid.

**Parameters:**

- `actor`: The actor to insert.

### `void getPotentialColliders(pixelroot32::core::Actor* actor, pixelroot32::core::Actor** outArray, int& count, int maxCount)`

**Description:**

Gets potential colliders for a given actor from the grid.

**Parameters:**

- `actor`: The actor to query for.
- `outArray`: Output array to store potential colliders.
- `count`: Reference to store the number of colliders found.
- `maxCount`: Maximum number of colliders to return.
