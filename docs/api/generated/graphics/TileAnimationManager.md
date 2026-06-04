# TileAnimationManager

<Badge type="info" text="Class" />

**Source:** `TileAnimation.h`

## Description

Manages tile animations for a tilemap.

Provides O(1) frame resolution via lookup table. All animation
definitions stored in PROGMEM. Zero dynamic allocations.

CRITICAL: Uses fixed-size arrays (no new/delete) to comply with
PixelRoot32's strict "zero allocation" policy for ESP32.

## Methods

### `uint8_t resolveFrame(uint8_t tileIndex)`

**Description:**

Resolve tile index to current animated frame.

**Parameters:**

- `tileIndex`: Base tile index from tilemap

**Returns:** Current frame index (may be same as input if not animated)

PERFORMANCE: O(1) array lookup, IRAM-friendly, no branches in hot path

### `void step(unsigned long deltaTimeMs)`

**Description:**

Advance animations from elapsed wall time (60 Hz logical ticks max).
Uses high-resolution time between calls (micros); deltaTimeMs is a fallback when
the clock does not advance between calls (e.g. unit tests without real time).

**Parameters:**

- `deltaTimeMs`: Fallback elapsed time in milliseconds.

### `void reset()`

**Description:**

Reset all animations to frame 0.

### `uint32_t getVisualSignature() const`

**Description:**

Fingerprint of the current resolved animation state (O(animCount)).

**Returns:** The visual signature of the current resolved animation state.

Stable across `step(deltaTimeMs)` calls until a visible frame advances (same contract as
`resolveFrame` / lookup table). Used by scenes to skip draw+present when output
would be identical (e.g. ESP32 SPI budget).

### `bool animatedTileAppearanceChanged(uint8_t storedTileIndex) const`

**Description:**

Checks if the resolved graphic for storedTileIndex changed since the snapshot taken at the
start of last step(); invalid indices return true so callers repaint conservatively.

**Parameters:**

- `storedTileIndex`: The tile index to check.

**Returns:** True if the resolved graphic for storedTileIndex changed since the snapshot taken at the
start of last step(); invalid indices return true so callers repaint conservatively.
