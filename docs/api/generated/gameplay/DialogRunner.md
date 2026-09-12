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

This implementation covers Inactive, ShowingText,
AwaitingAdvance and Finished fully, plus entry only into ShowingChoices --
a Choice line reaches that state and every DialogAction fed there is a
deliberate no-op. The four choice accessors (choiceCount(), choice(),
selectedChoice(), select()) and ShowingChoices' action handling are
additive and land later; nothing declared here changes shape or meaning
when they do.

Reentrancy: feed()/update()/start() may call the configured DialogEventFn
synchronously, and that callback is allowed to call back into this same
runner. See configure()'s onEvent for the exact contract.

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

### `bool start(const DialogScript& script, LineId first = 0)`

**Description:**

Binds `script` and enters `first`.

**Parameters:**

- `script`: Caller-owned, const, .rodata-resident script table. NOT
       copied; must outlive this runner.
- `first`: Line to enter first. Defaults to 0.

**Returns:** false when `script.lines` is null, `script.lineCount` is 0,
        `first` is out of range, or this call is reentrant (made from
        within `onEvent` -- see configure()). On any false return the
        runner is left fully Inactive: state(), currentLineId() and
        currentLine() all report "not on a line", even if a PRIOR
        successful start() had it pointing at a different script.
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
Advance in ShowingText and AwaitingAdvance. A reentrant call made
from within the configured DialogEventFn is also ignored -- see
configure()'s reentrancy contract.

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
       changes (line, page, or -- once choice selection is
       implemented -- the selected choice).

**Returns:** The counter's current value.

WRAPS: uint16_t, roughly 18 minutes of per-frame bumps at 60 FPS.
COMPARE BY INEQUALITY ONLY (`a != b`); never order it (`<`, `>`,
subtraction) -- after a wrap the ordering is meaningless.
