# Technical Evaluation: Flat Solver Architecture v2.0

## 1. Executive Summary
The transition from the legacy iterative sub-stepping solver to the current **Flat Solver v2.0** represents a significant leap in stability and performance for PixelRoot32. By moving away from state-clamping hacks (`resolveWorldBounds`) and adopting a strictly sequential pipeline with iterative relaxation, the system now provides stable stacking and collision resolution that fits within the tight 2ms physics budget of the ESP32-C3.

---

## 2. Structural Improvements (v1.0 vs v2.0)

| Feature | Legacy Solver (v1.0) | Flat Solver (v2.0) | Status |
| :--- | :--- | :--- | :--- |
| **Stability** | Jittery; suffered from "Floor Tunneling". | High stability; convergent relaxation. | **FIXED** |
| **State Management** | State corrupting `resolveWorldBounds()`. | Integration is independent of resolution. | **FIXED** |
| **Detection** | Broadphase clamped to viewport (O(N^2) risk). | Uniform Spatial Grid (Shared memory). | **OPTIMIZED** |
| **Memory** | High heap frequency in `moveAndSlide`. | Zero heap allocations in hot paths. | **FIXED** |
| **Actor Types** | Generic PhysicsActor only. | Static, Kinematic, and Rigid specialization. | **NEW** |
| **Shape Support** | AABB only. | AABB, Circle, and Mixed Circle-Box. | **NEW** |

---

## 3. Core Architectural Pillars

### 3.1 The Deterministic Pipeline
The solver now enforces a strict execution order in `CollisionSystem.cpp`:
1. **Grid Refresh**: Captures dynamic positions; skips `StaticActor`.
2. **Detection Pass**: Populates fixed-size `dynamicPairs` and `staticPairs` arrays.
3. **Iterative Relaxation**: Runs N passes (default: 8) of position correction.
4. **Final Arbiter**: Static bodies are resolved last in every relaxation step to ensure world boundaries are never violated.

### 3.2 Position Relaxation vs Impulse
Unlike higher-end engines (Box2D) that use complex impulse manifolds, v2.0 uses **Position-Based Dynamics (PBD)** principles:
- **Separation**: Objects are "pushed" apart by penetration depth.
- **Velocity Management**: Velocity is zeroed or reflected *only after* a stable position is reached. This eliminates the "bouncing box" syndrome seen in earlier builds.

---

## 4. Performance Profile (ESP32-C3)
The system is tuned to stay under **2ms per frame** during high-load scenarios:
- **Spatial Grid**: Uses a shared buffer of ~100KB, allowing multiple scenes without increasing DRAM consumption.
- **Narrowphase**: Optimized fixed-point square roots for distance checks (Circle vs AABB).
- **Stress Test**: Verified stable 60 FPS with:
    - 20+ Rigid Bodies (Mixed Shapes).
    - 100+ Static Obstacles (Walls/Floors).

---

## 5. Potential Risks & Recommendations

> [!IMPORTANT]
> **Circle-Box Edge Cases**: When a circle center is perfectly inside a box, the solver defaults to the closest edge. While functional, rapid tunneling at extremely high velocities (teleportation) could still occur if logic exceeds 128 simultaneous pairs (`PHYSICS_MAX_PAIRS`).

### Recommendations:
1. **Collision Layers**: Encourage users to use the `mask/layer` system to prune the broadphase early.
2. **Kinematic Buffer**: `KinematicActor` uses a fixed static array of 16. For games with very high entity density (bullets), this may need to be increased via `EngineConfig.h`.

---

## 6. Final Verdict
The **Flat Solver v2.0** is an exceptionally well-engineered solution for retro-style physics on modern microcontrollers. It successfully balances fixed-point performance constraints with a "pro-grade" API (Godot-inspired), making it the most robust component of the engine to date.
