# NesLinearCounter

<Badge type="info" text="Struct" />

**Source:** `AudioTypes.h`

## Description

NES APU linear counter state for the TRIANGLE channel.

The linear counter is a quarter-frame clocked 7-bit countdown shared
with the TRIANGLE channel. A write to $4008 sets the reload value
(lower 7 bits) and the reload flag. On the next quarter-clock:
  - if `reloadFlag` is set, the counter is loaded from `reloadValue`;
  - else if the counter is > 0, it is decremented;
  - if the shared `control` flag (i.e. `NesLengthCounter::halt`) is
    clear, the reload flag is also cleared.

The shared `halt` flag is read through the `lengthCounter` pointer,
which is bound in `AudioChannel::reset()`. This avoids duplicating
the flag and keeps `$4008`'s control bit and `$400B`'s halt bit
behaviour in lockstep with the canonical NES semantics.

When the linear counter is enabled (`linearEnabled == true`) and the
counter is 0, the TRIANGLE sequencer is gated per-sample (output 0,
phase frozen) but the voice is NOT disabled. Re-loading via a
$4008 write resumes the voice without needing a fresh play event.

Default OFF: `linearEnabled == false` on construction, so the
counter does not tick and the canonical track PCM is unchanged.
Hito 2 M5.

Source: https://www.nesdev.org/wiki/APU_Linear_Counter
