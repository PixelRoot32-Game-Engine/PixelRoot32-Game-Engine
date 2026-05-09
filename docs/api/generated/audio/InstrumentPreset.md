# InstrumentPreset

<Badge type="info" text="Struct" />

**Source:** `AudioMusicTypes.h`

## Description

Defines instrument characteristics for playback.

For melodic instruments (duty > 0):
  - defaultOctave: the base octave for notes
  - duty: duty cycle for PULSE wave (e.g., 0.5, 0.125)

For percussion instruments (duty == 0):
  - defaultOctave: drum type selector (1=Kick, 2=Snare, 3+=Hi-HAT)
  - defaultDuration: fixed duration for each hit (0.0 = use note.duration)
  - noisePeriod: LFSR period for noise channel (0 = calc from frequency, >0 = direct period)

## Methods

### `inline MusicNote makeNote(const InstrumentPreset& preset, Note note, float duration)`

**Description:**

Constructs a MusicNote using the preset's default octave.

**Parameters:**

- `preset`: The instrument preset defining default volume and octave.
- `note`: The musical note to play.
- `duration`: Note duration in seconds.

**Returns:** A MusicNote with the preset's base volume and default octave.

### `inline MusicNote makeNote(const InstrumentPreset& preset, Note note, uint8_t octave, float duration)`

**Description:**

Constructs a MusicNote with a specific octave override.

**Parameters:**

- `preset`: The instrument preset defining default volume and other parameters.
- `note`: The musical note to play.
- `octave`: The octave for this note (overrides preset default).
- `duration`: Note duration in seconds.

**Returns:** A MusicNote with the preset's base volume and specified octave.

### `inline MusicNote makeRest(float duration)`

**Description:**

Constructs a rest (silence) note.

**Parameters:**

- `duration`: Duration of the rest in seconds.

**Returns:** A MusicNote representing silence.

### `inline float instrumentToFrequency(const InstrumentPreset& preset, Note /*note*/, uint8_t /*octave*/)`

**Parameters:**

- `preset`: The instrument preset.
- `note`: The note (ignored for percussion).
- `octave`: The octave (ignored for percussion).

**Returns:** Frequency in Hz.
