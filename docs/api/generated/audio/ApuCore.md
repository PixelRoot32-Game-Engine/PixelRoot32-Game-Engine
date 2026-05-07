# ApuCore

<Badge type="info" text="Class" />

**Source:** `ApuCore.h`

## Description

Shared NES-style APU core used by every AudioScheduler.

Owns the 4 channels (2x PULSE, 1x TRIANGLE, 1x NOISE), the SPSC command
queue and the music sequencer. Platform-specific schedulers
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
`audio_mixer_lut` which is pre-fitted to the same curve.

## Methods

### `void init(int sampleRate)`

### `bool submitCommand(const AudioCommand& cmd)`

**Returns:** false if the SPSC queue was full.

### `void generateSamples(int16_t* stream, int length)`

### `uint32_t getDroppedCommands() const`

### `void setSequencerNoteLimit(size_t limit)`

### `size_t getSequencerNoteLimit() const`

### `size_t getDeferredNotes() const`

### `bool isMusicPlaying() const`

### `bool isMusicPaused() const`

### `int getSampleRate() const`

### `void reset()`

### `void setPostMixMono(void (*fn)(int16_t* mono, int length, void* user), void* user)`

### `void getAndResetProfileStats(ProfileEntry* out, uint8_t& count)`

### `size_t countEnabledVoicesForTesting() const`

### `size_t getSequencerMainNoteIndexForTesting() const`
