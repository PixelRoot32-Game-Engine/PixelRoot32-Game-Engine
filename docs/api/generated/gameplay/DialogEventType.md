# DialogEventType

<Badge type="info" text="Enum" />

**Source:** `DialogTypes.h`

## Description

Identifies which fields of a DialogEvent are meaningful.

Delivered through the single DialogEventFn callback rather than one
callback per kind, so a game's dialog handler is one switch statement
instead of four separate registrations.
