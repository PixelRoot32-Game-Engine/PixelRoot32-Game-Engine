# DialogLine

<Badge type="info" text="Struct" />

**Source:** `DialogTypes.h`

## Description

One line of a DialogScript: either shown text or a choice prompt.

36 bytes on ESP32, 64 on 64-bit native. The pre-portrait fields sum to 18
on ESP32 with no padding between them -- next, tag and autoAdvanceMs land
on offsets 8, 10 and 12 and are already aligned, plus 2 trailing padding
bytes rounding to the 4-byte alignment the two leading pointers impose
(20). The three portrait pointers add 12/24 for 32/56, plus the 1-byte
portraitPaletteSlot (33/57) rounded up to the pointer alignment (36/64).
This exact figure is the regression guard
`test_dialog_types_dialog_line_size_guard` pins, so growing this struct
is a conscious, reviewed change rather than silent drift in a game's
flash budget. Grew from 20/32 when the optional speaker portrait was
added (third post-MVP dialog item), and from 24/40 when 2bpp/4bpp
portraits joined it.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `text` | `const char*` | nullptr for a choice-only line. |
| `speaker` | `const char*` | nullptr means no speaker. |
| `next` | `LineId` | LineKind::Text only. |
| `tag` | `uint16_t` | Carried on LineEnter; 0 is a legal tag. |
| `autoAdvanceMs` | `uint16_t` | 0 waits for the player (AwaitingAdvance). |
| `firstChoice` | `ChoiceId` | Index into DialogScript::choices; must stay below 255. |
| `choiceCount` | `uint8_t` | Clamped to DialogMaxChoices, to the table, and below 255. |
| `kind` | `LineKind` | Discriminator; decides which fields above apply. |
| `flags` | `uint8_t` | kLineFlagAllowCancel, kLineFlagPortraitRight; see above. |
| `portrait` | `const graphics::Sprite*` | Optional speaker portrait, drawn 1:1 at the top of the content area (left by default, right with kLineFlagPortraitRight). Exactly one of the three pointers should be set: a 1bpp graphics::Sprite drawn in DialogBoxStyle::portraitInk, a graphics::Sprite2bpp, or a graphics::Sprite4bpp -- the two multi-color formats draw through portraitPaletteSlot instead of a tint, so one line can carry a higher-detail face than 1bpp allows. When more than one pointer is set, 4bpp wins, then 2bpp, then 1bpp. All three nullptr (the default, so existing 9-value initializers keep compiling) draws no portrait with identical geometry, as does DialogBoxStyle::portraitsEnabled set to false. Flash-resident asset data; never copied, never owned. The runner never reads these fields. |
| `portrait2bpp` | `const graphics::Sprite2bpp*` | 4-color face; same position and priority rules as `portrait` above. |
| `portrait4bpp` | `const graphics::Sprite4bpp*` | 16-color face; same position and priority rules as `portrait` above. |
| `portraitPaletteSlot` | `uint8_t` | Sprite palette slot (0..7) resolving 2bpp/4bpp portrait colors. Ignored for a 1bpp portrait, which uses portraitInk instead. |
