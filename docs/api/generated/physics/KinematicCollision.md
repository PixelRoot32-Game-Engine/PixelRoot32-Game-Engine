# KinematicCollision

<Badge type="info" text="Struct" />

**Source:** `CollisionSystem.h`

## Description

Contains information about a collision involving a KinematicActor.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `collider` | `pixelroot32::core::Actor*` | The other actor involved in the collision. |
| `normal` | `pixelroot32::math::Vector2` | The collision normal vector. |
| `position` | `pixelroot32::math::Vector2` | The position of the collision. |
| `travel` | `pixelroot32::math::Scalar` | Distance traveled before collision. |
| `remainder` | `pixelroot32::math::Scalar` | Remaining distance to travel. |
