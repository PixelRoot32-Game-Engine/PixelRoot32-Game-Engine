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

### `* Use this to inject touch points on ESP32(via TouchManager)`

### `bool hasTouchEvents() const`

**Description:**

Check if there are pending touch events.

**Returns:** true if there are events in the queue.

### `* On ESP32, call this once in setup() after touchManager.init()`

### `void connectInputToDrawer()`

**Description:**

Connect InputManager to Drawer for mouse-to-touch mapping.

**Parameters:**

- `inputManager`: Pointer to the InputManager.

Called automatically in init() for Native builds.

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
