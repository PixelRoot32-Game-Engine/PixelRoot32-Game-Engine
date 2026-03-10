# PixelRoot32 Physics System Reference – Flat Solver

> **Note:** For the complete physics system documentation with examples and API reference, visit the [official documentation](https://docs.pixelroot32.org/manual/game_development/physics_and_collisions/).

This document describes the **Flat Solver**, the current physics system in PixelRoot32. This version represents a major architectural overhaul from previous versions, focusing on stability, determinism, and microcontroller-friendly performance.

**Modular Compilation:** The entire physics system is only compiled when `PIXELROOT32_ENABLE_PHYSICS=1`. When disabled, all physics-related classes, collision detection, and solver components are excluded from the build, significantly reducing firmware size and RAM usage.

---

## 1. Overview: Flat Solver

### 1.1 Design Philosophy

- **Deterministic**: Fixed timestep (1/60s) ensures consistent behavior across hardware
- **Stable**: Proper separation of velocity and position solvers eliminates jitter
- **Microcontroller-Optimized**: No recursive sub-stepping, minimal memory overhead
- **Correct**: Implements proper impulse-based collision response

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
if (a->bounce && b->bounce) {
    contact.restitution = min(a->getRestitution(), b->getRestitution());
} else {
    contact.restitution = 0.0f; // No bounce if either body is absorbent
}
```

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

**ESP32 DRAM:** On boards with limited internal RAM, reducing `PHYSICS_MAX_CONTACTS` and `PHYSICS_MAX_PAIRS` (e.g. to 64) and/or `SPATIAL_GRID_MAX_STATIC_PER_CELL` and `SPATIAL_GRID_MAX_DYNAMIC_PER_CELL` (e.g. to 4) lowers `.dram0.bss` usage. See [Memory Management Guide](MEMORY_MANAGEMENT_GUIDE.md#esp32-dram-and-build-configuration).

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

---

**Document Version**: Flat Solver  
**Last Updated**: March 2026  
**Engine Version**: v1.1.0
