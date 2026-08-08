# CameraTween

<Badge type="info" text="Class" />

**Source:** `CameraTween.h`

## Description

Fixed-capacity camera tween pool with enum-based easing.

Owns a fixed array of N tween slots, each tracking a `from` → `to`
Vector2 interpolation with an independent duration and easing curve.
Every active tween writes its current interpolated position to the
camera via `Camera2D::setPosition()` on each `update()` call. When
multiple tweens are active, last-write-wins on the camera position.

Zero heap allocation — the slot array and all easing math live on the
stack or in the object's fixed-size storage. Copy and move are deleted
because the pool is its identity.

N Maximum number of simultaneously active tweens (default 4).
          When N=0, startTween() always returns kInvalidSlotId and
          activeCount() always returns 0.

## Methods

### `uint8_t startTween(pixelroot32::math::Vector2 from, pixelroot32::math::Vector2 to, uint16_t durationMs, TweenEasing easing)`

**Description:**

Start a new camera tween.

**Parameters:**

- `from`: Starting position (world coordinates).
- `to`: Target position (world coordinates).
- `durationMs`: Total duration in milliseconds, 1..65535.
                  0 is treated as an instant no-op (returns kInvalidSlotId
                  without consuming a slot).
- `easing`: Easing curve for the interpolation.

**Returns:** Slot id 0..N-1 on success, or kInvalidSlotId (0xFF) if the pool
        is full, N==0, or durationMs==0.

### `void update(uint16_t deltaTimeMs, Camera2D* camera)`

**Description:**

Advance all active tweens and write interpolated positions
       to the camera.

**Parameters:**

- `deltaTimeMs`: Milliseconds elapsed since the last frame.
- `camera`: Target camera (must not be nullptr when tweens are
                   active; safe nullptr when activeCount()==0).

### `bool isComplete(uint8_t slotId) const`

**Description:**

Check whether a given tween slot has completed.

**Parameters:**

- `slotId`: The slot id returned by startTween().

**Returns:** true if the slot exists, was previously started, and has
        finished (elapsed ≥ duration); false otherwise, including
        for invalid/out-of-range slotId.

A cancelled slot is also "complete" — cancel() marks it inactive.

### `uint8_t activeCount() const`

**Description:**

Number of currently active (running) tweens.

**Returns:** Count in 0..N.

### `void cancel(uint8_t slotId)`

**Description:**

Cancel a tween without writing to the camera.

**Parameters:**

- `slotId`: The slot id returned by startTween().

### `static uint32_t applyEasing(TweenEasing easing, uint32_t progressRaw)`

**Description:**

Compute eased progress in Q16.16 raw integer arithmetic.

**Parameters:**

- `easing`: One of the four TweenEasing values.
- `progressRaw`: Normalised progress in Q16.16 (0..ONE_Q16).

**Returns:** Eased progress in Q16.16 (0..ONE_Q16).

### `static constexpr uint8_t slotCount()`

**Description:**

Effective slot count (N==0 collapses to 1 dummy slot).
