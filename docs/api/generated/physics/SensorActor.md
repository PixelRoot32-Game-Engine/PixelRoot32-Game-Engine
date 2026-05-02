# SensorActor

<Badge type="info" text="Class" />

**Source:** `SensorActor.h`

**Inherits from:** [StaticActor](./StaticActor.md)

## Description

A static body that acts as a trigger: detects overlap but produces no physical response.

Use for collectibles, checkpoints, damage zones, or any area that should fire
onCollision() without pushing or blocking the other body.

## Inheritance

[StaticActor](./StaticActor.md) → `SensorActor`
