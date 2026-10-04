# DialogPortraitSize

<Badge type="info" text="Enum" />

**Source:** `DialogBox.h`

## Description

The closed set of speaker-portrait boxes DialogBox supports.

Values ARE pixel dimensions: a portrait box is square, side =
static_cast&lt;uint8_t>(size). Games author faces at exactly the box size;
a smaller sprite draws 1:1 top-left, a larger one is ignored entirely
(see DialogBoxStyle::portraitSize), so an oversized asset can never
overflow the panel or eat the text column. Deliberately closed --
an open width/height pair would let any size through and reintroduce
the overflow this enum exists to prevent. The renderer has no scaler
or source-rect clip for 2bpp/4bpp, so "draw it smaller" is not an
option: fixed boxes are the whole mechanism.
