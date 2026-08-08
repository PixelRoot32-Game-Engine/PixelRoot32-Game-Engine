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

## Properties

| Name | Type | Description |
|------|------|-------------|
| `constexpr` | `static` | Minimum snap magnitude for SnapPolicy::Step. |

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

### `pixelroot32::math::Vector2 moveAndSlide( pixelroot32::math::Vector2 velocity, pixelroot32::math::Scalar dt, pixelroot32::math::Vector2 upDirection = {0, -1}, SnapPolicy snapPolicy = SnapPolicy::None, pixelroot32::math::Vector2 snapVector = {}, pixelroot32::core::PhysicsActor** outFloorBody = nullptr)`

**Description:**

Moves the body while sliding along surfaces, with optional floor snap.

**Parameters:**

- `velocity`: World velocity in units/sec (motion = velocity * dt).
- `dt`: Delta time in seconds. Required — no default.
- `upDirection`: Up vector for floor/ceiling/wall classification.
- `snapPolicy`: Snap behavior after slide (default: None).
- `snapVector`: Max snap distance toward -upDirection; zero disables snap.
- `outFloorBody`: Optional pointer to receive the floor PhysicsActor if on a KINEMATIC floor.

**Returns:** Resolved velocity after slide (+ snap when Step). Assign to caller velocity.

::: tip
SnapPolicy::Continuous is reserved; debug builds assert once and behave as None.
:::

::: tip
When jumping, pass snapVector=zero explicitly — snap is not auto-disabled on upward velocity.
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

### `void setStrictTopSurfaceFloor(bool strict)`

**Description:**

Enables or disables top-surface validation for floor state and kinematic carry.

**Parameters:**

- `strict`: When true (default), floor contact requires horizontal overlap on the top face.

### `inline bool isStrictTopSurfaceFloor() const`

**Description:**

Returns whether top-surface floor validation is active.

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
