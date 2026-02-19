# PixelRoot32 / PR32: Physics System Reference – "The Flat Solver"

This document describes the design and implementation of the PixelRoot32 physics system, known as the **Flat Solver**. It covers the architectural decisions made to support stable, multi-object simulations on resource-constrained hardware like the ESP32.

- **Design Philosophy**:
  - **Deterministic**: Consistent behavior across different hardware targets.
  - **Low-Cost**: Optimized for non-FPU microcontrollers (using `Scalar` fixed-point).
  - **Memory-Efficient**: Drastically reduced DRAM footprint through shared spatial partitioning.
  - **Stable**: Uses iterative relaxation to handle stacked objects without excessive sub-stepping.
  - **Godot-Inspired**: Uses a familiar specialized actor hierarchy (Static, Kinematic, Rigid).

---

## 1. Overview: The Flat Solver Architecture

The physics system is designed as a "Flat Solver," meaning it performs resolution in a clear, sequential pipeline without the complexity of deep recursive sub-stepping.

### 1.1 The Simulation Pipeline
Every frame, the `Scene` executes the physics step in the following order:

1.  **Gravity Integration**: Applies global gravity to all `RigidActor` entities.
2.  **Broadphase (Spatial Grid)**: Efficiently identifies potential colliding pairs using a uniform grid.
3.  **Narrowphase**: Calculates collision manifolds (overlap normal and depth) for AABB and Circle shapes.
4.  **Iterative Relaxation**: Performs multiple passes (controlled by `PHYSICS_RELAXATION_ITERATIONS`) of position correction to resolve penetration.
5.  **Static Arbiter**: Forces `StaticActor` objects to be immovable, pushing dynamic bodies out.
6.  **Velocity Correction**: Updates velocities for bouncing and sliding based on the final contact normals.

---

## 2. Spatial Partitioning: Uniform Grid

To avoid $O(N^2)$ checks, the engine uses a **Uniform Spatial Grid**.

- **Grid Hashing**: Actors are mapped to cells based on their bounding boxes.
- **Cell Size**: Configurable via `SPATIAL_GRID_CELL_SIZE` (default 32px).
- **Shared Memory Optimization**: 
  - On memory-dense platforms, each scene has its own grid.
  - **ESP32 Optimization**: The engine uses **Static Shared Buffers** for `cells` and `cellCounts`. Since scenes update sequentially and the grid is cleared every frame, this reclaims ~100KB of DRAM.

---

## 3. Specialized Actor Hierarchy

Following modern engine standards, bodies are specialized to reduce logical complexity in the solver.

### 3.1 StaticActor
- **Role**: World geometry (floors, walls).
- **Behavior**: Infinite mass. Never moves.
- **Solver Logic**: Skipped during grid insertion if they are too large, or treated as the "ground truth" during relaxation.

### 3.2 KinematicActor
- **Role**: Players, moving platforms.
- **Behavior**: Movement is driven by game logic, not the solver.
- **Key Methods**:
  - `moveAndCollide()`: Stops at the first point of contact.
  - `move_and_slide()`: Automatically slides along surfaces, ideal for platformers.

### 3.3 RigidActor
- **Role**: Props, debris, physics-driven objects.
- **Behavior**: Fully automatic. Responds to gravity, impulses, and collisions.
- **Properties**: `mass`, `restitution` (bounciness), and `gravityScale`.

---

## 4. Collision Shapes and Manifolds

The engine supports mixed-shape collisions with optimized narrowphase algorithms.

| Interaction | Logic |
| :--- | :--- |
| **AABB vs AABB** | Standard SAT (Separating Axis Theorem) for axis-aligned rectangles. |
| **Circle vs Circle** | Distance check with overlap fallback to prevent "fusion" in perfectly aligned centers. |
| **Circle vs AABB** | Clamping the circle center to the box edges to find the closest point. |

### 4.1 Penetration Fallback
When centers of two circles overlap perfectly, the solver applies a vertical normal fallback to "break" the singularity and push objects apart stably.

---

## 5. Iterative Relaxation (Position Correction)

Unlike impulse-only solvers that suffer from "jitter" or "explosive" stacking, PixelRoot32 uses **Position Relaxation**.

1.  Calculate penetration depth $d$ and normal $n$.
2.  Move objects apart by $d \times k$, where $k$ is the relaxation factor.
3.  Repeat this $M$ times (where $M = PHYSICS\_RELAXATION\_ITERATIONS$).

This allows for very stable stacks of boxes or circles even at high gravity, as the solver converges on a non-penetrating state before the frame is rendered.

---

## 6. Configuration (EngineConfig.h)

Developers can tune the physics system for their specific hardware and game needs:

```cpp
// Maximum simultaneous collisions tracked
#define PHYSICS_MAX_PAIRS 128

// Accuracy vs Performance (Default: 8)
#define PHYSICS_RELAXATION_ITERATIONS 8

// Typical cell size for broadphase
#define SPATIAL_GRID_CELL_SIZE 32
```

---

## 7. Performance Tips for ESP32

1.  **Use StaticActors Whenever Possible**: They are significantly cheaper than dynamic bodies.
2.  **Limit Rigid Bodies**: On the ESP32-C3, aim for <20 simultaneous `RigidActor` objects for a stable 60 FPS.
3.  **Adjust Cell Size**: If your game has very large or very small sprites, matching `SPATIAL_GRID_CELL_SIZE` to the average actor size improves broadphase efficiency.
4.  **Shape Awareness**: AABB vs AABB is the cheapest check. Circle vs AABB is slightly more expensive due to square root operations (even with fixed-point optimizations).

---

## 8. Summary

The PixelRoot32 Physics System provides a balance between retro feel and modern stability. By combining a **Flat Solver** with **Spatial Partitioning** and **Specialized Actor Types**, it delivers a robust physics experience that fits within the tight memory constraints of the ESP32 while providing a professional, Godot-like API for developers.
