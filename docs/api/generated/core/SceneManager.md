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
