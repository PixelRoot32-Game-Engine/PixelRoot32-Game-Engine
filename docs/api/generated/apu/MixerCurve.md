# MixerCurve

<Badge type="info" text="Enum" />

**Source:** `AudioTypes.h`

## Description

Balance the summed voices carry into the output stage.

Orthogonal to `SoftClipMode`: the curve decides how much each voice
GROUP contributes to the mix, the clip mode decides what happens to the
resulting sum afterwards. The weighting runs before the clip curve and
before the DC blocker, so the compressor's nonlinearity acts on the
rebalanced signal rather than on a mix it saw first.

Owned by the sound profile (D-3 tier 1), never by a voice: the mix is a
property of the whole output, so there is deliberately no per-voice
setter and no `AudioCommandType` for it. A voice-level owner would let
one note change another one's level.

Declared next to `SoftClipMode` because the two are the two halves of
the same output stage and `AudioChannel` must not grow a field for
either of them.
