# MusicNote

<Badge type="info" text="Struct" />

**Source:** `AudioMusicTypes.h`

## Description

Represents a single note in a melody.

For percussion (note.preset && preset.duty == 0):
  - preset defines frequency and defaultDuration
  - octave still determines drum type if preset not available
