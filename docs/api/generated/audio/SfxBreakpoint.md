# SfxBreakpoint

<Badge type="info" text="Struct" />

**Source:** `AudioTypes.h`

## Description

Timed automation point for SFX duty steps or pitch envelope.

`value` is duty in [0,1] for duty steps, or frequency/clock Hz for pitch.
Tables are static/constexpr in exported banks; AudioEvent holds pointer+count.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `timeSec` | `float` | Offset from voice start (seconds); non-decreasing in a table. |
| `value` | `float` | Duty [0,1] or Hz > 0 depending on table context. |
