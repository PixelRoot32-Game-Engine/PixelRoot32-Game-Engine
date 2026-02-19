# PixelRoot32 Physics System Reference – Flat Solver v3.0

This document describes the **Flat Solver v3.0**, the current physics system in PixelRoot32. This version represents a major architectural overhaul from previous versions, focusing on stability, determinism, and microcontroller-friendly performance.

---

## 1. Overview: Flat Solver v3.0

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
3. Integrate Positions     → Update positions: p = p + v * dt
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

### 3.1 Broadphase: Spatial Grid

- Uniform grid with configurable cell size (default: 32px)
- Shared static buffers on ESP32 (saves ~100KB DRAM)
- Detects:
  - Rigid vs Rigid
  - Rigid vs Static
  - Rigid vs Kinematic (new in v3.0)

### 3.2 Narrowphase: Shape Interactions

| Interaction | Algorithm |
|-------------|-----------|
| AABB vs AABB | SAT (Separating Axis Theorem) |
| Circle vs Circle | Distance check with vertical fallback for perfect overlap |
| Circle vs AABB | Closest point clamping |

### 3.3 Contact Generation

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
pa->position = pa->position + velocity * FIXED_DT;
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

## 5. Actor Types

### 5.1 RigidActor

- Fully simulated: gravity, forces, collisions
- Position integrated by CollisionSystem
- Supports both CIRCLE and AABB shapes
- Use for: Balls, props, debris

### 5.2 StaticActor

- Immovable, infinite mass
- Participates in collisions but never moves
- Use for: Walls, floors, platforms

### 5.3 KinematicActor

- Moved by game logic, not physics
- Participates in collisions (pushes Rigid actors)
- Use for: Player, moving platforms
- **New in v3.0**: Properly detected in broadphase vs Rigid

---

## 6. Continuous Collision Detection (CCD)

### 6.1 When It Activates

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

### 6.2 Swept Test Algorithm

```cpp
// Simple swept circle vs AABB
// Samples 2-8 positions along movement vector
// Returns collision time and normal
bool sweptCircleVsAABB(circle, box, outTime, outNormal);
```

Use case: Prevents tunneling when ball moves extremely fast (> 3x radius per frame).

---

## 7. Configuration

### 7.1 Physics Constants

Tune in `CollisionSystem.h`:

```cpp
// For more stable stacking (slower)
static constexpr int VELOCITY_ITERATIONS = 4;  // Default: 2
static constexpr Scalar BIAS = toScalar(0.3f); // Default: 0.2

// For looser collision (faster)
static constexpr Scalar SLOP = toScalar(0.05f); // Default: 0.02
```

### 7.2 Per-Actor Properties

```cpp
// Restitution (bounciness): 0.0 to 1.0+
actor->setRestitution(toScalar(1.0f));  // Perfect bounce

// Friction: 0.0 (none) to 1.0 (high)
actor->setFriction(toScalar(0.0f));

// Gravity scale: 0.0 (no gravity) to 1.0+ (heavy)
actor->setGravityScale(toScalar(0.0f)); // No gravity

// Shape
actor->setShape(CollisionShape::CIRCLE);
actor->setRadius(toScalar(6));
```

---

## 8. Performance Guide

### 8.1 ESP32-C3 (Non-FPU)

- **Target**: < 20 dynamic bodies @ 60 FPS
- **Use AABB** over Circle when possible (cheaper)
- **CCD has overhead**: Only triggers when needed
- **Slop helps**: Skip unnecessary corrections

### 8.2 ESP32 (With FPU)

- **Target**: < 50 dynamic bodies @ 60 FPS
- Circles are fine
- Can increase VELOCITY_ITERATIONS to 4 for better stability

---

## 9. Migration from v0.8.x / v0.9.0

### 9.1 Key Changes

| Old (v0.8.x) | New (v3.0) |
|--------------|------------|
| Position integrated in `Actor::update()` | Position integrated in `CollisionSystem::integratePositions()` |
| Relaxation-based solver | Impulse-based velocity solver + Baumgarte position solver |
| `PHYSICS_RELAXATION_ITERATIONS` | `VELOCITY_ITERATIONS` (default: 2) |
| No CCD | CCD for fast circles |
| Kinematic vs Rigid detection broken | Fixed and working |
| Variable timestep | Fixed timestep (1/60s) |

### 9.2 Code Changes Required

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

### 9.3 Behavior Differences

1. **More stable stacking**: Impulse solver handles multiple contacts better
2. **Perfect elastic collisions**: Restitution 1.0 actually works now
3. **No more sticking**: Proper separation of velocity/position phases
4. **Deterministic**: Same inputs always produce same outputs

---

## 10. Best Practices

1. **Always set shape**: `setShape(CollisionShape::CIRCLE)` or `AABB`
2. **Set radius for circles**: `setRadius(toScalar(r))` (critical for CCD)
3. **Use collision layers**: Don't rely on expensive broadphase checks
4. **Keep callbacks light**: `onCollision()` should only notify, not modify physics
5. **Test on target hardware**: Physics feels different on ESP32-C3 vs PC

---

## References

- [API Reference](API_REFERENCE.md) - Class documentation
- [Architecture](ARCHITECTURE.md) - System design
- [PHYSICS_IMPROVEMENT_PLAN.md](../../../../../PHYSICS_IMPROVEMENT_PLAN.md) - Development history

---

**Document Version**: Flat Solver v3.0  
**Last Updated**: February 2026  
**Engine Version**: v0.9.1-dev
