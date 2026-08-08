# Scene

<Badge type="info" text="Class" />

**Source:** `Scene.h`

## Description

Represents a game level or screen containing entities.

init() is idempotent — may be called any number of times, each call
      leaves the state equivalent to a fresh init.

::: tip
init() is idempotent — may be called any number of times, each call
      leaves the state equivalent to a fresh init.
:::

## Methods

### `virtual void init()`

**Description:**

Initialises the scene. Called when entering the scene.

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

### `virtual void onRoomEnter(int fromIdx, int toIdx)`

**Description:**

Hook that fires when the scene's RoomGraph enters a new room.

**Parameters:**

- `fromIdx`: Index of the previous room (0xFFFF if first entry).
- `toIdx`: Index of the room being entered.

### `pixelroot32::gameplay::RoomGraphBase* getRoomGraph() const`

**Description:**

Type-erased accessor for the scene's room graph.

**Returns:** Pointer to the RoomGraphBase, or nullptr if none set.

### `void setRoomGraph(pixelroot32::gameplay::RoomGraphBase* g)`

**Description:**

Register a RoomGraph with this scene, called once during init().

**Parameters:**

- `g`: Pointer to a RoomGraphBase (typically a RoomGraph<N>
         owned by the subclass). Must outlive this scene.

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

### `inline pixelroot32::math::Vector2 getCameraEffectOffset() const`

**Description:**

Get the current summed offset from CameraEffectsSystem.

**Returns:** math::Vector2 offset (ZERO when no effects active or feature disabled).

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

### `inline bool shouldPrecede(Entity* key, Entity* at) const`

**Description:**

Insertion-sort predicate used by sortEntities(): whether `key`
must be placed before `at`.
