/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE

#include <cstdint>

namespace pixelroot32::gameplay {

using StateId = uint8_t;

/**
 * @class StateMachine
 * @brief Non-template finite state machine over a caller-owned, `const` state table.
 *
 * One shared `.cpp` compiles the transition/drain logic once, regardless of
 * how many actor types use it — the owner is type-erased via `void*`, the
 * same convention as `InteractionComponent` (design.md D1). The state table
 * itself is NOT owned or copied by the machine: `configure()` binds a
 * pointer, so the table must outlive the machine. The convention is a
 * `static const` array at namespace or class-static scope, which lands in
 * flash/`.rodata` and costs zero SRAM:
 *
 * @code
 * static const StateMachine::State kPlayerStates[] = {
 *     { onEnterIdle, onUpdateIdle, nullptr, static_cast<StateId>(PlayerState::IDLE) },
 *     { onEnterRun,  onUpdateRun,  nullptr, static_cast<StateId>(PlayerState::RUN)  },
 * };
 *
 * fsm.configure(this, kPlayerStates, kPlayerStateCount);
 * fsm.start(static_cast<StateId>(PlayerState::IDLE));
 * @endcode
 *
 * Transitions are immediate and synchronous: `requestState()` fully drains
 * any chained transition requested from `onEnter`/`onExit` before it
 * returns, without recursing (design.md D3). `getTimeInState()` is a
 * saturating `uint32_t` millisecond counter, reset to `0` before `onEnter`
 * runs on every real transition (design.md D2) — never `math::Scalar`,
 * which is `float` on native and overflows Q16.16 after 32.7 s on the C3.
 */
class StateMachine {
public:
    /// No previous/next state (initial entry, teardown, or "unknown").
    static constexpr StateId kInvalidStateId = 0xFF;

    using EnterFn  = void (*)(void* owner, StateId fromState);
    using UpdateFn = void (*)(void* owner, unsigned long deltaTime, uint32_t timeInStateMs);
    using ExitFn   = void (*)(void* owner, StateId toState);

    /// One row of the caller-owned state table. Pointers first (widest,
    /// strictest alignment), tag last — same packing rule as GameplayEvent (D2).
    struct State {
        EnterFn  onEnter  = nullptr;   ///< May be null.
        UpdateFn onUpdate = nullptr;   ///< May be null.
        ExitFn   onExit   = nullptr;   ///< May be null.
        StateId  id       = 0;         ///< Game enum value, cast at this boundary.
    };

    /**
     * @brief Binds the machine to a caller-owned state table.
     * @param owner Opaque pointer forwarded uncast to every callback.
     * @param table Non-owning pointer to a table that MUST outlive this
     *        machine (convention: `static const`). NOT copied.
     * @param stateCount Number of rows in `table`.
     *
     * Lookups match by `State::id`, never by row position — reordering the
     * table is safe as long as every id it declares is still present.
     */
    void configure(void* owner, const State* table, uint8_t stateCount);

    /**
     * @brief Enters `initialState`, firing only its `onEnter(owner, kInvalidStateId)`.
     *
     * Valid only from the un-started state (debug assert otherwise; a no-op
     * in release builds).
     */
    void start(StateId initialState);

    /**
     * @brief Accumulates `deltaTime` into time-in-state (saturating), then
     *        invokes at most one `onUpdate` — that of the state that was
     *        current when this call was entered.
     *
     * A transition triggered inline by that `onUpdate` does not also run the
     * newly entered state's `onUpdate` this call. No-op when `!isRunning()`.
     *
     * @warning Ordering hazard when transitioning from inside `onUpdate`.
     * `deltaTime` is added to time-in-state BEFORE `onUpdate` is dispatched,
     * and `requestState()` resets time-in-state to `0`. So a call that
     * transitions from within `onUpdate` returns with `getTimeInState() == 0`:
     * that frame's `deltaTime` was attributed to the state being LEFT, and the
     * state being entered starts from zero having consumed none of it.
     *
     * That accounting is deliberate — the time really was spent in the old
     * state — but it differs from the common hand-rolled shape, where a
     * `changeState()` zeroes an accumulator and the frame's delta is then
     * added to the NEW state in the same tick. Code ported from that shape
     * onto `getTimeInState()` loses one frame's worth of elapsed time per
     * transition. It is usually invisible (a sub-frame phase shift in an
     * animation) and therefore easy to ship unnoticed.
     *
     * If you need "reset, then accumulate this frame" semantics, either call
     * `requestState()` BEFORE `update()` rather than from inside `onUpdate`,
     * or keep your own accumulator and reset it from `onEnter`.
     */
    void update(unsigned long deltaTime);

    /**
     * @brief Requests a transition to `nextState`.
     * @return `false` if `nextState` matches no row in the configured table
     *         (machine left unchanged); `true` otherwise — including the
     *         no-op case where `nextState == getCurrentState()`.
     *
     * Immediate and synchronous: the transition, and any further transition
     * requested from `onEnter`/`onExit`, are fully drained before this call
     * returns. Never recurses — a chained request made while already
     * transitioning is queued and drained by the initiating call, up to
     * `kMaxChainedTransitions` iterations (design.md D3).
     */
    bool requestState(StateId nextState);

    /**
     * @brief Forces a real `onExit`/`onEnter` cycle on the current state and
     *        resets `getTimeInState()` to `0`.
     *
     * Distinct from `requestState(getCurrentState())`, which is a no-op —
     * this is the explicitly-named verb for a genuine re-entry.
     */
    void restartState();

    /**
     * @brief Returns the machine to the un-started state. Fires no callback.
     *
     * Teardown only: dispatching `onExit` here could run through an owner
     * that is already gone (e.g. during a scene reset).
     */
    void reset();

    StateId  getCurrentState()  const { return current_; }
    StateId  getPreviousState() const { return previous_; }

    /// Milliseconds since the current state was entered, saturating at
    /// `UINT32_MAX`. Read `update()`'s ordering warning before deriving a
    /// per-frame value (such as an animation index) from this while also
    /// transitioning from inside `onUpdate`.
    uint32_t getTimeInState()   const { return timeInStateMs_; }
    bool     isRunning()        const { return current_ != kInvalidStateId; }

    /// Saturating count of transition chains clamped at kMaxChainedTransitions.
    uint8_t  getTransitionOverflowCount() const { return transitionOverflows_; }

private:
    static constexpr uint8_t kMaxChainedTransitions = 8;

    const State* findState(StateId id) const;

    void*        owner_                = nullptr;
    const State* states_               = nullptr;
    uint32_t     timeInStateMs_        = 0;
    StateId      current_              = kInvalidStateId;
    StateId      previous_             = kInvalidStateId;
    StateId      pending_              = kInvalidStateId;
    uint8_t      stateCount_           = 0;
    bool         inTransition_         = false;
    uint8_t      transitionOverflows_  = 0;
};

}

#endif // PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE
