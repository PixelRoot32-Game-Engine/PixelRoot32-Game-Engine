# Circle Physics Extension ("Flat Solver" Addon)

This document outlines the extension of the "Flat Solver" physics system to support circular bodies, as a complement to the existing AABB-based system.

## Design Goals
1. **Consistency**: Follow all principles in `physics_redesign_plan.md` (iterative relaxation, static arbiter, zero heap allocs).
2. **Specialization**: Add support for Circle-Circle and Circle-Box collision detection and resolution.
3. **ESP32 Optimization**: Use existing fixed-point math for distance and normalization.

## Architectural Changes

### 1. Shape Awareness
`PhysicsActor` will be updated to include a `CollisionShape` identifier:
- `Shape::AABB` (Default)
- `Shape::CIRCLE`

### 2. Manifold Resolution (The "Flat Solver" Dispatch)
The `CollisionSystem::resolve()` method will be updated to handle three case pairs:

#### Case A: AABB vs AABB (Existing)
- Uses the current minimum separation axis (X or Y overlap) logic.

#### Case B: Circle vs Circle
- **Detection**: Distance between centers $d < (r1 + r2)$.
- **Resolution**:
  - Normal $n = normalize(centerA - centerB)$.
  - Penetration $p = (r1 + r2) - d$.
  - Correct positions along $n$ by $p$ (shared 50/50 for Rigid-Rigid, 100% for Rigid-Static).
  - Zero/Bounce approaching velocity along $n$.

#### Case C: Circle vs Box (AABB)
- **Detection**: Check if distance from circle center to closest point on AABB is less than radius.
- **Resolution**:
  - Find closest point $P$ on AABB to circle center $C$.
  - Delta vector $v = C - P$.
  - Normal $n = normalize(v)$.
  - Penetration $p = r - length(v)$.
  - Correct positions along $n$ by $p$.

## Implementation Plan

### Phase 1: Core Definitions
- Modify `PhysicsActor.h` to add `CollisionShape` and `radius`.
- Add `CircleActor` class to `lib/physics`.

### Phase 2: Solver Extension
- Update `CollisionSystem.cpp` to implement circle manifold generation.
- Ensure velocity zeroing (anti-vibration) works correctly for circular normals.

### Phase 3: Demo & Comparison
- Update `PhysicsDemoScene.cpp` to spawn both Boxes and Circles.
- Compare stability and performance on ESP32.

---
*Note: This extension maintains the Physics Budget of ~2ms per frame by using optimized fixed-point square root / division where required.*
