/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "gameplay/DialogRunner.h"

#if PIXELROOT32_ENABLE_DIALOG

#include <cstdint>

namespace pixelroot32::gameplay {

namespace {

/// Sets a bool true for its lifetime and always restores its PRIOR value on
/// scope exit, covering every return path of the function it guards without
/// a try/finally -- unavailable here since the engine builds -fno-exceptions.
/// Zero heap, zero virtual dispatch: a reference and a bool copy, nothing
/// this class doesn't already have on the stack.
class ScopedDispatchGuard {
public:
    explicit ScopedDispatchGuard(bool& flag) : flag_(flag), previous_(flag) { flag_ = true; }
    ~ScopedDispatchGuard() { flag_ = previous_; }
    ScopedDispatchGuard(const ScopedDispatchGuard&) = delete;
    ScopedDispatchGuard& operator=(const ScopedDispatchGuard&) = delete;

private:
    bool& flag_;
    bool previous_;
};

}  // namespace

void DialogRunner::configure(void* owner, DialogEventFn onEvent) {
    owner_ = owner;
    onEvent_ = onEvent;
}

bool DialogRunner::start(const DialogScript& script, LineId first) {
    if (dispatching_) return false;  // Reentrant from inside onEvent: ignored.
    ScopedDispatchGuard guard(dispatching_);

    if (script.lines == nullptr || script.lineCount == 0 || first >= script.lineCount) {
        // Reset fully, mirroring stop()/finish(): a rejected script must not
        // leave the runner pointing at whatever a PRIOR successful start()
        // left active -- currentLineId()/currentLine() must report "not on
        // a line", not a stale id into a script the caller may now free.
        // revision() only bumps when this call actually detached a
        // previously-active session; calling this repeatedly on an
        // already-Inactive runner must not look like a visible change.
        const bool wasAttached = (state_ != DialogState::Inactive) || (current_ != kNoLine);
        state_ = DialogState::Inactive;
        script_ = nullptr;
        current_ = kNoLine;
        selected_ = kNoChoice;
        page_ = 0;
        pageCount_ = 1;
        if (wasAttached) ++revision_;
        return false;
    }

    script_ = &script;
    enterLine(first);
    return true;
}

void DialogRunner::stop() {
    // Teardown only (mirrors StateMachine::reset): fires nothing, so it is
    // safe to call even when the owner is already gone.
    //
    // Defensive callers invoke this without checking isActive() first, so
    // revision() must only bump when there was an actual session to detach
    // -- the same wasAttached guard start()'s failure path and
    // setPageCount() already apply. Otherwise a no-op stop() on an idle
    // runner would look redraw-worthy to a presenter polling revision().
    const bool wasAttached = (state_ != DialogState::Inactive) || (current_ != kNoLine);
    state_ = DialogState::Inactive;
    current_ = kNoLine;
    selected_ = kNoChoice;
    page_ = 0;
    pageCount_ = 1;
    if (wasAttached) ++revision_;
}

void DialogRunner::feed(DialogAction action) {
    if (dispatching_) return;  // Reentrant from inside onEvent: ignored.
    ScopedDispatchGuard guard(dispatching_);

    switch (state_) {
        case DialogState::ShowingText:
        case DialogState::AwaitingAdvance:
            if (action == DialogAction::Advance || action == DialogAction::Confirm) {
                resolveAdvance();
            }
            break;

        case DialogState::ShowingChoices:
            // Entry only: Up/Down/Confirm/Cancel handling for ShowingChoices
            // is not implemented yet; every action here is a deliberate
            // no-op until it is.
            break;

        case DialogState::Inactive:
        case DialogState::Finished:
        default:
            break;
    }
}

void DialogRunner::update(unsigned long deltaTimeMs) {
    if (dispatching_) return;  // Reentrant from inside onEvent: ignored.
    if (state_ != DialogState::ShowingText) return;
    ScopedDispatchGuard guard(dispatching_);

    // Saturating add (mirrors StateMachine::update): the timer "gets stuck
    // at maximum" rather than wrapping, which would never satisfy an `>=`
    // time guard.
    const uint32_t dt = static_cast<uint32_t>(deltaTimeMs);
    timeInLineMs_ = (timeInLineMs_ > UINT32_MAX - dt) ? UINT32_MAX : timeInLineMs_ + dt;

    const DialogLine& line = script_->lines[current_];
    if (timeInLineMs_ >= line.autoAdvanceMs) {
        resolveAdvance();
    }
}

void DialogRunner::setPageCount(uint8_t pageCount) {
    if (current_ == kNoLine) return;  // No current line: nothing to page,
                                       // nothing player-visible to bump
                                       // revision() for (Inactive/Finished).

    const uint8_t newCount = (pageCount == 0) ? 1 : pageCount;
    bool changed = false;

    if (newCount != pageCount_) {
        pageCount_ = newCount;
        changed = true;
    }
    if (page_ >= pageCount_) {
        page_ = static_cast<uint8_t>(pageCount_ - 1);
        changed = true;
    }
    if (changed) ++revision_;
}

bool DialogRunner::isActive() const {
    return state_ == DialogState::ShowingText || state_ == DialogState::AwaitingAdvance ||
           state_ == DialogState::ShowingChoices;
}

const DialogLine* DialogRunner::currentLine() const {
    if (current_ == kNoLine || script_ == nullptr || current_ >= script_->lineCount) {
        return nullptr;
    }
    return &script_->lines[current_];
}

void DialogRunner::enterLine(LineId id) {
    if (id == kNoLine || script_ == nullptr || id >= script_->lineCount) {
        finish(kNoLine);
        return;
    }

    const DialogLine& line = script_->lines[id];

    current_ = id;
    page_ = 0;
    pageCount_ = 1;
    timeInLineMs_ = 0;
    selected_ = kNoChoice;

    // state_ is assigned BEFORE emit() below, for every kind including
    // End, so a callback that reacts to LineEnter by reading state()
    // always sees a value consistent with current_ already pointing at
    // this line -- never the previous line's state with the new line's id.
    switch (line.kind) {
        case LineKind::Choice:
            state_ = DialogState::ShowingChoices;
            break;
        case LineKind::End:
            state_ = DialogState::Finished;
            break;
        case LineKind::Text:
        default:
            state_ = (line.autoAdvanceMs > 0) ? DialogState::ShowingText
                                               : DialogState::AwaitingAdvance;
            break;
    }

    ++revision_;
    emit(DialogEventType::LineEnter, id, kNoChoice, line.tag);

    // The callback may have called stop() -- the only mutator reachable
    // during dispatch, since feed()/update()/start() are no-ops here. If
    // it ran, finish() must NOT, or it would resurrect the session stop()
    // just tore down: Finished again, a second revision() bump, and an
    // Ended event stop() promises never fires. `current_ == id` is exact:
    // stop() is the only thing that can move current_ while dispatching_,
    // and it always moves it to kNoLine, never to another real line id.
    if (line.kind == LineKind::End && current_ == id) {
        finish(id);
    }
}

void DialogRunner::resolveAdvance() {
    if (static_cast<uint8_t>(page_ + 1) < pageCount_) {
        ++page_;
        timeInLineMs_ = 0;
        ++revision_;
        return;
    }

    const DialogLine& line = script_->lines[current_];
    if (line.next == kNoLine) {
        finish(current_);
    } else {
        enterLine(line.next);
    }
}

void DialogRunner::finish(LineId at) {
    uint16_t tag = 0;
    if (at != kNoLine && script_ != nullptr && at < script_->lineCount) {
        tag = script_->lines[at].tag;
    }

    state_ = DialogState::Finished;
    current_ = kNoLine;
    ++revision_;
    emit(DialogEventType::Ended, at, kNoChoice, tag);
}

void DialogRunner::emit(DialogEventType type, LineId line, ChoiceId choice, uint16_t tag) const {
    if (onEvent_ == nullptr) return;
    const DialogEvent event{type, line, choice, tag};
    onEvent_(owner_, event);
}

}  // namespace pixelroot32::gameplay

#endif  // PIXELROOT32_ENABLE_DIALOG
