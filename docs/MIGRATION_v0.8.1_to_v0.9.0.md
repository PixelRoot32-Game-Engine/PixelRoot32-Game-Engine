# Migration Guide: v0.8.1-dev → v0.9.0-dev

## Overview

This guide documents the changes required to migrate examples from version 0.8.1-dev to 0.9.0-dev. The main migration involves changing the C++ standard from C++11 to C++17, adopting smart pointers (`std::unique_ptr`) for entity memory management, implementing the **Scalar Math** system for cross-platform compatibility, and the introduction of the **Flat Solver** physics engine.

---

## Configuration Changes (platformio.ini)

### 1. Updated C++ Standard

**Before:**

```ini
build_flags = 
    -std=c++11
```

**After:**

```ini
build_unflags = -std=gnu++11
build_flags = 
    -std=gnu++17
    -fno-exceptions
```

### 2. Test Configuration

**New:**

```ini
[platformio]
test_dir = lib/PixelRoot32-Game-Engine/test
```

### 3. Profiling Flag Enabled

Added `-D PIXELROOT32_ENABLE_PROFILING` for performance analysis on all platforms.

### 4. Debug Overlay for Native

For the `native` environment, enabled by default:

```ini
-D PIXELROOT32_ENABLE_DEBUG_OVERLAY
```

---

## Source Code Changes (src/)

### 1. Header Includes

**Smart Pointers**: Add in all files using smart pointers:

```cpp
#include <memory>
```

**Engine Config**: Replace `#include "EngineConfig.h"` with:

```cpp
#include "platforms/EngineConfig.h"
```

(The deprecated `include/EngineConfig.h` forwarding header has been removed).

### 2. Replacing Raw Pointers with std::unique_ptr

**Change Pattern:**

| Previous Type | New Type |
|---------------|------------|
| `Type*` | `std::unique_ptr<Type>` |
| `std::vector<Type*>` | `std::vector<std::unique_ptr<Type>>` |

**Example - Member Declarations:**

**Before:**

```cpp
class MenuScene : public Scene {
private:
    UILabel* titleLabel;
    UIButton* gamesButton;
    std::vector<BrickActor*> bricks;
};
```

**After:**

```cpp
class MenuScene : public Scene {
private:
    std::unique_ptr<UILabel> titleLabel;
    std::unique_ptr<UIButton> gamesButton;
    std::vector<std::unique_ptr<BrickActor>> bricks;
};
```

### 3. Object Creation

**Before:**

```cpp
titleLabel = new UILabel("Examples", 0, menu::TITLE_Y, Color::White, menu::TITLE_FONT_SIZE);
addEntity(titleLabel);
```

**After:**

```cpp
titleLabel = std::make_unique<UILabel>("Examples", 0, menu::TITLE_Y, Color::White, menu::TITLE_FONT_SIZE);
addEntity(titleLabel.get());
```

### 4. Manual Cleanup Removal

**Before:**

```cpp
Scene::~Scene() {
    if (background) {
        removeEntity(background);
        delete background;
        background = nullptr;
    }
}
```

**After:**

```cpp
Scene::~Scene() {
    // std::unique_ptr handles cleanup automatically
}
```

### 5. Accessing Objects in Vectors

**Before:**

```cpp
for(auto* b : bricks) {
    removeEntity(b);
    delete b;
}
bricks.clear();
```

**After:**

```cpp
for(auto& b : bricks) {
    removeEntity(b.get());
}
bricks.clear(); // std::unique_ptr releases memory automatically
```

### 6. Safe Handling of getCurrentScene()

**Before:**

```cpp
PongScene* pongScene = static_cast<PongScene*>(engine.getCurrentScene());
```

**After:**

```cpp
PongScene* pongScene = static_cast<PongScene*>(engine.getCurrentScene().value_or(nullptr));
```

### 7. Entity Position Refactoring (x, y -> position)

The `Entity` class (and all subclasses like `Actor`) has been refactored to use `Vector2` for positioning instead of separate `x` and `y` scalars. This improves vector math operations and physics integration.

**Member Access:**

**Before:**

```cpp
entity->x += speed;
if (entity->y > 200) { ... }
```

**After:**

```cpp
entity->position.x += speed;
if (entity->position.y > 200) { ... }
// Or using Vector2 methods:
entity->position += Vector2(speed, 0);
```

**Constructors:**
Constructors still support passing `x` and `y` as separate arguments for convenience, but they are stored in `position`.

```cpp
// Still valid:
MyEntity(Scalar x, Scalar y) : Entity(x, y, 16, 16, EntityType::ACTOR) {}

// New alternative:
MyEntity(Vector2 pos) : Entity(pos, 16, 16, EntityType::ACTOR) {}
```

---

## Migration to Scalar Math (Fixed-Point Support)

### Overview

Version 0.9.0 introduces the **Math Policy Layer**, which abstracts numerical representations to support both FPU-enabled platforms (ESP32, ESP32-S3) and integer-only platforms (ESP32-C3, ESP32-S2) with a single codebase.

- **`Scalar`**: A type alias that resolves to `float` on FPU platforms and `Fixed16` (16.16 fixed-point) on others.
- **`Vector2`**: Now uses `Scalar` components instead of `float`.

### 1. Basic Type Replacement

Replace `float` with `Scalar` in your game logic, physics, and entity positions.

**Before:**

```cpp
float x, y;
float speed = 2.5f;
Vector2 velocity; // Previously float-based
```

**After:**

```cpp
using pixelroot32::math::Scalar;

Scalar x, y;
Scalar speed = pixelroot32::math::toScalar(2.5f);
Vector2 velocity; // Now Scalar-based
```

### 2. Handling Literals

When assigning floating-point literals to `Scalar` variables, use the `toScalar()` helper or explicit casts to ensure compatibility with `Fixed16`.

```cpp
// math/Scalar.h
#include "math/Scalar.h"

// ...

// Preferred:
Scalar gravity = math::toScalar(9.8f);

// Also valid (but less portable if type changes):
Scalar damping = Scalar(0.95f);
```

### 3. Math Functions

Use `pixelroot32::math::MathUtil` or `Scalar` member functions instead of `std::` math functions, as `Fixed16` is not compatible with `std::sin`, `std::sqrt`, etc.

**Before:**

```cpp
#include <cmath>

float dist = std::sqrt(x*x + y*y);
float angle = std::atan2(y, x);
float val = std::abs(input);
```

**After:**

```cpp
#include "math/MathUtil.h"

// Use lengthSquared() to avoid sqrt() when comparing distances
if (pos.lengthSquared() < range * range) { ... }

// If you really need sqrt:
Scalar dist = math::sqrt(val);

// Absolute value
Scalar val = math::abs(input);
```

### 4. Rendering (Scalar to int)

The `Renderer` still works with integer coordinates (`int`). You must convert `Scalar` positions to `int` when drawing.

**Before:**

```cpp
renderer.drawSprite(sprite, x, y, Color::White); // implicit cast float->int
```

**After:**

```cpp
// Explicit cast is safer and clarifies intent
renderer.drawSprite(sprite, static_cast<int>(x), static_cast<int>(y), Color::White);
```

### 5. Random Numbers

Use `math::randomScalar()` instead of `rand()` or `float` based random generation to ensure consistent behavior across platforms.

```cpp
Scalar randVal = math::randomScalar(0, 10); // Returns Scalar between 0 and 10
```

---

## Physics System Migration (API Changes)

### Overview

Version 0.9.0 refines the physics API to better distinguish between static and dynamic actors, and integrates the **Scalar Math** system for consistent physics simulation across platforms.

### 1. Actor Types

Explicitly choose the correct actor type for your entity:

- **`RigidActor`**: For dynamic objects that move, bounce, and respond to gravity (e.g., Balls, Player characters, Debris).
- **`StaticActor`**: For immovable environmental objects (e.g., Walls, Floors, Platforms). These are optimized and do not run physics integration.
- **`KinematicActor`**: For moving objects that ignore forces but push other objects (e.g., Moving Platforms, Elevators).

**Before:**

```cpp
class Wall : public PhysicsActor { ... }; // Generic PhysicsActor used for everything
```

**After:**

```cpp
class Wall : public StaticActor { ... }; // Specialized for static objects
```

### 2. Initialization and Properties

Physics properties must now be set using `Scalar` values.

**Before:**

```cpp
setRestitution(1.0f);
setFriction(0.5f);
setGravityScale(1.0f);
```

**After:**

```cpp
using pixelroot32::math::toScalar;

setRestitution(toScalar(1.0f));
setFriction(toScalar(0.0f));
setGravityScale(toScalar(1.0f));
```

### 3. Collision Configuration

Use `setShape` to define the collision geometry (default is `BOX`) and configure collision layers using bitmasks.

```cpp
// Set shape
setShape(pixelroot32::core::CollisionShape::CIRCLE);

// Set Layers
setCollisionLayer(Layers::BALL);
setCollisionMask(Layers::PADDLE | Layers::WALL);
```

### 4. Position & Rendering

`RigidActor` maintains the position of the **top-left corner** of the bounding box (AABB), even for Circles. When rendering a Circle, you may need to offset to the center.

**Example (BallActor):**

```cpp
// Constructor passes top-left position to RigidActor
BallActor::BallActor(Vector2 pos, int radius)
    : RigidActor(Vector2(pos.x - radius, pos.y - radius), radius * 2, radius * 2) { ... }

// Draw needs to offset back to center if drawing a circle from center
void BallActor::draw(Renderer& renderer) {
    renderer.drawFilledCircle((int)position.x + radius, (int)position.y + radius, radius, Color::White);
}
```

---

## Modified Files (Examples)

### MenuScene.cpp / MenuScene.h

- All UI pointers (`UILabel*`, `UIButton*`, `UIVerticalLayout*`) converted to `std::unique_ptr`
- Methods `setupMainMenu()`, `setupGamesMenu()`, etc., updated to use `std::make_unique`

### CameraDemoScene.cpp / CameraDemoScene.h

- `PlayerCube* gPlayer` → `std::unique_ptr<PlayerCube> player`
- Removed global pointer `gPlayer`, now a class member
- Updated to use `Scalar` for position and movement.

### Games/BrickBreaker/

- `PaddleActor*`, `BallActor*`, `ParticleEmitter*` → `std::unique_ptr`
- `std::vector<BrickActor*>` → `std::vector<std::unique_ptr<BrickActor>>`

### Games/Pong/

- `PaddleActor* leftPaddle/rightPaddle`, `BallActor* ball` → `std::unique_ptr`
- Added `std::vector<std::unique_ptr<Entity>> ownedEntities` for additional entities

### Games/Snake/

- `SnakeBackground* background` → `std::unique_ptr<SnakeBackground>`
- `std::vector<SnakeSegmentActor*> segmentPool` → `std::vector<std::unique_ptr<SnakeSegmentActor>>`
- `snakeSegments` keeps raw pointers (non-owning references)

### Games/SpaceInvaders/

- Conditional use of arena vs smart pointers based on `PIXELROOT32_ENABLE_SCENE_ARENA`
- `#ifdef` blocks to differentiate memory management
- **Fixed-Point Migration**: Updated `AlienActor` and `SpaceInvadersScene` to use `Scalar` for coordinates and movement.

### Games/Metroidvania/

- Added explicit constructors/destructors
- Tilemap layers managed with `std::vector<std::unique_ptr<Entity>>`

### DualPaletteTest/ and FontTest/

- `TestBackground*`, `TestSprite*`, `TestText*` → `std::unique_ptr`
- Removed manual cleanup code in destructors

---

## Important Considerations

### 1. Scene Arena Compatibility

When `PIXELROOT32_ENABLE_SCENE_ARENA` is defined, continue using the memory arena. Smart pointer changes mainly apply when the arena is disabled.

```cpp
#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
    player = arenaNew<PlayerActor>(arena, x, y);
    addEntity(player);
#else
    player = std::make_unique<PlayerActor>(x, y);
    addEntity(player.get());
#endif
```

### 2. Forward Declarations

Some classes require additional forward declarations:

```cpp
class PlayerCube;  // Instead of full #include
```

### 3. Methods Returning Pointers

If a method returns a pointer to an object managed by `unique_ptr`:

```cpp
// Header
ParticleEmitter* getParticleEmiter() { return explosionEffect.get(); }

// Usage
std::unique_ptr<ParticleEmitter> explosionEffect;
```

---

## Migration Benefits

1. **Memory Safety**: Elimination of memory leaks through RAII
2. **Cleaner Code**: No need for manual `delete`
3. **Dangling Pointer Prevention**: `std::unique_ptr` automatically invalidates
4. **Disabled Exceptions**: `-fno-exceptions` reduces binary size
5. **Modern C++17**: Access to features like `std::optional`, `if constexpr`, etc.
6. **Performance (C3/S2)**: `Fixed16` provides hardware-accelerated-like performance on chips without FPU.
7. **Cross-Platform Compatibility**: Code runs efficiently on both FPU and non-FPU devices without changes.

---

## Post-Migration Verification

1. Compile with all platforms defined in `platformio.ini`:

   ```bash
   pio run -e esp32dev
   pio run -e esp32c3
   pio run -e native
   ```

2. Run tests if available:

   ```bash
   pio test
   ```

3. Verify there are no memory leaks (especially in scenes that are recreated)
4. Verify FPS improvement on ESP32-C3 (should be ~30 FPS vs ~24 FPS before migration).

---

## Physics System Overhaul: Flat Solver

### Overview

Version 0.9.0 introduces **Flat Solver**, a major architectural overhaul of the physics system. This is NOT a breaking API change, but physics behavior will differ significantly.

### Key Changes

| Aspect | Legacy Behavior | Flat Solver |
|--------|---------|---------------|
| **Solver Type** | Relaxation-based position solver | Impulse-based velocity + Baumgarte position |
| **Timestep** | Variable deltaTime | Fixed 1/60s |
| **Pipeline** | Integrated → Detect → Relax | Detect → Velocity → Position → Penetration |
| **Iterations** | `PHYSICS_RELAXATION_ITERATIONS` (8) | `VELOCITY_ITERATIONS` (2) |
| **CCD** | None | Selective for fast circles |
| **Kinematic vs Rigid** | Broken detection | Fixed and working |

### Behavioral Differences

#### 1. Perfect Elastic Collisions Now Work

**Legacy Behavior:** Restitution 1.0 would lose energy or cause objects to stick to walls.

**Flat Solver:**

```cpp
ball->setRestitution(toScalar(1.0f));  // Actually works now!
ball->setFriction(toScalar(0.0f));     // Perfect energy conservation
ball->setGravityScale(toScalar(0.0f)); // No gravity interference
```

**Result**: Objects bounce forever without losing energy (tested: 1000+ bounces, 0% energy loss).

#### 2. Position Integration Moved

**Legacy Behavior:**

```cpp
void RigidActor::update(unsigned long dt) {
    // You integrated position manually
    position.x += velocity.x * dt / 1000.0f;
    position.y += velocity.y * dt / 1000.0f;
}
```

**Flat Solver:**

```cpp
void RigidActor::update(unsigned long deltaTime) {
    // ONLY integrate velocity (forces)
    // Position is handled by CollisionSystem::integratePositions()
    integrate(CollisionSystem::FIXED_DT);
}
```

**Important**: Do NOT integrate position in your Actor's update method. The CollisionSystem now handles this automatically after the velocity solver.

#### 3. Pipeline Order Matters

The new execution order is critical for stability:

```
Frame Start
│
├─ 1. detectCollisions()       → Find all overlaps
├─ 2. solveVelocity()          → Apply impulse responses
├─ 3. integratePositions()     → Update positions: p = p + v * dt
├─ 4. solvePenetration()       → Baumgarte position correction
└─ 5. triggerCallbacks()       → Call onCollision()
```

**Why this order?**

- Velocity must be solved before position integration (prevents energy loss)
- Position integration must happen before penetration correction (allows proper separation)
- Callbacks happen last so gameplay sees final state

#### 4. CCD (Continuous Collision Detection)

**New in Flat Solver**: Automatic CCD for fast-moving circles.

```cpp
// CCD activates when: velocity * dt > radius * CCD_THRESHOLD
// Default CCD_THRESHOLD = 3.0

// Example: Ball with radius 6px
// CCD activates when speed > 1080 px/s (6 * 3 / (1/60))
```

**No code changes required** - it activates automatically when needed.

**Use case**: Prevents tunneling when ball moves extremely fast.

#### 5. Kinematic vs Rigid Detection Fixed

**Legacy Behavior**: KinematicActor vs RigidActor collisions didn't work reliably.

**Flat Solver**: Fixed and working correctly.

```cpp
// Now works correctly:
class Paddle : public KinematicActor { ... };
class Ball : public RigidActor { ... };

// Ball correctly detects collision with paddle
```

### Code Migration Examples

#### Example 1: Ball Actor

**Legacy:**

```cpp
void BallActor::update(unsigned long deltaTime) {
    Scalar dt = toScalar(deltaTime * 0.001f);
    
    // Manual position integration
    position += velocity * dt;
    
    // Manual bounce logic (workaround for broken physics)
    if (hitWall) {
        velocity.y = -velocity.y;
    }
}
```

**New (Flat Solver):**

```cpp
void BallActor::update(unsigned long deltaTime) {
    // Physics handles position integration
    RigidActor::update(deltaTime);
}

void BallActor::onCollision(Actor* other) {
    // No manual bounce needed!
    // Physics system handles it automatically with restitution
    
    // Only gameplay-specific logic here
    if (other->isInLayer(Layers::PADDLE)) {
        playSound(600.0f);
    }
}
```

#### Example 2: Setting Up Physics Properties

**Legacy:**

```cpp
auto ball = std::make_unique<BallActor>(x, y, radius);
ball->setRestitution(1.0f);  // Would lose energy anyway
ball->bounce = true;
// Had to manually handle bounces in onCollision
```

**New (Flat Solver):**

```cpp
auto ball = std::make_unique<BallActor>(x, y, radius);
ball->setRestitution(toScalar(1.0f));  // Now works perfectly
ball->setFriction(toScalar(0.0f));
ball->setGravityScale(toScalar(0.0f));
ball->setShape(CollisionShape::CIRCLE);
ball->setRadius(toScalar(radius));  // Important for CCD!
ball->bounce = true;
// Physics handles everything automatically
```

#### Example 3: Collision Layers

**No changes required** - works the same:

```cpp
ball->setCollisionLayer(Layers::BALL);
ball->setCollisionMask(Layers::PADDLE | Layers::WALL);
```

### Configuration Changes

#### Constants (in CollisionSystem.h)

```cpp
// New constants
static constexpr Scalar FIXED_DT = toScalar(1.0f / 60.0f);  // Fixed timestep
static constexpr Scalar SLOP = toScalar(0.02f);              // Ignore small penetration
static constexpr Scalar BIAS = toScalar(0.2f);               // Position correction factor
static constexpr Scalar VELOCITY_THRESHOLD = toScalar(0.5f); // Zero restitution below this
static constexpr int VELOCITY_ITERATIONS = 2;                // Was: PHYSICS_RELAXATION_ITERATIONS (8)
static constexpr Scalar CCD_THRESHOLD = toScalar(3.0f);      // CCD activation threshold
```

#### Tuning for Your Game

**More stable stacking** (slower):

```cpp
static constexpr int VELOCITY_ITERATIONS = 4;  // Default: 2
static constexpr Scalar BIAS = toScalar(0.3f); // Default: 0.2
```

**Faster, looser collisions**:

```cpp
static constexpr Scalar SLOP = toScalar(0.05f); // Default: 0.02
```

### Performance Notes

| Metric | Legacy | Flat Solver |
|--------|--------|--------|
| **Iterations** | 8 (relaxation) | 2 (impulse) |
| **Speed** | Baseline | ~10-15% faster on ESP32-C3 |
| **Stability** | Jitter on stacks | Stable stacking |
| **Determinism** | Variable dt | Fixed dt, reproducible |
| **Memory** | ~100KB (shared grid) | Same |

### Testing Checklist

After migrating, verify:

- [ ] **Restitution**: Objects with restitution 1.0 bounce forever without energy loss
- [ ] **No sticking**: Objects don't get stuck in walls (tested: 0 stuck frames in 6000+)
- [ ] **Stacking**: Multiple objects stack without jitter or explosions
- [ ] **Kinematic**: Kinematic vs Rigid collisions work (e.g., paddle hits ball)
- [ ] **CCD**: Fast objects (>1000 px/s) don't tunnel through walls
- [ ] **Callbacks**: onCollision() still fires correctly
- [ ] **Performance**: FPS maintained or improved

### Troubleshooting

#### Problem: Objects fall through floors

**Cause**: You might still be integrating position manually.

**Fix**: Remove position integration from your Actor::update(). Let CollisionSystem handle it.

#### Problem: No collisions detected

**Cause**: Missing shape or radius configuration.

**Fix**:

```cpp
actor->setShape(CollisionShape::CIRCLE);
actor->setRadius(toScalar(radius));  // Critical for circles!
```

#### Problem: Bounces feel wrong

**Cause**: Using old manual bounce logic alongside new system.

**Fix**: Remove manual velocity reflections from onCollision(). Let restitution handle it.

---

## References

- [C++ Core Guidelines - Smart Pointers](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-resource)
- [PlatformIO Build Flags](https://docs.platformio.org/en/latest/projectconf/sections/env/options/build/build_flags.html)
- [Fixed-Point Arithmetic (Wikipedia)](https://en.wikipedia.org/wiki/Fixed-point_arithmetic) - Theory behind Q format and integer math.
- [Q (number format)](https://en.wikipedia.org/wiki/Q_(number_format)) - Understanding the Q16.16 format used in PixelRoot32.
- [Physics System Reference](PHYSICS_SYSTEM_REFERENCE.md) - Complete Flat Solver documentation
- [API Reference](API_REFERENCE.md) - CollisionSystem API
