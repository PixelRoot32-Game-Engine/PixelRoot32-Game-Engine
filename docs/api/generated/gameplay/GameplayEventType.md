# GameplayEventType

<Badge type="info" text="Enum" />

**Source:** `GameplayEventType.h`

## Description

Tag identifying the meaning of a GameplayEvent.

`None` is the default/uninitialized tag. `TriggerEnter`/`TriggerExit` are
published by the actor-interaction-triggers capability. `Interact` is
reserved for manual interact dispatch. Game-defined event codes MUST start
at `UserBase` (64) or above so they never collide with engine-reserved
values added in future phases.
