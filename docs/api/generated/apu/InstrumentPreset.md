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
