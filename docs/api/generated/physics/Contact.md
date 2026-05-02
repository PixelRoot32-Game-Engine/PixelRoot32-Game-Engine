# Contact

<Badge type="info" text="Struct" />

**Source:** `CollisionSystem.h`

## Description

Represents a contact point between two physics bodies.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `bodyA` | `pixelroot32::core::PhysicsActor*` | First body in the contact. |
| `bodyB` | `pixelroot32::core::PhysicsActor*` | Second body in the contact. |
| `normal` | `pixelroot32::math::Vector2` | Contact normal vector. |
| `contactPoint` | `pixelroot32::math::Vector2` | Point of contact. |
| `penetration` | `pixelroot32::math::Scalar` | Penetration depth. |
| `restitution` | `pixelroot32::math::Scalar` | Combined restitution coefficient. |
| `isSensorContact` | `bool` | True if either body is a sensor; no physics response applied. |
