# Memory Management Guide - PixelRoot32 C++17

**Document Version:** 1.0  
**Last Updated:** February 2026  
**Engine Version:** v1.0.0

## Overview

This guide covers modern memory management practices in PixelRoot32 using C++17 features. The engine has transitioned from manual memory management to smart pointers and RAII (Resource Acquisition Is Initialization) patterns for improved safety and maintainability.

---

## Engine Memory Limits

Understanding the engine's memory limits is crucial for developing stable games on resource-constrained platforms.

### Hard Limits (Compile-Time Constants)

| Limit | Default Value | Configurable | Description |
|-------|--------------|--------------|-------------|
| **Max Entities** | 32 | ✅ via `MAX_ENTITIES` | Maximum entities per scene |
| **Max Layers** | 3 | ✅ via `MAX_LAYERS` | Maximum render layers (0=Bg, 1=Game, 2=UI) |
| **Max Physics Pairs** | 128 | ✅ via `PHYSICS_MAX_PAIRS` | Maximum collision pairs per frame |
| **Spatial Grid Cell Size** | 32px | ✅ via `SPATIAL_GRID_CELL_SIZE` | Size of uniform grid cells |
| **Max Entities Per Grid Cell** | 24 | ✅ via `SPATIAL_GRID_MAX_ENTITIES_PER_CELL` | Grid cell capacity |
| **Velocity Iterations** | 2 | ✅ via `PR32_VELOCITY_ITERATIONS` | Physics solver iterations |

### Memory Footprint by Resolution

| Resolution | Framebuffer | Scaling LUTs | Total (approx) |
|------------|-------------|--------------|----------------|
| **128x128** | ~16 KB | ~1 KB | **~17 KB** |
| **160x160** | ~25 KB | ~1.5 KB | **~26.5 KB** |
| **240x240** | ~57 KB | ~2 KB | **~59 KB** |

> **Note:** These values are for TFT (16-bit) displays. OLED displays use significantly less memory.

### Per-Entity Memory Costs

| Component | Memory Cost |
|-----------|-------------|
| **Base Entity** | ~32 bytes |
| **Actor** | ~64 bytes |
| **PhysicsActor** | ~128 bytes |
| **KinematicActor** | ~144 bytes |
| **Sprite (1bpp)** | (width * height / 8) bytes |
| **Sprite (2bpp)** | (width * height / 4) bytes |
| **Sprite (4bpp)** | (width * height / 2) bytes |

### Recommended Maximums for Stable Performance

| Platform | Entities | Dynamic Physics Objects | Sprites | Notes |
|----------|----------|------------------------|---------|-------|
| **ESP32 (classic)** | 32 | 16 | 64 | 520KB SRAM total |
| **ESP32-S3** | 48 | 24 | 96 | 512KB SRAM + PSRAM |
| **ESP32-C3** | 24 | 12 | 48 | 400KB SRAM, no FPU |

### Configuration Examples

**For maximum performance (128x128, low entity count):**

```cpp
// platformio.ini build_flags
-D LOGICAL_WIDTH=128
-D LOGICAL_HEIGHT=128
-D MAX_ENTITIES=24
-D PHYSICS_MAX_PAIRS=64
```

**For richer scenes (with PSRAM):**

```cpp
// platformio.ini build_flags
-D LOGICAL_WIDTH=240
-D LOGICAL_HEIGHT=240
-D MAX_ENTITIES=64
-D PHYSICS_MAX_PAIRS=256
```

### Runtime Memory Monitoring

```cpp
// In your scene or debug overlay
void debugMemory() {
    #ifndef PLATFORM_NATIVE
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t totalHeap = ESP.getHeapSize();
        uint32_t minFreeHeap = ESP.getMinFreeHeap();  // Since boot
        
        Serial.printf("Heap: %u/%u bytes free (min: %u)\n", 
                      freeHeap, totalHeap, minFreeHeap);
                      
        // Warn if below safety threshold (e.g., 20KB)
        if (freeHeap < 20480) {
            Serial.println("WARNING: Low memory!");
        }
    #endif
}
```

### Heap Fragmentation Warning

Long-running games may experience heap fragmentation. Symptoms:

- Gradual decrease in free heap despite stable entity count
- Sudden crashes when allocating new objects
- Performance degradation over time

**Mitigation strategies:**

1. Use **Object Pooling** for bullets/particles
2. Pre-allocate in `Scene::init()`, not during gameplay
3. Use **SceneArena** for temporary allocations
4. Avoid frequent `std::vector` reallocations (reserve capacity upfront)

---

## Key Changes in v0.9.0

The engine migrated from C++11 to C++17 and adopted modern memory management patterns:

- **Smart Pointers:** `std::unique_ptr` for exclusive ownership
- **RAII:** Automatic resource management
- **Zero Manual Delete:** No explicit `delete` calls needed
- **Move Semantics:** Efficient ownership transfer

---

## Smart Pointer Patterns

### Basic Usage

**Creating Objects:**

```cpp
// Modern approach (v0.9.0+)
auto player = std::make_unique<PlayerActor>(position, width, height);
auto bullet = std::make_unique<BulletActor>(x, y, velocity);

// Pass to scene (non-owning)
scene.addEntity(player.get());
scene.addEntity(bullet.get());
```

**Ownership Transfer:**

```cpp
// Transfer ownership to engine
auto customRenderer = std::make_unique<CustomRenderer>(config);
engine.setRenderer(std::move(customRenderer));

// Custom display driver
auto display = std::make_unique<CustomDisplay>(width, height);
DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(display.release(), width, height);
```

### Container Storage

**Vector of Game Objects:**

```cpp
class GameScene : public Scene {
private:
    std::vector<std::unique_ptr<EnemyActor>> enemies;
    std::vector<std::unique_ptr<Projectile>> projectiles;
    std::unique_ptr<PlayerActor> player;
    
public:
    void spawnEnemy(Vector2 position) {
        auto enemy = std::make_unique<EnemyActor>(position, 32, 32);
        enemies.push_back(std::move(enemy));
        addEntity(enemies.back().get());
    }
    
    void removeEnemy(EnemyActor* enemy) {
        // Find and remove from vector
        enemies.erase(
            std::remove_if(enemies.begin(), enemies.end(),
                [enemy](const std::unique_ptr<EnemyActor>& e) {
                    return e.get() == enemy;
                }
            ), enemies.end()
        );
        // Scene will handle entity removal
    }
};
```

---

## Object Pooling with Smart Pointers

### Fixed-Size Pool Pattern

**Modern Pool Implementation:**

```cpp
class BulletPool {
private:
    static constexpr size_t MAX_BULLETS = 50;
    std::array<std::unique_ptr<BulletActor>, MAX_BULLETS> pool;
    std::bitset<MAX_BULLETS> activeFlags;
    
public:
    void init() {
        // Pre-allocate all bullets
        for (size_t i = 0; i < MAX_BULLETS; ++i) {
            pool[i] = std::make_unique<BulletActor>(0, 0, 0, 0);
            pool[i]->setEnabled(false);
        }
    }
    
    BulletActor* spawn(Vector2 position, Vector2 velocity) {
        for (size_t i = 0; i < MAX_BULLETS; ++i) {
            if (!activeFlags[i]) {
                activeFlags[i] = true;
                pool[i]->reset(position, velocity); // Custom reset method
                pool[i]->setEnabled(true);
                return pool[i].get();
            }
        }
        return nullptr; // Pool exhausted
    }
    
    void despawn(BulletActor* bullet) {
        for (size_t i = 0; i < MAX_BULLETS; ++i) {
            if (activeFlags[i] && pool[i].get() == bullet) {
                activeFlags[i] = false;
                pool[i]->setEnabled(false);
                break;
            }
        }
    }
};
```

---

## RAII for Resources

### Audio Resource Management

```cpp
class AudioManager {
private:
    std::unique_ptr<AudioEngine> audioEngine;
    std::unique_ptr<MusicPlayer> musicPlayer;
    
public:
    AudioManager(const AudioConfig& config) {
        audioEngine = std::make_unique<AudioEngine>(config);
        musicPlayer = std::make_unique<MusicPlayer>();
    }
    
    ~AudioManager() {
        // Automatic cleanup - no manual delete needed
        // AudioEngine and MusicPlayer are automatically destroyed
    }
    
    void playSound(const AudioEvent& event) {
        audioEngine->playEvent(event);
    }
};
```

### Display Resource Management

```cpp
class DisplayManager {
private:
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<DrawSurface> surface;
    
public:
    DisplayManager(const DisplayConfig& config) {
        // Create custom surface
        surface = std::make_unique<CustomDrawSurface>(config.width, config.height);
        
        // Create renderer with surface
        renderer = std::make_unique<Renderer>(
            PIXELROOT32_CUSTOM_DISPLAY(surface.get(), config.width, config.height)
        );
        
        // Transfer ownership
        surface.release(); // Renderer now owns the surface
    }
};
```

---

## Memory Safety Patterns

### Avoiding Common Pitfalls

**❌ Don't: Mix raw pointers and smart pointers**

```cpp
// Bad - potential double delete
Actor* rawPtr = new Actor();
std::unique_ptr<Actor> smartPtr(rawPtr);
scene.addEntity(rawPtr); // Dangerous!
```

**✅ Do: Use .get() for non-owning access**

```cpp
auto actor = std::make_unique<Actor>();
scene.addEntity(actor.get()); // Safe - scene doesn't own
actors.push_back(std::move(actor)); // Transfer ownership
```

**❌ Don't: Use after move**

```cpp
auto actor = std::make_unique<Actor>();
scene.addEntity(std::move(actor));
actor->update(); // ❌ Undefined behavior - actor is nullptr
```

**✅ Do: Check before use after potential move**

```cpp
auto actor = std::make_unique<Actor>();
if (condition) {
    scene.addEntity(std::move(actor));
}
if (actor) { // ✅ Safe - check if still valid
    actor->update();
}
```

---

## Performance Considerations

### Move Semantics Efficiency

```cpp
class GameScene {
private:
    std::vector<std::unique_ptr<Actor>> entities;
    
public:
    // Efficient - uses move semantics
    void addEntity(std::unique_ptr<Actor> entity) {
        entities.push_back(std::move(entity));
    }
    
    // Even more efficient - perfect forwarding
    template<typename T, typename... Args>
    void createEntity(Args&&... args) {
        auto entity = std::make_unique<T>(std::forward<Args>(args)...);
        entities.push_back(std::move(entity));
        Scene::addEntity(entities.back().get());
    }
};
```

### Memory Fragmentation Prevention

- **Pre-allocation:** Create objects in `init()` or constructor
- **Fixed-size containers:** Use `std::array` instead of `std::vector` when size is known
- **Object pooling:** Reuse objects instead of creating/destroying
- **Move semantics:** Transfer ownership instead of copying

---

## Hardware-Specific Memory (ESP32)

When working with high-performance drivers (TFT, I2S), memory must be allocated with specific capabilities.

### DMA-Capable Memory

For SPI or I2S transfers to work without CPU intervention, the buffers must be in a specific region of SRAM.

```cpp
// Correct way to allocate a DMA buffer
uint16_t* dmaBuffer = (uint16_t*)heap_caps_malloc(
    bufferSize, 
    MALLOC_CAP_DMA | MALLOC_CAP_8BIT
);

// Always check for success
if (dmaBuffer == nullptr) {
    // Fallback or error
}

// Memory allocated with heap_caps_malloc must be freed with heap_caps_free
heap_caps_free(dmaBuffer);
```

### Memory-Performance Trade-offs (v1.0.0)

In v1.0.0, the `TFT_eSPI_Drawer` uses double-buffering for DMA. Increasing `LINES_PER_BLOCK` improves throughput but increases memory usage linearly:

- **Baseline**: 20 lines = ~10KB (at 240 width)
- **Optimized**: 60 lines = ~30KB
- **Max**: 120 lines = ~60KB (Half frame)

> [!IMPORTANT]
> Non-FPU platforms like ESP32-C3 have more limited SRAM. Be cautious when increasing DMA block sizes or logical resolutions.

---

## Migration from Manual Memory Management

### Before (C++11 Style)

```cpp
class OldGame {
private:
    Actor* player;
    std::vector<Actor*> enemies;
    
public:
    void init() {
        player = new PlayerActor(100, 100, 32, 32);
        for (int i = 0; i < 10; i++) {
            enemies.push_back(new EnemyActor(rand() % 200, rand() % 100, 16, 16));
        }
    }
    
    ~OldGame() {
        delete player;
        for (auto enemy : enemies) {
            delete enemy;
        }
    }
};
```

### After (C++17 Style)

```cpp
class NewGame {
private:
    std::unique_ptr<PlayerActor> player;
    std::vector<std::unique_ptr<EnemyActor>> enemies;
    
public:
    void init() {
        player = std::make_unique<PlayerActor>(100, 100, 32, 32);
        for (int i = 0; i < 10; i++) {
            auto enemy = std::make_unique<EnemyActor>(rand() % 200, rand() % 100, 16, 16);
            enemies.push_back(std::move(enemy));
        }
    }
    
    // ✅ No manual destructor needed!
};
```

---

## Debugging Memory Issues

### Common Tools

```cpp
// Track object creation/destruction
class DebugActor : public Actor {
    static int instanceCount;
public:
    DebugActor() { instanceCount++; }
    ~DebugActor() { instanceCount--; }
    static int getInstanceCount() { return instanceCount; }
};

// Use in Scene
void update() {
    Serial.print("Active actors: ");
    Serial.println(DebugActor::getInstanceCount());
}
```

### Memory Leak Detection

- **ESP32:** Use `ESP.getFreeHeap()` to monitor memory
- **Native:** Use Valgrind or AddressSanitizer
- **PlatformIO:** Enable memory checking in test builds

---

## Best Practices Summary

1. **Always use `std::make_unique`** for object creation
2. **Use `.get()`** for non-owning raw pointer access
3. **Use `std::move()`** for ownership transfer
4. **Pre-allocate** in constructors or `init()` methods
5. **Avoid manual `delete`** - let RAII handle cleanup
6. **Use object pooling** for frequently created/destroyed objects
7. **Check pointers** after potential move operations
8. **Monitor memory usage** on constrained platforms

---

## References

- **C++ Smart Pointers:** <https://en.cppreference.com/book/intro/smart_pointers>
- **RAII Pattern:** <https://en.cppreference.com/w/cpp/language/raii>
- **Move Semantics:** <https://en.cppreference.com/w/cpp/utility/move>
- **PixelRoot32 Migration Guide:** See `MIGRATION_v0.8.1_to_v0.9.0.md`
