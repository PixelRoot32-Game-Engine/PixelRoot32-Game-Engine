# DialogChoice

<Badge type="info" text="Struct" />

**Source:** `DialogTypes.h`

## Description

One selectable option on a DialogState::ShowingChoices line.

12 bytes on ESP32 (two 4-byte pointers), 24 on 64-bit native -- this exact
figure is the regression guard `test_dialog_types_dialog_choice_size_guard`
pins, so growing this struct is a conscious, reviewed change rather
than silent drift in a game's flash budget. Grew from 8/16 when the
optional second-column `detail` literal was added (multi-column option
rows, second post-MVP dialog item).

## Properties

| Name | Type | Description |
|------|------|-------------|
| `text` | `const char*` | Main label, left-aligned. Flash literal. Never copied. |
| `detail` | `const char*` | Optional second column (e.g. a price), right-aligned. Flash literal. nullptr (the default) draws the classic single-column row with identical geometry. Never wrapped, like `text`: keep `text + detail` within the panel's content width (see DialogBoxStyle). Inserted as the second field in 1.12.0, so positional 3-value initializers ({text, next, tag}) no longer compile -- pass an explicit nullptr (or a detail literal) in second position. |
| `next` | `LineId` | kNoLine ends the dialog. |
| `tag` | `uint16_t` | Opaque game code. |
