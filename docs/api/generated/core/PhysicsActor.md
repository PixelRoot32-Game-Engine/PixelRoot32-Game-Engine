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

### `bool isPhysicsBody() const`

**Description:**

Checks if this actor is a physics-enabled body.

**Returns:** true.

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

### `pixelroot32::math::Scalar getMass() const`

**Description:**

Gets the mass of the actor.

**Returns:** Mass as Scalar.

### `void setGravityScale(pixelroot32::math::Scalar scale)`

**Description:**

Sets the gravity scale.

**Parameters:**

- `scale`: Multiplier for the world gravity.

### `pixelroot32::math::Scalar getGravityScale() const`

**Description:**

Gets the gravity scale.

**Returns:** Gravity scale as Scalar.

### `virtual void integrate(pixelroot32::math::Scalar dt)`

**Description:**

Integrates velocity to update position.

**Parameters:**

- `dt`: Delta time in seconds (as Scalar).

### `virtual void resolveWorldBounds()`

**Description:**

Resolves collisions with the defined world or custom bounds.

### `void setVelocity(T x, T y)`

**Description:**

Sets the linear velocity of the actor using floats.

**Parameters:**

- `x`: Horizontal velocity.
- `y`: Vertical velocity.

### `void setVelocity(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y)`

**Description:**

Sets the linear velocity of the actor using Scalars.

**Parameters:**

- `x`: Horizontal velocity.
- `y`: Vertical velocity.

### `void setVelocity(const pixelroot32::math::Vector2& v)`

**Description:**

Sets the linear velocity of the actor using a Vector2.

**Parameters:**

- `v`: Velocity vector.

### `pixelroot32::math::Scalar getVelocityX() const`

**Description:**

Gets the horizontal velocity.

**Returns:** Horizontal velocity as Scalar.

### `pixelroot32::math::Scalar getVelocityY() const`

**Description:**

Gets the vertical velocity.

**Returns:** Vertical velocity as Scalar.

### `const pixelroot32::math::Vector2& getVelocity() const`

**Description:**

Gets the velocity vector.

**Returns:** Reference to the velocity Vector2.

### `void setRestitution(pixelroot32::math::Scalar r)`

**Description:**

Sets the restitution (bounciness) of the actor.

**Parameters:**

- `r`: Restitution value (0.0 to 1.0+). 1.0 means no energy is lost on bounce.

### `pixelroot32::math::Scalar getRestitution() const`

**Description:**

Gets the restitution (bounciness) of the actor.

**Returns:** Restitution as Scalar.

### `void setFriction(pixelroot32::math::Scalar f)`

**Description:**

Sets the friction coefficient.

**Parameters:**

- `f`: Friction value (0.0 means no friction).

### `CollisionShape getShape() const`

**Description:**

Gets the collision shape type.

**Returns:** The CollisionShape of this actor.

### `void setShape(CollisionShape s)`

**Description:**

Sets the collision shape type.

**Parameters:**

- `s`: The new CollisionShape.

### `pixelroot32::math::Scalar getRadius() const`

**Description:**

Gets the radius (only for Shape::CIRCLE).

**Returns:** Radius as Scalar.

### `void setRadius(pixelroot32::math::Scalar r)`

**Description:**

Sets the radius and updates width/height to match diameter.

**Parameters:**

- `r`: Radius value.

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

### `pixelroot32::math::Vector2 getPreviousPosition() const`

**Description:**

Gets the previous frame position.

**Returns:** The position from the previous physics frame.

### `void setPosition(pixelroot32::math::Vector2 pos)`

**Description:**

Sets the position and syncs previous position.

**Parameters:**

- `pos`: The new position.

### `virtual void onWorldCollision()`

**Description:**

Callback triggered when this actor collides with world boundaries.
