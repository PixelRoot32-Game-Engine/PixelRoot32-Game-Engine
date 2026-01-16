# API Reference

This document provides a complete reference for the PixelRoot32 Game Engine public API.

## Global Configuration

The `Config.h` file contains global configuration constants for the engine.

### Constants

- **`DISPLAY_WIDTH`**
    The width of the display in pixels. Default is `240`.

- **`DISPLAY_HEIGHT`**
    The height of the display in pixels. Default is `240`.

## Core Module

The Core module provides the fundamental building blocks of the engine, including the main application loop, entity management, and scene organization.

### Engine

**Inherits:** None

The main engine class that manages the game loop and core subsystems. `Engine` acts as the central hub, initializing and managing the Renderer, InputManager, and SceneManager. It runs the main game loop, handling timing (delta time), updating the current scene, and rendering frames.

#### Public Methods

-   **`Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig, const AudioConfig& audioConfig)`**
    Constructs the engine with custom display, input and audio configurations.

-   **`Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig)`**
    Constructs the engine with custom display and input configurations.

-   **`Engine(const DisplayConfig& displayConfig)`**
    Constructs the engine with custom display configuration and default input settings.

- **`void init()`**
    Initializes the engine subsystems. Must be called before `run()`.

- **`void run()`**
    Starts the main game loop. This method contains the infinite loop that calls `update()` and `draw()` repeatedly.

- **`unsigned long getDeltaTime() const`**
    Returns the time elapsed since the last frame in milliseconds.

- **`void setScene(Scene* newScene)`**
    Sets the current active scene.

- **`Scene* getCurrentScene() const`**
    Retrieves the currently active scene.

- **`Renderer& getRenderer()`**
    Provides access to the Renderer subsystem.

- **`InputManager& getInputManager()`**
    Provides access to the InputManager subsystem.

- **`AudioEngine& getAudioEngine()`**
    Provides access to the AudioEngine subsystem.

---

## Audio Module

The Audio module provides a NES-like audio system with Pulse, Triangle, and Noise channels.

### AudioEngine

**Inherits:** None

The core class managing audio generation and playback.

#### Public Methods

- **`AudioEngine(const AudioConfig& config)`**
    Constructs the AudioEngine.

- **`void init()`**
    Initializes the audio backend.

- **`void update(unsigned long deltaTime)`**
    Updates audio logic (envelopes, sequencers).

- **`void generateSamples(int16_t* stream, int length)`**
    Fills the buffer with audio samples.

- **`void playEvent(const AudioEvent& event)`**
    Plays a one-shot audio event on the first available channel of the requested type.

### Data Structures

#### WaveType (Enum)
- `PULSE`: Square wave with variable duty cycle.
- `TRIANGLE`: Triangle wave (fixed volume/duty).
- `NOISE`: Pseudo-random noise.

#### AudioEvent (Struct)
Structure defining a sound effect to be played.
- **`WaveType type`**: Type of waveform to use.
- **`float frequency`**: Frequency in Hz.
- **`float duration`**: Duration in seconds.
- **`float volume`**: Volume level (0.0 to 1.0).
- **`float duty`**: Duty cycle for Pulse waves (0.0 to 1.0, typically 0.125, 0.25, 0.5, 0.75).

### AudioConfig

Configuration struct for the audio system.

- **`AudioBackend* backend`**: Pointer to the platform-specific audio backend (e.g., SDL2, I2S).
- **`int sampleRate`**: Audio sample rate in Hz (default: 22050).

---

### Entity

**Inherits:** None

Abstract base class for all game objects. Entities are the fundamental building blocks of the scene. They have a position, size, and lifecycle methods (`update`, `draw`).

#### Properties

- **`float x, y`**: Position in world space.
- **`int width, height`**: Dimensions of the entity.
- **`bool isVisible`**: If false, the entity is skipped during rendering.
- **`bool isEnabled`**: If false, the entity is skipped during updates.

#### Public Methods

- **`Entity(float x, float y, int w, int h, EntityType t)`**
    Constructs a new Entity.

- **`void setVisible(bool v)`**
    Sets the visibility of the entity.

- **`void setEnabled(bool e)`**
    Sets the enabled state of the entity.

- **`virtual void update(unsigned long deltaTime)`**
    Updates the entity's logic. Must be overridden by subclasses.

- **`virtual void draw(Renderer& renderer)`**
    Renders the entity. Must be overridden by subclasses.

---

### Actor

**Inherits:** [Entity](#entity)

An Entity capable of physical interaction and collision. Actors extend Entity with collision layers and masks. They are used for dynamic game objects like players, enemies, and projectiles.

#### Public Methods

- **`Actor(float x, float y, int w, int h)`**
    Constructs a new Actor.

- **`void setCollisionLayer(CollisionLayer l)`**
    Sets the collision layer this actor belongs to.

- **`void setCollisionMask(CollisionLayer m)`**
    Sets the collision layers this actor interacts with.

- **`bool isInLayer(uint16_t targetLayer) const`**
    Checks if the Actor belongs to a specific collision layer.

- **`virtual Rect getHitBox()`**
    Gets the hitbox for collision detection. Must be implemented by subclasses.

- **`virtual void onCollision(Actor* other)`**
    Callback invoked when a collision occurs with another Actor.

---

### Scene

**Inherits:** None

Represents a game level or screen containing entities. A Scene manages a collection of Entities and a CollisionSystem. It is responsible for updating and drawing all entities it contains.

#### Public Methods

- **`virtual void init()`**
    Called when the scene is initialized or entered.

- **`virtual void update(unsigned long deltaTime)`**
    Updates all entities in the scene and handles collisions.

- **`virtual void draw(Renderer& renderer)`**
    Draws all visible entities in the scene.

- **`void addEntity(Entity* entity)`**
    Adds an entity to the scene.

- **`void removeEntity(Entity* entity)`**
    Removes an entity from the scene.

- **`void clearEntities()`**
    Removes all entities from the scene.

---

### SceneManager

**Inherits:** None

Manages the stack of active scenes. Allows for scene transitions (replacing) and stacking (push/pop), useful for pausing or menus.

#### Public Methods

- **`void setCurrentScene(Scene* newScene)`**
    Replaces the current scene with a new one.

- **`void pushScene(Scene* newScene)`**
    Pushes a new scene onto the stack, pausing the previous one.

- **`void popScene()`**
    Removes the top scene from the stack, resuming the previous one.

- **`Scene* getCurrentScene() const`**
    Gets the currently active scene.

---

## Graphics Module

The Graphics module handles everything related to drawing to the screen, including text, shapes, bitmaps, and particle effects.

### Renderer

**Inherits:** None

High-level graphics rendering system. Provides a unified API for drawing shapes, text, and images, abstracting the underlying hardware implementation.

#### Public Methods

- **`void beginFrame()`**
    Prepares the buffer for a new frame (clears screen).

- **`void endFrame()`**
    Finalizes the frame and sends the buffer to the display.

- **`void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size)`**
    Draws a string of text.

- **`void drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size)`**
    Draws text centered horizontally at a given Y coordinate.

- **`void drawFilledCircle(int x, int y, int radius, uint16_t color)`**
    Draws a filled circle.

- **`void drawCircle(int x, int y, int radius, uint16_t color)`**
    Draws a circle outline.

- **`void drawRectangle(int x, int y, int width, int height, uint16_t color)`**
    Draws a rectangle outline.

- **`void drawFilledRectangle(int x, int y, int width, int height, uint16_t color)`**
    Draws a filled rectangle.

- **`void drawLine(int x1, int y1, int x2, int y2, uint16_t color)`**
    Draws a line between two points.

- **`void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color)`**
    Draws a bitmap image.

- **`void drawPixel(int x, int y, uint16_t color)`**
    Draws a single pixel.

- **`void setDisplaySize(int w, int h)`**
    Sets the logical display size.

- **`void setDisplayOffset(int x, int y)`**
    Sets a global offset for all drawing operations.

- **`void setContrast(uint8_t level)`**
    Sets the display contrast/brightness (0-255).

- **`void setFont(const uint8_t* font)`**
    Sets the font for text rendering.

---

### Color

**Inherits:** None

Enumeration of the 32 built-in colors available in the engine's palette.

#### Values

- `Black`, `White`, `LightGray`, `DarkGray`
- `Red`, `DarkRed`, `Green`, `DarkGreen`, `Blue`, `DarkBlue`
- `Yellow`, `Orange`, `Brown`
- `Purple`, `Pink`, `Cyan`
- `LightBlue`, `LightGreen`, `LightRed`
- `Navy`, `Teal`, `Olive`
- `Gold`, `Silver`
- `Transparent` (special value)
- `DebugRed`, `DebugGreen`, `DebugBlue`

#### Public Methods

- **`uint16_t resolveColor(Color color)`**
    Converts a `Color` enum value to its corresponding RGB565 `uint16_t` representation.

---

### DisplayConfig

**Inherits:** None

Configuration settings for initializing displays.

#### Properties

- **`rotation`**: Display rotation (0-3).
- **`width`**: Display width in pixels.
- **`height`**: Display height in pixels.
- **`xOffset, yOffset`**: Offsets for display rendering.

---

### DrawSurface

**Inherits:** None

Abstract interface for platform-specific drawing operations. Implementations of this class handle the low-level communication with the display hardware (or window system).

---

### UIElement

**Inherits:** [Entity](#entity)

Base class for all user interface elements.

---

### UIButton

**Inherits:** [UIElement](#uielement)

A clickable button UI element. Supports callback functions and state management (selected/pressed).

#### Public Methods

- **`UIButton(std::string t, uint8_t index, float x, float y, float w, float h, std::function<void()> callback)`**
    Constructs a button.

- **`void setStyle(uint16_t textCol, uint16_t bgCol, bool drawBg)`**
    Configures visual style.

- **`void setSelected(bool selected)`**
    Sets the selection state.

- **`void press()`**
    Manually triggers the button's action.

---

### UILabel

**Inherits:** [UIElement](#uielement)

A simple text label UI element.

#### Public Methods

- **`UILabel(std::string t, float x, float y, uint16_t col, uint8_t sz)`**
    Constructs a label.

- **`void setText(const std::string& t)`**
    Updates the label's text.

- **`void centerX(int screenWidth)`**
    Centers the label horizontally.

---

### ParticleEmitter

**Inherits:** [Entity](#entity)

Manages a pool of particles to create visual effects (fire, smoke, explosions).

#### Public Methods

- **`ParticleEmitter(float x, float y, const ParticleConfig& cfg)`**
    Constructs a new emitter with specific configuration.

- **`void burst(float x, float y, int count)`**
    Emits a burst of particles from a specific location.

---

### ParticleConfig

**Inherits:** None

Configuration parameters for a particle emitter.

#### Properties

- **`startColor, endColor`**: Life cycle colors.
- **`minSpeed, maxSpeed`**: Speed range.
- **`gravity`**: Y-axis force.
- **`friction`**: Velocity damping.
- **`minLife, maxLife`**: Lifetime range.

---

### ParticlePresets

**Inherits:** None

Namespace containing predefined `ParticleConfig` constants for common effects:

- `Fire`
- `Explosion`
- `Sparks`
- `Smoke`
- `Dust`

---

## Input Module

Handles input from physical buttons or keyboard.

### InputManager

**Inherits:** None

Handles input polling, debouncing, and state tracking.

#### Public Methods

- **`bool isButtonPressed(uint8_t buttonIndex) const`**
    Returns true if button was just pressed this frame.

- **`bool isButtonReleased(uint8_t buttonIndex) const`**
    Returns true if button was just released this frame.

- **`bool isButtonDown(uint8_t buttonIndex) const`**
    Returns true if button is currently held down.

- **`bool isButtonClicked(uint8_t buttonIndex) const`**
    Returns true if button was clicked (pressed and released).

---

### InputConfig

**Inherits:** None

Configuration structure for `InputManager`. Defines the mapping between logical inputs and physical pins.

---

## Physics Module

### CollisionSystem

**Inherits:** None

Manages collision detection between entities using AABB checks and collision layers.

#### Public Methods

- **`void addEntity(Entity* e)`**
    Registers an entity for collision checks.

- **`void removeEntity(Entity* e)`**
    Unregisters an entity.

- **`void update()`**
    Performs collision detection checks and triggers `onCollision` callbacks.

---

### DefaultLayers

**Inherits:** None

Namespace with common collision layer constants:

- `kNone`: 0 (No collision)
- `kAll`: 0xFFFF (Collides with everything)

---

### PhysicsActor

**Inherits:** [Actor](#actor)

An actor with basic 2D physics properties similar to a RigidBody2D. It extends the base Actor class by adding velocity, acceleration, friction, restitution (bounciness), and world boundary collision resolution.

#### Public Methods

- **`PhysicsActor(float x, float y, float w, float h)`**
    Constructs a PhysicsActor.

- **`void update(unsigned long deltaTime)`**
    Updates the actor state, applying physics integration and checking world boundary collisions.

- **`WorldCollisionInfo getWorldCollisionInfo() const`**
    Gets information about collisions with the world boundaries.

- **`virtual void onWorldCollision()`**
    Callback triggered when this actor collides with world boundaries.

- **`void setVelocity(float x, float y)`**
    Sets the linear velocity of the actor.

- **`void setRestitution(float r)`**
    Sets the restitution (bounciness). 1.0 means perfect bounce, < 1.0 means energy loss.

- **`void setFriction(float f)`**
    Sets the friction coefficient (0.0 means no friction).

- **`void setLimits(LimitRect limits)`**
    Sets custom movement limits for the actor.

- **`void setWorldSize(int width, int height)`**
    Defines the world size for boundary checking, used as default limits.

---

### LimitRect

**Inherits:** None

Bounding rectangle for world-collision resolution. Defines the limits of the play area.

#### Properties

- **`left`**: Left boundary (-1 for no limit).
- **`top`**: Top boundary (-1 for no limit).
- **`right`**: Right boundary (-1 for no limit).
- **`bottom`**: Bottom boundary (-1 for no limit).

#### Public Methods

- **`LimitRect(int l, int t, int r, int b)`**
    Constructs a LimitRect with specific bounds.

- **`int width() const`**
    Calculates the width of the limit area.

- **`int height() const`**
    Calculates the height of the limit area.

---

### WorldCollisionInfo

**Inherits:** None

Information about world collisions in the current frame. Holds flags indicating which sides of the play area the actor collided with.

#### Properties

- **`left`**: True if collided with the left boundary.
- **`right`**: True if collided with the right boundary.
- **`top`**: True if collided with the top boundary.
- **`bottom`**: True if collided with the bottom boundary.

---

## Math Module

### MathUtil

**Inherits:** None

Collection of helper functions.

#### Public Methods

- **`float lerp(float a, float b, float t)`**
    Linear interpolation.
- **`float clamp(float v, float min, float max)`**
    Clamps a value between min and max.

---

## UI Module

The UI module provides classes for creating user interfaces.

### UIElement

**Inherits:** [Entity](#entity)

Base class for all user interface elements (buttons, labels, etc.). Sets the `EntityType` to `UI_ELEMENT`.

#### Public Methods

- **`UIElement(float x, float y, float w, float h)`**
    Constructs a new UIElement.

### UIButton

**Inherits:** [UIElement](#uielement)

A clickable button UI element. Supports both physical (keyboard/gamepad) and touch input. Can trigger a callback function when pressed.

#### Public Methods

- **`UIButton(std::string t, uint8_t index, float x, float y, float w, float h, std::function<void()> callback, TextAlignment textAlign = TextAlignment::CENTER, int fontSize = 2)`**
    Constructs a new UIButton.
    - `t`: Button label text.
    - `index`: Navigation index (for D-pad navigation).
    - `x, y`: Position.
    - `w, h`: Size.
    - `callback`: Function to call when clicked/pressed.
    - `textAlign`: Text alignment (LEFT, CENTER, RIGHT).
    - `fontSize`: Text size multiplier.

- **`void setStyle(Color textCol, Color bgCol, bool drawBg)`**
    Configures the button's visual style.

- **`void setSelected(bool selected)`**
    Sets the selection state (e.g., focused via D-pad).

- **`bool getSelected() const`**
    Checks if the button is currently selected.

- **`void handleInput(const InputManager& input)`**
    Handles input events. Checks for touch events within bounds or confirmation buttons if selected.

- **`void press()`**
    Manually triggers the button's action.

### UILabel

**Inherits:** [UIElement](#uielement)

A simple text label UI element. Displays a string of text on the screen. Auto-calculates its bounds based on text length and size.

#### Public Methods

- **`UILabel(std::string t, float x, float y, Color col, uint8_t sz)`**
    Constructs a new UILabel.

- **`void setText(const std::string& t)`**
    Updates the label's text. Recalculates dimensions.

- **`void setVisible(bool v)`**
    Sets visibility.

- **`void centerX(int screenWidth)`**
    Centers the label horizontally on the screen.

