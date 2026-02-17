# Migration Guide v0.9.0-dev

This document explains how to migrate projects based on PixelRoot32 Game Engine from version 0.8.1-dev to 0.9.0-dev. The update adopts C++17 and introduces explicit entity ownership using smart pointers.

## 1. Project Configuration (platformio.ini)

The PlatformIO configuration has been updated to support C++17 and performance-oriented flags.

### Key Changes

* C++ standard updated from C++11 to C++17.
* Added flags to disable exceptions and optimize performance.
* Explicit test directory configured.

### Updated Configuration

Update your `platformio.ini` with the following:

```ini
[platformio]
test_dir = lib/PixelRoot32-Game-Engine/test

[env:...]
; ... other settings ...
build_unflags = -std=gnu++11
build_flags = 
    -std=gnu++17
    -fno-exceptions
    -O2
    -Isrc
    ; ... rest of flags ...
```

#### Environment-specific changes observed in the branch diff

* esp32c3 ([platformio.ini](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Samples/blob/master/platformio.ini)):
  * Added `build_unflags = -std=gnu++11`
  * Switched `-std=c++11` to `-std=gnu++17`
  * Added `-fno-exceptions`
* esp32dev ([platformio.ini](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Samples/blob/master/platformio.ini)):
  * Added `build_unflags = -std=gnu++11`
  * Switched to `-std=gnu++17` and added `-fno-exceptions`
* native ([platformio.ini](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Samples/blob/master/platformio.ini)):
  * Switched to `-std=gnu++17` and added `-fno-exceptions`

## 2. Source Changes (src)

The main change is the adoption of smart pointers (`std::unique_ptr`) for entity management, replacing raw pointers (`new` / `delete`). Scenes now explicitly own their entities, improving memory safety and clarifying ownership.

### 2.1 Entity Ownership in Scenes

Previously, entities were created with `new` and registered in the engine, with implicit lifetime management. Now, the Scene must explicitly own its entities.

#### Previous Pattern (v0.8.1-dev)

```cpp
// header (.h)
PaddleActor* player;

// implementation (.cpp)
player = new PaddleActor(...);
addEntity(player);
```

#### New Pattern (v0.9.0-dev)

```cpp
// header (.h)
#include <memory>
std::unique_ptr<PaddleActor> player;

// implementation (.cpp)
player = std::make_unique<PaddleActor>(...);
addEntity(player.get());
```

### 2.2 Dynamic Entities (Owned Lists)

For entities that are not direct class members (e.g., backgrounds, particles, bullets), use a container to retain ownership.

#### In the header (.h)

```cpp
#include <vector>
#include <memory>

// Container for scene-owned entities
std::vector<std::unique_ptr<pixelroot32::core::Entity>> ownedEntities;
```

#### In the implementation (.cpp)

```cpp
// Creating a dynamic entity (e.g., background)
auto bg = std::make_unique<PongBackground>(...);
addEntity(bg.get());
ownedEntities.push_back(std::move(bg));
```

### 2.3 Resource Cleanup

Clear the owned container when resetting or destroying the scene:

```cpp
void MyScene::reset() {
    ownedEntities.clear();
    // ... re-initialization ...
}
```

### 2.4 Concrete changes seen in examples

* MenuScene ([MenuScene.cpp](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Samples/blob/master/src/Menu/MenuScene.cpp), [MenuScene.h](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Samples/blob/master/src/Menu/MenuScene.h)):
  * `UILabel`, `UIButton`, `UIVerticalLayout` instances now created with `std::make_unique`.
  * `addEntity(ptr.get())` used instead of passing raw `new` pointer.
  * Member fields updated to `std::unique_ptr<...>`.
* PongScene ([PongScene.cpp](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Samples/blob/master/src/examples/Games/Pong/PongScene.cpp), [PongScene.h](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Samples/blob/master/src/examples/Games/Pong/PongScene.h)):
  * Added `ownedEntities` to keep ownership of non-member entities (e.g., background).
  * `PaddleActor` and `BallActor` changed to `std::unique_ptr<...>`.
  * On `init`, `ownedEntities.clear()` ensures a clean start before pushing new owned objects.
* Other examples:
  * Similar refactors from raw pointers to `std::unique_ptr` and use of `.get()` when registering with the engine.
  * Add `#include <memory>` (and `<vector>` when using owned containers).

## Migration Checklist

1. Update `platformio.ini`: replace `-std=c++11` with `-std=gnu++17`, add `build_unflags = -std=gnu++11`, and `-fno-exceptions`.
2. Update scene headers:
   * Include `<memory>` and `<vector>` if needed.
   * Replace raw members (`Type* var`) with `std::unique_ptr<Type> var`.
   * Add `std::vector<std::unique_ptr<pixelroot32::core::Entity>> ownedEntities` if dynamic entities are used.
3. Update scene implementations:
   * Use `std::make_unique<Type>(...)` instead of `new`.
   * Call `addEntity(myEntity.get())` when registering with the engine.
   * Store dynamic entities into `ownedEntities` using `std::move()`.

## Before/After (PongScene)

Before:

```cpp
leftPaddle = new PaddleActor(...);
addEntity(leftPaddle);
```

After:

```cpp
leftPaddle = std::make_unique<PaddleActor>(...);
addEntity(leftPaddle.get());
```
