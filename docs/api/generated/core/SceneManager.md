# SceneManager

<Badge type="info" text="Class" />

**Source:** `SceneManager.h`

## Description

Manages the stack of active scenes.

The SceneManager allows for scene transitions (replacing scenes) and
stacking scenes (push/pop), which is useful for pausing or menus.

## Methods

### `void setCurrentScene(Scene* newScene)`

**Description:**

Replaces the current scene with a new one.

**Parameters:**

- `newScene`: The new scene to switch to.

### `void pushScene(Scene* newScene)`

**Description:**

Pushes a new scene onto the stack, pausing the previous one.

**Parameters:**

- `newScene`: The new scene to become active.

### `void popScene()`

**Description:**

Removes the top scene from the stack, resuming the previous one.

### `void update(unsigned long dt)`

**Description:**

Updates the currently active scene.

**Parameters:**

- `dt`: Delta time in ms.

### `void draw(pixelroot32::graphics::Renderer& renderer)`

**Description:**

Draws the currently active scene.

**Parameters:**

- `renderer`: The renderer to use.

### `void adviseFramebufferBeforeBeginFrame(pixelroot32::graphics::Renderer& renderer)`

**Description:**

Lets stacked scenes advertise framebuffer prep (runs before Renderer::beginFrame).

### `std::optional<Scene*> getCurrentScene() const`

**Description:**

Gets the currently active scene.

**Returns:** Optional pointer to the top scene on the stack.

### `bool aggregateShouldRedrawFramebuffer() const`

**Description:**

True if any scene on the stack needs a framebuffer pass this iteration.

### `int getSceneCount() const`

**Description:**

Gets the number of scenes in the stack.

**Returns:** The number of scenes.

### `bool isEmpty() const`

**Description:**

Checks if the scene stack is empty.

**Returns:** True if there are no scenes.

### `bool isTransitioning() const`

**Description:**

Whether a scene transition is currently active.

**Returns:** true when TransitionState != Idle.

### `TransitionState getTransitionState() const`

**Description:**

Get the current transition state.

**Returns:** The active TransitionState.

### `void setTransitionEffect(pixelroot32::graphics::TransitionEffect* effect)`

**Description:**

Provide a pointer to the Engine-owned TransitionEffect instance.

**Parameters:**

- `effect`: Non-owning pointer to the TransitionEffect.

Called by Engine::init(). The Engine owns the TransitionEffect;
SceneManager only drives it (init, update) during transitions.
