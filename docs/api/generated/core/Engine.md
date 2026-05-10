# Engine

<Badge type="info" text="Class" />

**Source:** `Engine.h`

## Description

The main engine class that manages the game loop and core subsystems.

Engine acts as the central hub of the game engine. It initializes and manages
the Renderer, InputManager, AudioEngine, SceneManager, and optionally Touch system.
It runs the main game loop, handling timing (delta time), updating the current 
scene, and rendering frames.

## Touch Pipeline (PIXELROOT32_ENABLE_TOUCH)

When touch is enabled (default), Engine automatically:
  1. Processes mouse events (Native) or receives injected touch points (ESP32)
  2. Applies gesture detection (Click, DoubleClick, LongPress, Drag)
  3. Sends events to the current scene via Scene::processTouchEvents()

The order inside Scene::processTouchEvents is guaranteed:
  UIManager::processEvents (marks consumed) → onUnconsumedTouchEvent (virtual).

Set PIXELROOT32_ENABLE_TOUCH=0 in platform defines to disable (saves ~200 bytes).

## Methods

### `void init()`

**Description:**

Initializes the engine subsystems.

### `void run()`

**Description:**

Starts the main game loop.

### `unsigned long getDeltaTime() const`

**Description:**

Gets the time elapsed since the last frame.

**Returns:** The delta time in milliseconds.

### `unsigned long getMillis() const`

**Description:**

Gets the number of milliseconds since the engine started.

**Returns:** The time in milliseconds.

### `void setScene(Scene* newScene)`

**Description:**

Sets the current active scene.

**Parameters:**

- `newScene`: Pointer to the new Scene to become active.

### `std::optional<Scene*> getCurrentScene() const`

**Description:**

Retrieves the currently active scene.

**Returns:** Optional pointer to the current Scene, or nullopt if none is set.

### `void setRenderer(pixelroot32::graphics::Renderer&& newRenderer)`

**Description:**

Replaces the current renderer instance.

**Parameters:**

- `newRenderer`: R-value reference to the new Renderer to use.

### `pixelroot32::graphics::Renderer& getRenderer()`

**Description:**

Provides access to the Renderer subsystem.

**Returns:** Reference to the current Renderer.

### `pixelroot32::input::InputManager& getInputManager()`

**Description:**

Provides access to the InputManager subsystem.

**Returns:** Reference to the InputManager    .

### `pixelroot32::input::TouchEventDispatcher& getTouchDispatcher()`

**Description:**

Provides access to the touch event system.

**Returns:** Reference to the TouchEventDispatcher.

Use this to inject touch points on ESP32 (via TouchManager):
  engine.getTouchDispatcher().processTouch(id, pressed, x, y, timestamp);

### `bool hasTouchEvents() const`

**Description:**

Check if there are pending touch events.

**Returns:** true if there are events in the queue.

### `void setTouchManager(pixelroot32::input::TouchManager* touchManager)`

**Description:**

Set the TouchManager for automatic touch processing.

**Parameters:**

- `touchManager`: Pointer to the TouchManager instance.

On ESP32, call this once in setup() after touchManager.init():
  touchManager.init();
  engine.setTouchManager(&touchManager);

Then in loop(), just call engine.run() - Engine handles:
  - Polling touchManager.getTouchPoints() each frame
  - Detecting touch release (when count goes from >0 to 0)
  - Sending gesture events to the current scene

This eliminates the need to manually inject touch points or track release state.

### `void connectInputToDrawer()`

**Description:**

Connect InputManager to Drawer for mouse-to-touch mapping.

**Parameters:**

- `inputManager`: Pointer to the InputManager.

Called automatically in init() for Native builds.

### `graphics::ui::UIManager& getUIManager()`

**Description:**

Provides access to the UI system via the current scene.

**Returns:** Reference to the current scene's UIManager.

::: tip
Asserts if no scene is currently active.
:::

### `pixelroot32::audio::AudioEngine& getAudioEngine()`

**Description:**

Provides access to the AudioEngine subsystem.

**Returns:** Reference to the AudioEngine.

### `pixelroot32::audio::MusicPlayer& getMusicPlayer()`

**Description:**

Provides access to the MusicPlayer subsystem.

**Returns:** Reference to the MusicPlayer.

### `const PlatformCapabilities& getPlatformCapabilities() const`

**Description:**

Gets the capabilities of the current hardware platform.

**Returns:** Reference to the PlatformCapabilities.

### `void update()`

**Description:**

Updates the game logic.

### `void draw()`

**Description:**

Renders the current frame.

### `void drawDebugOverlay(pixelroot32::graphics::Renderer& r)`

**Description:**

Draws a debug overlay with real-time engine metrics.
Shows FPS, CPU usage (estimated), and RAM usage.
