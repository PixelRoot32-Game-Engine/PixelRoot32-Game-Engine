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

### `void applyRGB565(uint16_t* buffer, int width, int height)`

**Description:**

Apply the transition effect to an RGB565 framebuffer.

**Parameters:**

- `buffer`: Pointer to RGB565 pixel data (can be nullptr — safe no-op).
- `width`: Buffer width in pixels.
- `height`: Buffer height in pixels.

Used on native/SDL2 path where no 8bpp sprite buffer exists.
Iris zeroes pixels outside the circle. Fade darkens/brightens via channel scaling.

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

Override the iris center for both Out and In phases (backward compat).

**Parameters:**

- `cx`: X-coordinate of the iris center.
- `cy`: Y-coordinate of the iris center.

Sets both Out and In phases to the same center. Equivalent to calling
both setIrisOutCenter() and setIrisInCenter().
Only relevant for Iris transitions. Default is buffer center.
Call after init() and before apply().

### `void setIrisOutCenter(int cx, int cy)`

**Description:**

Override the iris center for the Out (closing) phase.

**Parameters:**

- `cx`: X-coordinate of the iris center.
- `cy`: Y-coordinate of the iris center.

Sets the center used when the iris closes (Out direction).
Only relevant for Iris transitions. Default is buffer center.
Call after init() and before apply().

### `void setIrisInCenter(int cx, int cy)`

**Description:**

Override the iris center for the In (opening) phase.

**Parameters:**

- `cx`: X-coordinate of the iris center.
- `cy`: Y-coordinate of the iris center.

Sets the center used when the iris opens (In direction).
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

Apply the fade LUT to the entire 8bpp buffer.

### `void applyIris(uint8_t* buffer, int width, int height)`

**Description:**

Apply the iris wipe to the entire 8bpp buffer.

### `void applyIrisRGB565(uint16_t* buffer, int width, int height)`

**Description:**

Apply the iris wipe to an RGB565 buffer.

### `void applyFadeRGB565(uint16_t* buffer, int width, int height)`

**Description:**

Apply the fade to an RGB565 buffer via channel scaling.

### `void init(TransitionType /*type*/, TransitionDirection /*direction*/, unsigned long /*durationMs*/)`

**Description:**

Stub TransitionEffect — all methods are no-ops when feature is disabled.
