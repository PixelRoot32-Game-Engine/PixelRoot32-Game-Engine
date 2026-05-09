# SpriteAnimationFrame

<Badge type="info" text="Struct" />

**Source:** `Renderer.h`

## Description

Single animation frame that can reference either a Sprite or a MultiSprite.

Exactly one of the pointers is expected to be non-null for a valid frame.
This allows the same animation system to drive both simple and layered
sprites without exposing bit-level details to game code.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `Sprite` | `const` | Optional pointer to a simple 1bpp sprite frame. |
| `MultiSprite` | `const` | Optional pointer to a layered sprite frame. |
