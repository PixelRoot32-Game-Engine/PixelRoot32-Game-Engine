# PixelRoot32 Physics System Reference – Flat Solver

> **Note:** For the complete physics system documentation with examples and API reference, visit the [official documentation](https://docs.pixelroot32.org/manual/game_development/physics_and_collisions/).

This document describes the **Flat Solver**, the current physics system in PixelRoot32. This version represents a major architectural overhaul from previous versions, focusing on stability, determinism, and microcontroller-friendly performance.

**Modular Compilation:** The entire physics system is only compiled when `PIXELROOT32_ENABLE_PHYSICS=1`. When disabled, all physics-related classes, collision detection, and solver components are excluded from the build, significantly reducing firmware size and RAM usage.

---

## 1. Overview: Flat Solver

### 1.1 Design Philosophy

- **Deterministic**: Fixed timestep (1/60s) ensures consistent behavior across hardware.
- **Stable**: Proper separation of velocity and position solvers eliminates jitter.
- **Hardware-Optimized**: Uses `Fixed16` on non-FPU microcontrollers (ESP32-C3/C6) for high-performance math without the overhead of floating-point emulation.
- **Precise Rounding**: Uses `MathUtil` rounding functions to ensure that small penetrations and velocities are handled consistently.
- **Correct**: Implements proper impulse-based collision response.

### 1.2 The Simulation Pipeline

Every frame, the `CollisionSystem` executes physics in strict order:

```
1. Detect Collisions       → Identify all overlapping pairs
2. Solve Velocity          → Apply impulse-based collision response
3. Integrate Positions     → Update positions: p = p + v * FIXED_DT
4. Solve Penetration       → Baumgarte stabilization + Slop
5. Trigger Callbacks       → Notify gameplay code (onCollision)
```

This order is critical:

- **Velocity is solved before position integration** (prevents energy loss)
- **Position integration happens before penetration correction** (allows proper separation)
- **Callbacks happen last** (gameplay can inspect final state)

---

## 2. Key Constants

```cpp
static constexpr Scalar FIXED_DT = toScalar(1.0f / 60.0f);  // Fixed timestep
static constexpr Scalar SLOP = toScalar(0.02f);              // Ignore penetration < 2cm
static constexpr Scalar BIAS = toScalar(0.2f);               // 20% correction per frame
static constexpr Scalar VELOCITY_THRESHOLD = toScalar(0.5f); // Zero restitution below this
static constexpr int VELOCITY_ITERATIONS = 2;                // Impulse solver iterations
static constexpr Scalar CCD_THRESHOLD = toScalar(3.0f);      // CCD activation threshold

// vPhysics Scheduler constants
static constexpr Scalar VELOCITY_DAMPING = toScalar(0.999f);  // Per-frame velocity damping
static constexpr Scalar MAX_VELOCITY = toScalar(500.0f);     // Maximum velocity cap (units/s)
```

---

## 2.1 PhysicsScheduler

The **PhysicsScheduler** implements a **fixed timestep with time accumulator** pattern to ensure consistent physics simulation regardless of frame rate variations (critical for ESP32).

### 2.1.1 Why Do We Need It?

On ESP32, frame rates vary widely (30-60 FPS) due to WiFi/BT interrupts and limited resources. Without a scheduler:

- 30 FPS → ~33ms/frame → 1 physics step per frame → **slower physics**
- 60 FPS → ~16ms/frame → 1 physics step per frame → **faster physics**

This caused inconsistent motion speeds between ESP32 and Native targets.

### 2.1.2 How It Works

```
Frame 1 (16ms): accumulator = 16000µs → 0 steps (accumulate)
Frame 2 (16ms): accumulator = 32000µs → 1 step, accumulator = 15333µs
Frame 3 (16ms): accumulator = 31333µs → 1 step, accumulator = 14666µs
Frame 4 (16ms): accumulator = 30666µs → 1 step, accumulator = 14000µs
```

### 2.1.3 Key Features

| Feature | Description |
|---------|-------------|
| **Time Accumulator** | Accumulates real microseconds, never discards time |
| **Adaptive Steps** | 2 steps normal, up to 4 when behind (catch-up mode) |
| **No Clamping** | Preserves real time for catch-up, avoids "slow motion" |
| **ESP32 Optimized** | Early skip for stationary bodies, IRAM_ATTR |

### 2.1.4 Integration

The PhysicsScheduler is owned by `Scene` and called in `Scene::update()`:

```cpp
// Scene.cpp
void Scene::update(unsigned long deltaTime) {
    // 1. Logic update - entities update game logic only
    for (int i = 0; i < entityCount; i++) {
        entities[i]->update(deltaTime);
    }
    
    // 2. Physics update with fixed timestep scheduler
    #if PIXELROOT32_ENABLE_PHYSICS
        uint32_t deltaMicros = static_cast<uint32_t>(deltaTime * 1000);
        physicsScheduler.update(deltaMicros, collisionSystem);
    #endif
}
```

### 2.1.5 Build Flags

```ini
# platformio.ini
-D PIXELROOT32_ENABLE_PHYSICS_FIXED_TIMESTEP=1  ; Enable scheduler (profile_full/arcade)
-D PIXELROOT32_VELOCITY_DAMPING=0.999           ; Per-frame damping (default)
-D PIXELROOT32_MAX_VELOCITY=500                  ; Max velocity (units/s, default)
-D PIXELROOT32_HAS_FAST_RSQRT=1                   ; Enable fast reciprocal sqrt
```

---

## 3. Collision Detection

### 3.1 Broadphase: Dual-Layer Spatial Grid

- **Static layer**: Contains only STATIC bodies. Rebuilt only when entities are added or removed (`markStaticDirty()`). Not cleared each frame.
- **Dynamic layer**: Contains RIGID and KINEMATIC bodies. Cleared and refilled every frame.
- **Query**: `getPotentialColliders()` merges results from both layers (per cell), with deduplication via `Actor::queryId`.
- **Config**: `SPATIAL_GRID_CELL_SIZE` (default 32px), `SPATIAL_GRID_MAX_STATIC_PER_CELL` (12), `SPATIAL_GRID_MAX_DYNAMIC_PER_CELL` (12). Reduces per-frame cost when many static tiles are present.

### 3.2 Narrowphase: Shape Interactions

| Interaction | Algorithm |
|-------------|-----------|
| AABB vs AABB | SAT (Separating Axis Theorem) |
| Circle vs Circle | Distance check with vertical fallback for perfect overlap |
| Circle vs AABB | Closest point clamping |

### 3.3 Contact Generation and Pool

- Contacts are stored in a **fixed-size array** (`PHYSICS_MAX_CONTACTS`, default 128). No heap allocation in the hot path.
- When the contact count would exceed the maximum, additional contacts are **dropped** (no crash). Tune `PHYSICS_MAX_CONTACTS` in `EngineConfig.h` if needed.
- Each contact carries `isSensorContact = true` when either body is a sensor; these are skipped in the velocity and penetration solvers.

### 3.4 Contact Restitution

When a contact is generated, the solver pre-calculates the restitution coefficient:

```cpp
// Combined restitution
if (a->isBounce() && b->isBounce()) {
    contact.restitution = min(a->getRestitution(), b->getRestitution());
} else {
    contact.restitution = 0.0f; // No bounce if either body is absorbent
}
```

---

> **Note (v1.2.0+)**: The `bounce` property now defaults to `true` (previously `false`). Use `setBounce(false)` to disable bounce behavior. The property uses packed flags internally for memory efficiency.

---

## 4. The Solver

### 4.1 Velocity Solver (Impulse-Based)

```cpp
// Sequential impulse solver (2 iterations)
for (int iter = 0; iter < 2; iter++) {
    for (auto& contact : contacts) {
        // Calculate relative velocity along normal
        Vector2 rv = bodyA->velocity - bodyB->velocity;
        Scalar vn = rv.dot(contact.normal);
        
        // Only resolve approaching velocities
        if (vn > 0) continue;
        
        // Apply velocity threshold for restitution
        Scalar e = (abs(vn) < VELOCITY_THRESHOLD) ? 0 : contact.restitution;
        
        // Calculate impulse
        Scalar j = -(1 + e) * vn / (invMassA + invMassB);
        
        // Apply to bodies
        bodyA->velocity += contact.normal * j * invMassA;
        bodyB->velocity -= contact.normal * j * invMassB;
    }
}
```

### 4.2 Position Integration

```cpp
// Only for Rigid bodies
body->position += body->velocity * FIXED_DT;
```

Note: Position integration is done **after** velocity solver but **before** penetration correction.

### 4.3 Penetration Solver (Baumgarte + Slop)

```cpp
// Skip small penetrations (slop)
if (contact.penetration <= SLOP) continue;

// Baumgarte stabilization: correct only a portion per frame
Scalar correction = (contact.penetration - SLOP) * BIAS;
Vector2 correctionVec = contact.normal * correction / totalInvMass;

// Apply position correction (doesn't affect velocity!)
bodyA->position += correctionVec * invMassA;
bodyB->position -= correctionVec * invMassB;
```

---

## 5. Sensors and One-Way Platforms

### 5.1 Sensors (Triggers)

- **`PhysicsActor::setSensor(true)`**: The body generates collision events and `onCollision()` is called, but **no impulse** and **no penetration correction** are applied.
- Use for: collectibles, checkpoints, damage zones, area triggers.
- **SensorActor** (include `physics/SensorActor.h`): A `StaticActor` subclass that calls `setSensor(true)` in the constructor.
- In `onCollision`, you can check `other->isSensor()` to distinguish triggers from solid bodies.

### 5.2 One-Way Platforms

- **`PhysicsActor::setOneWay(true)`**: The body blocks only when the other body is "landing from above". The validation uses **spatial crossing detection** to determine approach direction:
  - Checks if the collision normal points upward (actor above platform)
  - Verifies the actor crossed the platform surface from above using `previousPosition`
  - Confirms the actor is moving downward or stationary
  - **Rejects horizontal collisions** (prevents getting stuck on platform edges/corners)
- Contacts from below (e.g. jumping through the platform) are rejected.
- Use for: platforms the player can jump through from below and land on from above.
- Applies in both discrete narrowphase and in the CCD path (swept circle vs static AABB).
- **Implementation**: Uses `CollisionSystem::validateOneWayPlatform()` which compares `PhysicsActor::previousPosition` with current position to detect spatial crossing. The validation includes a tolerance check (`abs(normal.y) < 0.1`) to reject side collisions.

#### Algorithm: validateOneWayPlatform()

```cpp
bool validateOneWayPlatform(PhysicsActor* actor, PhysicsActor* platform, 
                            const Vector2& collisionNormal) {
    // 1. Must be marked as one-way
    if (!platform->isOneWay()) return true;
    
    // 2. Normal must be mostly vertical (reject side collisions)
    Scalar absNormalY = abs(collisionNormal.y);
    if (absNormalY < 0.1) return false;
    
    // 3. Normal must point UP (push actor upward)
    if (collisionNormal.y >= 0) return false;
    
    // 4. Check spatial crossing: was actor above platform?
    Scalar platformTop = platform->getHitBox().position.y;
    Scalar previousBottom = actor->getPreviousPosition().y + actor->height;
    Scalar currentBottom = actor->position.y + actor->height;
    
    bool crossedFromAbove = (previousBottom <= platformTop) && 
                           (currentBottom >= platformTop);
    
    // 5. Actor must be moving down or stationary
    bool movingDown = actor->getVelocity().y >= 0;
    
    return crossedFromAbove && movingDown;
}
```

**Test Coverage**: `test/unit/test_collision_system/test_collision_system.cpp` includes:

- `test_one_way_platform_crossing_from_above`
- `test_one_way_platform_crossing_from_below`
- `test_one_way_platform_wrong_normal_direction`
- `test_one_way_platform_moving_upward`
- `test_one_way_platform_large_delta_movement`
- `test_one_way_platform_velocity_sign_change`
- `test_one_way_platform_stationary_on_surface`
- `test_one_way_platform_not_one_way`

### 5.3 Sensors and Kinematic Bodies

Sensors interact with Kinematic bodies differently than solid bodies:

#### KinematicActor + Sensor

- **Sensors do not block kinematic movement**: `KinematicActor::moveAndCollide()` and `moveAndSlide()` skip `isSensor()` bodies with `continue`.
- **Overlap still triggers `onCollision()`**: When a kinematic body overlaps a sensor, the collision system generates a contact and fires the callback, but no position correction or velocity response is applied.
- This allows sensors to detect overlap (collectibles, triggers) without physically blocking the player.

```cpp
// In KinematicActor::moveAndCollide():
for (auto& physOther : potentialColliders) {
    if (physOther->isSensor()) {
        continue;  // Sensors do not block kinematic movement
    }
    // ... collision resolution for solid bodies
}
```

#### Pattern: Sensor in onCollision

```cpp
void PlayerActor::onCollision(Actor* other) override {
    if (other->isSensor()) {
        // Check userData for tile-based sensors
        if (other->getUserData()) {
            uintptr_t packed = reinterpret_cast<uintptr_t>(other->getUserData());
            uint16_t tx, ty;
            TileFlags flags;
            unpackTileData(packed, tx, ty, flags);
            
            if (flags & TILE_COLLECTIBLE) {
                // Collect item (sensor blocks nothing, so player passes through)
                collectItem(tx, ty);
            }
            if (flags & TILE_DAMAGE) {
                // Take damage (player continues moving)
                takeDamage();
            }
        }
    }
}
```

---

## 6. Tile Attributes (Physics)

For tile-based colliders, the engine provides **`physics/TileAttributes.h`** with two APIs:

### 6.1 Flags-based API (recommended)

- **`TileFlags`**: Bit flags (`TILE_NONE`, `TILE_SOLID`, `TILE_SENSOR`, `TILE_DAMAGE`, `TILE_COLLECTIBLE`, `TILE_ONEWAY`, `TILE_TRIGGER`). One byte per tile; no strings at runtime.
- **`packTileData(x, y, flags)`** / **`unpackTileData(packed, x, y, flags)`**: Encode tile coords (10+10 bits) and flags (8 bits) into a single value for `setUserData()`.
- **`TileBehaviorLayer`**: Struct holding `data` (dense `uint8_t` array), `width`, `height`. Exported by the Tilemap Editor; use with **`getTileFlags(layer, x, y)`** for O(1) lookup with bounds checking.
- **`isSensorTile(flags)`** / **`isOneWayTile(flags)`** / **`isSolidTile(flags)`**: Derive sensor/one-way/solid from flags when building `StaticActor` or `SensorActor`.

**Builder workflow:** For each tile with `flags != TILE_NONE`, create `StaticActor` or `SensorActor`, call `setSensor(isSensorTile(flags))`, `setOneWay(isOneWayTile(flags))`, and `setUserData(reinterpret_cast<void*>(packTileData(tx, ty, flags)))`, then `scene.addEntity(...)`.

### 6.2 Legacy behavior enum

- **`TileCollisionBehavior`**: `SOLID`, `SENSOR`, `ONE_WAY_UP`, `DAMAGE`, `DESTRUCTIBLE`.
- **`packTileData(x, y, behavior)`** / **`unpackTileData(..., behavior)`**: Same encoding with 4-bit behavior (deprecated for new code).

### 6.3 Consumible tiles (Phase 7)

When a tile is consumed (e.g. coin collected), remove its body and hide it visually:

1. **`scene.removeEntity(tileActor)`** so the CollisionSystem no longer considers it.
2. **`tilemap->setTileActive(tileX, tileY, false)`** so `drawTileMap` skips it (reuses `runtimeMask`; no separate consumed mask).

**`physics/TileConsumptionHelper.h`** wraps this: **`TileConsumptionHelper`** (constructor: scene, tilemap, config) provides **`consumeTile(tileActor, tileX, tileY)`** and **`consumeTileFromUserData(tileActor, packedUserData)`** (only consumes if `TILE_COLLECTIBLE`). Convenience **`consumeTileFromCollision(tileActor, packedUserData, scene, tilemap)`** for use inside `onCollision`. Destruction and damage logic remain in game code using `getUserData()` and flags.

### 6.4 TileCollisionBuilder (High-Level API)

**Include**: `physics/TileCollisionBuilder.h`

The `TileCollisionBuilder` class provides a high-level builder that generates physics bodies from a `TileBehaviorLayer` with a single function call. This is the recommended way to populate physics for tilemap-based levels.

#### Configuration

```cpp
struct TileCollisionBuilderConfig {
    uint8_t tileWidth;      // Tile width in world units (e.g., 16)
    uint8_t tileHeight;     // Tile height in world units (e.g., 16)
    uint16_t maxEntities;   // Safety limit (0xFFFF = unlimited)
    
    TileCollisionBuilderConfig(uint8_t w = 16, uint8_t h = 16, uint16_t max = 0xFFFF);
};
```

#### Workflow

```
Tilemap Editor Export
    ↓
TileBehaviorLayer { data: uint8_t[], width, height }
    ↓
TileCollisionBuilder::buildFromBehaviorLayer()
    ↓
For each tile (x, y) with flags != 0:
    ├── O(1) lookup: getTileFlags(layer, x, y)
    ├── Create body: isSensorTile(flags) ? SensorActor : StaticActor
    ├── Configure: setSensor(), setOneWay()
    ├── Pack data: setUserData(packTileData(x, y, flags))
    ├── Set layers: setCollisionLayer(), setCollisionMask()
    └── Add to scene: scene.addEntity()
```

#### Usage

```cpp
#include "physics/TileCollisionBuilder.h"

void GameScene::init() override {
    // Layer exported by Tilemap Editor
    TileBehaviorLayer layer = { behaviorData, 32, 32 };
    
    // One-liner
    int count = buildTileCollisions(*this, layer, 16, 16, 0);
    
    // Or explicit config
    TileCollisionBuilderConfig config(16, 16, 2048);
    TileCollisionBuilder builder(*this, config);
    int entities = builder.buildFromBehaviorLayer(layer, 0);
}
```

#### Memory Considerations

- Each created actor is a heap allocation (`new StaticActor` / `new SensorActor`).
- A 32×32 tilemap with every tile solid = 1024 bodies.
- Call `scene.clearEntities()` before rebuilding to avoid duplicates.
- On ESP32 with limited DRAM, consider using `maxEntities` as a safety limit.

---

## 7. Actor Types

### 7.1 RigidActor

- Fully simulated: gravity, forces, collisions
- Position integrated by CollisionSystem
- Supports both CIRCLE and AABB shapes
- Use for: Balls, props, debris

### 7.2 StaticActor

- Immovable, infinite mass
- Participates in collisions but never moves
- Use for: Walls, floors, platforms

### 7.3 KinematicActor

- Moved by game logic, not physics
- Participates in collisions (pushes Rigid actors)
- Use for: Player, moving platforms
- Properly detected in broadphase vs Rigid

---

## 8. Continuous Collision Detection (CCD)

### 8.1 When It Activates

CCD is used only when necessary:

```cpp
bool needsCCD(PhysicsActor* body) {
    // Only for circles
    if (body->getShape() != CIRCLE) return false;
    
    // Activate when: velocity * dt > radius * 3
    Scalar speed = body->getVelocity().length();
    Scalar movement = speed * FIXED_DT;
    Scalar threshold = body->getRadius() * CCD_THRESHOLD;
    
    return movement > threshold;
}
```

### 8.2 Swept Test Algorithm

```cpp
// Simple swept circle vs AABB
// Samples 2-8 positions along movement vector
// Returns collision time and normal
bool sweptCircleVsAABB(circle, box, outTime, outNormal);
```

Use case: Prevents tunneling when ball moves extremely fast (> 3x radius per frame).

---

## 9. Configuration

### 9.1 Physics Constants

Tune in `CollisionSystem.h` or override via `platforms/EngineConfig.h` / build flags (e.g. `-D PHYSICS_MAX_CONTACTS=64`):

```cpp
// Contact pool size (fixed array, no heap)
#define PHYSICS_MAX_CONTACTS 128

// Spatial grid: static = rebuilt when entities change; dynamic = per frame
#define SPATIAL_GRID_MAX_STATIC_PER_CELL  12
#define SPATIAL_GRID_MAX_DYNAMIC_PER_CELL 12
```

**ESP32 DRAM:** On boards with limited internal RAM, reducing `PHYSICS_MAX_CONTACTS` and `PHYSICS_MAX_PAIRS` (e.g. to 64) and/or `SPATIAL_GRID_MAX_STATIC_PER_CELL` and `SPATIAL_GRID_MAX_DYNAMIC_PER_CELL` (e.g. to 4) lowers `.dram0.bss` usage. See [Memory Management Guide](ARCH_MEMORY_SYSTEM.md#esp32-dram-and-build-configuration).

Solver tuning (in code):

```cpp
// For more stable stacking (slower)
static constexpr int VELOCITY_ITERATIONS = 4;  // Default: 2
static constexpr Scalar BIAS = toScalar(0.3f); // Default: 0.2

// For looser collision (faster)
static constexpr Scalar SLOP = toScalar(0.05f); // Default: 0.02
```

**Note:** These constants are only compiled when `PIXELROOT32_ENABLE_PHYSICS=1`.

**Note:** These constants are only compiled when `PIXELROOT32_ENABLE_PHYSICS=1`.

### 9.2 Per-Actor Properties

```cpp
// Restitution (bounciness): 0.0 to 1.0+
physicsActor->setRestitution(toScalar(1.0f));  // Perfect bounce

// Friction: 0.0 (none) to 1.0 (high)
physicsActor->setFriction(toScalar(0.0f));

// Gravity scale: 0.0 (no gravity) to 1.0+ (heavy)
physicsActor->setGravityScale(toScalar(0.0f)); // No gravity

// Shape
physicsActor->setShape(CollisionShape::CIRCLE);
physicsActor->setRadius(toScalar(6));
```

---

## 10. Performance Guide

### 10.1 ESP32-C3 (Non-FPU)

- **Target**: < 20 dynamic bodies @ 60 FPS
- **Use AABB** over Circle when possible (cheaper)
- **CCD has overhead**: Only triggers when needed
- **Slop helps**: Skip unnecessary corrections
- **Memory Impact**: Physics system disabled saves ~12KB RAM

**Modular Compilation:** On memory-constrained platforms, consider disabling physics entirely with `PIXELROOT32_ENABLE_PHYSICS=0` and using simple AABB checks instead.

### 10.2 ESP32 (With FPU)

- **Target**: < 50 dynamic bodies @ 60 FPS
- Circles are fine
- Can increase VELOCITY_ITERATIONS to 4 for better stability
- **Memory Impact**: Physics system disabled saves ~12KB RAM

---

## 11. Migration from v0.8.x / v0.9.0

### 11.1 Key Changes

| Old (v0.8.x) | New |
|--------------|------------|
| Position integrated in `Actor::update()` | Position integrated in `CollisionSystem::integratePositions()` |
| Relaxation-based solver | Impulse-based velocity solver + Baumgarte position solver |
| `PHYSICS_RELAXATION_ITERATIONS` | `VELOCITY_ITERATIONS` (default: 2) |
| No CCD | CCD for fast circles |
| Kinematic vs Rigid detection broken | Fixed and working |
| Variable timestep | Fixed timestep (1/60s) |

### 11.2 Code Changes Required

**Before:**

```cpp
void RigidActor::update(unsigned long dt) {
    // Integrated position here
    position += velocity * dt;
}
```

**After:**

```cpp
void RigidActor::update(unsigned long deltaTime) {
    // Only integrate velocity (forces)
    // Position handled by CollisionSystem
    integrate(CollisionSystem::FIXED_DT);
}
```

### 11.3 Behavior Differences

1. **More stable stacking**: Impulse solver handles multiple contacts better
2. **Perfect elastic collisions**: Restitution 1.0 actually works now
3. **No more sticking**: Proper separation of velocity/position phases
4. **Deterministic**: Same inputs always produce same outputs

---

## 12. Best Practices

1. **Always set shape**: `setShape(CollisionShape::CIRCLE)` or `AABB`
2. **Set radius for circles**: `setRadius(toScalar(r))` (critical for CCD)
3. **Use collision layers**: Don't rely on expensive broadphase checks
4. **Keep callbacks light**: `onCollision()` should only notify, not modify physics
5. **Test on target hardware**: Physics feels different on ESP32-C3 vs PC
6. **Consider modular compilation**: For simple games, disabling physics (`PIXELROOT32_ENABLE_PHYSICS=0`) and using basic AABB checks can save significant RAM and firmware size

---

## References

- [API Reference](API_REFERENCE.md) - Class documentation
- [Architecture](ARCHITECTURE.md) - System design
- [Migration Guide v1.2.0](MIGRATION_v1.2.0.md) - PhysicsActor flags packing changes

---

**Document Version**: Flat Solver  
**Last Updated**: April 2026  
**Engine Version**: v1.2.0
