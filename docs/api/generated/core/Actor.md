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

### `bool isInLayer(uint16_t targetLayer) const`

### `virtual Rect getHitBox()`

### `virtual bool isPhysicsBody() const`

### `virtual void onCollision(Actor* other)`

**Description:**

Callback invoked when a collision occurs. Notification only — no physics changes.

**Parameters:**

- `other`: The actor that this actor collided with.
