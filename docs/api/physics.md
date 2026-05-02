# API Reference: Physics Module

> **Source of truth:**
> - `include/core/Actor.h`, `include/core/PhysicsActor.h`
> - `include/physics/CollisionSystem.h`
> - `include/physics/PhysicsScheduler.h`
> - `include/physics/KinematicActor.h`, `include/physics/RigidActor.h`
> - `include/physics/StaticActor.h`, `include/physics/SensorActor.h`
> - `include/physics/TileAttributes.h`
> - `include/physics/TileCollisionBuilder.h`, `include/physics/TileConsumptionHelper.h`

## Overview

*(Requires `PIXELROOT32_ENABLE_PHYSICS=1`)*

The Physics module provides a custom, highly optimized 2D physics engine. It uses a **Flat Solver** architecture optimized for embedded devices without hardware floating-point units, resolving collisions through discrete SAT (Separating Axis Theorem) and iterative impulse resolution.

## Key Concepts

### Actor

`Actor` is the base class for entities that occupy space in the world. An `Actor` is technically an `Entity`, but specifically manages spatial bounds (`width`, `height`, `x`, `y`).

**Collision Shapes:**
- `RECTANGLE` (Default): AABB (Axis-Aligned Bounding Box) collision.
- `CIRCLE`: Radial collision, great for players or projectiles.

### PhysicsActor

The core class of the physics system. It extends `Actor` with physical properties and behaviors.

**Key Physical Properties:**
- `velocity`: The current speed and direction (units per second).
- `mass` / `invMass`: Determines how the actor responds to impulses (0 mass = infinite mass/static).
- `restitution`: Bounciness (0.0 to 1.0).
- `friction`: Resistance to sliding.
- `drag`: Air resistance or fluid drag.
- `collisionLayer` & `collisionMask`: Bitmasks used to filter which actors collide.

**Collision Resolution:**
- Handled internally by the `CollisionSystem`.
- Users interact via `moveAndCollide()` (for kinematic bodies) or `applyImpulse()` (for rigid bodies).

### Default Layers

| Macro | Value | Description |
|-------|-------|-------------|
| `LAYER_DEFAULT` | `0x0001` | Default layer for actors. |
| `LAYER_PLAYER` | `0x0002` | Typically used for the player character. |
| `LAYER_ENEMY` | `0x0004` | Typically used for enemies. |
| `LAYER_ENVIRONMENT` | `0x0008` | Used for solid world boundaries/tiles. |

## Actor Types

### StaticActor

A `PhysicsActor` with infinite mass that does not move. Used for walls, floors, and platforms.

```cpp
auto wall = scene.createEntity<pixelroot32::physics::StaticActor>();
wall->setSize(100, 20);
wall->setPosition(50, 200);
wall->setCollisionLayer(LAYER_ENVIRONMENT);
```

### SensorActor

An actor that detects overlaps but does not physically collide or stop other actors.

```cpp
auto trigger = scene.createEntity<pixelroot32::physics::SensorActor>();
trigger->setSize(30, 30);
trigger->setOnOverlap([](PhysicsActor* self, PhysicsActor* other, const WorldCollisionInfo& info) {
    if (other->getCollisionLayer() == LAYER_PLAYER) {
        // Player entered the zone
    }
});
```

### KinematicActor

An actor whose movement is fully controlled by the game logic (not forces/impulses), but it stops when hitting solid objects (like a `StaticActor`).

```cpp
auto platform = scene.createEntity<pixelroot32::physics::KinematicActor>();
platform->setSize(40, 10);
// In update loop:
platform->moveAndCollide(Vector2(50.0f * dt, 0)); 
```

### RigidActor

An actor entirely driven by the physics simulation (gravity, impulses, velocity). Best for physics objects like crates or a bouncing ball.

```cpp
auto crate = scene.createEntity<pixelroot32::physics::RigidActor>();
crate->setSize(16, 16);
crate->setMass(10.0f);
crate->setRestitution(0.4f);
// In game logic:
crate->applyImpulse(Vector2(0.0f, -200.0f)); // Jump/bounce
```

### CircleActor (Pattern)

While not a specific class, setting the collision shape to `CIRCLE` transforms the actor:
```cpp
auto ball = scene.createEntity<pixelroot32::physics::RigidActor>();
ball->setCollisionShape(CollisionShape::CIRCLE);
ball->setSize(16, 16); // Sets radius to 8
```

## Architecture Notes

### CollisionSystem (The Flat Solver)

The `CollisionSystem` is attached to a `Scene`. It manages the broadphase (Spatial Grid) and narrowphase collision detection.
- Uses **Discrete Collision Detection**.
- Solves penetration using projection (positional correction).
- Solves velocities using iterative impulses.
- Emits overlap and collision callbacks.

### PhysicsScheduler

Ensures the physics simulation runs at a fixed time step regardless of the rendering frame rate. This guarantees deterministic jumps and collision responses.
- Default timestep: `1/60.0f` seconds.
- Cap: `MAX_FRAME_ACCUMULATOR` prevents the "spiral of death" during lag spikes.

## Configuration & Data Structures

### WorldCollisionInfo

Struct passed to collision callbacks.
- `normal`: The collision normal (pointing away from the other object).
- `penetration`: Depth of the overlap.
- `contactPoint`: The estimated point of impact.

### LimitRect

A structural boundary used to restrict actor movement (e.g., keeping the player inside the camera view or level bounds).

### CollisionSystem Constants

| Constant | Description |
|----------|-------------|
| `PHYSICS_MAX_PAIRS` | Max broadphase collision pairs (default: 128). |
| `PHYSICS_MAX_CONTACTS` | Max simultaneous narrowphase contacts (default: 128). |
| `VELOCITY_ITERATIONS` | Number of passes in the impulse solver (default: 2). |

## Tile Collision Utilities

### TileAttributes

Custom metadata attached to tiles. Managed via `TileConsumptionHelper`.

> [!IMPORTANT]
> Since attributes are stored in Flash memory on ESP32, use **`PIXELROOT32_STRCMP_P`** or **`PIXELROOT32_MEMCPY_P`** to compare or copy strings from attributes.

### TileConsumptionHelper

A utility to consume tilemap arrays and extract custom properties (like solid/water/damage flags).

### TileCollisionBuilder

A utility that converts grid-based tilemaps into optimized `StaticActor` collision blocks. It merges adjacent solid tiles into larger rectangles to drastically reduce the broadphase entity count.

**Example Usage:**

```cpp
using pixelroot32::physics::TileCollisionBuilder;
using pixelroot32::physics::CollisionBox;

std::vector<CollisionBox> boxes;
TileCollisionBuilder::buildOptimizedBoxes(
    mapWidth, mapHeight, tileSize,
    [&](int x, int y) {
        return isTileSolid(x, y); // Your logic
    },
    boxes
);

// Spawn a StaticActor for each resulting box
for (const auto& box : boxes) {
    auto actor = scene.createEntity<StaticActor>();
    actor->setPosition(box.x, box.y);
    actor->setSize(box.width, box.height);
}
```

## Related Documentation

- [API Reference](index.md) - Main index
- [Core Module](core.md) - Scene and Entity
- [Math Module](math.md) - Vector2 and Scalar