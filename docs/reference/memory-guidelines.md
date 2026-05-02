# Memory Guidelines - PixelRoot32

Practical memory management patterns for embedded game development.

**For complete C++17 memory guide:** [memory-system.md](../architecture/memory-system.md)

---

## 🎯 Golden Rule

> **Zero allocation in the game loop**

Pre-allocate during `init()`. Pool and recycle during gameplay. Never `new` or `malloc` in `update()` or `draw()`.

---

## 💾 Smart Pointers (C++17)

### Init-Time Ownership

Use `std::unique_ptr` for objects created during initialization:

```cpp
class GameScene : public Scene {
    std::unique_ptr<PlayerActor> player;
    std::vector<std::unique_ptr<EnemyActor>> enemies;
    
public:
    void init() override {
        // ✅ Create during init
        player = std::make_unique<PlayerActor>(100, 100);
        addEntity(player.get());  // Non-owning access
        
        for (int i = 0; i < 10; ++i) {
            auto enemy = std::make_unique<EnemyActor>(x, y);
            addEntity(enemy.get());
            enemies.push_back(std::move(enemy));
        }
    }
};
```

**⚠️ Not for hot paths:** `unique_ptr` is for **init-time only**, not `update()`/`draw()`.

---

## 🔄 Object Pooling

### Fixed-Size Pool Pattern

```cpp
class BulletPool {
    static constexpr size_t MAX_BULLETS = 50;
    std::array<Bullet, MAX_BULLETS> pool;
    std::bitset<MAX_BULLETS> active;
    
public:
    void init() {
        // Pre-initialize all bullets
        for (auto& bullet : pool) {
            bullet.setEnabled(false);
        }
    }
    
    Bullet* spawn(Vector2 pos, Vector2 vel) {
        for (size_t i = 0; i < MAX_BULLETS; ++i) {
            if (!active[i]) {
                active[i] = true;
                pool[i].reset(pos, vel);
                pool[i].setEnabled(true);
                return &pool[i];
            }
        }
        return nullptr;  // Pool exhausted
    }
    
    void despawn(Bullet* bullet) {
        for (size_t i = 0; i < MAX_BULLETS; ++i) {
            if (active[i] && &pool[i] == bullet) {
                active[i] = false;
                pool[i].setEnabled(false);
                return;
            }
        }
    }
};
```

### Pool vs unique_ptr

| Pattern | Use For | When |
|---------|---------|------|
| `unique_ptr` | Scene ownership | `init()` |
| Object pool | High-rotation entities | `update()` |
| Fixed array | Static entities | Compile-time |

---

## 🧱 Fixed Arrays Over Vectors

### ❌ Vector (May Reallocate)

```cpp
// ❌ Dangerous: May allocate in game loop
std::vector<Particle> particles;

void update(unsigned long dt) {
    particles.push_back(newParticle);  // May reallocate!
}
```

### ✅ Fixed Array (Safe)

```cpp
// ✅ Safe: Fixed size, no allocation
std::array<Particle, MAX_PARTICLES> particles;
std::bitset<MAX_PARTICLES> particleActive;

void spawnParticle(Vector2 pos) {
    for (size_t i = 0; i < MAX_PARTICLES; ++i) {
        if (!particleActive[i]) {
            particleActive[i] = true;
            particles[i].reset(pos);
            return;
        }
    }
}
```

---

## 📝 String Handling

### Avoid std::string Copies

```cpp
// ❌ Bad: Copies string
void setName(std::string name);  // Copy!

// ✅ Good: No copy
void setName(std::string_view name);  // View

// ✅ Good: Stack buffer
void logStatus(int score) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Score: %d", score);
    log(buffer);
}
```

---

## 🏗️ Scene Arenas

For temporary allocations during a scene:

```cpp
// Enable in platformio.ini
-DPIXELROOT32_ENABLE_SCENE_ARENA=1

// Usage
void MyScene::init() {
    arena.allocate(4096);  // 4KB scratch space
}

void* MyScene::getScratchBuffer(size_t size) {
    return arena.allocate(size);  // No malloc!
}
```

**Trade-off:** Arena cannot grow. Plan worst-case size.

---

## ⚡ Memory Monitoring (ESP32)

```cpp
void checkMemory() {
    #ifndef PLATFORM_NATIVE
        uint32_t free = ESP.getFreeHeap();
        uint32_t minFree = ESP.getMinFreeHeap();
        
        log("Heap: %u bytes free (min: %u)", free, minFree);
        
        if (free < 20480) {  // 20KB threshold
            log(LogLevel::Warning, "Low memory!");
        }
    #endif
}
```

---

## 📚 Related Documentation

| Document | Topic |
|----------|-------|
| [architecture/memory-system.md](../architecture/memory-system.md) | Complete C++17 memory guide |
| [performance/esp32-performance.md](./performance/esp32-performance.md) | Hot paths, optimization |
| [gameplay-guidelines.md](../guide/gameplay-guidelines.md) | Pool patterns, anti-patterns |

---

*Memory is a constraint, not a limitation. Design around it.*
