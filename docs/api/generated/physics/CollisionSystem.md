# CollisionSystem

<Badge type="info" text="Class" />

**Source:** `CollisionSystem.h`

## Description

Manages physics simulation and collision detection for all actors.

## Methods

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

### `size_t getEntityCount() const`

**Description:**

Gets the total number of registered entities.

**Returns:** Number of entities.

### `void clear()`

**Description:**

Clears the collision system state.
