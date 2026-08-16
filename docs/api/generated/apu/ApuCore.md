# ApuCore

<Badge type="info" text="Class" />

**Source:** `ApuCore.h`

## Description

Shared NES-style APU core used by every AudioScheduler.

Owns an eight-voice pool (MAX_VOICES = 8), the SPSC command queue and the
music sequencer. Voice slots 0–3 are reserved for sequencer tracks
(trackIdx maps 1:1 to slot); slots 4–7 are reserved for PLAY_EVENT / SFX
and sequencer percussion hits (shared subpool, steal policy confined to 4–7). Platform-specific schedulers
(DefaultAudioScheduler, ESP32AudioScheduler, NativeAudioScheduler) are
thin orchestrators that decide *when* generateSamples() runs; all
synthesis, mixing and sequencing lives here to eliminate the three-way
duplication that existed before.

Mixing curve:
  per channel: s_c = wave_c(phase) * volume_c * MIXER_SCALE
  sum:         S   = Σ s_c                         (bounded to [-1.6, 1.6])
  compressor:  y   = S / (1 + |S| * MIXER_K)
  output:      y * masterVolume * 32767, passed through a single-pole
               DC-blocker to remove offset + transient clicks.

On cores without an FPU (ESP32-C3) the integer-optimised path uses
`audio_mixer_lut`, which is pre-fitted to the same curve and renders
at the same level (Hito 5 M12). It applies MIXER_SCALE once, after
summation rather than per channel, because the table's mapping already
contains the 0.4 — see AudioMixerLUT.h.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `audioTimeSamples` | `uint64_t` | Global sample counter at capture. |
| `peak` | `float` | Peak sample magnitude [0.0 - 1.0]. |
| `clipped` | `bool` | Whether any sample exceeded ±32767. |

## Methods

### `void init(int sampleRate)`

**Description:**

Configures the output sample rate.

**Parameters:**

- `sampleRate`: Sample rate in Hz (e.g., 22050, 44100).

Safe to call before start(). Pre-calculates internal timing values.

### `bool submitCommand(const AudioCommand& cmd)`

**Description:**

Enqueues a command for processing.

**Parameters:**

- `cmd`: The audio command to enqueue.

**Returns:** true if the command was enqueued, false if the queue was full.

### `void generateSamples(int16_t* stream, int length)`

**Description:**

Generates audio samples.

**Parameters:**

- `stream`: Output buffer (mono, int16 samples).
- `length`: Number of samples to generate.

Processes all pending commands, updates music sequencer, and synthesizes
voices into the output buffer. Applies master volume, bitcrush, and DC blocking.

### `uint32_t getDroppedCommands() const`

**Description:**

Returns the total number of commands dropped since construction. @return Monotonic count.

**Returns:** Monotonic count.

### `void setSequencerNoteLimit(size_t limit)`

**Description:**

Sets the maximum notes processed per audio frame.

**Parameters:**

- `limit`: Max notes [1-1000], clamped to safe bounds internally.

Bounded processing prevents audio starvation when many notes queue up.

### `size_t getSequencerNoteLimit() const`

**Description:**

Gets current max notes per frame setting. @return Current limit.

**Returns:** Current limit.

### `size_t getDeferredNotes() const`

**Description:**

Returns notes deferred to next frame due to note limit. @return Deferred count.

**Returns:** Deferred count.

### `bool isMusicPlaying() const`

**Description:**

Reports if music is currently playing. @return true if playing.

**Returns:** true if playing.

### `bool isMusicPaused() const`

**Description:**

Reports if music is paused. @return true if paused.

**Returns:** true if paused.

### `uint64_t getMusicGlobalTick() const`

**Description:**

Current absolute sequencer tick (transport clock). Thread-safe.

### `uint64_t getMusicPlayStartTick() const`

**Description:**

Absolute tick at which the current piece started (MUSIC_PLAY / MUSIC_SEEK anchor). Thread-safe.

### `int getSampleRate() const`

**Description:**

Gets the configured sample rate. @return Sample rate in Hz.

**Returns:** Sample rate in Hz.

### `void reset()`

**Description:**

Resets all state to initial values.

### `void setPostMixMono(void (*fn)(int16_t* mono, int length, void* user), void* user)`

**Description:**

Sets an optional post-mix callback.

**Parameters:**

- `fn`: Function pointer: void(int16_t* mono, int length, void* user).
- `user`: User data passed to the callback.

Called after bitcrush on the final mono buffer. Runs in audio thread context.

### `void setVoicePulseDutyMode(int slot, uint8_t nesDutyIndex)`

**Description:**

Set the PULSE duty mode for a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range ignored.
- `nesDutyIndex`: 0..3 for NES discrete duty, 4..254 treated as
                    continuous (defensive), 255 for continuous.

### `void setVoiceNesNoiseLutMode(int slot, uint8_t lutIndex)`

**Description:**

Set the NES NOISE period LUT mode for a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range ignored.
- `lutIndex`: 0..15 to select a LUT entry; 16..254 are invalid
                and stored as 255; 255 = derive from frequency.

### `void setHpfCutoffHz(float cutoffHz)`

**Description:**

Set the master high-pass (DC blocker) corner frequency.

**Parameters:**

- `cutoffHz`: Corner in Hz. Values <= 0 bypass the filter
                entirely. Clamped internally so the recursive
                coefficient always stays inside (0, 1).

### `void setSoftClipMode(SoftClipMode mode)`

**Description:**

Set the master output shaping curve.

**Parameters:**

- `mode`: Curve to apply. Unknown values are ignored.

### `void setVoiceNesOptions(int slot, const VoiceNesOptions& opts)`

**Description:**

Apply a whole NES option set to a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range ignored.
- `opts`: Options to apply. A default-constructed value disarms
            every NES sub-unit on the voice.

### `VoiceNesOptions getVoiceNesOptions(int slot) const`

**Description:**

Read back a voice's current NES options.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns a
            default-constructed value.

### `void setNesFrameCounterMode(int mode)`

**Description:**

Set the NES frame counter mode.

**Parameters:**

- `mode`: 0 = 4-step (240/120/60 Hz, IRQ at 60 Hz),
            1 = 5-step (~192/~96 Hz, no IRQ).

Mirrors the $4017 write side effects: schedules a timer reset on
the next tick, and clears the IRQ flag. Values other than 0 or 1
are silently ignored. Calling with the current mode is a no-op
(idempotent). Default is OFF — the counter does not tick and
the canonical track PCM is unaffected.

### `void setNesFrameCounterInhibit(bool inhibit)`

**Description:**

Inhibit the frame interrupt flag.

**Parameters:**

- `inhibit`: true = clear the IRQ flag and prevent it from being
                     set on quarter/half clocks.

Mirrors the I bit ($4017 bit 6).

### `bool getNesFrameCounterIrq() const`

**Description:**

Returns the current frame interrupt flag.

**Returns:** true if the IRQ flag is set (only ever true in mode 0).

Mirrors a read of $4015 (but does NOT clear the flag).

### `void setVoiceNesLength(int slot, uint8_t lengthIndex, bool halt)`

**Description:**

Load the NES length counter for a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range ignored.
- `lengthIndex`: 0..31 (NES LUT index). Values > 31 are clamped to 31.
- `halt`: Halt flag: true = freeze the counter (do not decrement).

Mirrors the length load side effect of a write to
$4003/$4007/$400B/$400F. The counter is immediately usable: the
next half-clock tick from the NES frame counter will decrement
from `nes_apu::kNesLengthLut[lengthIndex]`. If the voice's NES
channel enable is clear (`nesLengthCounter.enabled == false`),
this call is a no-op (writes to a disabled NES channel are
silently lost).

Two side effects come with the load, matching the hardware:
the M7 envelope start flag is raised (`$4003`/`$4007`/`$400F`)
and the M5 linear counter reload flag is raised (`$400B`).
Both are set unconditionally; their dispatchers gate on voice
type and on their own opt-in flag, so a voice that did not opt
into those sub-units never observes them.

Thread safety: this is a direct mutation of the voice pool and
is NOT safe to call from a thread other than the audio thread.
Use `AudioCommandType::TRIGGER_NES_LENGTH` for the queued path.

Hito 2 M4; side effects completed in M7 and M13.

### `void setVoiceNesChannelEnabled(int slot, bool enabled)`

**Description:**

Set the NES channel enable for a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range ignored.
- `enabled`: true = channel is on, false = forced silence.

Mirrors the effect of writing to $4015. When cleared, the
length counter is forced to 0 and the voice is silenced
(enabled=false, envelope OFF, currentLevel=0). When set, the
counter can be loaded via `setVoiceNesLength` but no immediate
effect on the counter value is taken.

`remainingSamples` is left untouched (M4 is orthogonal to the
pre-Hito-2 duration mechanism). Hito 2 M4.

### `void setVoiceNesLinearCounter(int slot, uint8_t reloadValue, bool reloadFlag)`

**Description:**

Load the NES linear counter for a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range ignored.
- `reloadValue`: 7-bit reload value (high bits are masked off).
- `reloadFlag`: true = load `reloadValue` into `counter` on the
                  next quarter-clock; cleared automatically if
                  the shared halt/control flag is clear.

Mirrors the linear counter load side effect of a write to
$4008. The values are always stored; the counter only acts on
them when `linearEnabled` is true (i.e. the consumer has opted
in via `setVoiceNesLinearEnabled`). The TRIANGLE voice is the
only consumer in this library.

Hito 2 M5.

### `void setVoiceNesLinearEnabled(int slot, bool enabled)`

**Description:**

Set the NES linear counter enable for a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range ignored.
- `enabled`: true = the linear counter ticks at quarter-frame
                     rate and gates the TRIANGLE sequencer per-sample.

Mirrors the linear counter enable bit ($4008 bit 7, "length
counter / linear counter enable"). When false, the counter is
bypassed entirely (the gate is treated as always open and the
counter does not tick). The TRIANGLE voice is the only consumer
in this library; SINE/SAW/PULSE/NOISE keep it false.

Hito 2 M5.

### `void setVoiceNesEnvelope(int slot, bool loop, bool constVolume, uint8_t volume)`

**Description:**

Configure the NES envelope unit for a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range ignored.
- `loop`: L bit (shared with length counter `halt`). When true,
            the decay level wraps from 0 back to 15 instead of
            silencing the channel. The value is stored in the
            envelope struct, but the live read at quarter-clock
            time comes from `nesLengthCounter.halt` (same bit).
- `constVolume`: C bit. When true, the output is the
                   constant `volume` (VVVV); when false, the
                   output is the decremented `decayLevel`.
- `volume`: VVVV (4 bits, clamped). Constant volume (C=1) or
              divider reload value (C=0).

Mirrors the $4000 / $4004 / $400C write side effects. Does NOT
set the start flag (that comes from `setVoiceNesLength`).
Non-PULSE / non-NOISE voices still have the field written so
callers can introspect the configuration, but the
quarter-clock dispatch skips them.

Hito 2 M7.

### `void setVoiceNesEnvelopeEnabled(int slot, bool enabled)`

**Description:**

Set the NES envelope enable for a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range ignored.
- `enabled`: true = the quarter-frame dispatch ticks the
                     envelope and its `output` replaces the ADSR
                     level as the per-sample volume source
                     (PULSE / NOISE only).

When false, the quarter-frame dispatch is a no-op for this
voice and the ADSR envelope continues to drive the volume.
TRIANGLE / SINE / SAW voices keep this false permanently.

Hito 2 M7.

### `void setVoiceNesSweep(int slot, bool enabled, uint8_t period, bool negate, uint8_t shift, bool isPulse2)`

**Description:**

Configure the NES sweep unit for a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range ignored.
- `enabled`: E bit. When false, the timer-update branch is
                    disabled (but the muting flag is still evaluated —
                    the "NES bug").
- `period`: P (3 bits, clamped to 0..7). Divider period in
              half-clocks. P+1 half-clocks elapse between
              timer updates.
- `negate`: N bit. When true, the target is computed by
              subtracting `change` from the current timer
              (with the PULSE 1/2 formula difference).
- `shift`: SSS (3 bits, clamped to 0..7). Right-shift amount
             for the change calculation. 0 disables the
             timer-update branch.
- `isPulse2`: true = PULSE 2 (two's complement negate:
                     `target = period - change`); false =
                     PULSE 1 (ones' complement negate:
                     `target = period - change - 1`).

Mirrors the $4001 / $4005 write side effect. Sets
`reloadFlag = true` so the next half-clock reloads the
divider counter from `period + 1`. Non-PULSE voices are
silently ignored by the half-clock dispatch (the field is
still updated so callers can introspect the configuration).

Hito 2 M6.

### `void setVoiceNesSweepUnitEnabled(int slot, bool enabled)`

**Description:**

Opt a voice slot into (or out of) the NES sweep unit.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range ignored.
- `enabled`: true to place the voice under NES sweep control.

This is the per-voice participation gate, NOT the `$4001`/`$4005`
E bit — that one is the `enabled` argument of
`setVoiceNesSweep`. Same role as `setVoiceNesChannelEnabled`
(M4), `setVoiceNesLinearEnabled` (M5) and
`setVoiceNesEnvelopeEnabled` (M7).

Default OFF. While it is off the half-clock dispatch skips the
voice entirely and `muted` is held false, so a PULSE voice
driven by the legacy float frequency path keeps playing when
the frame counter is switched on. Toggling this re-evaluates
`muted` immediately in both directions.

Hito 2 M6.

### `void setVoiceNesTimer(int slot, uint16_t timer11)`

**Description:**

Set the 11-bit NES timer period for a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range ignored.
- `timer11`: 11-bit NES timer period (clamped to 0..2047).

Mirrors the $4002 / $4006 / low-3-bits-of-$4003/$4007 timer
load. Updates the per-voice sweep unit's `timer` field, then
re-syncs the float / Q32 phase increments so the oscillator
frequency matches `111860.8 / (timer + 1)` Hz immediately
(the hot path does not need to wait for the next half-clock).

Also recomputes the `muted` flag right away, so the PULSE
output gate sees the correct value before the next
half-clock tick.

Hito 2 M6.

### `void getAndResetProfileStats(ProfileEntry* out, uint8_t& count)`

**Description:**

Reads and clears all profile entries.

**Parameters:**

- `out`: Array of ProfileEntry to fill.
- `count`: On input: max entries. On output: actual count written.

### `size_t countEnabledVoicesForTesting() const`

**Description:**

Test-only: counts voices with enabled==true.

**Returns:** Active voice count in [0, MAX_VOICES].

::: tip
Available only when UNIT_TEST is defined (native_test). Not for game code.
:::

### `size_t getSequencerMainNoteIndexForTesting() const`

**Description:**

Test-only: main music track note index after the last sequencer run.

**Returns:** Index into track 0's note array.

::: tip
Available only when UNIT_TEST is defined (native_test). Not for game code.
:::

### `bool isVoiceEnabledForTesting(int slot) const`

**Description:**

Test-only: reports whether a voice slot is currently synthesizing.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns false.

**Returns:** true if voices[slot].enabled.

::: tip
Available only when UNIT_TEST is defined (native_test). Not for game code.
:::

### `EnvelopeState::Stage getActiveVoiceStageForTesting(WaveType type) const`

**Description:**

Test-only: envelope stage of the first enabled voice of a wave type.

**Parameters:**

- `type`: Wave type to search for.

**Returns:** Envelope stage, or EnvelopeState::Stage::OFF if no match.

### `int getTrackVoiceSlotForTesting(size_t track_index) const`

**Description:**

Test-only: fixed music voice slot when the melodic gate is active, else -1.

**Parameters:**

- `track_index`: Sequencer track [0, MAX_MUSIC_TRACKS).

### `EnvelopeState::Stage getTrackVoiceStageForTesting(size_t track_index) const`

**Description:**

Test-only: envelope stage of a music track's fixed voice slot.

**Parameters:**

- `track_index`: Sequencer track [0, MAX_MUSIC_TRACKS).

**Returns:** Envelope stage, or EnvelopeState::Stage::OFF if disabled/out-of-range.

### `bool isMusicTrackVoiceActiveForTesting(size_t track_index) const`

**Description:**

Test-only: reports whether a music track has an active melodic gate.

**Parameters:**

- `track_index`: Sequencer track [0, MAX_MUSIC_TRACKS); out-of-range returns false.

**Returns:** true after a melodic note until Rest note-off or lifecycle reset.

::: tip
Available only when UNIT_TEST is defined (native_test). Not for game code.
:::

### `uint32_t getVoiceNoisePeriodForTesting(int slot) const`

**Description:**

Test-only: NOISE LFSR period in samples for a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** NOISE LFSR period in samples if slot is valid, 0 otherwise.

### `uint64_t getVoiceRemainingSamplesForTesting(int slot) const`

**Description:**

Test-only: remaining sample gate for a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** remaining samples if slot is valid, 0 otherwise.

### `bool isVoiceLoopForTesting(int slot) const`

**Description:**

Test-only: whether a voice slot is in continuous loop mode.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns false.

**Returns:** true if voice is in loop mode.

### `float getVoiceFrequencyForTesting(int slot) const`

**Description:**

Test-only: current voice frequency in Hz (melodic / noise clock).

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** Frequency in Hz if slot is valid, 0 otherwise.

### `void setVoiceTriangleOctaveUpForTesting(int slot, bool enabled)`

**Description:**

Test-only: pre-set the M16 triangleOctaveUp flag on a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range ignored.
- `enabled`: true to double the effective frequency on the next
               initVoiceFromEvent that targets a TRIANGLE voice.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `float getVoiceDutyCycleForTesting(int slot) const`

**Description:**

Test-only: current PULSE duty cycle [0,1].

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

### `float getVoicePhaseForTesting(int slot) const`

**Description:**

Test-only: current oscillator phase [0,1) for a voice slot.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** Float phase in [0,1), or 0 if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `float getVoiceDutySweepPerSampleForTesting(int slot) const`

**Description:**

Test-only: continuous dutySweep delta per sample (0 when duty stepped).

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

### `void resetNesFrameCounterForTesting()`

**Description:**

Test-only: reset the NES frame counter to APU cycles = 0.

::: tip
Available only when UNIT_TEST is defined. Not for game code.

Mirrors the $4017 write side effect directly: zeros the cycle
accumulator and current step, clears the IRQ flag and the reset
pending flag. Does NOT change mode or irqInhibit.
:::

### `int getNesFrameCounterQuarterTickCountForTesting() const`

**Description:**

Test-only: cumulative count of quarter-frame ticks dispatched
       by the frame counter since the last reset.

### `int getNesFrameCounterHalfTickCountForTesting() const`

**Description:**

Test-only: cumulative count of half-frame ticks dispatched
       by the frame counter since the last reset.

### `int getNesFrameCounterLinearTickCountForTesting() const`

**Description:**

Test-only: cumulative count of linear-counter ticks dispatched
       by the frame counter since the last reset.

### `int getNesFrameCounterSweepTickCountForTesting() const`

**Description:**

Test-only: cumulative count of sweep-unit ticks dispatched
       by the frame counter since the last reset.

### `int getNesFrameCounterIrqSetCountForTesting() const`

**Description:**

Test-only: cumulative count of IRQ-flag false→true transitions
       since the last reset. Does NOT count repeated ticks where the
       flag was already set.

### `double getNesFrameCounterApuCyclesForTesting() const`

**Description:**

Test-only: current APU-cycle accumulator of the frame counter.

### `int getNesFrameCounterCurrentStepForTesting() const`

**Description:**

Test-only: current step of the frame counter (0 = wrap point).

### `uint16_t getVoiceNesLengthCounterForTesting(int slot) const`

**Description:**

Test-only: current `counter` value of a voice's NES length counter.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** Counter value in [0, 254], or 0 if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `uint8_t getVoiceNesLengthIndexForTesting(int slot) const`

**Description:**

Test-only: 0..31 NES LUT index currently loaded on a voice.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** NES length index in [0, 31], or 0 if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `bool getVoiceNesLengthHaltForTesting(int slot) const`

**Description:**

Test-only: current `halt` flag of a voice's NES length counter.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns false.

**Returns:** Halt flag, or false if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `bool getVoiceNesChannelEnabledForTesting(int slot) const`

**Description:**

Test-only: whether a voice's NES channel is enabled ($4015).

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns false.

**Returns:** Channel enable flag, or false if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `uint8_t getVoiceNesLinearCounterForTesting(int slot) const`

**Description:**

Test-only: current `counter` value of a voice's NES linear counter.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** Counter value in [0, 127], or 0 if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `uint8_t getVoiceNesLinearReloadValueForTesting(int slot) const`

**Description:**

Test-only: 7-bit reload value currently loaded on a voice's linear counter.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** Reload value in [0, 127], or 0 if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `bool getVoiceNesLinearReloadFlagForTesting(int slot) const`

**Description:**

Test-only: current `reloadFlag` of a voice's NES linear counter.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns false.

**Returns:** Reload flag, or false if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `bool getVoiceNesLinearEnabledForTesting(int slot) const`

**Description:**

Test-only: current `linearEnabled` flag of a voice's NES linear counter.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns false.

**Returns:** Linear enabled flag, or false if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `uint16_t getVoiceNesSweepTimerForTesting(int slot) const`

**Description:**

Test-only: current 11-bit NES timer period of a voice's sweep unit.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** Timer period in [0, 2047], or 0 if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `uint8_t getVoiceNesSweepDividerForTesting(int slot) const`

**Description:**

Test-only: current divider counter of a voice's sweep unit.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** Divider counter, or 0 if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `bool getVoiceNesSweepEnabledForTesting(int slot) const`

**Description:**

Test-only: E bit of a voice's sweep unit.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns false.

**Returns:** sweepEnabled flag, or false if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `bool getVoiceNesSweepNegateForTesting(int slot) const`

**Description:**

Test-only: N bit (negate) of a voice's sweep unit.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns false.

**Returns:** Negate flag, or false if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `uint8_t getVoiceNesSweepShiftForTesting(int slot) const`

**Description:**

Test-only: SSS (shift) of a voice's sweep unit.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** Shift amount in [0, 7], or 0 if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `bool getVoiceNesSweepMutedForTesting(int slot) const`

**Description:**

Test-only: continuously-evaluated `muted` flag of a voice's sweep unit.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns false.

**Returns:** true if the voice is currently silenced by the sweep muting
        conditions (current period < 8 OR target > 0x7FF), even
        when the sweep is "disabled" (E=0 or SSS=0 — the NES bug).

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `bool getVoiceNesSweepUnitEnabledForTesting(int slot) const`

**Description:**

Test-only: per-voice opt-in flag of the NES sweep unit.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns false.

**Returns:** true if the voice is under NES sweep control. Distinct
        from `getVoiceNesSweepEnabledForTesting`, which reports
        the `$4001`/`$4005` E bit.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `uint8_t getVoiceNesEnvelopeOutputForTesting(int slot) const`

**Description:**

Test-only: current `output` of a voice's NES envelope.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** 0..15 (constVolume ? volume : decayLevel), or 0 if slot invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `uint8_t getVoiceNesEnvelopeDecayLevelForTesting(int slot) const`

**Description:**

Test-only: current `decayLevel` of a voice's NES envelope.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** Decay level in [0, 15], or 0 if slot invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `uint8_t getVoiceNesEnvelopeDividerForTesting(int slot) const`

**Description:**

Test-only: current `dividerCounter` of a voice's NES envelope.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** Divider counter, or 0 if slot invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `bool getVoiceNesEnvelopeStartFlagForTesting(int slot) const`

**Description:**

Test-only: `startFlag` of a voice's NES envelope.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns false.

**Returns:** Start flag, or false if slot invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `bool getVoiceNesEnvelopeEnabledForTesting(int slot) const`

**Description:**

Test-only: `envelopeEnabled` of a voice's NES envelope.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns false.

**Returns:** Envelope enable flag, or false if slot invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `bool getVoiceNesEnvelopeConstVolumeForTesting(int slot) const`

**Description:**

Test-only: `constVolume` of a voice's NES envelope.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns false.

**Returns:** Const volume flag, or false if slot invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `uint8_t getVoiceNesEnvelopeVolumeForTesting(int slot) const`

**Description:**

Test-only: VVVV of a voice's NES envelope (constant volume
       level, and the divider reload value in decay mode).

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 0.

**Returns:** Volume in [0, 15], or 0 if slot is invalid.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `uint8_t getVoiceNesNoiseLutIndexForTesting(int slot) const`

**Description:**

Test-only: current NOISE LUT index of a voice.

**Parameters:**

- `slot`: Voice index [0, MAX_VOICES); out-of-range returns 255.

**Returns:** 0..15 when a LUT entry is armed, 255 when off.

::: tip
Available only when UNIT_TEST is defined. Not for game code.
:::

### `float getHpfRForTesting() const`

**Description:**

Test-only: current HPF feedback coefficient R.

### `int32_t getHpfRQ15ForTesting() const`

**Description:**

Test-only: Q15 mirror of the HPF coefficient.

### `bool getHpfEnabledForTesting() const`

**Description:**

Test-only: false when the HPF is bypassed.

### `float getHpfCutoffHzForTesting() const`

**Description:**

Test-only: requested corner in Hz; negative = legacy coefficient.

### `SoftClipMode getSoftClipModeForTesting() const`

**Description:**

Test-only: current master output shaping curve.

### `void resyncTrackSequencerToTick(size_t track_index, uint64_t target_tick)`

### `void recomputeHpfCoefficients()`

### `void tickAllNesEnvelopes()`

### `void tickAllNesLinearCounters()`

### `void tickAllNesLengthCounters()`

### `void tickAllNesSweepUnits()`

### `void tickNesFrameCounter(double apuCyclesPerSample)`
