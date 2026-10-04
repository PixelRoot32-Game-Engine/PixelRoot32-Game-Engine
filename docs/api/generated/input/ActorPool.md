# ActorPool

<Badge type="info" text="Struct" />

**Source:** `ActorTouchController.h`

## Description

Fixed-size pool for managing draggable actors

Uses a fixed array to avoid dynamic memory allocation.
Maximum 8 actors can be registered for touch dragging.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `kMaxActors` | `static constexpr uint8_t` | Maximum number of actors in the pool |
| `actors` | `pixelroot32::core::Actor*` | Array of actor pointers |
| `count` | `uint8_t` | Current number of registered actors |
