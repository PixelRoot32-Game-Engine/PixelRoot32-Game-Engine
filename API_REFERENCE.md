# API Reference

This document provides a complete reference for the PixelRoot32 Game Engine public API.

## Core Module

The Core module provides the fundamental building blocks of the engine, including the main application loop, entity management, and scene organization.

### Engine

**Inherits:** None

The main engine class that manages the game loop and core subsystems. `Engine` acts as the central hub, initializing and managing the Renderer, InputManager, and SceneManager. It runs the main game loop, handling timing (delta time), updating the current scene, and rendering frames.

#### Public Methods

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

## Math Module

### MathUtil

**Inherits:** None

Collection of helper functions.

#### Public Methods

- **`float lerp(float a, float b, float t)`**
    Linear interpolation.
- **`float clamp(float v, float min, float max)`**
    Clamps a value between min and max.
