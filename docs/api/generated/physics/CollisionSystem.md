# CollisionSystem

<Badge type="info" text="Class" />

**Source:** `CollisionSystem.h`

## Description

Manages physics simulation and collision detection for all actors.

## Methods

### `void addEntity(pixelroot32::core::Entity* e)`

**Description:**

Adds an entity to the collision system.

**Parameters:**

- `e`: Pointer to the entity to add.

### `void removeEntity(pixelroot32::core::Entity* e)`

**Description:**

Removes an entity from the collision system.

**Parameters:**

- `e`: Pointer to the entity to remove.

### `void update()`

**Description:**

Performs one complete physics update step.

### `void detectCollisions()`

**Description:**

Detects collisions between all registered bodies.

### `void solveVelocity()`

**Description:**

Solves velocities for all contacts.

### `void integratePositions()`

**Description:**

Integrates positions for all dynamic bodies.

### `void solvePenetration()`

**Description:**

Solves penetration to separate overlapping bodies.

### `void triggerCallbacks()`

**Description:**

Triggers collision callbacks for all valid contacts.

### `void setInteractionTracker(pixelroot32::gameplay::InteractionTracker* tracker)`

**Description:**

Sets the (optional, non-owning) interaction tracker that
       observes trigger enter/exit edges from triggerCallbacks().

### `size_t getEntityCount() const`

**Description:**

Gets the total number of registered entities.

**Returns:** Number of entities.

### `void clear()`

**Description:**

Clears the collision system state.

### `bool checkCollision(pixelroot32::core::Actor* actor, pixelroot32::core::Actor** outArray, int& count, int maxCount)`

**Description:**

Checks for collisions with a specific actor.

**Parameters:**

- `actor`: The actor to check.
- `outArray`: Array to store colliding actors.
- `count`: Reference to store the number of collisions found.
- `maxCount`: Maximum number of collisions to return.

**Returns:** True if any collisions were found.

### `int queryRadius(pixelroot32::math::Vector2 center, pixelroot32::math::Scalar radius, CollisionLayer mask, pixelroot32::core::Actor** outArray, int maxCount)`

**Description:**

Layer-aware radius query, backed by SpatialGrid::queryRadius().

**Parameters:**

- `center`: Query circle center.
- `radius`: Query circle radius (clamped/asserted, see above).
- `mask`: Caller-supplied layer mask; only `other->layer` is tested.
- `outArray`: Output array to store matching actors.
- `maxCount`: Maximum number of actors to write to outArray.

**Returns:** Number of actors written to outArray.

::: warning
Cross-scene visibility limitation (accepted, not fixed by
this capability): see SpatialGrid::queryRadius(). A query issued
against this CollisionSystem's grid MAY return static geometry
registered by another simultaneously-alive scene's
CollisionSystem/SpatialGrid. Pre-existing, documented, unguarded.
:::

### `int queryBox(const pixelroot32::core::Rect& box, CollisionLayer mask, pixelroot32::core::Actor** outArray, int maxCount)`

**Description:**

Layer-aware box query, backed by SpatialGrid::queryBox().

**Parameters:**

- `box`: Query rectangle.
- `mask`: Caller-supplied layer mask; only `other->layer` is tested.
- `outArray`: Output array to store matching actors.
- `maxCount`: Maximum number of actors to write to outArray.

**Returns:** Number of actors written to outArray.

### `bool needsCCD(pixelroot32::core::PhysicsActor* body) const`

**Description:**

Checks if a body requires continuous collision detection (CCD).

**Parameters:**

- `body`: The physics actor to check.

**Returns:** True if CCD is required.

### `bool sweptCircleVsAABB(pixelroot32::core::PhysicsActor* circle, pixelroot32::core::PhysicsActor* box, pixelroot32::math::Scalar& outTime, pixelroot32::math::Vector2& outNormal)`

**Description:**

Performs a swept collision test between a circle and an AABB.

**Parameters:**

- `circle`: The moving circle.
- `box`: The static AABB.
- `outTime`: Reference to store the time of impact [0, 1].
- `outNormal`: Reference to store the collision normal.

**Returns:** True if a collision occurs.

### `bool validateOneWayPlatform( pixelroot32::core::PhysicsActor* actor, pixelroot32::core::PhysicsActor* platform, const pixelroot32::math::Vector2& collisionNormal, pixelroot32::math::Scalar motionStartHitboxBottom = pixelroot32::math::toScalar(-1.0f)`

**Description:**

Validates if a collision with a one-way platform should occur.

**Parameters:**

- `actor`: The moving actor.
- `platform`: The one-way platform.
- `collisionNormal`: The contact normal.
- `motionStartHitboxBottom`: Hitbox bottom at the start of the current move probe
       (used by kinematic moveAndCollide). Negative to use previousPosition instead.

**Returns:** True if the collision is valid and should be resolved.
