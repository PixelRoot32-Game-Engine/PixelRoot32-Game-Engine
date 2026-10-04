# NesSweepUnit

<Badge type="info" text="Struct" />

**Source:** `AudioTypes.h`

## Description

NES APU sweep unit state for the PULSE channels.

Mirrors the $4001 / $4005 sweep register pair. Operates on the
11-bit timer period (NES PULSE NTSC = 111860.8 / (timer+1) Hz).
Clocks at half-frame rate (120 Hz NTSC mode 0); on each tick the
unit computes a target period, evaluates the muting conditions,
and optionally updates `timer` from the target.

The muting flag is evaluated CONTINUOUSLY (the "NES bug" from
https://www.nesdev.org/wiki/APU_Sweep) — even when `sweepEnabled`
is false or `shift` is 0, the dispatcher still computes the
target and updates `muted`. The hot path (PULSE generation)
checks `muted` per-sample and gates the output to 0.

That continuous evaluation is scoped to voices that opted in via
`unitEnabled`. Without that gate the rule `period < 8` would fire
on the default `timer == 0`, so merely starting the frame counter
would silence every PULSE voice driven by the legacy float
frequency path. See `unitEnabled` below.

Default state: all fields zero / false. The canonical track never
enables NES mode, so the rendered PCM is byte-for-byte identical.
Hito 2 M6.

Source: https://www.nesdev.org/wiki/APU_Sweep

## Properties

| Name | Type | Description |
|------|------|-------------|
| `unitEnabled` | `bool` | Per-voice opt-in for the whole sweep unit. NOT a hardware register bit — it is the same "does this voice participate in the NES model at all" gate as `NesLengthCounter::enabled`, `NesLinearCounter::linearEnabled` and `NesEnvelope::envelopeEnabled`. Set it with `ApuCore::setVoiceNesSweepUnitEnabled`. |
| `sweepEnabled` | `bool` | E bit ($4001 bit 7 / $4005 bit 7). Enables the timer update path. |
| `period` | `uint8_t` | P (3 bits) — divider period (NTSC mode 0: half-frame rate). |
| `negate` | `bool` | N bit — negate (true = subtract, false = add). |
| `shift` | `uint8_t` | SSS (3 bits) — shift amount. 0 disables the timer-update branch. |
| `reloadFlag` | `bool` | Set by $4001 / $4005 write; cleared on the next half-clock tick. |
| `dividerCounter` | `uint8_t` | Current divider counter. |
| `isPulse2` | `bool` | true = PULSE 2 (two's complement negate); false = PULSE 1 (ones' complement negate). |
| `timer` | `uint16_t` | 11-bit NES timer period (0..2047). Source of truth for the oscillator freq. |
| `muted` | `bool` | Continuously-evaluated muting flag. True when the channel is silenced (current period < 8 OR target > 0x7FF). Test-only field exposed by the `getVoiceNesSweepMutedForTesting` accessor. |
