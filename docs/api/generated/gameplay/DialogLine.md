# DialogLine

<Badge type="info" text="Struct" />

**Source:** `DialogTypes.h`

## Description

One line of a DialogScript: either shown text or a choice prompt.

20 bytes on ESP32, 32 on 64-bit native. The fields sum to 18 on ESP32
with no padding between them -- next, tag and autoAdvanceMs land on
offsets 8, 10 and 12 and are already aligned. The extra 2 bytes are
trailing padding, rounding the struct to the 4-byte alignment its two
leading pointers impose. This exact figure is
the regression guard
`test_dialog_types_dialog_line_size_guard` pins, so growing this struct
is a conscious, reviewed change rather than silent drift in a game's
flash budget.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `char` | `const` | nullptr for a choice-only line. |
| `next` | `LineId` | LineKind::Text only. |
| `tag` | `uint16_t` | Carried on LineEnter; 0 is a legal tag. |
| `autoAdvanceMs` | `uint16_t` | 0 waits for the player (AwaitingAdvance). |
| `firstChoice` | `ChoiceId` | Index into DialogScript::choices. |
| `choiceCount` | `uint8_t` | Clamped to config::DialogMaxChoices at runtime. |
| `kind` | `LineKind` | Discriminator; decides which fields above apply. |
| `flags` | `uint8_t` | kLineFlagAllowCancel; unknown bits are ignored, not rejected. |
