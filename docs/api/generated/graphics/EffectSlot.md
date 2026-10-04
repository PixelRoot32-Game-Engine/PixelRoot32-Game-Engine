# EffectSlot

<Badge type="info" text="Struct" />

**Source:** `CameraEffects.h`

## Description

Per-slot state for a single camera effect (20 bytes).

## Properties

| Name | Type | Description |
|------|------|-------------|
| `duration` | `unsigned long` | Total duration in ms. |
| `elapsed` | `unsigned long` | Elapsed time in ms. |
| `direction` | `math::Vector2` | Normalized direction (Punch/Offset only). |
