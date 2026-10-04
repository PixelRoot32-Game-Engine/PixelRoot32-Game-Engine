# AudioCommand

<Badge type="info" text="Struct" />

**Source:** `AudioTypes.h`

## Description

Internal command to communicate between game and audio threads.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `masterBitcrushBits` | `uint8_t` | Used when type == SET_MASTER_BITCRUSH (clamped 0–15; 0 = off). |
| `seekOffsetTicks` | `uint64_t` | Used when type == MUSIC_SEEK: elapsed ticks from music play start. |
| `nesLengthIndex` | `uint8_t` | Used when type == TRIGGER_NES_LENGTH: LUT index (clamped 0–31). |
| `nesLengthHalt` | `bool` | Used when type == TRIGGER_NES_LENGTH: halt flag latched on the voice. |
| `nesOptions` | `VoiceNesOptions` | Used when type == SET_NES_OPTIONS: the options to apply. |
