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

### SegmentWall (Pattern)

Setting the collision shape to `SEGMENT` turns a (usually static) actor into a
line-segment wall at any angle — diagonal cushions, ramps, cut corners:
```cpp
auto cushion = scene.createEntity<pixelroot32::physics::StaticActor>();
cushion->setCollisionShape(CollisionShape::SEGMENT);
// Endpoints relative to the actor's position; one segment per actor.
cushion->setSegment(Vector2(0, 20), Vector2(20, 0));
```
Only circle-vs-segment pairs collide; the contact normal is perpendicular to
the segment (radial at the end points) and restitution applies as usual.

## Architecture Notes

### CollisionSystem (The Flat Solver)

The `CollisionSystem` is attached to a `Scene`. It manages the broadphase (Spatial Grid) and narrowphase collision detection.
- Uses **Discrete Collision Detection**.
- Solves penetration using projection (positional correction).
- Solves velocities using iterative impulses.
- Emits overlap and collision callbacks.

### PhysicsScheduler

Ensures the physics simulation runs at a fixed time step regardless of the rendering frame rate. This makes jumps and collision responses repeatable within the same build on the same target (same inputs applied on the same physics steps). It does not guarantee identical results across different hardware/targets: `Scalar` is `float` on FPU targets and `Fixed16` elsewhere, float results may differ between CPUs/compilers, and inputs read once per frame can land on different physics steps under different frame timing (see issue #244).
- Default timestep: `1/60.0f` seconds.
- Cap: `MAX_FRAME_ACCUMULATOR` prevents the "spiral of death" during lag spikes.

### Rest queries

- `PhysicsActor::isAtRest()` reports whether a body stopped (exactly zero velocity).
- `CollisionSystem::allBodiesAtRest()` reports whether every registered physics body stopped, so a game knows a turn is over without iterating its own entities.
- With `PHYSICS_REST_THRESHOLD` set, slow unforced rigid bodies snap to exactly zero velocity instead of creeping (`0` disables the snap).

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
| `PHYSICS_MAX_ENTITIES` | Max bodies in physics (default: 64). Past it, the body is never added. |
| `PHYSICS_MAX_PAIRS` | Max broadphase collision pairs (default: 128). |
| `PHYSICS_MAX_CONTACTS` | Max simultaneous narrowphase contacts (default: 128). Past it, the contact is not resolved. |
| `PHYSICS_MAX_CANDIDATES_PER_BODY` | Max broadphase candidates narrow-phase tested per body (default: 64). |
| `SPATIAL_GRID_MAX_STATIC_PER_CELL` | Max static bodies registered per grid cell (default: 12). Past it, the body is not registered in that cell. |
| `SPATIAL_GRID_MAX_DYNAMIC_PER_CELL` | Max moving bodies registered per grid cell (default: 12). Past it, the body is not registered in that cell. |
| `VELOCITY_ITERATIONS` | Number of passes in the impulse solver (default: 2). |

In debug builds (`PIXELROOT32_DEBUG_MODE`) the first hit of each limit logs a warning naming the limit and the flag that raises it; see the [capacity limits table](../architecture/physics-subsystem.md#911-capacity-limits-and-what-happens-at-each-one-issue-243).

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