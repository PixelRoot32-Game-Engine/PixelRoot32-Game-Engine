# TransitionDirection

<Badge type="info" text="Enum" />

**Source:** `TransitionEffect.h`

## Description

Direction of the transition effect.

Out: starts fully visible and transitions to black/invisible.
In:  starts black/invisible and transitions to fully visible.

## Methods

### `uint16_t smoothstepQ8(uint16_t t)`

**Description:**

Q8.8 smoothstep easing function for transition progress.

**Parameters:**

- `t`: Q8.8 input in [0, 256] (0.0 to 1.0 in fixed point).

**Returns:** Q8.8 eased value in [0, 256].

Implements smoothstep(t) = t^2 * (3 - 2t) using fixed-point Q8.8 math:
result = (t^2 * (768 - 2t)) / 65536, clamped to [0, 256].

Provides smoother acceleration/deceleration than linear interpolation,
used by DiagonalWipe to avoid harsh pixel boundaries during the wipe.
