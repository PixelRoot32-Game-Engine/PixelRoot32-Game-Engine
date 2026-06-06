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

### `bool moveAndCollide(pixelroot32::math::Vector2 motion, KinematicCollision* outCollision = nullptr, bool testOnly = false, pixelroot32::math::Scalar safeMargin = pixelroot32::math::Scalar(0.08f), bool recoveryAsCollision = false)`

**Description:**

Moves the body along a vector and stops at the first collision.

**Parameters:**

- `motion`: The relative movement vector.
- `outCollision`: Pointer to store collision data if a hit occurs.
- `testOnly`: If true, checks for collision without moving.
- `safeMargin`: Extra margin for collision recovery.
- `recoveryAsCollision`: If true, depenetration is reported as collision.

**Returns:** true if a collision occurred.

### `void moveAndSlide(pixelroot32::math::Vector2 velocity, pixelroot32::math::Vector2 upDirection = {0, -1}, pixelroot32::core::PhysicsActor** outFloorBody = nullptr, pixelroot32::math::Scalar dt = pixelroot32::physics::CollisionSystem::FIXED_DT)`

**Description:**

Moves the body while sliding along surfaces.

**Parameters:**

- `velocity`: The velocity vector.
- `upDirection`: The up vector used to differentiate floor/ceiling (optional).
- `outFloorBody`: Optional pointer to receive the floor PhysicsActor if on a KINEMATIC floor.
- `dt`: Delta time used to calculate the movement (default: FIXED_DT).

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

### `void draw(pixelroot32::graphics::Renderer& renderer)`

**Description:**

Draws the actor.

**Parameters:**

- `renderer`: Reference to the renderer.

### `void updateFloorState(bool onFloorThisFrame, pixelroot32::core::PhysicsActor* floorBodyResult)`

**Description:**

Updates floor persistence state.

**Parameters:**

- `onFloorThisFrame`: Whether floor contact was detected this frame.
- `floorBodyResult`: The floor body if on floor, nullptr otherwise.
