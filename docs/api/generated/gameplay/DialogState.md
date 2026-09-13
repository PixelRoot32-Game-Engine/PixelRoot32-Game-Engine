# DialogState

<Badge type="info" text="Enum" />

**Source:** `DialogTypes.h`

## Description

The five states DialogRunner can be in; feed() is total over State x DialogAction.

ShowingText and AwaitingAdvance both mean "on a text line" but are kept
distinct so a per-line auto-advance timer (ShowingText) and a
player-driven wait (AwaitingAdvance) are each independently observable
and testable, rather than folding both into one state with a hidden
autoAdvanceMs branch. Finished is kept separate from Inactive so a game
can read the dialog's result on the frame after it ends without racing
a reset back to Inactive.
