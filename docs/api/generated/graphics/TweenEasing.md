# TweenEasing

<Badge type="info" text="Enum" />

**Source:** `CameraTween.h`

## Description

Easing curves for camera tween interpolation.

All four curves map a normalised progress [0, 1] to an eased value [0, 1].
The easing math is computed in Q16.16 fixed-point (raw values 0..65536) so
that the hot path contains zero floating-point operations on the Fixed16
(ESP32-C3) scalar path.

## Methods

### `inline pixelroot32::math::Scalar scalarFromQ16(uint32_t rawProgress)`

**Description:**

Convert a Q16.16 raw progress value (0..65536) to Scalar (0.0..1.0).
