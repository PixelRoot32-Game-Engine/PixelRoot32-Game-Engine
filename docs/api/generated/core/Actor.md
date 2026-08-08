# Actor

<Badge type="info" text="Class" />

**Source:** `Actor.h`

**Inherits from:** [Entity](./Entity.md)

## Description

An Entity capable of physical interaction and collision.

Adds collision layers and masks for dynamic game objects like players,
enemies, and projectiles.

## Inheritance

[Entity](./Entity.md) → `Actor`

## Properties

| Name | Type | Description |
|------|------|-------------|
| `entityId` | `uint16_t` | Unique id per CollisionSystem registration; used for pair deduplication. |
| `queryId` | `int` | Used for optimized grid queries. |
| `layer` | `pixelroot32::physics::CollisionLayer` | The collision layer this actor belongs to. |
| `mask` | `pixelroot32::physics::CollisionLayer` | The collision layers this actor interacts with. |
| `collisionSystem` | `pixelroot32::physics::CollisionSystem*` | Reference to the collision system. |

## Methods

### `void setCollisionLayer(pixelroot32::physics::CollisionLayer l)`

**Description:**

Sets the collision layer this actor belongs to.

**Parameters:**

- `l`: The collision layer.

### `void setCollisionMask(pixelroot32::physics::CollisionLayer m)`

**Description:**

Sets the collision mask defining which layers this actor interacts with.

**Parameters:**

- `m`: The collision mask.

### `void update(unsigned long deltaTime)`

**Description:**

Updates the actor's state.

**Parameters:**

- `deltaTime`: Time elapsed since last update in milliseconds.

### `bool isInLayer(uint16_t targetLayer) const`

**Description:**

Checks if the actor belongs to a specific collision layer.

**Parameters:**

- `targetLayer`: The layer bitmask to check.

**Returns:** true if the actor is in the specified layer.

### `virtual Rect getHitBox()`

**Description:**

Gets the axis-aligned bounding box (hitbox) for this actor.

**Returns:** The Rect representing the hitbox.

### `virtual bool isPhysicsBody() const`

**Description:**

Checks if this actor is a physics-driven body (e.g., RigidActor).

**Returns:** true if it is a physics body, false otherwise.

### `virtual void onCollision(Actor* other)`

**Description:**

Callback invoked when a collision occurs. Notification only — no physics changes.

**Parameters:**

- `other`: The actor that this actor collided with.
