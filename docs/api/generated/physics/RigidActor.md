# RigidActor

<Badge type="info" text="Class" />

**Source:** `RigidActor.h`

**Inherits from:** [PhysicsActor](../core/PhysicsActor.md)

## Description

A physics body fully simulated by the engine.

Rigid actors respond to gravity, forces, and impulses. They are used for
dynamic objects that should behave naturally, like falling crates or debris.

## Inheritance

[PhysicsActor](../core/PhysicsActor.md) → `RigidActor`

## Methods

### `void applyForce(const pixelroot32::math::Vector2& f)`

**Description:**

Applies a force to the center of mass.

**Parameters:**

- `f`: Force vector.

### `void applyImpulse(const pixelroot32::math::Vector2& j)`

**Description:**

Applies an instantaneous impulse (velocity change).

**Parameters:**

- `j`: Impulse vector.

### `void integrate(pixelroot32::math::Scalar dt)`

**Description:**

Integrates forces and velocity to update position.

**Parameters:**

- `dt`: Delta time as Scalar.

### `void update(unsigned long deltaTime)`

**Description:**

Logic update called every frame.

**Parameters:**

- `deltaTime`: Elapsed time in ms.

### `void draw(pixelroot32::graphics::Renderer& renderer)`

**Description:**

Draws the actor.

**Parameters:**

- `renderer`: Reference to the renderer.
