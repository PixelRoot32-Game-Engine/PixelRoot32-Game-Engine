# InteractionComponent

<Badge type="info" text="Struct" />

**Source:** `InteractionComponent.h`

## Description

Opt-in interaction hooks for a game actor — composition, not inheritance.

D3's core decision: no new virtual methods and no new members on `Actor`
itself. A game actor that wants enter/exit/interact semantics holds one of
these as a member and registers it with an `InteractionTracker`:

tracker.registerActor(this, &interaction);   // after Scene::addEntity assigned entityId
Every actor that does NOT opt in pays exactly zero bytes for this
capability — `Actor` (include/core/Actor.h) is untouched by this file.

`onInteract()` is manual-call only. The engine never auto-fires it from
enter/exit signals, physics contact resolution, or scene lifecycle events
— only explicit game code calling `invokeInteract()` triggers it, because
the engine has no facing-direction or action-binding concept to key an
automatic invocation on.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `owner` | `void*` | Usually the game actor's `this`. |
| `onEnter` | `TriggerFn` | Called once when a tracked pair begins contact. |
| `onExit` | `TriggerFn` | Called once when a tracked pair's contact ends. |
| `onInteract` | `InteractFn` | Manual-call only — never invoked by the engine. |

## Methods

### `void invokeInteract(core::Actor* instigator)`

**Description:**

Invokes the interact callback. Manual-call only.

**Parameters:**

- `instigator`: The actor initiating the interaction (e.g. the player).
