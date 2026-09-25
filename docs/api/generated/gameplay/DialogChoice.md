# DialogChoice

<Badge type="info" text="Struct" />

**Source:** `DialogTypes.h`

## Description

One selectable option on a DialogState::ShowingChoices line.

8 bytes on ESP32 (4-byte pointer), 16 on 64-bit native -- this exact
figure is the regression guard `test_dialog_types_dialog_choice_size_guard`
pins, so growing this struct is a conscious, reviewed change rather
than silent drift in a game's flash budget.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `char` | `const` | Flash literal. Never copied. |
| `next` | `LineId` | kNoLine ends the dialog. |
| `tag` | `uint16_t` | Opaque game code. |
