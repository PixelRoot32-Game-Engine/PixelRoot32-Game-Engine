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
