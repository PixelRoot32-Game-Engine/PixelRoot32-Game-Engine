# Migration Guide: v0.8.1-dev → v0.9.0-dev

## Overview

This guide documents the changes required to migrate examples from version 0.8.1-dev to 0.9.0-dev. The main migration involves changing the C++ standard from C++11 to C++17 and adopting smart pointers (`std::unique_ptr`) for entity memory management.

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

Add in all files using smart pointers:

```cpp
#include <memory>
```

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

---

## Modified Files

### MenuScene.cpp / MenuScene.h

- All UI pointers (`UILabel*`, `UIButton*`, `UIVerticalLayout*`) converted to `std::unique_ptr`
- Methods `setupMainMenu()`, `setupGamesMenu()`, etc., updated to use `std::make_unique`

### CameraDemoScene.cpp / CameraDemoScene.h

- `PlayerCube* gPlayer` → `std::unique_ptr<PlayerCube> player`
- Removed global pointer `gPlayer`, now a class member

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

---

## References

- [C++ Core Guidelines - Smart Pointers](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-resource)
- [PlatformIO Build Flags](https://docs.platformio.org/en/latest/projectconf/sections/env/options/build/build_flags.html)
