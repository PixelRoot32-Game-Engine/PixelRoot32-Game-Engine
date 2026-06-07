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

### `pixelroot32::math::Vector2 moveAndSlideWithSnap(pixelroot32::math::Vector2 velocity, pixelroot32::math::Vector2 snap, pixelroot32::math::Vector2 upDirection, pixelroot32::math::Scalar dt)`

**Description:**

Moves the body while sliding along surfaces, then snaps to floor.

**Parameters:**

- `velocity`: The velocity vector in units/sec (NOT pre-scaled by dt).
- `snap`: Snap vector toward floor. Pass zero to disable.
- `upDirection`: Up direction for floor detection.
- `dt`: Delta time. REQUIRED — same dt used by the game loop for
          input scaling consistency. No default.

**Returns:** The actual velocity after slide and snap processing.
        Assign to your velocity variable to replace post-slide zeroing.

Performs standard moveAndSlide along velocity, then pushes the AABB
along -upDirection by |snap| to attach to the floor. Returns the
actual velocity after collisions and snap are resolved.

::: tip
When jumping, caller MUST pass a zero snap vector explicitly.
      The engine does NOT auto-disable snap on upward velocity.
:::

::: tip
Snap magnitudes below MIN_SNAP (4.0) are treated as disabled.
:::

### `inline bool is_on_ceiling() const`

**Description:**

Returns true if the body collided with the ceiling.

### `inline bool is_on_floor() const`

**Description:**

Returns true if the body collided with the floor this frame.

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

### `void slide(pixelroot32::math::Vector2& currentMotion, pixelroot32::math::Vector2 upDirection, pixelroot32::core::PhysicsActor*& localFloorBody)`

**Description:**

Internal slide loop. Iterates moveAndCollide to slide along surfaces.

**Parameters:**

- `currentMotion`: In/out: the motion vector to process (modified by slides).
- `upDirection`: Up vector for floor/ceiling/wall classification.
- `localFloorBody`: Out: set if floor collision is on a KINEMATIC body.
