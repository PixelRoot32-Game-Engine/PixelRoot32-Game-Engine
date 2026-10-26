# NetParseResult

<Badge type="info" text="Enum" />

**Source:** `NetProtocol.h`

## Description

Outcome of decoding a frame.

The order of checks is fixed: TooShort before BadMagic, magic before
version, version before type, type before flags, flags before the payload
ceiling, and the declared length last. Callers therefore see exactly one
reason per malformed frame.
