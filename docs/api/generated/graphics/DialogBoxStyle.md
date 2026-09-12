# DialogBoxStyle

<Badge type="info" text="Struct" />

**Source:** `DialogBox.h`

## Description

Every visual and layout knob DialogBox needs to draw a panel.

Pointer first, tags last (the same field-packing convention
DialogRunner and DialogTypes follow, copied from StateMachine): `font`
leads, the 14 scalar/enum fields follow.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `Font` | `const` | nullptr uses FontManager's default. |
| `lineSpacing` | `uint8_t` | Extra px between wrapped body lines. |
| `fixedPosition` | `bool` | true: setOffsetBypass(true) while drawing, ignoring the camera. |
