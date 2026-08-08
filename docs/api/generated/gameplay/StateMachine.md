# StateMachine

<Badge type="info" text="Class" />

**Source:** `StateMachine.h`

## Description

Non-template finite state machine over a caller-owned, `const` state table.

One shared `.cpp` compiles the transition/drain logic once, regardless of
how many actor types use it — the owner is type-erased via `void*`, the
same convention as `InteractionComponent` (design.md D1). The state table
itself is NOT owned or copied by the machine: `configure()` binds a
pointer, so the table must outlive the machine. The convention is a
`static const` array at namespace or class-static scope, which lands in
flash/`.rodata` and costs zero SRAM:

static const StateMachine::State kPlayerStates[] = {
    { onEnterIdle, onUpdateIdle, nullptr, static_cast<StateId>(PlayerState::IDLE) },
    { onEnterRun,  onUpdateRun,  nullptr, static_cast<StateId>(PlayerState::RUN)  },
};

fsm.configure(this, kPlayerStates, kPlayerStateCount);
fsm.start(static_cast<StateId>(PlayerState::IDLE));
Transitions are immediate and synchronous: `requestState()` fully drains
any chained transition requested from `onEnter`/`onExit` before it
returns, without recursing (design.md D3). `getTimeInState()` is a
saturating `uint32_t` millisecond counter, reset to `0` before `onEnter`
runs on every real transition (design.md D2) — never `math::Scalar`,
which is `float` on native and overflows Q16.16 after 32.7 s on the C3.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `onEnter` | `EnterFn` | May be null. |
| `onUpdate` | `UpdateFn` | May be null. |
| `onExit` | `ExitFn` | May be null. |
| `id` | `StateId` | Game enum value, cast at this boundary. |

## Methods

### `void configure(void* owner, const State* table, uint8_t stateCount)`

**Description:**

Binds the machine to a caller-owned state table.

**Parameters:**

- `owner`: Opaque pointer forwarded uncast to every callback.
- `table`: Non-owning pointer to a table that MUST outlive this
       machine (convention: `static const`). NOT copied.
- `stateCount`: Number of rows in `table`.

Lookups match by `State::id`, never by row position — reordering the
table is safe as long as every id it declares is still present.

### `void start(StateId initialState)`

**Description:**

Enters `initialState`, firing only its `onEnter(owner, kInvalidStateId)`.

### `void update(unsigned long deltaTime)`

**Description:**

Accumulates `deltaTime` into time-in-state (saturating), then
       invokes at most one `onUpdate` — that of the state that was
       current when this call was entered.

::: warning
Ordering hazard when transitioning from inside `onUpdate`.
`deltaTime` is added to time-in-state BEFORE `onUpdate` is dispatched,
and `requestState()` resets time-in-state to `0`. So a call that
transitions from within `onUpdate` returns with `getTimeInState() == 0`:
that frame's `deltaTime` was attributed to the state being LEFT, and the
state being entered starts from zero having consumed none of it.

That accounting is deliberate — the time really was spent in the old
state — but it differs from the common hand-rolled shape, where a
`changeState()` zeroes an accumulator and the frame's delta is then
added to the NEW state in the same tick. Code ported from that shape
onto `getTimeInState()` loses one frame's worth of elapsed time per
transition. It is usually invisible (a sub-frame phase shift in an
animation) and therefore easy to ship unnoticed.

If you need "reset, then accumulate this frame" semantics, either call
`requestState()` BEFORE `update()` rather than from inside `onUpdate`,
or keep your own accumulator and reset it from `onEnter`.
:::

### `bool requestState(StateId nextState)`

**Description:**

Requests a transition to `nextState`.

**Returns:** `false` if `nextState` matches no row in the configured table
        (machine left unchanged); `true` otherwise — including the
        no-op case where `nextState == getCurrentState()`.

Immediate and synchronous: the transition, and any further transition
requested from `onEnter`/`onExit`, are fully drained before this call
returns. Never recurses — a chained request made while already
transitioning is queued and drained by the initiating call, up to
`kMaxChainedTransitions` iterations (design.md D3).

### `void restartState()`

**Description:**

Forces a real `onExit`/`onEnter` cycle on the current state and
       resets `getTimeInState()` to `0`.

### `void reset()`

**Description:**

Returns the machine to the un-started state. Fires no callback.

### `StateId getCurrentState() const`

### `StateId getPreviousState() const`

### `uint32_t getTimeInState() const`

### `bool isRunning() const`

### `uint8_t getTransitionOverflowCount() const`
