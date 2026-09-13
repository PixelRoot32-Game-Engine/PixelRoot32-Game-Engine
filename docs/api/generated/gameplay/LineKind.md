# LineKind

<Badge type="info" text="Enum" />

**Source:** `DialogTypes.h`

## Description

Distinguishes what a DialogLine presents when the runner enters it.

DialogRunner::enterLine() switches on this to pick the entry state: a
`Text` line becomes ShowingText or AwaitingAdvance depending on
DialogLine::autoAdvanceMs, a `Choice` line goes straight to
ShowingChoices, and `End` finishes the dialog. Keeping this a plain
three-value enum -- rather than inferring the kind from which fields
happen to be populated -- makes a malformed script fail loudly instead
of guessing.
