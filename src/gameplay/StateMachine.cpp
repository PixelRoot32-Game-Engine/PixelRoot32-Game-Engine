/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "gameplay/StateMachine.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE

#include <cassert>
#include <cstdint>

namespace pixelroot32::gameplay {

void StateMachine::configure(void* owner, const State* table, uint8_t stateCount) {
    assert(table != nullptr && "configure: table is null");
    assert(stateCount > 0 && "configure: stateCount is zero");
    owner_ = owner;
    states_ = table;
    stateCount_ = stateCount;
}

const StateMachine::State* StateMachine::findState(StateId id) const {
    for (uint8_t i = 0; i < stateCount_; ++i) {
        if (states_[i].id == id) return &states_[i];
    }
    return nullptr;
}

void StateMachine::start(StateId initialState) {
    assert(!isRunning() && "start: machine already started (call reset() first)");
    if (isRunning()) return;

    const State* row = findState(initialState);
    assert(row != nullptr && "start: initialState not found in the configured table");
    if (!row) return;

    previous_ = kInvalidStateId;
    current_ = initialState;
    timeInStateMs_ = 0;
    if (row->onEnter) row->onEnter(owner_, kInvalidStateId);
}

void StateMachine::update(unsigned long deltaTime) {
    if (!isRunning()) return;

    // Saturating add (design.md D2): a state "gets stuck at maximum" rather
    // than "gets younger", which would never satisfy an `>=` time guard.
    const uint32_t dt = static_cast<uint32_t>(deltaTime);
    timeInStateMs_ = (timeInStateMs_ > UINT32_MAX - dt) ? UINT32_MAX : timeInStateMs_ + dt;

    // Rule 5: at most one onUpdate, for the state current on entry. If it
    // transitions inline, the newly entered state's onUpdate does not also
    // run this call.
    const State* row = findState(current_);
    if (row && row->onUpdate) row->onUpdate(owner_, deltaTime, timeInStateMs_);
}

bool StateMachine::requestState(StateId nextState) {
    if (findState(nextState) == nullptr) return false;               // unknown id
    if (nextState == current_) return true;                          // Rule 1: no-op
    if (inTransition_) { pending_ = nextState; return true; }         // Rule 4: queued

    inTransition_ = true;
    StateId target = nextState;
    uint8_t depth = 0;
    for (; depth < kMaxChainedTransitions; ++depth) {
        pending_ = kInvalidStateId;   // only requests made THIS iteration count

        const StateId from = current_;
        const State* fromRow = (from != kInvalidStateId) ? findState(from) : nullptr;
        if (fromRow && fromRow->onExit) fromRow->onExit(owner_, target);  // sees time-in-state of `from`

        previous_ = from;
        current_ = target;
        timeInStateMs_ = 0;   // reset BEFORE onEnter

        const State* targetRow = findState(target);
        if (targetRow && targetRow->onEnter) targetRow->onEnter(owner_, from);  // sees time-in-state == 0

        if (pending_ == kInvalidStateId) break;                       // settled
        if (pending_ == current_) { pending_ = kInvalidStateId; break; }  // Rule 1 inside the drain
        target = pending_;
    }
    if (depth == kMaxChainedTransitions && transitionOverflows_ < 0xFF) {
        ++transitionOverflows_;   // saturating diagnostic
    }
    pending_ = kInvalidStateId;
    inTransition_ = false;
    return true;
}

void StateMachine::restartState() {
    if (!isRunning()) return;
    assert(!inTransition_ && "restartState: must not be called re-entrantly from onEnter/onExit");
    if (inTransition_) return;

    const StateId self = current_;
    const State* row = findState(self);

    inTransition_ = true;
    if (row && row->onExit) row->onExit(owner_, self);
    previous_ = self;
    current_ = self;
    timeInStateMs_ = 0;
    if (row && row->onEnter) row->onEnter(owner_, self);
    // A transition requested from either callback during a restart is
    // discarded rather than drained — design.md gives restartState() no
    // chained-transition contract, unlike requestState()'s Rule 4.
    pending_ = kInvalidStateId;
    inTransition_ = false;
}

void StateMachine::reset() {
    // Teardown only (Rule 6): fires nothing, so it is safe to call even
    // when the owner is already gone. Configuration (owner_/states_/
    // stateCount_) is left intact so the machine can start() again without
    // a fresh configure() call. transitionOverflows_ is a monotonic
    // diagnostic (mirrors GameplayEventBus::droppedCount_) and is not
    // cleared by reset() either.
    current_ = kInvalidStateId;
    previous_ = kInvalidStateId;
    pending_ = kInvalidStateId;
    timeInStateMs_ = 0;
    inTransition_ = false;
}

}

#endif // PIXELROOT32_ENABLE_GAMEPLAY_STATE_MACHINE
