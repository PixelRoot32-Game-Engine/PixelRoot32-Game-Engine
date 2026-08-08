# SceneArena

<Badge type="info" text="Struct" />

**Source:** `Scene.h`

## Description

Bump allocator for objects whose lifetime is one scene.

Hands out aligned slices of a caller-supplied buffer by advancing an offset.
There is no individual free: reset() rewinds the offset to zero and releases
everything at once, which is why allocations must not outlive the scene.
Destructors are never run, so only trivially destructible types are safe.

allocate() returns nullptr when the request does not fit, and callers must
check. Prefer arenaNew() over calling allocate() directly.

Gated by platforms::config::EnableSceneArena. With the arena
      disabled every allocate() returns nullptr and init()/reset() are
      no-ops, so callers keep their fallback path.

::: tip
Gated by platforms::config::EnableSceneArena. With the arena
      disabled every allocate() returns nullptr and init()/reset() are
      no-ops, so callers keep their fallback path.
:::

## Methods

### `void init(void* memory, std::size_t size)`

### `void reset()`

### `void* allocate(std::size_t size, std::size_t alignment)`
