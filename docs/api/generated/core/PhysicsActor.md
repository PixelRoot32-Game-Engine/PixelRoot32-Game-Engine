# PhysicsActor

<Badge type="info" text="Class" />

**Source:** `PhysicsActor.h`

**Inherits from:** [Actor](./Actor.md)

## Description

An actor with basic 2D physics properties using adaptable Scalar type.

Handles velocity, acceleration (via integration), and collision with world boundaries.
Automatically adapts to use float or Fixed16 based on the platform configuration.

## Inheritance

[Actor](./Actor.md) → `PhysicsActor`

## Methods

### `void setLimits(const LimitRect& limitRect)`

**Description:**

Sets custom movement limits for the actor.

**Parameters:**

- `limitRect`: The LimitRect structure defining the boundaries.

### `void setLimits(int left, int top, int right, int bottom)`

**Description:**

Sets custom movement limits for the actor.

**Parameters:**

- `left`: Left limit.
- `top`: Top limit.
- `right`: Right limit.
- `bottom`: Bottom limit.

### `void setWorldBounds(int w, int h)`

**Description:**

Defines the world size for boundary checking.

**Parameters:**

- `w`: Width of the world.
- `h`: Height of the world.

### `void setWorldSize(int w, int h)`

**Description:**

Legacy alias for setWorldBounds.

**Parameters:**

- `w`: Width of the world.
- `h`: Height of the world.

### `WorldCollisionInfo getWorldCollisionInfo() const`

**Description:**

Gets information about collisions with the world boundaries.

**Returns:** A WorldCollisionInfo struct containing collision flags.

### `void resetWorldCollisionInfo()`

**Description:**

Resets the world collision flags for the current frame.

### `PhysicsBodyType getBodyType() const`

**Description:**

Gets the simulation body type.

**Returns:** The PhysicsBodyType of this actor.

### `void setBodyType(PhysicsBodyType type)`

**Description:**

Sets the simulation body type.

**Parameters:**

- `type`: The new PhysicsBodyType.

### `void setMass(float m)`

**Description:**

Sets the mass of the actor.

**Parameters:**

- `m`: Mass value.

### `virtual void resolveWorldBounds()`

**Description:**

Resolves collisions with the defined world or custom bounds.

### `void setVelocity(T x, T y)`

**Description:**

Sets the linear velocity of the actor using floats.

**Parameters:**

- `x`: Horizontal velocity.
- `y`: Vertical velocity.

### `CollisionShape getShape() const`

**Description:**

Gets the collision shape type.

**Returns:** The CollisionShape of this actor.

### `void setShape(CollisionShape s)`

**Description:**

Sets the collision shape type.

**Parameters:**

- `s`: The new CollisionShape.

### `void setUserData(void* data)`

**Description:**

Set user data pointer for custom metadata.

**Parameters:**

- `data`: Opaque pointer. Engine does not manage lifetime.

### `void* getUserData() const`

**Description:**

Get user data pointer.

**Returns:** Pointer set via setUserData, or nullptr if never set.

### `void setSensor(bool s)`

**Description:**

Sets whether this body is a sensor (trigger).

**Parameters:**

- `s`: true = sensor (events only, no physics response); false = solid (default).

### `bool isSensor() const`

**Description:**

Returns true if this body is a sensor (trigger).

### `void setOneWay(bool w)`

**Description:**

Sets whether this body is a one-way platform (blocks only from one side).

**Parameters:**

- `w`: true = one-way (e.g. land from above, pass through from below); false = solid from all sides.

### `bool isOneWay() const`

**Description:**

Returns true if this body is a one-way platform.

### `void setBounce(bool b)`

**Description:**

Sets whether this body bounces on collision.

**Parameters:**

- `b`: true = bounce (velocity reflected on static contact); false = no bounce (velocity zeroed).

### `bool isBounce() const`

**Description:**

Returns true if this body bounces on collision.

### `void updatePreviousPosition()`

**Description:**

Updates the previous position to the current position.

### `virtual void onWorldCollision()`

**Description:**

Callback triggered when this actor collides with world boundaries.
