# SoundProfile

<Badge type="info" text="Enum" />

**Source:** `AudioTypes.h`

## Description

Named character sets applied to every voice.

`Legacy` is the frozen compatibility contract: it reproduces the 2.0.0
sound. `Nes` and `Hybrid` select profile-owned character from E1 onwards
(envelope curve, weighted mix, unipolar noise, stepped duty); the profile
table in `docs/ABI.md` says what each one owns at a given phase.
