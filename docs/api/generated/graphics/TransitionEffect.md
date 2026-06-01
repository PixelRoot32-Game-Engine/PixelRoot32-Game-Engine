# TransitionEffect

<Badge type="info" text="Class" />

**Source:** `TransitionEffect.h`

## Description

Manages a single scene transition with zero runtime allocation.

Pre-computes a 256-byte LUT for Fade effects per frame (LUT is computed
in apply()). Iris uses (x-cx)²+(y-cy)² > r² with no sqrt. All state is
fixed-size — no heap allocation in update() or apply().

Typical lifecycle:
  effect.init(Fade, Out, 500);
  while (effect.isActive()) {
      effect.update(dt);
      effect.apply(buffer, width, height);
  }

## Methods

### `void init(TransitionType type, TransitionDirection direction, unsigned long durationMs)`

**Description:**

Initialise the effect with type, direction and duration.

**Parameters:**

- `type`: Fade or Iris transition.
- `direction`: Out (visible→hidden) or In (hidden→visible).
- `durationMs`: Total duration of the transition in milliseconds.

### `void update(unsigned long deltaTimeMs)`

**Description:**

Advance the effect timer.

**Parameters:**

- `deltaTimeMs`: Time elapsed since last frame in ms.

Clamps elapsed to duration — isActive() returns false once elapsed ≥ duration.

### `void apply(uint8_t* buffer, int width, int height)`

**Description:**

Apply the transition effect to an 8bpp framebuffer.

**Parameters:**

- `buffer`: Pointer to the 8bpp pixel data (can be nullptr — safe no-op).
- `width`: Buffer width in pixels.
- `height`: Buffer height in pixels.

Fade: pre-computes LUT from current progress, then maps each pixel through it.
Iris: zeroes pixels whose (x-cx)²+(y-cy)² exceeds current radius² (no sqrt).

Safe to call when not active — returns immediately with no side effects.

### `bool isActive() const`

**Description:**

Check whether the transition is still running.

**Returns:** true while elapsed < duration.

### `float getProgress() const`

**Description:**

Get normalised progress of the transition.

**Returns:** 0.0 at start, 1.0 at completion.

### `void setIrisCenter(int cx, int cy)`

**Description:**

Override the iris center for non-centered circle wipes.

**Parameters:**

- `cx`: X-coordinate of the iris center.
- `cy`: Y-coordinate of the iris center.

Only relevant for Iris transitions. Default is buffer center.
Call after init() and before apply().

### `void computeFadeLut(uint8_t* lut, uint16_t scaledProgress) const`

**Description:**

Fill a 256-byte LUT for the current fade direction and progress.

**Parameters:**

- `scaledProgress`: Progress in Q8.8 format (0..256, where 256 = 1.0).

Out: lut[i] = i * (256-p) / 256 — dims to black.
In:  lut[i] = i * p     / 256 — brightens from black.

### `void applyFade(uint8_t* buffer, int width, int height)`

**Description:**

Apply the fade LUT to the entire buffer.

### `void applyIris(uint8_t* buffer, int width, int height)`

**Description:**

Apply the iris wipe to the entire buffer.

### `void init(TransitionType /*type*/, TransitionDirection /*direction*/, unsigned long /*durationMs*/)`

**Description:**

Stub TransitionEffect — all methods are no-ops when feature is disabled.
