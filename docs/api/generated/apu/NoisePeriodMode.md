# NoisePeriodMode

<Badge type="info" text="Enum" />

**Source:** `AudioTypes.h`

## Description

Whether a note keeps the NOISE period it authored (E3/A1).

`Nes` and `Hybrid` arm the NES period LUT (index 0, `$400E = $00`), which
retunes the LFSR clock and therefore discards the note's own
`noisePeriod`. That is the right default for a chip-style lead, and the
wrong one for a curated drum kit: a kick authored with a 60-sample period
becomes one-sample white hiss.

`Note` is the third tier of the D-3 table winning inside its own domain:
the note keeps the period it declared for that note, while the slot is
left free so the next note gets the profile's LUT again. It says nothing
about polarity — `noiseUnipolar` stays profile character, and
`VoiceNesOptions::noiseUnipolar` remains the per-voice opt-out.

`Legacy` already answers "derive the period from the note", so `Note` is
inert there and no frozen fixture moves.

Declared here, next to `EnvelopeCurve`, because the character hook and
the preset both need it. Unlike `EnvelopeCurve` it is deliberately NOT
stored on `AudioChannel`: the request is resolved in `initVoiceFromEvent`
and passed to `applyProfileCharacter`, so no byte of the pinned channel
or core size changes.
