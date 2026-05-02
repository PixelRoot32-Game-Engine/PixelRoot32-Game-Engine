# Scene

<Badge type="info" text="Class" />

**Source:** `Scene.h`

## Description

Represents a game level or screen containing entities.

## Methods

### `virtual void init()`

**Description:**

Initializes the scene. Called when entering the scene.

### `virtual void initUI()`

**Description:**

Initialize the UI system for this scene.
Called during scene init. Add UI elements here.

### `virtual void updateUI(unsigned long deltaTime)`

**Description:**

Update the UI system.

**Parameters:**

- `deltaTime`: Time elapsed in ms.

### `* Execution order(deterministic)`

### `virtual void update(unsigned long deltaTime)`

**Description:**

Updates all entities in the scene and handles collisions.

**Parameters:**

- `deltaTime`: Time elapsed in ms.

### `virtual bool shouldRedrawFramebuffer() const`

**Description:**

When false, Engine may skip `draw()` and `present()` for this iteration (after `update()`).

### `void addEntity(Entity* entity)`

**Description:**

Adds an entity to the scene.

**Parameters:**

- `entity`: Pointer to the Entity to add.

### `void removeEntity(Entity* entity)`

**Description:**

Removes an entity from the scene.

**Parameters:**

- `entity`: Pointer to the Entity to remove.

### `void clearEntities()`

**Description:**

Removes all entities from the scene.
