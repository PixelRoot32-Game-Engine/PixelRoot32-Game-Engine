# Scene

<Badge type="info" text="Class" />

**Source:** `Scene.h`

## Description

Represents a game level or screen containing entities.

## Methods

### `virtual void init()`

**Description:**

Initializes the scene. Called when entering the scene.

### `virtual void initUI()`

**Description:**

Initialize the UI system for this scene.
Called during scene init. Add UI elements here.

### `virtual void updateUI(unsigned long deltaTime)`

**Description:**

Update the UI system.

**Parameters:**

- `deltaTime`: Time elapsed in ms.

### `pixelroot32::graphics::ui::UIManager& getUIManager()`

**Description:**

Get the UI manager for this scene.

**Returns:** Reference to the UIManager

### `virtual void processTouchEvents(pixelroot32::input::TouchEvent* events, uint8_t count)`

**Description:**

Central touch pipeline entry point (call once per frame).

**Parameters:**

- `events`: Mutable buffer — consumed flags are set in-place.
- `count`: Number of events in the buffer.

### `virtual void onUnconsumedTouchEvent(const pixelroot32::input::TouchEvent& event)`

**Description:**

Hook for scene-specific handling of unconsumed touch events.

**Parameters:**

- `event`: The touch event (not consumed by UI).

### `virtual void update(unsigned long deltaTime)`

**Description:**

Updates all entities in the scene and handles collisions.

**Parameters:**

- `deltaTime`: Time elapsed in ms.

### `virtual void draw(pixelroot32::graphics::Renderer& renderer)`

**Description:**

Draws all visible entities in the scene.

**Parameters:**

- `renderer`: The renderer to use.

### `virtual void adviseFramebufferBeforeBeginFrame(pixelroot32::graphics::Renderer& renderer)`

**Description:**

Advises the scene that the framebuffer is about to be drawn.

**Parameters:**

- `renderer`: The renderer to use.

Optional hook: run immediately before Renderer::beginFrame().
Scenes using StaticTilemapLayerCache should call adviseFramebufferBeforeBeginFrame with the same
layers/camera sampling as StaticTilemapLayerCache::draw so dirty-region clears align with framebuffer memcpy restores.

### `virtual bool shouldRedrawFramebuffer() const`

**Description:**

When false, Engine may skip `draw()` and `present()` for this iteration (after `update()`).

### `void addEntity(Entity* entity)`

**Description:**

Adds an entity to the scene.

**Parameters:**

- `entity`: Pointer to the Entity to add.

### `void removeEntity(Entity* entity)`

**Description:**

Removes an entity from the scene.

**Parameters:**

- `entity`: Pointer to the Entity to remove.

### `void clearEntities()`

**Description:**

Removes all entities from the scene.
