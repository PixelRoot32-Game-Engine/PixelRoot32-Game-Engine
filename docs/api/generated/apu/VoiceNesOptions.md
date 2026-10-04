# VoiceNesOptions

<Badge type="info" text="Struct" />

**Source:** `AudioTypes.h`

## Description

Every per-voice NES opt-in in one place (Hito 4 M14).

Each field already has a dedicated setter; this bundles them so a
caller can put a voice into (or out of) NES mode in a single atomic
write, and so the whole configuration can cross the game/audio thread
boundary through `AudioCommandType::SET_NES_OPTIONS`. The individual
setters remain the right tool for changing one thing.

All defaults are "off", so a default-constructed instance disarms a
voice completely.

NOTE: `noiseLfsrShort` is deliberately NOT here even though the
original M14 sketch listed it. That flag is owned per NOTE by
`InstrumentPreset` and rewritten by `initVoiceFromEvent` on every
trigger, unlike the per-SLOT modes below. Putting it in this struct
would give one field two owners, and the preset would silently win on
the next note.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `lengthCounterEnabled` | `bool` | Arms the M4 length counter (`$4015` channel enable). |
| `linearCounterEnabled` | `bool` | Arms the M5 linear counter (TRIANGLE only). |
| `envelopeEnabled` | `bool` | Arms the M7 envelope (PULSE / NOISE only). |
| `sweepUnitEnabled` | `bool` | Arms the M6 sweep unit (PULSE only). NOT the `$4001` E bit. |
| `pulseDutyIndex` | `uint8_t` | M2 duty mode: 0..3 select NES patterns, 255 = continuous. |
| `noiseLutIndex` | `uint8_t` | M8 noise period: 0..15 select a LUT entry, 255 = from frequency. |
