# API Reference: Physics Module

This document covers the physics system, collision detection, actors, and tile collision helpers in PixelRoot32.

> **Note:** This is part of the [API Reference](../API_REFERENCE.md). See the main index for complete documentation.

---

## Physics Module Overview

The Physics module provides a high-performance "Flat Solver" optimized for microcontrollers. It handles collision detection, position resolution, and physical integration for different types of bodies.

---

## Actor

**Include:** `core/Actor.h`

**Inherits:** [Entity](API_CORE.md#entity)

The base class for all objects capable of collision. Actors extend Entity with collision layers, masks, and shape definitions. Note: You should typically use a specialized subclass like `RigidActor` or `KinematicActor` instead of this base class.

### Constants

- **`enum CollisionShape`**
  - `AABB`: Axis-aligned bounding box (Rectangle).
  - `CIRCLE`: Circular collision body.

### Properties

- **`uint16_t entityId`**: Unique id assigned by `CollisionSystem::addEntity` (used for pair deduplication). `0` = unregistered.
- **`int queryId`**: Used internally by the spatial grid for deduplication in `getPotentialColliders`.
- **`CollisionLayer layer`**: Bitmask representing the layers this actor belongs to.
- **`CollisionLayer mask`**: Bitmask representing the layers this actor scans for collisions.

### Public Methods

- **`Actor(Scalar x, Scalar y, int w, int h)`**
    Constructs a new Actor.

- **`void setCollisionLayer(CollisionLayer l)`**
    Sets the collision layer this actor belongs to.

- **`void setCollisionMask(CollisionLayer m)`**
    Sets the collision layers this actor interacts with.

- **`bool isInLayer(uint16_t targetLayer) const`**
    Checks if the Actor belongs to a specific collision layer.

- **`virtual Rect getHitBox()`**
    Returns the bounding rectangle for AABB detection or the bounding box of the circle.

- **`virtual void onCollision(Actor* other)`**
    Callback invoked when a collision is detected. **Note:** All collision responses (velocity/position changes) are handled by the `CollisionSystem`. This method is for gameplay notifications only.

---

## PhysicsActor

**Include:** `core/PhysicsActor.h`

**Inherits:** [Actor](#actor)

Base class for all physics-enabled bodies. It provides the core integration and response logic used by the `CollisionSystem`.

### Properties

- **`Vector2 velocity`**: Current movement speed in pixels/second.
- **`Vector2 previousPosition`**: Position from the previous physics frame (used for spatial crossing detection).
- **`Scalar mass`**: Mass of the body (Default: `1.0`).
- **`Scalar restitution`**: Bounciness factor (0.0 = no bounce, 1.0 = perfect bounce).
- **`Scalar friction`**: Friction coefficient (not yet fully implemented in solver).
- **`Scalar gravityScale`**: Multiplier for global gravity (Default: `1.0`).
- **`CollisionShape shape`**: The geometric shape used for detection (Default: `AABB`).
- **`Scalar radius`**: Radius used if `shape` is `CIRCLE` (Default: `0`).
- **`bool bounce`** (property accessor): If `true`, the actor will bounce off surfaces based on its restitution (Default: `true`). Supports both `actor->bounce = false` (property syntax) and explicit `actor->setBounce(false)` / `actor->isBounce()` methods. Internally stored in packed flags.
- **`bool sensor`** (property accessor): When true, the body generates collision events but does not produce physical response (no impulse, no penetration correction). Use for triggers, collectibles. Supports both `actor->sensor = true` (property syntax) and explicit `actor->setSensor(true)` / `actor->isSensor()` methods. Internally stored in packed flags.
- **`bool oneWay`** (property accessor): When true, the body only blocks from one side (e.g. one-way platform: land from above, pass through from below). Supports both `actor->oneWay = true` (property syntax) and explicit `actor->setOneWay(true)` / `actor->isOneWay()` methods. Internally stored in packed flags.
- **`void* userData`**: Optional pointer or packed value (e.g. tile coordinates) for game logic. Use `physics::packTileData` / `unpackTileData` from `physics/TileAttributes.h` for tile metadata.

### Constructors

- **`PhysicsActor(Scalar x, Scalar y, int w, int h)`**
    Constructs a new PhysicsActor.

- **`PhysicsActor(Vector2 position, int w, int h)`**
    Constructs a new PhysicsActor using a position vector.

### Public Methods

- **`void update(unsigned long deltaTime)`**
    Updates the actor state, applying physics integration and checking world boundary collisions.

- **`void setVelocity(const Vector2& v)`**
    Sets the linear velocity of the actor. Also supports `(Scalar x, Scalar y)` and `(float x, float y)`.

- **`const Vector2& getVelocity() const`**
    Gets the current velocity vector.

- **`void updatePreviousPosition()`**
    Updates the previous position to the current position. Should be called at the start of each physics frame to track position history for spatial crossing detection (e.g., one-way platforms). This is automatically called by `CollisionSystem::update()`.

- **`Vector2 getPreviousPosition() const`**
    Gets the position from the previous physics frame. Used internally for one-way platform validation.

- **`void setPosition(Vector2 pos)`**
    Sets the position and syncs previous position. When position is set directly (not via physics integration), the previous position is also updated to prevent false crossing detection.

- **`void setRestitution(Scalar r)`**
    Sets the restitution (bounciness). 1.0 means perfect bounce, < 1.0 means energy loss.

- **`void setFriction(Scalar f)`**
    Sets the friction coefficient (0.0 means no friction).

- **`void setSensor(bool s)`**
    Sets whether this body is a sensor (trigger). Sensors fire `onCollision` but do not receive impulse or penetration correction.

- **`bool isSensor() const`**
    Returns true if this body is a sensor.

- **`void setOneWay(bool w)`**
    Sets whether this body is a one-way platform (blocks only from one side, e.g. from above).

- **`bool isOneWay() const`**
    Returns true if this body is a one-way platform.

- **`void setBounce(bool b)`**
    Sets whether this body bounces off surfaces (uses restitution). When false, velocity is zeroed on contact instead of reflected.

- **`bool isBounce() const`**
    Returns true if this body has bounce enabled.

- **`void setUserData(void* ptr)`**
    Sets optional user data (e.g. tile coordinates or game-specific pointer).

- **`void* getUserData() const`**
    Gets the current user data.

- **`void setLimits(const LimitRect& limits)`**
    Sets custom movement limits for the actor.

- **`void setWorldBounds(int w, int h)`**
    Defines the world size for boundary checking, used as default limits.

- **`WorldCollisionInfo getWorldCollisionInfo() const`**
    Gets information about collisions with the world boundaries.

- **`void resetWorldCollisionInfo()`**
    Resets the world collision flags for the current frame.

- **`PhysicsBodyType getBodyType() const`**
    Gets the simulation body type (STATIC, KINEMATIC, or RIGID).

- **`void setBodyType(PhysicsBodyType type)`**
    Sets the simulation body type.

- **`void setMass(float m)`**
    Sets the mass of the actor.

- **`pixelroot32::math::Scalar getMass() const`**
    Gets the mass of the actor.

- **`void setGravityScale(pixelroot32::math::Scalar scale)`**
    Sets the gravity scale multiplier.

- **`pixelroot32::math::Scalar getGravityScale() const`**
    Gets the gravity scale multiplier.

- **`CollisionShape getShape() const`**
    Gets the collision shape type (AABB or CIRCLE).

- **`void setShape(CollisionShape s)`**
    Sets the collision shape type.

- **`pixelroot32::math::Scalar getRadius() const`**
    Gets the radius (only meaningful for CIRCLE shape).

- **`void setRadius(pixelroot32::math::Scalar r)`**
    Sets the radius and updates width/height to match diameter.

- **`void integrate(pixelroot32::math::Scalar dt)`**
    Integrates velocity to update position.

- **`void resolveWorldBounds()`**
    Resolves collisions with the defined world or custom bounds.

- **`virtual void onWorldCollision()`**
    Callback triggered when this actor collides with world boundaries. Override to implement custom behavior.

- **`void setWorldSize(int w, int h)`**
    Alias for `setWorldBounds()`.

- **`pixelroot32::math::Scalar getVelocityX() const`**
    Gets the horizontal velocity component.

- **`pixelroot32::math::Scalar getVelocityY() const`**
    Gets the vertical velocity component.

---

## LimitRect

**Inherits:** None

Bounding rectangle for world-collision resolution. Defines the limits of the play area.

### Properties

- **`left`**: Left boundary (-1 for no limit).
- **`top`**: Top boundary (-1 for no limit).
- **`right`**: Right boundary (-1 for no limit).
- **`bottom`**: Bottom boundary (-1 for no limit).

### Constructors

- **`LimitRect(int l, int t, int r, int b)`**
    Constructs a LimitRect with specific bounds.

---

## WorldCollisionInfo

**Inherits:** None

Information about world collisions in the current frame. Holds flags indicating which sides of the play area the actor collided with.

### Properties

- **`left`**: True if collided with the left boundary.
- **`right`**: True if collided with the right boundary.
- **`top`**: True if collided with the top boundary.
- **`bottom`**: True if collided with the bottom boundary.

---

## StaticActor

**Inherits:** [PhysicsActor](#physicsactor)

An immovable body that other objects can collide with. Ideal for floors, walls, and level geometry. Static bodies are placed in the **static layer** of the spatial grid (rebuilt only when entities are added or removed), reducing per-frame cost in levels with many tiles.

### Constructors

- **`StaticActor(Scalar x, Scalar y, int w, int h)`**
    Constructs a new StaticActor.

- **`StaticActor(Vector2 position, int w, int h)`**
    Constructs a new StaticActor using a position vector.

**Example:**

```cpp
auto floor = std::make_unique<StaticActor>(0, 230, 240, 10);
floor->setCollisionLayer(Layers::kWall);
scene->addEntity(floor.get());
```

---

## SensorActor

**Inherits:** [StaticActor](#staticactor)

A static body that acts as a **trigger**: it generates `onCollision` callbacks but does not produce any physical response (no impulse, no penetration correction). Use for collectibles, checkpoints, damage zones, or area triggers.

**Include:** `physics/SensorActor.h`

**Constructors:** Same as `StaticActor`; internally calls `setSensor(true)`.

```cpp
SensorActor coin(x, y, 16, 16);
coin.setCollisionLayer(Layers::kCollectible);
scene->addEntity(&coin);
// In player's onCollision: if (other->isSensor()) { collectCoin(other); }
```

---

## KinematicActor

**Inherits:** [PhysicsActor](#physicsactor)

A body that is moved manually via code but still interacts with the physics world (stops at walls, pushes objects). Ideal for players and moving platforms.

### Constructors

- **`KinematicActor(Scalar x, Scalar y, int w, int h)`**
    Constructs a new KinematicActor.

- **`KinematicActor(Vector2 position, int w, int h)`**
    Constructs a new KinematicActor using a position vector.

### Public Methods

- **`bool moveAndCollide(Vector2 relativeMove)`**
    Moves the actor by `relativeMove`. If a collision occurs, it stops at the point of contact and returns `true`.
- **`Vector2 moveAndSlide(Vector2 velocity)`**
    Moves the actor, sliding along surfaces if it hits a wall or floor. Returns the remaining velocity.

- **`bool is_on_ceiling() const`**
    Returns true if the body collided with the ceiling during the last `moveAndSlide` call.

- **`bool is_on_floor() const`**
    Returns true if the body collided with the floor during the last `moveAndSlide` call.

- **`bool is_on_wall() const`**
    Returns true if the body collided with a wall during the last `moveAndSlide` call.

**Example:**

```cpp
void Player::update(unsigned long dt) {
    Vector2 motion(0, 0);
    if (input.isButtonDown(0)) motion.x += 100 * dt / 1000.0f;
    
    // Automatic sliding against walls
    moveAndSlide(motion);
}
```

---

## RigidActor

**Inherits:** [PhysicsActor](#physicsactor)

A body fully simulated by the physics engine. It is affected by gravity, forces, and collisions with other bodies. Ideal for debris, boxes, and physical props.

### Constructors

- **`RigidActor(Scalar x, Scalar y, int w, int h)`**
    Constructs a new RigidActor.

- **`RigidActor(Vector2 position, int w, int h)`**
    Constructs a new RigidActor using a position vector.

### Properties

- **`bool bounce`** (property accessor): Whether the object should use restitution for bounces. Supports `actor->bounce = true` (property syntax) or explicit `actor->setBounce(true)` / `actor->isBounce()` methods. Internally stored in packed flags.

**Example:**

```cpp
auto box = std::make_unique<RigidActor>(100, 0, 16, 16);
box->setCollisionLayer(Layers::kProps);
box->bounce = true; // Make it bouncy
scene->addEntity(box.get());
```

---

## CircleActor (Pattern)

While the engine defines `RigidActor` and `StaticActor`, creating a circular object is done by setting the `shape` property.

**Structure:**

```cpp
class MyCircle : public RigidActor {
public:
    MyCircle(Scalar x, Scalar y, Scalar r) : RigidActor(x, y, r*2, r*2) {
        shape = CollisionShape::CIRCLE;
        radius = r;
    }
};
```

---

## Collision Primitives

**Inherits:** None

Lightweight geometric primitives and helpers used by the physics and collision systems.

### Types

- **`struct Circle`**
    Represents a circle in 2D space.

  - `Scalar x, y` – center position.
  - `Scalar radius` – circle radius.

- **`struct Segment`**
    Represents a line segment between two points.

  - `Scalar x1, y1` – start point.
  - `Scalar x2, y2` – end point.

### Helper Functions

- **`bool intersects(const Circle& a, const Circle& b)`**
    Returns true if two circles overlap.

- **`bool intersects(const Circle& c, const Rect& r)`**
    Returns true if a circle overlaps an axis-aligned rectangle.

- **`bool intersects(const Segment& s, const Rect& r)`**
    Returns true if a line segment intersects an axis-aligned rectangle.

- **`bool sweepCircleVsRect(const Circle& start, const Circle& end, const Rect& rect, Scalar& tHit)`**
    Performs a simple sweep test between two circle positions against a rectangle.  
    Returns true if a collision occurs between `start` and `end`, writing the normalized hit time in `tHit` (`0.0f` = at `start`, `1.0f` = at `end`).

---

## DefaultLayers

**Inherits:** None

Namespace with common collision layer constants:

- `kNone`: 0 (No collision)
- `kAll`: 0xFFFF (Collides with everything)

---

## CollisionSystem

**Inherits:** None

The central physics system implementing **Flat Solver**. Manages collision detection and resolution with fixed timestep for deterministic behavior. Uses a **dual-layer spatial grid** (static + dynamic) to minimize per-frame work when many static tiles are present, and a **fixed-size contact pool** (`PHYSICS_MAX_CONTACTS`, default 128; overridable via build flags) to avoid heap allocations in the hot path.

### Key Logic: "The Flat Solver"

The solver executes in strict order:

1. **Detect Collisions**: Rebuilds static grid if dirty, clears dynamic layer, inserts RIGID/KINEMATIC into dynamic layer; queries grid for potential pairs; narrowphase and contact generation. Contacts are stored in a fixed array; excess contacts are dropped when the pool is full.
2. **Solve Velocity**: Impulse-based collision response (2 iterations by default); sensor contacts are skipped.
3. **Integrate Positions**: Updates positions: `p = p + v * dt` (RIGID only).
4. **Solve Penetration**: Baumgarte stabilization with slop threshold; sensor contacts skipped.
5. **Trigger Callbacks**: Calls `onCollision()` for all contacts.

### Public Constants

- **`FIXED_DT`**: Fixed timestep (`1/60s`)
- **`SLOP`**: Minimum penetration to correct (`0.02f`)
- **`BIAS`**: Position correction factor (`0.2f`)
- **`VELOCITY_ITERATIONS`**: Impulse solver iterations (`2`)
- **`VELOCITY_THRESHOLD`**: Zero restitution below this speed (`0.5f`)
- **`CCD_THRESHOLD`**: CCD activation threshold (`3.0f`)

### Public Methods

- **`void update()`**  
  Executes the full physics pipeline. Called automatically by `Scene::update()`.

- **`void detectCollisions()`**  
  Broadphase and narrowphase detection. Populates contact list.

- **`void solveVelocity()`**  
  Impulse-based velocity solver. Applies collision response.

- **`void integratePositions()`**  
  Updates positions using velocity. Only affects `RigidActor`.

- **`void solvePenetration()`**  
  Position correction using Baumgarte stabilization.

- **`void triggerCallbacks()`**  
  Invokes `onCollision()` for all contacts.

- **`bool needsCCD(PhysicsActor* body)`**  
  Returns true if body needs Continuous Collision Detection (fast-moving circles).

- **`bool sweptCircleVsAABB(PhysicsActor* circle, PhysicsActor* box, Scalar& outTime, Vector2& outNormal)`**  
  Performs swept test for CCD. Returns collision time (0.0-1.0) and normal.

- **`bool validateOneWayPlatform(PhysicsActor* actor, PhysicsActor* platform, const Vector2& collisionNormal)`**
  Validates whether a one-way platform collision should be resolved based on spatial crossing detection. Returns `true` if the collision should be resolved (actor crossed from above), `false` otherwise. This method checks:
  - If the platform is a one-way platform
  - If the collision normal points upward (actor above platform)
  - If the actor crossed the platform surface from above (using previous position)
  - If the actor is moving downward or stationary

---

## PhysicsScheduler

**Include:** `physics/PhysicsScheduler.h`

**Namespace:** `pixelroot32::physics`

The PhysicsScheduler implements a **fixed timestep with time accumulator** pattern to ensure consistent physics simulation regardless of frame rate variations. This is critical for ESP32 where frame rates vary due to WiFi/BT interrupts.

### Public Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `FIXED_DT_MICROS` | 16667 µs | Fixed timestep (60 Hz) |
| `MAX_STEPS_NORMAL` | 1 | Max steps per frame under normal conditions (v1.2.2+: reduced from 2 for ESP32-C3 stability) |
| `MAX_STEPS_BACKLOG` | 4 | Max steps when behind (catch-up mode) |

### CollisionSystem Constants

| Constant | Default | Description |
|----------|---------|-------------|
| `VELOCITY_DAMPING` | 0.999f | Per-frame velocity damping factor |
| `MAX_VELOCITY` | 500.0f | Maximum velocity cap (units/s) |

These can be overridden at compile time:
```ini
-D PIXELROOT32_VELOCITY_DAMPING=0.995
-D PIXELROOT32_MAX_VELOCITY=300
```

### Performance Optimizations (v1.2.2+)

- **Skip Invisible Entities**: Collision detection skips entities where `isVisible() == false`. This optimization improves performance by avoiding unnecessary collision checks for hidden entities.
- **Centralized Velocity Integration**: Velocity integration is now centralized in `CollisionSystem::update()` rather than in `Actor::update()`. This ensures a single integration path where velocities are updated before positions.

### Public Methods

- **`void init()`**
  Initializes the scheduler. Resets accumulator and step counter.

- **`uint8_t update(uint32_t realDeltaMicros, CollisionSystem& collisionSystem)`**
  Updates physics with fixed timestep. Accumulates real time and executes as many steps as fit (max 2-4). Returns number of steps executed.

- **`uint8_t getStepsExecuted() const`**
  Returns number of physics steps executed in last update (for debugging/profiling).

- **`uint32_t getAccumulator() const`**
  Returns current accumulator value in microseconds (for debugging/profiling).

### Usage

The PhysicsScheduler is automatically used by `Scene`. You typically don't need to interact with it directly:

```cpp
// Scene automatically calls physicsScheduler.update() with fixed timestep
// Enable in platformio.ini:
build_flags =
    -D PIXELROOT32_ENABLE_PHYSICS_FIXED_TIMESTEP=1
```
  - Rejects horizontal collisions (side collisions with one-way platforms)

- **`size_t getEntityCount() const`**  
  Returns number of entities in the system.

- **`void clear()`**
  Removes all entities, resets the contact count, and clears the spatial grid (both static and dynamic layers).

---

## TileAttributes (Physics)

**Include:** `physics/TileAttributes.h`  
**Namespace:** `pixelroot32::physics`

Helpers for encoding tile metadata in `PhysicsActor::userData`, used by tilemap collision builders and game logic. Supports both a **flags-based** API (recommended for new code) and a legacy **behavior enum** API.

### TileFlags (recommended)

- **`enum TileFlags : uint8_t`**: Bit flags for tile behavior (1 byte per tile, no strings at runtime). Values: `TILE_NONE`, `TILE_SOLID`, `TILE_SENSOR`, `TILE_DAMAGE`, `TILE_COLLECTIBLE`, `TILE_ONEWAY`, `TILE_TRIGGER` (bits 6–7 reserved).
- **`packTileData(uint16_t x, uint16_t y, TileFlags flags)`**: Packs coords (10+10 bits) and flags (8 bits) into `uintptr_t` for `setUserData()`.
- **`unpackTileData(uintptr_t packed, uint16_t& x, uint16_t& y, TileFlags& flags)`**: Unpacks for use in `onCollision`.
- **`getTileFlags(const TileBehaviorLayer& layer, int x, int y)`**: O(1) lookup with bounds check; returns `TILE_NONE` (0) when out of bounds.
- **`isSensorTile(TileFlags flags)`** / **`isOneWayTile(TileFlags flags)`** / **`isSolidTile(TileFlags flags)`**: Derive physics config from flags for the collision builder.

### TileBehaviorLayer

- **`struct TileBehaviorLayer`**: `const uint8_t* data`, `uint16_t width`, `uint16_t height`. Points to a dense array (1 byte per tile) exported by the Tilemap Editor. Use with `getTileFlags()` for O(1) lookups.

### Legacy (deprecated for new code)

- **`enum class TileCollisionBehavior`**: `SOLID`, `SENSOR`, `ONE_WAY_UP`, `DAMAGE`, `DESTRUCTIBLE`.
- **`packTileData(x, y, TileCollisionBehavior)`** / **`unpackTileData(..., TileCollisionBehavior&)`**: Same encoding with 4-bit behavior.
- **`packCoord(x, y)`** / **`unpackCoord(packed, x, y)`**: Legacy 16+16 bit encoding for coords only.

---

## TileConsumptionHelper

**Include:** `physics/TileConsumptionHelper.h`  
**Namespace:** `pixelroot32::physics`

Helper for **consumible tiles** (e.g. coins, pickups): removes the tile's physics body from the scene and updates the tilemap's `runtimeMask` so the tile is no longer drawn. Reuses `TileMapGeneric::runtimeMask` (no separate consumed mask).

- **`struct TileConsumptionConfig`**: Optional config: `updateTilemap`, `logConsumption`, `validateCoordinates`.
- **`TileConsumptionHelper(Scene& scene, void* tilemap, const TileConsumptionConfig& config)`**: Constructor. `tilemap` is `TileMapGeneric*` (any of `Sprite`, `Sprite2bpp`, `Sprite4bpp`).
- **`bool consumeTile(Actor* tileActor, uint16_t tileX, uint16_t tileY)`**: Removes `tileActor` from the scene and sets `setTileActive(tileX, tileY, false)` on the tilemap. If `tileActor == nullptr`, only updates the tilemap mask.
- **`bool consumeTileFromUserData(Actor* tileActor, uintptr_t packedUserData)`**: Unpacks coords/flags from userData and consumes only if `TILE_COLLECTIBLE` is set.
- **`bool isTileConsumed(uint16_t tileX, uint16_t tileY) const`**: Returns whether the tile is inactive in the tilemap.
- **`bool restoreTile(uint16_t tileX, uint16_t tileY)`**: Sets the tile active again (visual only; does not re-add a physics body).

**Convenience functions:**

- **`consumeTileFromCollision(tileActor, packedUserData, scene, tilemap, config)`**: One-shot consumption from an `onCollision` callback.
- **`consumeTilesBatch(scene, tilemap, tiles[][2], count, config)`**: Updates `runtimeMask` for multiple tiles (no entity removal; use for clearing areas or reset).

---

## TileCollisionBuilder

**Include:** `physics/TileCollisionBuilder.h`  
**Namespace:** `pixelroot32::physics`

High-level builder that generates `StaticActor` or `SensorActor` bodies from a `TileBehaviorLayer`. Iterates all tiles with non-zero flags, creates the appropriate physics body, configures it (sensor, one-way), packs coords and flags into `userData`, and adds it to the scene. This is the recommended way to populate physics for tilemap-based levels.

### TileCollisionBuilderConfig

```cpp
struct TileCollisionBuilderConfig {
    uint8_t tileWidth;      // Width of each tile in world units (e.g., 16)
    uint8_t tileHeight;     // Height of each tile in world units (e.g., 16)
    uint16_t maxEntities;   // Maximum entities to create (safety limit)

    TileCollisionBuilderConfig(uint8_t w = 16, uint8_t h = 16, uint16_t max = 0xFFFF);
};
```

### Class Definition

```cpp
class TileCollisionBuilder {
public:
    TileCollisionBuilder(pixelroot32::core::Scene& scene, 
                         const TileCollisionBuilderConfig& config = TileCollisionBuilderConfig());

    int buildFromBehaviorLayer(const TileBehaviorLayer& layer, uint8_t layerIndex = 0);
    int getEntitiesCreated() const;
    void reset();
};
```

### Public Methods

- **`TileCollisionBuilder(Scene& scene, const TileCollisionBuilderConfig& config)`**  
  Constructs the builder bound to a scene.

- **`int buildFromBehaviorLayer(const TileBehaviorLayer& layer, uint8_t layerIndex = 0)`**  
  Iterates all tiles in the layer. For each tile with `flags != TILE_NONE`:
  - Creates `StaticActor` (solid, one-way) or `SensorActor` (sensor, damage, collectible)
  - Configures via `setSensor()` / `setOneWay()` from flags
  - Sets `setCollisionLayer(kDefaultItemCollisionLayer)` and `setCollisionMask(kDefaultItemCollisionMask)`
  - Calls `setUserData(reinterpret_cast<void*>(packTileData(x, y, flags)))`
  - Adds to scene via `scene.addEntity()`
  
  Returns the total number of entities created.

- **`int getEntitiesCreated() const`**  
  Returns the count from the last `buildFromBehaviorLayer()` call.

- **`void reset()`**  
  Resets `entitiesCreated` to 0. Does not clear the scene.

### Convenience Helper

```cpp
inline int buildTileCollisions(
    pixelroot32::core::Scene& scene,
    const TileBehaviorLayer& layer,
    uint8_t tileWidth = 16,
    uint8_t tileHeight = 16,
    uint8_t layerIndex = 0
);
```

One-liner that creates a builder, calls `buildFromBehaviorLayer()`, and returns the count.

### Usage Example

```cpp
#include "physics/TileCollisionBuilder.h"

void GameScene::init() override {
    // Behavior layer exported by Tilemap Editor (dense uint8_t[] array)
    TileBehaviorLayer layer = { behaviorData, 32, 32 };

    // Basic usage (one-liner)
    int count = buildTileCollisions(*this, layer, 16, 16, 0);

    // Or with explicit config
    TileCollisionBuilderConfig config(16, 16, 2048);  // 16x16 tiles, max 2048 bodies
    TileCollisionBuilder builder(*this, config);
    int entities = builder.buildFromBehaviorLayer(layer, 0);
}
```

### Integration with onCollision

After collision bodies are created, use `userData` in callbacks to identify the tile:

```cpp
void PlayerActor::onCollision(Actor* other) override {
    if (other->getUserData()) {
        uintptr_t packed = reinterpret_cast<uintptr_t>(other->getUserData());
        uint16_t tx, ty;
        TileFlags flags;
        unpackTileData(packed, tx, ty, flags);

        if (flags & TILE_COLLECTIBLE) {
            TileConsumptionHelper helper(*scene, tilemap);
            helper.consumeTileFromUserData(other, packed);
        }
        if (flags & TILE_DAMAGE) {
            takeDamage();
        }
    }
}
```

### Memory Considerations

- Each created actor is a heap allocation (`new StaticActor` / `new SensorActor`).
- Call `scene.clearEntities()` before rebuilding to avoid duplicates.
- On ESP32, keep `maxEntities` reasonable; 32×32 tiles with every tile solid = 1024 bodies.

---

## Related Documentation

- [API Reference](../API_REFERENCE.md) - Main index
- [API Core](API_CORE.md) - Engine, Entity, Scene
- [API Graphics](API_GRAPHICS.md) - Rendering and tilemaps