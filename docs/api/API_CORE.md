# API Reference: Core Module

This document covers the core engine classes, entity system, and scene management in PixelRoot32.

> **Note:** This is part of the [API Reference](../API_REFERENCE.md). See the main index for complete documentation.

---

## Engine

**Inherits:** None

The main engine class that manages the game loop and core subsystems. `Engine` acts as the central hub, initializing and managing the Renderer, InputManager, and SceneManager. It runs the main game loop, handling timing (delta time), updating the current scene, and rendering frames.

### Public Methods

- **`Engine(DisplayConfig&& displayConfig, const InputConfig& inputConfig, const AudioConfig& audioConfig)`**
    Constructs the engine with custom display, input and audio configurations.

- **`Engine(DisplayConfig&& displayConfig, const InputConfig& inputConfig)`**
    Constructs the engine with custom display and input configurations.

- **`Engine(DisplayConfig&& displayConfig)`**
    Constructs the engine with custom display configuration and default input settings.

- **`void init()`**
    Initializes the engine subsystems. Must be called before `run()`.

- **`void run()`**
    Starts the main game loop. This method contains the infinite loop that calls `update()` and `draw()` repeatedly.

- **`unsigned long getDeltaTime() const`**
    Returns the time elapsed since the last frame in milliseconds.

- **`unsigned long getMillis() const`**
    Returns the number of milliseconds since the engine started.

- **`void setScene(Scene* newScene)`**
    Sets the current active scene.

- **`std::optional<Scene*> getCurrentScene() const`**
    Retrieves the currently active scene, or std::nullopt if no scene is active.

- **`Renderer& getRenderer()`**
    Provides access to the Renderer subsystem.

- **`void setRenderer(pixelroot32::graphics::Renderer&& newRenderer)`**
    Replaces the current renderer instance.

- **`InputManager& getInputManager()`**
    Provides access to the InputManager subsystem.

- **`TouchEventDispatcher& getTouchDispatcher()`**
    Provides access to the touch system for injecting touch points. Use this on ESP32 to connect TouchManager with Engine's touch processing pipeline.
  - **Note**: Only available if `PIXELROOT32_ENABLE_TOUCH=1`

- **`bool hasTouchEvents() const`**
    Returns true if there are pending touch events in the queue.
  - **Note**: Only available if `PIXELROOT32_ENABLE_TOUCH=1`

- **`void setTouchManager(pixelroot32::input::TouchManager* touchManager)`**
    Registers a TouchManager instance for automatic touch processing on ESP32. When set, Engine automatically:
    - Polls `touchManager.getTouchPoints()` each frame
    - Detects touch releases (when count goes from >0 to 0)
    - Processes touch events through the internal TouchEventDispatcher
    - Sends gesture events to the current scene via `Scene::processTouchEvents()`

    Usage (ESP32):

    ```cpp
    touchManager.init();
    engine.setTouchManager(&touchManager);  // 1 línea
    
    void loop() {
        touchManager.update(frameDt);
        engine.run();  // Engine maneja todo automáticamente
    }
    ```

  - **Note**: Only available if `PIXELROOT32_ENABLE_TOUCH=1`

- **`AudioEngine& getAudioEngine()`**
    Provides access to the AudioEngine subsystem.
  - **Note**: Only available if `PIXELROOT32_ENABLE_AUDIO=1`

- **`MusicPlayer& getMusicPlayer()`**
    Provides access to the MusicPlayer subsystem.
  - **Note**: Only available if `PIXELROOT32_ENABLE_AUDIO=1`

- **`const PlatformCapabilities& getPlatformCapabilities() const`**
    Returns the detected hardware capabilities for the current platform.

---

## PlatformCapabilities (Struct)

**Namespace:** `pixelroot32::platforms`

A structure that holds detected hardware capabilities, used to optimize task pinning and threading.

- **`bool hasDualCore`**: True if the hardware has more than one CPU core.
- **`int coreCount`**: Total number of CPU cores detected.
- **`int audioCoreId`**: Recommended CPU core for audio tasks.
- **`int mainCoreId`**: Recommended CPU core for the main game loop.
- **`int audioPriority`**: Recommended priority for audio tasks.

### Static Methods

- **`static PlatformCapabilities detect()`**: Automatically detects hardware capabilities based on the platform and configuration. It respects the defaults defined in `platforms/PlatformDefaults.h` and any compile-time overrides.

---

## DisplayConfig (Struct)

Configuration settings for display initialization and scaling.

- **`DisplayType type`**: Type of display (ST7789, ST7735, OLED_SSD1306, OLED_SH1106, NONE, CUSTOM).
- **`int rotation`**: Display rotation (0-3 or degrees).
- **`uint16_t physicalWidth`**: Actual hardware width.
- **`uint16_t physicalHeight`**: Actual hardware height.
- **`uint16_t logicalWidth`**: Virtual rendering width.
- **`uint16_t logicalHeight`**: Virtual rendering height.
- **`int xOffset`**: X coordinate offset for hardware alignment.
- **`int yOffset`**: Y coordinate offset for hardware alignment.

### Pin Configuration (Optional)

- **`uint8_t clockPin`**: SPI SCK / I2C SCL.
- **`uint8_t dataPin`**: SPI MOSI / I2C SDA.
- **`uint8_t csPin`**: SPI CS (Chip Select).
- **`uint8_t dcPin`**: SPI DC (Data/Command).
- **`uint8_t resetPin`**: Reset pin.
- **`bool useHardwareI2C`**: If true, uses hardware I2C peripheral (default true).

---

## Optional: Debug Statistics Overlay (build flag)

When the engine is built with the preprocessor define **`PIXELROOT32_ENABLE_DEBUG_OVERLAY`**, the engine draws a technical overlay with real-time metrics.

- **Metrics Included**:
  - **FPS**: Frames per second (green).
  - **RAM**: Memory used in KB (cyan). ESP32 specific.
  - **CPU**: Estimated processor load percentage based on frame processing time (yellow).
- **Behavior**: The metrics are drawn in the top-right area of the screen, fixed and independent of the camera.
- **Performance**: Values are recalculated and formatted only every **16 frames** (`DEBUG_UPDATE_INTERVAL`); the cached strings are drawn every frame. This ensures minimal overhead while providing useful development data.
- **Usage**: Add to your build flags, e.g. in `platformio.ini`:  
  `build_flags = -D PIXELROOT32_ENABLE_DEBUG_OVERLAY`  
  This flag is also available in `EngineConfig.h`.
- **Internal**: Implemented by the private method `Engine::drawDebugOverlay(Renderer& r)`.

---

## Entity

**Inherits:** None

Abstract base class for all game objects. Entities are the fundamental building blocks of the scene. They have a position, size, and lifecycle methods (`update`, `draw`).

### Properties

- **`Scalar x, y`**: Position in world space.
- **`int width, height`**: Dimensions of the entity.
- **`bool isVisible`**: If false, the entity is skipped during rendering.
- **`bool isEnabled`**: If false, the entity is skipped during updates.
- **`unsigned char renderLayer`**: Logical render layer for this entity (0 = background, 1 = gameplay, 2 = UI).

### Public Methods

- **`Entity(Scalar x, Scalar y, int w, int h, EntityType t)`**
    Constructs a new Entity.

- **`void setVisible(bool v)`**
    Sets the visibility of the entity.

- **`void setEnabled(bool e)`**
    Sets the enabled state of the entity.

- **`unsigned char getRenderLayer() const`**
    Returns the current render layer.

- **`void setRenderLayer(unsigned char layer)`**
    Sets the logical render layer for this entity. The value is clamped to the range `[0, MAX_LAYERS - 1]`.

- **`virtual void update(unsigned long deltaTime)`**
    Updates the entity's logic. Must be overridden by subclasses.

- **`virtual void draw(Renderer& renderer)`**
    Renders the entity. Must be overridden by subclasses.

### Modular Compilation Notes

The Entity class is always available. However, specialized subclasses may be affected by modular compilation flags:

- **UI Elements** (UIButton, UILabel, etc.): Only available if `PIXELROOT32_ENABLE_UI_SYSTEM=1`
- **Physics Actors** (PhysicsActor, RigidActor, etc.): Only available if `PIXELROOT32_ENABLE_PHYSICS=1`
- **Particle Systems**: Only available if `PIXELROOT32_ENABLE_PARTICLES=1`

---

## Scene

**Inherits:** None

Represents a game level or screen containing entities. A Scene manages a collection of Entities and a CollisionSystem. It is responsible for updating and drawing all entities it contains.

### Public Methods

- **`virtual void init()`**
    Called when the scene is initialized or entered.

- **`virtual void update(unsigned long deltaTime)`**
    Updates all entities in the scene and runs the physics pipeline. Touch **`UITouchElement`** instances are updated here only when they are registered as **entities** (e.g. via a **`UILayout`** with **`addEntity`**); **`UIManager::update`** is a no-op.

- **`virtual void draw(Renderer& renderer)`**
    Draws all visible entities in the scene, iterating them by logical render layers (0 = background, 1 = gameplay, 2 = UI). Touch widgets draw through this path as entities; **`UIManager::draw`** is a no-op.

- **`virtual void processTouchEvents(TouchEvent* events, uint8_t count)`**
    Runs the central touch pipeline for one frame: if `PIXELROOT32_ENABLE_UI_SYSTEM`, **`UIManager::processEvents`** runs first and may mark events consumed; then **`onUnconsumedTouchEvent`** is called for each unconsumed event. Override in a subclass only if you need preprocessing before the base implementation; otherwise override **`onUnconsumedTouchEvent`** for gameplay.

- **`virtual void onUnconsumedTouchEvent(const TouchEvent& event)`**
    Hook for touch not handled by UI. Default is no-op. Typical use: forward to **`ActorTouchController::handleTouch`** or custom gestures.

- **`void addEntity(Entity* entity)`**
    Adds an entity to the scene.
    > **Note:** The scene does **not** take ownership of the entity. You must ensure the entity remains valid as long as it is in the scene (typically by holding it in a `std::unique_ptr` within your Scene class).

- **`void removeEntity(Entity* entity)`**
    Removes a specific entity from the scene (and from the collision system when physics is enabled).

- **`void clearEntities()`**
    Removes all entities from the scene. Does not delete the entity objects.

### Overriding scene limits (MAX_LAYERS / MAX_ENTITIES)

The engine defines default limits in `platforms/EngineConfig.h`: `MAX_LAYERS` (default 3) and `MAX_ENTITIES` (default 32). These are guarded with `#ifndef`, so you can override them from your project without modifying the engine.

> **Note:** The default of 3 for `MAX_LAYERS` is due to ESP32 platform constraints (memory and draw-loop cost). On native/PC you can safely use a higher value; on ESP32, increasing it may affect performance or memory.

**Compiler flags (recommended)**

In your project (e.g. in `platformio.ini`), add the defines to `build_flags` for the environment you use:

```ini
build_flags =
    -DMAX_LAYERS=5
    -DMAX_ENTITIES=64
```

The compiler defines `MAX_LAYERS` and `MAX_ENTITIES` before processing any `.cpp` file. Because `Scene.h` uses `#ifndef MAX_LAYERS` / `#ifndef MAX_ENTITIES`, it will not redefine them and your values will be used. This affects how many render layers are drawn (see `Scene::draw`) and, on Arduino, the capacity of the scene entity queue when constructed with `MAX_ENTITIES`.

---

## SceneManager

**Inherits:** None

Manages the stack of active scenes. Allows for scene transitions (replacing) and stacking (push/pop), useful for pausing or menus.

### Public Methods

- **`void setCurrentScene(Scene* newScene)`**
    Replaces the current scene with a new one.

- **`void pushScene(Scene* newScene)`**
    Pushes a new scene onto the stack, pausing the previous one.

- **`void popScene()`**
    Removes the top scene from the stack, resuming the previous one.

- **`std::optional<Scene*> getCurrentScene() const`**
    Gets the currently active scene, or std::nullopt if no scene is active.

---

## SceneArena (Memory Management)

**Include:** `core/Scene.h`

**Namespace:** `pixelroot32::core`

**Inherits:** None

An optional memory arena for zero-allocation scenes. This feature is enabled via the `PIXELROOT32_ENABLE_SCENE_ARENA` macro. It allows scenes to pre-allocate a fixed memory block for temporary data or entity storage, avoiding heap fragmentation on embedded devices.

On ESP32, the main trade-off is:

- **Benefits**: predictable memory usage, no `new`/`delete` in the scene, reduced fragmentation.
- **Costs**: the buffer size is fixed (over-allocating wastes RAM, under-allocating returns `nullptr`), and all allocations are freed only when the arena is reset or the scene ends.

### Public Methods

- **`void init(void* memory, std::size_t size)`**
    Initializes the arena with a pre-allocated memory buffer.

- **`void reset()`**
    Resets the allocation offset to zero. This "frees" all memory in the arena instantly.

- **`void* allocate(std::size_t size, std::size_t alignment)`**
    Allocates a block of memory from the arena. Returns `nullptr` if the arena is full.

---

## Related Documentation

- [API Reference](API_REFERENCE.md) - Main index
- [API Physics](API_PHYSICS.md) - Physics actors and collision system
- [API Graphics](API_GRAPHICS.md) - Rendering and sprites
- [API UI](API_UI.md) - User interface system