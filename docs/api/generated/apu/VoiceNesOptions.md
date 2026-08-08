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
