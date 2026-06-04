# KinematicActor

<Badge type="info" text="Class" />

**Source:** `KinematicActor.h`

**Inherits from:** [PhysicsActor](../core/PhysicsActor.md)

## Description

A physics body moved via script/manual velocity with collision detection.

Kinematic actors are not affected by world gravity or forces but can detect
and react to collisions during movement. They provide methods like 
moveAndSlide for complex character movement.

## Inheritance

[PhysicsActor](../core/PhysicsActor.md) → `KinematicActor`

## Methods

### `inline bool is_on_ceiling() const`

**Description:**

Returns true if the body collided with the ceiling.

### `inline bool is_on_floor() const`

**Description:**

Returns true if the body collided with the floor.

### `const pixelroot32::math::Vector2& getFloorVelocity() const`

**Description:**

Gets the persisted floor velocity from the last KINEMATIC floor contact.

**Returns:** Reference to the floor velocity vector.

### `void clearFloorVelocity()`

**Description:**

Clears floor velocity and state.

### `inline bool is_on_wall() const`

**Description:**

Returns true if the body collided with a wall.

### `void updateFloorState(bool onFloorThisFrame, pixelroot32::core::PhysicsActor* floorBodyResult)`

**Description:**

Updates floor persistence state.

**Parameters:**

- `onFloorThisFrame`: Whether floor contact was detected this frame.
- `floorBodyResult`: The floor body if on floor, nullptr otherwise.
