# Physics System: Root Cause Analysis & ESP32-Optimized Redesign

## Design Decisions (Approved)

1. **Walls-only**: Remove `resolveWorldBounds()` entirely. World boundaries are handled exclusively by `StaticActor` walls.
2. **Optional bounce**: Bouncing is controlled by a flag (`bounce = true/false`) on `RigidActor`. Default: `false` (land and stop).
3. **Notification-only `onCollision()`**: The callback remains for game logic but does **not** modify velocity or position. All physics responses happen inside the solver.

---

## 1. Root Cause Diagnosis

After auditing every file in the physics pipeline, **5 systemic flaws** were identified that explain persistent tunneling.

---

### Flaw 1: `resolveWorldBounds()` fights the Collision Solver

> **CRITICAL** — This is the **primary cause** of floor tunneling.

Every `RigidActor::update()` calls `integrate()` then `resolveWorldBounds()`. This clamping+bouncing runs **before** `CollisionSystem::update()` even fires. When the solver later tries to resolve the box against the `StaticActor` floor, the box's position has *already been teleported* by `resolveWorldBounds()`, making the solver's overlap calculations wrong.

```
Frame Timeline (BROKEN):
  1. Scene::prepare()         → capture prevPosition ✓
  2. RigidActor::update()     → integrate() moves box DOWN past floor
                              → resolveWorldBounds() TELEPORTS it to Y=230 ← CORRUPTS STATE
  3. CollisionSystem::update()→ box is now at Y=230, but StaticActor floor is at Y=110
                              → solver sees no overlap → DOES NOTHING
```

**Fix:** Remove `resolveWorldBounds()` from all `update()` methods.

---

### Flaw 2: `PhysicsActor::onCollision()` corrupts velocity

```cpp
void PhysicsActor::onCollision(Actor* other) {
    velocity.x = -velocity.x * restitution;  // BLINDLY reverses X every collision!
}
```

This runs **during detection**, **before** resolution. Every collision reverses X velocity regardless of axis or direction. Then the solver *also* modifies velocity. Result: chaos.

**Fix:** Make `onCollision()` notification-only (empty default).

---

### Flaw 3: Sub-stepping re-detects on stale grid data

`CollisionSystem::update()` is called 4 times per frame. Each call rebuilds the grid and re-detects. But since `integrate()` only runs once, the sub-steps just repeat the same work.

**Fix:** Single detection pass + iterative relaxation (position-only corrections, no re-detection).

---

### Flaw 4: SpatialGrid clamps positions to logical screen area

```cpp
if (ix < 0) ix = 0; 
if (ix >= cols) ix = cols - 1;
```

Off-screen walls (X=-100) get clamped to cell 0, causing detection confusion.

**Fix:** Static actors bypass the grid entirely. They are checked directly against all rigid bodies.

---

### Flaw 5: `KinematicActor::moveAndCollide` heap-allocates per sub-step

```cpp
std::vector<Actor*> collisions;  // HEAP ALLOC in inner loop!
```

48 heap allocations per frame on ESP32.

**Fix:** Use a fixed-size static array.

---

## 2. ESP32 Design Requirements

| Constraint | Design Response |
|---|---|
| No FPU (C3) | Fixed-point math (already done ✓) |
| ~160KB RAM | Flat arrays, zero `std::vector` in hot paths |
| 160MHz single-core | O(n) broadphase, < 20 actors total |
| ~60 FPS target | Physics budget: **~2ms per frame** max |
| Simple 2D games | Only AABB, no rotation, no joints |

### What we need:
1. **Gravity** → accelerate rigid bodies downward
2. **AABB overlap detection** → find intersecting hitboxes
3. **Position correction** → push overlapping bodies apart
4. **Velocity zeroing on static contact** → stop bodies when they land (or bounce if flag is set)
5. **Kinematic movement** → player stops at walls

---

## 3. Proposed Architecture: "Flat Solver"

```
Frame Timeline (CORRECT):
  1. Entity::update()           → integrate() only (no world bounds)
  2. CollisionSystem::update()  → Single detection pass
                                → N relaxation iterations (position-only)
                                → Static bodies resolved LAST (final arbiter)
                                → Zero/bounce velocity on contact
  3. Scene::draw()              → Render final positions
```

### Key Principles:

1. **Single responsibility per phase.** Rigid bodies only integrate during `update()`. Solver owns all responses.
2. **Solver owns all collision responses.** `onCollision` = pure notification.
3. **Iterative relaxation, not sub-stepping.** 2-3 position correction passes without re-detecting.
4. **Static bodies are special.** Separate array, always checked, never in grid.
5. **Zero heap allocations.** All arrays are `static` or fixed-size.
6. **Optional bounce flag.** Per-body `bool bounce = false`. When false, velocity is zeroed on static contact.

---

## 4. Implementation Phases

### Phase A: Remove Conflicting Systems
- `PhysicsActor::update()` → remove `resolveWorldBounds()` call
- `RigidActor::update()` → remove `resolveWorldBounds()` call
- `PhysicsActor::onCollision()` → empty body (notification only)

### Phase B: Simplify the Solver
- Rewrite `CollisionSystem::update()` — single detection pass + N relaxation iterations
- Rewrite `CollisionSystem::resolve()` — position correction + velocity zeroing/bouncing
- Add `staticBodies[]` separate from grid

### Phase C: Fix Grid + Kinematic
- `SpatialGrid::insert()` → skip static actors
- `KinematicActor::moveAndCollide()` → replace `std::vector` with static array

### Phase D: Clean Up
- Remove `Scene::prepare()`, `prevPosition` capture, and sub-stepping loop
- Remove `prepare()` call from `Engine.cpp`
- Remove `prevPosition` from `Actor.h`
- Update `PhysicsDemoScene.cpp` — on-screen walls only

### Phase E: Verify
- Build and run PhysicsDemo natively
- Test: boxes fall, land on floor, stack stably
- Test: player pushes boxes into walls, no tunneling
- Test: 12+ boxes stress test
