# InteractionTracker

<Badge type="info" text="Class" />

**Source:** `InteractionTracker.h`

## Description

Detects enter/exit edges on the per-frame physics contact set and
       dispatches InteractionComponent callbacks (design.md D3).

Driven from `CollisionSystem::triggerCallbacks()`:
  - `beginFrame()` before the contact loop
  - `recordPair()` inside the loop, after the unchanged `onCollision()` calls
  - `endFrame()` after the loop — diffs `currPairs` against `prevPairs` and
    fires `onEnter`/`onExit` for every pair whose presence changed.

`currPairs`/`prevPairs` are sized off `config::PhysicsMaxContacts` (the
same capacity `CollisionSystem` already uses for its contact array), so
this invents no new capacity concept. The registry is a small, separate,
fixed-size array of at most `GAMEPLAY_MAX_INTERACTIVE_ACTORS` entries.

Zero heap allocation: every member is a fixed-size array. `registerActor()`
asserts `actor->entityId != 0` — an actor must be added to a
`CollisionSystem` (which assigns `entityId`) BEFORE it is registered here;
registering earlier is an ordering mistake that the assert catches
immediately rather than silently producing an actor that never receives
edges (design.md Risks section).

**Contact edges are tracked for every physics contact**, not only
registered ones — `recordPair()` is called from `triggerCallbacks()` for
every `Contact`, so a bus-published `TriggerEnter`/`TriggerExit` fires for
any pair whose presence changed, independent of registration. Component
callback dispatch, in contrast, only fires for a pair's registered side(s):
the `other` actor pointer passed to a callback is resolved through the
registry, so it is only non-null when the counterpart actor is ALSO
registered. A game that needs the counterpart's identity in the callback
(not just via the bus event's raw entity ids) must register both sides.

## Methods

### `void registerActor(core::Actor* actor, InteractionComponent* comp)`

**Description:**

Registers an actor's InteractionComponent for enter/exit dispatch.

**Parameters:**

- `actor`: The actor to track. MUST already be registered with a
       CollisionSystem (i.e. `actor->entityId != 0`).
- `comp`: Non-owning pointer to the actor's InteractionComponent.
       The caller retains ownership and lifetime responsibility.

No-op (asserts in debug) if the registry is already at
`kMaxRegistered` capacity or if `actor`/`comp` is null.

### `void unregisterActor(core::Actor* actor)`

**Description:**

Unregisters a previously-registered actor. Safe to call for an
       actor that was never registered (no-op).

### `void setEventBus(GameplayEventBus* bus)`

**Description:**

Sets the (optional) event bus enter/exit edges are published to.

### `void beginFrame()`

**Description:**

Call once at the start of the per-frame contact loop.

### `void recordPair(core::Actor* a, core::Actor* b)`

**Description:**

Records one physics contact pair for this frame's diff.

**Parameters:**

- `a`: First body in the contact (may be null-checked by the caller
       the same way `triggerCallbacks()` already does).
- `b`: Second body in the contact.

Records ALL contact pairs, not only registered ones — the registry
lookup happens only for pairs whose presence actually changed, in
`endFrame()`.

### `void endFrame()`

**Description:**

Diffs `currPairs` against `prevPairs` and dispatches
       onEnter/onExit for every pair whose presence changed, then
       rotates curr into prev for the next frame.

### `void reset()`

**Description:**

Clears prevPairs/currPairs and the registry.
