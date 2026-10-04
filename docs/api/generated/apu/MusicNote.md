# MusicNote

<Badge type="info" text="Struct" />

**Source:** `AudioMusicTypes.h`

## Description

Represents a single note in a melody.

For percussion (note.preset && preset.duty == 0):
  - preset defines frequency and defaultDuration
  - octave still determines drum type if preset not available

## Properties

| Name | Type | Description |
|------|------|-------------|
| `duration` | `float` | Sequencer advance in beats (quarter = 1.0; ApuCore TICKS_PER_BEAT = 4). Use 0.0 to fire a stacked hit without advancing tempo (same-step drums). |
