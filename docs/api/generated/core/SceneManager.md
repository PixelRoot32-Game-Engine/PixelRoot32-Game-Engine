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

### `void transitionToScene(Scene* newScene, pixelroot32::graphics::TransitionType type, unsigned long durationMs)`

**Description:**

Start a transition from the current scene to a new one.

**Parameters:**

- `newScene`: The target scene to transition to.
- `type`: Fade, Iris, or DiagonalWipe transition effect.
- `durationMs`: Duration of each phase (Out and In) in ms.

Ignored if a transition is already running (state != Idle).
The full cycle is: FadingOut (durationMs) → SceneSwap → FadingIn (durationMs) → Idle.

### `void transitionToScene(Scene* newScene, pixelroot32::graphics::TransitionType type, unsigned long durationMs, int irisOutCx, int irisOutCy, int irisInCx, int irisInCy)`

**Description:**

Start a transition with direction-specific iris centers.

**Parameters:**

- `newScene`: The target scene to transition to.
- `type`: Fade, Iris, or DiagonalWipe transition effect.
- `durationMs`: Duration of each phase (Out and In) in ms.
- `irisOutCx`: Iris center X for Out (closing) phase.
- `irisOutCy`: Iris center Y for Out (closing) phase.
- `irisInCx`: Iris center X for In (opening) phase.
- `irisInCy`: Iris center Y for In (opening) phase.

Stores the centers and re-applies them after each effect.init()
(which resets centers to -1). Only meaningful for Iris transitions.

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

### `void setGameplayEventBus(pixelroot32::gameplay::GameplayEventBus* bus)`

**Description:**

Provide a pointer to the Engine-owned GameplayEventBus instance.

**Parameters:**

- `bus`: Non-owning pointer to the GameplayEventBus.

Called by Engine::init(). The Engine owns the bus; SceneManager only
drains it (clear()) on every SceneSwap — see setCurrentScene().
