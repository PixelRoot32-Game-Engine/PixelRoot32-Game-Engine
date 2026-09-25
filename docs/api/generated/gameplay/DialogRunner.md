# DialogRunner

<Badge type="info" text="Class" />

**Source:** `DialogRunner.h`

## Description

Headless five-state dialog machine over a caller-owned, const DialogScript.

No Renderer, no InputManager, no Font: it consumes semantic DialogActions
and knows nothing about pixels, which is what lets Top Down City adopt it
alone, without DialogBox, and keeps the RAM guard below meaningful without
a graphics include in this header. Zero heap allocation in
every path -- feed()/update()/start() only ever mutate this object's own
fixed fields. Table-ownership, function-pointer and packing conventions
are copied from gameplay/StateMachine.h: the caller-owned
script is bound, not copied, and must outlive the runner.

This implementation covers all five states fully, including
ShowingChoices: a Choice line reaches that state, Up/Down move the
selection (clamped, never wrapping), select() sets it directly for touch
hit-testing, Confirm emits ChoiceConfirmed and follows the chosen
DialogChoice::next, and Cancel is honored only when the line's flags
allow it. Advance and None remain no-ops in this state, and Up, Down and
Confirm are no-ops on a line with zero usable choices.

Cancel does not leave the line. An allowed Cancel only emits
DialogEventType::Cancelled: the runner stays in ShowingChoices on the
same line, with the same selection. A game that wants Cancel to close
the dialog calls stop() itself, which is legal from inside the
DialogEventFn that receives Cancelled.

Reentrancy: feed()/update()/start() may call the configured DialogEventFn
synchronously, and that callback is allowed to call back into this same
runner. See configure() below for the exact contract.

Choosing the next line from game state after a choice: a start() made
from inside the DialogEventFn is dropped, so restart the runner from the
game's frame code instead, after feed() returns. Point the choice's
DialogChoice::next at kNoLine, read the highlighted choice BEFORE feeding
Confirm (choice() returns nullptr once the runner has left
ShowingChoices; the pointer addresses the caller-owned script, so it
stays valid afterwards), and start the chosen line if the runner
finished:


```cpp
const DialogChoice* picked = runner.choice(runner.selectedChoice());
runner.feed(DialogAction::Confirm);
if (picked != nullptr && picked->tag == kTagBuy &&
    runner.state() == DialogState::Finished) {
    runner.start(kShopScript, canAfford() ? kLineThanks : kLineNoMoney);
}
```


Within that frame the events arrive in this order: ChoiceConfirmed and
Ended from the choice line, then LineEnter from the started line. The
runner is Finished only between feed() and start(), so a presenter that
draws after this code never sees a frame without a current line. If the
callback called stop() on ChoiceConfirmed, state() is Inactive and the
restart is skipped.

## Methods

### `void configure(void* owner, DialogEventFn onEvent)`

**Description:**

Binds the event sink invoked for every DialogEvent.

**Parameters:**

- `owner`: Opaque pointer forwarded uncast to every callback; may be null.
- `onEvent`: Callback invoked synchronously from feed()/update()/start();
       a null callback silently drops every event.

Reentrancy contract: `onEvent` is free to read anything on this
runner, but a call it makes back into feed(), update() or start() on
this same runner is ignored -- not queued, not run after the
in-progress call finishes, simply dropped. Without this, a script
whose `DialogLine::next` values form a cycle, paired with a callback
that reacts to `LineEnter` by feeding another action, would recurse
without a bound and overflow the stack on ESP32. stop() is the one
exception: it never emits and cannot recurse, so it remains legal
(and useful, e.g. to abort a dialog from inside a tag handler) to
call from within `onEvent`.

Because a start() made from `onEvent` is dropped, a callback cannot
choose the next line from game state after a choice. The class
description shows the supported same-frame restart pattern.

### `bool start(const DialogScript& script, LineId first = 0)`

**Description:**

Binds `script` and enters `first`.

**Parameters:**

- `script`: Caller-owned, const, .rodata-resident script table. NOT
       copied; must outlive this runner.
- `first`: Line to enter first. Defaults to 0.

**Returns:** false when `script.lines` is null, `script.lineCount` is 0,
        or `first` is out of range -- the runner is left fully
        Inactive: state(), currentLineId() and currentLine() all
        report "not on a line", even if a PRIOR successful start()
        had it pointing at a different script.

        ALSO false, but with NO effect on this runner at all, when
        called reentrantly from within the configured DialogEventFn
        (see configure()): the outer call in flight owns the session
        and must not be torn down under it. A bool cannot express
        three outcomes, so the return alone does not separate
        "rejected" from "ignored, still running" -- only the caller
        can, since a reentrant call is reachable only from code that
        already knows it is mid-dispatch. Deliberate limit, not an
        oversight. To pick the next line from game state after a
        choice, restart from frame code once feed() returns; the
        class description shows that pattern.

        Returns true otherwise, after entering `first` (which itself
        may finish immediately if `first`'s kind is LineKind::End).

### `void stop()`

**Description:**

Returns to Inactive. Fires no event.

### `void feed(DialogAction action)`

**Description:**

Applies one semantic action.

**Parameters:**

- `action`: The action to apply.

Total over DialogState x DialogAction: an
action illegal in the current state is silently ignored -- no state
change, no revision() bump, no event, no crash. Confirm aliases
Advance in ShowingText and AwaitingAdvance. In ShowingChoices: Up/Down
move selectedChoice() (clamped at the boundaries, never wrapping;
revision() bumps only when the selection actually changed), Confirm
emits ChoiceConfirmed and follows the chosen DialogChoice::next
(kNoLine finishes the dialog), Cancel is honored only when the line's
flags allow it (see DialogTypes.h's kLineFlagAllowCancel), and
Advance/None stay no-ops. The selection clamps rather than wraps
because config::DialogMaxChoices caps a line at 4 options and
DialogBox draws them all at once: wrapping would buy no reach the
player does not already have, while clamping makes "I am at the end
of the list" unambiguous and keeps select()'s contract monotonic. An honored Cancel only emits Cancelled and
leaves the runner on the line; closing the dialog is the game's call
to stop(). Up, Down and Confirm are no-ops on a line with zero usable
choices -- there is nothing to move to or confirm -- while an allowed
Cancel still emits Cancelled there. A reentrant call made from within
the configured DialogEventFn is also ignored -- see configure()'s
reentrancy contract.

### `void update(unsigned long deltaTimeMs)`

**Description:**

Ticks the per-line auto-advance timer.

**Parameters:**

- `deltaTimeMs`: Milliseconds elapsed since the previous update()
       call; accumulates saturating at UINT32_MAX.

No-op unless state() == ShowingText -- AwaitingAdvance and
ShowingChoices wait for feed(), never the clock. Also a no-op,
dropping `deltaTimeMs` entirely, when called reentrantly from within
the configured DialogEventFn (see configure()).

### `void setPageCount(uint8_t pageCount)`

**Description:**

Declares how many pages the CURRENT line's text occupies.

**Parameters:**

- `pageCount`: Total pages for the current line; 0 is treated as 1.

The runner is headless and cannot derive this itself:
the presenter (DialogBox, or the game) supplies it after wrapping.
Resets to 1 on every line entry, so a runner with no presenter
behaves as exactly one page per line. Clamps the current page into
range and bumps revision() only when something actually changed.

No-op, entirely, when there is no current line (Inactive or
Finished) -- there is nothing to page and nothing player-visible
changes, so revision() correctly does not bump either. Calling this
on a fresh or a just-finished runner is otherwise easy to reach (an
async text-wrap result landing a frame after the player advances
past the last line is the realistic trigger) and must not look like
a redraw-worthy change to a presenter polling revision().

### `bool isActive() const`

**Description:**

Whether the runner is on an active line.

**Returns:** true in ShowingText, AwaitingAdvance or ShowingChoices; false
        in Inactive or Finished.

### `DialogState state() const`

**Description:**

The runner's current state.

**Returns:** The current DialogState.

### `const DialogLine* currentLine() const`

**Description:**

The current line's data.

**Returns:** Pointer to the current DialogLine, or nullptr when the
        runner is not on one (Inactive, Finished, or a bad id).

### `LineId currentLineId() const`

**Description:**

The current line's id.

**Returns:** The current LineId, or kNoLine when not on a line.

### `uint8_t page() const`

**Description:**

The current page of the current line.

**Returns:** The zero-based page index.

### `uint8_t pageCount() const`

**Description:**

The total page count of the current line.

**Returns:** The value last supplied via setPageCount(), or 1 by default.

### `uint16_t revision() const`

**Description:**

A change counter, incremented whenever anything player-visible
       changes (line, page, or the selected choice).

**Returns:** The counter's current value.

WRAPS: uint16_t, roughly 18 minutes of per-frame bumps at 60 FPS.
COMPARE BY INEQUALITY ONLY (`a != b`); never order it (`<`, `>`,
subtraction) -- after a wrap the ordering is meaningless.

### `uint8_t choiceCount() const`

**Description:**

The current line's effective choice count.

**Returns:** 0 unless state() == ShowingChoices. Otherwise the current
        line's DialogLine::choiceCount, clamped to
        config::DialogMaxChoices and further clamped so the
        addressed range never leaves DialogScript::choices and never
        reaches the kNoChoice sentinel value (see DialogTypes.h for
        why a line can never address a choice at or past index 255).
        0 whenever any of those clamps leaves nothing usable, e.g. a
        DialogLine::firstChoice that is itself out of range or equal
        to kNoChoice.

        WHY CLAMP RATHER THAN REJECT THE SCRIPT IN start(): a
        malformed choice range is a per-line authoring error, and
        start() may be asked to run a script long before the offending
        line is ever reached. Failing the whole script there shows the
        player nothing at all, on a device with no console to explain
        why; clamping degrades exactly that one line to "no usable
        choices" and leaves the rest of the script playable. Keeping
        all three clamps in this one function is also what stops the
        bounds policy from being split across two places that can
        disagree.

### `const DialogChoice* choice(ChoiceId index) const`

**Description:**

The choice at `index` on the current ShowingChoices line.

**Parameters:**

- `index`: Zero-based index, local to the current line (not an
       offset into DialogScript::choices).

**Returns:** nullptr when state() != ShowingChoices or `index >=
        choiceCount()`. Never dereferences DialogScript::choices
        outside the range choiceCount() already bounds.

### `ChoiceId selectedChoice() const`

**Description:**

The currently selected choice on a ShowingChoices line.

**Returns:** selected_, or kNoChoice when state() != ShowingChoices or the
        current line has zero usable choices (choiceCount() == 0).
        Reset to 0 on entering a ShowingChoices line with at least
        one usable choice, and to kNoChoice on entering any other
        line, or a ShowingChoices line with none.

### `bool select(ChoiceId index)`

**Description:**

Sets the selection directly, for touch hit-testing.

**Parameters:**

- `index`: Zero-based index, local to the current line.

**Returns:** false, changing nothing, when state() != ShowingChoices or
        `index >= choiceCount()`. Both rejection causes collapse to
        the same single postcondition -- "no effect" -- so this
        bool's false has exactly one meaning, unlike start()'s two
        reentrancy-dependent outcomes (see start() above).

        Returns true otherwise, including when `index` already equals
        the current selection; revision() bumps only when the
        selection actually changed, not on every successful call.

        Callable from within the configured DialogEventFn -- it
        never calls emit(), so it cannot recurse -- and the
        selection it sets holds for exactly as long as the runner
        stays on the same line. Two things end that before the
        dispatching call (feed(), update() or start()) returns: a
        stop() the callback makes itself, from any event, and
        ChoiceConfirmed, which fires while the runner is still on
        the line it is about to leave -- the transition after it
        either enters the chosen next line, which resets the
        selection, or finishes, which leaves the state every choice
        accessor gates on. So drive a touch hit-test from LineEnter,
        not from ChoiceConfirmed.
