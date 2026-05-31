# CameraEffectsSystem

<Badge type="info" text="Class" />

**Source:** `CameraEffects.h`

## Description

Manages up to 4 simultaneous camera effects with round-robin insertion.

Effects are updated with a delta time and the resulting summed offset
can be fetched for application to a Camera2D. Provides a fast no-op path
via hasActiveEffects().

## Methods

### `void update(unsigned long deltaTimeMs)`

**Description:**

Advance all active effect timers.

**Parameters:**

- `deltaTimeMs`: Time elapsed since last frame in ms.

### `math::Vector2 getOffset() const`

**Description:**

Get the summed offset from all active effects.

**Returns:** Summed math::Vector2 offset. ZERO if no effects are active.

Early-outs when hasActiveEffects() is false — zero loop iteration.

### `bool hasActiveEffects() const`

**Description:**

Fast check for any active effects.

**Returns:** true if at least one slot is active.

### `void triggerShake(math::Scalar amp, unsigned long dur)`

**Description:**

Trigger a shake effect (random oscillation).

**Parameters:**

- `amp`: Maximum amplitude of the shake.
- `dur`: Duration in ms.

### `void triggerPunch(math::Scalar amp, unsigned long dur, const math::Vector2& dir)`

**Description:**

Trigger a punch effect (directional impulse with decay).

**Parameters:**

- `amp`: Initial amplitude.
- `dur`: Duration in ms.
- `dir`: Normalized direction of the punch.

### `void triggerOffset(math::Scalar amp, unsigned long dur)`

**Description:**

Trigger a constant offset effect.

**Parameters:**

- `amp`: Amplitude/displacement amount.
- `dur`: Duration in ms.

The offset direction defaults to RIGHT (1, 0).
Displacement stays constant for the full duration, then snaps to zero.

### `void cancelAll()`

**Description:**

Cancel all active effects immediately.

### `math::Vector2 computeShakeOffset(const EffectSlot& s) const`

**Description:**

Compute a single frame's shake offset using Xorshift32.

**Parameters:**

- `s`: The shake effect slot.

**Returns:** Random offset vector bounded by slot amplitude.

### `math::Vector2 computeDecayOffset(const EffectSlot& s) const`

**Description:**

Compute offset with linear decay (Punch) or constant (Offset).

**Parameters:**

- `s`: The effect slot (Punch or Offset type).

**Returns:** Decayed or constant offset vector.

### `uint8_t allocSlot()`

**Description:**

Find the next available slot index (round-robin).

### `void update(unsigned long /*deltaTimeMs*/)`

**Description:**

Stub CameraEffectsSystem — always returns zero offset.
