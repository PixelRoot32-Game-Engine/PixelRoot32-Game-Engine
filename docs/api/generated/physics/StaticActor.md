# StaticActor

<Badge type="info" text="Class" />

**Source:** `StaticActor.h`

**Inherits from:** [PhysicsActor](../core/PhysicsActor.md)

## Description

A physics body that does not move.

Static actors are used for environment elements like floors, walls, and platforms
that should block other actors but are themselves immovable.
They are optimized to skip integration and world bound resolution.

## Inheritance

[PhysicsActor](../core/PhysicsActor.md) → `StaticActor`
