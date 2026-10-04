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

## Properties

| Name | Type | Description |
|------|------|-------------|
| `nesAccurate` | `bool` | Hito 2 M17. When true, `initVoiceFromEvent` resets the voice's NES sub-unit fields to the canonical silent state from the nesdev APU_basics `@regs` array before the note starts, so NES mode always begins from a known point instead of inheriting whatever the previous note left on that slot. |
| `noiseShortMode` | `bool` | @deprecated Use `noiseLfsrShort`. Will be removed in a future major. |
