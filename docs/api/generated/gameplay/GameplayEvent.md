# GameplayEvent

<Badge type="info" text="Struct" />

**Source:** `GameplayEvent.h`

## Description

Fixed-size POD carried by the GameplayEventBus.

One flat struct, no union, no variable-length payload. Field declaration
order (pointer, then Scalar, then the two ids, then the tag) is chosen for
alignment, not for readability — do not reorder without re-checking the
byte budget in design.md (16 B on ESP32-C3, 24 B on native).

`userData` is an untyped escape hatch: the engine never interprets,
manages, or frees it, mirroring the existing `PhysicsActor::userData`
contract (include/core/PhysicsActor.h).

## Properties

| Name | Type | Description |
|------|------|-------------|
| `userData` | `void*` | Untyped escape hatch (never owned by the engine). |
| `value` | `math::Scalar` | Position / quantity / damage-style datum. |
| `entityIdA` | `uint16_t` | "Who" — the primary actor's entity id. |
| `entityIdB` | `uint16_t` | "With whom" (0 = none). |
| `type` | `GameplayEventType` | Event tag. |
