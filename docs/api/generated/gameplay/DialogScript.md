# DialogScript

<Badge type="info" text="Struct" />

**Source:** `DialogTypes.h`

## Description

The caller-owned, immutable table a DialogRunner is started with.

Lives in .rodata as flash data, never copied and never owned by the
runner: this is what keeps DialogRunner headless and heap-free, since
the runner only ever holds a pointer to a script the game already
allocated statically. Must outlive every DialogRunner started from it.
