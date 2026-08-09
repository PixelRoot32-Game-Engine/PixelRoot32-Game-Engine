# UISpriteRef

<Badge type="info" text="Struct" />

**Source:** `UISpriteRef.h`

## Description

Non-owning, format-tagged pointer to one sprite plus its draw
       parameters.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `storage` | `Storage` | The referenced sprite. |
| `format` | `UISpriteFormat` | Live member of `storage`. |
| `tint` | `Color` | Mono only; ignored otherwise. |
| `paletteSlot` | `uint8_t` | 2bpp/4bpp only; ignored otherwise. |
