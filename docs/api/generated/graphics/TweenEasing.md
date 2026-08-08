# TweenEasing

<Badge type="info" text="Enum" />

**Source:** `CameraTween.h`

## Description

Easing curves for camera tween interpolation.

All four curves map a normalised progress [0, 1] to an eased value [0, 1].
The easing math is computed in Q16.16 fixed-point (raw values 0..65536) so
that the hot path contains zero floating-point operations on the Fixed16
(ESP32-C3) scalar path.
