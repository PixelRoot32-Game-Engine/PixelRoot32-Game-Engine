# EnvelopeCurve

<Badge type="info" text="Enum" />

**Source:** `AudioTypes.h`

## Description

Shape of the ADSR decay and release ramps.

`Linear` is the frozen 2.0.0 shape: a constant step per sample from the
attack peak down to the sustain level. `Exponential` is the S-DSP-style
decay `level -= ((level - 1) >> 8) + 1`, which is what makes a plucked
note sound plucked instead of organ-like — the ear integrates amplitude
roughly logarithmically, so a linear ramp is heard as "holds, then cuts".

The attack stays linear on purpose (P1): a linear ramp is predictable and
curving it buys nothing audible.

Declared here, next to `SweepCurve`, because the type is needed by both
the preset (the note's request) and `AudioChannel` (the per-voice
resolved mirror), and this is the header `AudioMusicTypes.h` already
builds on — the reverse include would be circular.
