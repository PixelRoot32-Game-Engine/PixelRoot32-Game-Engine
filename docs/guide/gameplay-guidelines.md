# Gameplay Guidelines - PixelRoot32

Patterns and best practices for game feel, mechanics, and common implementations.

---

## 🎮 Core Principles

### Frame-Rate Independence

Always multiply movement by `deltaTime`:

```cpp
// ✅ Good: Frame-rate independent
void update(unsigned long dt) {
    x += speed * math::toScalar(dt * 0.001f);
}

// ❌ Bad: Frame-rate dependent (different speeds at different FPS)
void update(unsigned long dt) {
    x += speed;  // Bug!
}
```

### Logic/Visual Decoupling

For infinite runners and auto-scrollers:

- **Logic progression** (obstacle spacing, spawn timing): Constant in real time
- **Visual speed**: Can increase for difficulty without affecting game logic

```cpp
// ✅ Decoupled: Logic constant, visual varies
void update(unsigned long dt) {
    // Game logic: constant spacing
    spawnTimer += dt;
    if (spawnTimer > SPAWN_INTERVAL) {
        spawnObstacle();
        spawnTimer = 0;
    }
    
    // Visual: can speed up for effect
    scrollX += visualSpeed * dt;
}
```

---

## 🕹️ Game Feel

### Snappy Controls

For fast-paced games, prefer higher values to reduce "floatiness":

```cpp
// ❌ Floaty
constexpr Scalar GRAVITY = math::toScalar(0.3f);
constexpr Scalar JUMP_FORCE = math::toScalar(8.0f);

// ✅ Snappy
constexpr Scalar GRAVITY = math::toScalar(0.6f);
constexpr Scalar JUMP_FORCE = math::toScalar(12.0f);
```

### Slopes & Ramps on Tilemaps

Treat contiguous ramp tiles as a single logical slope:

```cpp
// ✅ Linear interpolation over world X
Scalar getRampHeight(int worldX) {
    // Ramp from y=80 to y=48 across 4 tiles (64 pixels)
    Scalar t = math::toScalar((worldX - rampStartX) / 64.0f);
    return math::lerp(math::toScalar(80.0f), math::toScalar(48.0f), t);
}
```

Keep gravity and jump parameters identical between flat ground and ramps for consistent jump timing.

---

## 🏗️ Architecture Patterns

### Tuning Constants

Extract gameplay values to a dedicated header:

```cpp
// GameConstants.h
namespace GameConstants {
    constexpr Scalar PLAYER_SPEED = math::toScalar(120.0f);  // px/sec
    constexpr Scalar GRAVITY = math::toScalar(0.6f);
    constexpr Scalar JUMP_FORCE = math::toScalar(12.0f);
    constexpr int MAX_BULLETS = 50;
}
```

Benefits:
- Designers can tweak without touching logic
- Single source of truth
- Easy balance testing

### State Management with `reset()`

Reuse actors across game sessions instead of destroying/recreating:

```cpp
class PlayerActor : public PhysicsActor {
public:
    void reset(Vector2 startPos) {
        position = startPos;
        velocity = Vector2::zero();
        health = MAX_HEALTH;
        isActive = true;
    }
};

// In scene
void onGameOver() {
    player->reset(START_POSITION);  // ✅ Reuse
    // NOT: player = new PlayerActor();  // ❌ Allocates
}
```

### Component Pattern

| Actor Type | Use For | Example |
|------------|---------|---------|
| `Actor` | Static objects | Walls, platforms |
| `PhysicsActor` | Moving objects | Player, enemies |
| `KinematicActor` | Controlled movement | Player with input |
| `SensorActor` | Triggers | Goal zones, hazards |

---

## 🐛 Anti-Patterns (Common Mistakes)

### 1. No Delta Time

```cpp
// ❌ WRONG: Different behavior at different FPS
void update(unsigned long dt) {
    x += speed;
    y += velocity.y;
}

// ✅ CORRECT: Consistent regardless of FPS
void update(unsigned long dt) {
    Scalar dtSec = math::toScalar(dt * 0.001f);
    x += speed * dtSec;
    y += velocity.y * dtSec;
}
```

### 2. Logic in draw()

```cpp
// ❌ WRONG: Game logic in render
void draw(Renderer& r) {
    if (player->x > 100) {  // Logic!
        spawnEnemy();
    }
    player->draw(r);
}

// ✅ CORRECT: Logic in update, render in draw
void update(unsigned long dt) {
    if (player->position.x > ENEMY_SPAWN_X) {
        spawnEnemy();
    }
}

void draw(Renderer& r) {
    player->draw(r);  // Pure rendering
}
```

### 3. Runtime Allocation in Game Loop

```cpp
// ❌ WRONG: Allocates every frame
void update(unsigned long dt) {
    if (shootPressed) {
        auto bullet = std::make_unique<Bullet>(x, y);  // BAD!
        scene.addEntity(bullet.get());
    }
}

// ✅ CORRECT: Pool pattern
class BulletPool {
    std::array<Bullet, MAX_BULLETS> bullets;
    std::bitset<MAX_BULLETS> active;
    
public:
    void spawn(Vector2 pos) {
        for (size_t i = 0; i < MAX_BULLETS; ++i) {
            if (!active[i]) {
                active[i] = true;
                bullets[i].reset(pos);
                return;
            }
        }
    }
};
```

### 4. std::rand() in Hot Paths

```cpp
// ❌ WRONG: Slow, uses division
void update(unsigned long dt) {
    if (std::rand() % 100 < 5) {  // Expensive!
        spawnParticle();
    }
}

// ✅ CORRECT: Fast Xorshift
void update(unsigned long dt) {
    if (math::randomRange(0, 100) < 5) {  // Optimized
        spawnParticle();
    }
}
```

### 5. Magic Numbers

```cpp
// ❌ WRONG: What do these mean?
if (player.y > 200) { ... }
if (enemy.hp < 25) { ... }

// ✅ CORRECT: Named constants
constexpr Scalar GROUND_Y = math::toScalar(200.0f);
constexpr int CRITICAL_HEALTH = 25;

if (player.position.y > GROUND_Y) { ... }
if (enemy.health < CRITICAL_HEALTH) { ... }
```

### 6. Using std::vector in Game Loop

```cpp
// ❌ WRONG: Potential reallocation
void update(unsigned long dt) {
    enemies.push_back(new Enemy());  // May allocate!
}

// ✅ CORRECT: Fixed-size pool
std::array<Enemy, MAX_ENEMIES> enemies;
std::bitset<MAX_ENEMIES> enemyActive;

void spawnEnemy() {
    for (size_t i = 0; i < MAX_ENEMIES; ++i) {
        if (!enemyActive[i]) {
            enemyActive[i] = true;
            enemies[i].reset();
            return;
        }
    }
}
```

---

## 📚 Related Documentation

| Document | Topic |
|----------|-------|
| [Coding Style](coding-style.md) | C++ conventions |
| [Memory](memory.md) | Pool patterns, allocation |
| [UI Guidelines](ui-guidelines.md) | UI layouts, HUDs |
| [Performance](performance/esp32-performance.md) | Hot paths, optimization |

---

*Good games feel responsive, consistent, and intentional.*
