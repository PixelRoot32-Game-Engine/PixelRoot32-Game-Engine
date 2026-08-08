# NesEnvelope

<Badge type="info" text="Struct" />

**Source:** `AudioTypes.h`

## Description

NES APU envelope unit state for PULSE 1, PULSE 2, and NOISE.

Mirrors the $4000 / $4004 / $400C write side effects (loop flag,
constant volume flag, VVVV) and the $4003 / $4007 / $400F length
load side effect (start flag). Clocks at quarter-frame rate
(240 Hz NTSC mode 0) via the M3 `tickAllNesEnvelopes()` dispatcher.

The loop flag is SHARED with the length counter (`halt` bit):
`tickAllNesEnvelopes()` reads `lengthCounter->halt` to decide whether
the decay level should wrap to 15 on underflow (loop) or silence
the channel (!loop). The `lengthCounter` pointer is bound in
`AudioChannel::reset()` so the two sub-units see the same bit.

When `envelopeEnabled == true` on a PULSE or NOISE voice, the
envelope's `output` (0..15) REPLACES the ADSR `envelope.currentLevel`
as the per-sample volume source. When the envelope reaches `0` and
loop is clear, the voice is silenced directly (matches the NES
hardware, which gates the channel on the envelope output).

Default state: `envelopeEnabled == false` on construction, so the
quarter-frame dispatch is a no-op and the canonical track PCM
remains byte-for-byte identical. Hito 2 M7.

Source: https://www.nesdev.org/wiki/APU_Envelope
