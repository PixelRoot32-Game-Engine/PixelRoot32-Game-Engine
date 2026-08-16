# NesLengthCounter

<Badge type="info" text="Struct" />

**Source:** `AudioTypes.h`

## Description

NES APU length counter state for a single voice.

The length counter is a half-frame clocked countdown. Once loaded
(via `ApuCore::setVoiceNesLength`) it decrements at ~120 Hz (mode 0)
or ~96 Hz (mode 1). When the counter reaches 0, the voice is silenced
directly. While `halt` is set, the counter is frozen (does not
decrement).

The `enabled` field mirrors the NES channel enable bit ($4015):
when clear, the counter is forced to 0 and the voice is silenced.
When set, the counter can be loaded and decremented.

Default state: enabled=false, counter=0, halt=false. SINE/SAW voices
keep `enabled=false` permanently and are unaffected by the dispatch
loop. Hito 2 M4.

Source: https://www.nesdev.org/wiki/APU_Length_Counter
