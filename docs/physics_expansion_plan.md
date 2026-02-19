# Physics System Expansion: Static, Kinematic, and Rigid Actors

This plan outlines the implementation of three new specialized actor types to enhance the engine's physics capabilities. These types are inspired by Godot Engine's design but adapted for the PixelRoot32 engine's fixed-point/performance-oriented architecture.

## 1. Conceptual Analysis

| Type | Responsibilities | Movement Control | Interactions |
| :--- | :--- | :--- | :--- |
| **Static** | Walls, floors, non-moving obstacles. | None (Fixed position). | Blocks Kinematic and Rigid bodies. Infinitely heavy. |
| **Kinematic** | Players, moving platforms, triggered objects. | Manual (via script/velocity). | Pushes Rigid bodies. Not affected by forces/gravity. |
| **Rigid** | Props, debris, projectile-like objects. | Automatic (Engine solver). | Affected by gravity, forces, and collisions. |
| **Grid (Opt)** | Spatial partitioning layer. | Internal grid hashing. | Reduces O(N^2) checks to O(N). |

### PhysicsActor vs Delegation
- **PhysicsActor (Base)**: Keeps common state (`position`, `hitbox`, `layers`) and common logic (`onCollision` callback).
- **Delegated to Subclasses**: Specific movement integration (`Rigid`), sliding/stepping logic (`Kinematic`), and specialized physics properties.

## 2. Architecture Design
- **Hierarchy**: `Actor` -> `PhysicsActor` -> {`StaticActor`, `KinematicActor`, `RigidActor`}.
- **Composition vs Inheritance**: Inheritance is preferred here to maintain compatibility with the existing `Actor` system and simplify the `CollisionSystem` update logic.
- **Performance**: Use of `Scalar` (Fixed16/Float) is maintained. `StaticActor` will be flagged to skip update loops, saving CPU cycles on ESP32.

## 3. Implementation Plan by Phases

### Phase 1: Base Refactor [x]
- Update `PhysicsActor` with `PhysicsBodyType` enum.
- Decouple velocity integration into virtual methods.
- Add `mass` and `gravityScale` properties.

### Phase 2: StaticActor Implementation [x]
- Create `StaticActor` class.
- Ensure `CollisionSystem` recognizes it as an immovable obstacle.
- Success: `StaticActor` objects stay fixed even when "hit" by other actors.

### Phase 3: KinematicActor Implementation [x]
- Implement `moveAndCollide(Vector2 relative_move)`.
- Implement basic `moveAndSlide()` for platformer/top-down standard movement.
- Success: Actors can move and stop precisely at walls without overlapping.

### Phase 4: RigidActor Implementation [x]
- Implement force integration (Euler or Semi-implicit Euler).
- Integrate gravity support into the physics step.
- Success: `RigidActor` falls and accelerates correctly.

### Phase 5: Collision System & Solver [x]
- Update `CollisionSystem` update loop to handle different interactions.
- Implement impulse-based resolution or simple position correction for Rigid bodies.
- Success: Bodies bounce off each other based on restitution.

### Phase 6: Spatial Partitioning (Uniform Grid) [x]
- Implement a `SpatialGrid` class to divide the world into fixed cells.
- Update `CollisionSystem` to query only occupied/neighboring cells.
- Success: Significant reduction in collision checks for scenes with >50 entities.

### Phase 7: Testing & Performance Profiling [x]
- Create a `PhysicsExtensionDemo` in the samples repository.
- Benchmark collision solver and grid optimization on target hardware (ESP32).
- Success: Stable 60fps with at least 20 dynamic physical bodies and 100+ static obstacles.

## 4. Technical Risks
- **Memory Overhead**: A Uniform Grid requires a buffer; we'll use a fixed-size arena or sparse hash if memory is tight.
- **Fixed-Point Precision**: Cumulative errors in impulse resolution might cause "jitter".
- **Performance**: Recursive `moveAndSlide` calls can be expensive; limits on "slide steps" will be implemented.
- **Backward Compatibility**: Existing `PhysicsActor` behavior must be preserved or cleanly migrated to `Kinematic` default.

## 5. Success Criteria
- [x] `StaticActor` prevents movement of `Kinematic` and `Rigid`.
- [x] `KinematicActor` can slide along walls using `moveAndSlide`.
- [x] `RigidActor` responds to gravity and bounces off surfaces.
- [x] Unit tests for all 3 types pass in CI.
- [x] No regression in existing Sample games.
