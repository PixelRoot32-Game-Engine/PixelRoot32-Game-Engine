# DialogAction

<Badge type="info" text="Enum" />

**Source:** `DialogTypes.h`

## Description

The semantic input vocabulary DialogRunner::feed() accepts.

Deliberately abstracted away from any physical input (touch tap, D-pad,
button) so the same runner works unmodified whether a game drives it
from touch (Chess) or buttons (Top Down City); the game translates its
own input into one of these actions before calling feed().
