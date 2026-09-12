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

        case DialogState::ShowingChoices: {
            const DialogLine* linePtr = currentLine();
            if (linePtr == nullptr) break;
            const DialogLine& line = *linePtr;
            const uint8_t count = effectiveChoiceCount(line);

            switch (action) {
                case DialogAction::Up:
                    if (count > 0 && selected_ > 0) {
                        --selected_;
                        ++revision_;
                    }
                    break;

                case DialogAction::Down:
                    if (count > 0 && selected_ < static_cast<ChoiceId>(count - 1)) {
                        ++selected_;
                        ++revision_;
                    }
                    break;

                case DialogAction::Confirm: {
                    if (count == 0) break;  // Nothing to confirm.

                    // Capture everything emit() below's callback could
                    // invalidate BEFORE calling it -- the same reasoning as
                    // enterLine()'s trailing finish() gate.
                    const LineId enteredLine = current_;
                    const ChoiceId chosenIndex = selected_;
                    const DialogChoice& chosen =
                        script_->choices[static_cast<uint16_t>(line.firstChoice) + chosenIndex];
                    const LineId nextLine = chosen.next;
                    const uint16_t choiceTag = chosen.tag;

                    emit(DialogEventType::ChoiceConfirmed, enteredLine, chosenIndex, choiceTag);

                    // The callback may have called stop() -- the only
                    // mutator reachable during dispatch (feed()/update()/
                    // start() are dispatching_-gated no-ops), and it always
                    // moves current_ to kNoLine, never to another real line
                    // id. Gate the follow-on transition on the runner
                    // still being on the same line, exactly the predicate
                    // enterLine() uses for its own trailing finish().
                    if (current_ == enteredLine) {
                        if (nextLine == kNoLine) {
                            finish(enteredLine);
                        } else {
                            enterLine(nextLine);
                        }
                    }
                    break;
                }

                case DialogAction::Cancel: {
                    // Explicit mask of the one defined bit, never `flags`
                    // truthiness: an unrelated/reserved bit must not be
                    // read as "allow cancel" (see DialogTypes.h's
                    // kLineFlagAllowCancel doc for why unknown bits are
                    // ignored rather than rejecting the line).
                    if ((line.flags & kLineFlagAllowCancel) == 0) break;

                    // No trailing state mutation after this emit() at all
                    // -- the runner deliberately does not invent a
                    // transition Cancel was not specified to make, so
                    // there is nothing here for a callback's stop() (or
                    // anything else) to race against.
                    emit(DialogEventType::Cancelled, current_, kNoChoice, line.tag);
                    break;
                }

                case DialogAction::Advance:
                case DialogAction::None:
                default:
                    break;
            }
            break;
        }

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

uint8_t DialogRunner::effectiveChoiceCount(const DialogLine& line) const {
    if (script_ == nullptr || script_->choices == nullptr) return 0;

    // kNoChoice collision guard: firstChoice itself at or past the
    // sentinel addresses nothing usable at all. (For any script whose
    // choiceCount also happens to be small, the table-bound check and the
    // maxByIndexLimit arithmetic below independently reach the same
    // answer; this explicit check is what keeps that true even for a
    // script carrying 256+ choices, which is the case those two cannot
    // cover on their own -- see DialogTypes.h's firstChoice/choiceCount
    // doc.)
    if (line.firstChoice >= kNoChoice) return 0;

    // Out of the script's own choices table entirely.
    if (line.firstChoice >= script_->choiceCount) return 0;

    // Never let the addressed range reach kNoChoice (0xFF): the number of
    // indices from firstChoice up to, but excluding, kNoChoice.
    const uint16_t maxByIndexLimit = static_cast<uint16_t>(kNoChoice) - line.firstChoice;
    // Never read past DialogScript::choices.
    const uint16_t maxByScript =
        static_cast<uint16_t>(script_->choiceCount) - line.firstChoice;

    uint16_t count = line.choiceCount;
    if (count > maxByIndexLimit) count = maxByIndexLimit;
    if (count > maxByScript) count = maxByScript;
    if (count > platforms::config::DialogMaxChoices) count = platforms::config::DialogMaxChoices;

    return static_cast<uint8_t>(count);
}

uint8_t DialogRunner::choiceCount() const {
    if (state_ != DialogState::ShowingChoices) return 0;
    const DialogLine* line = currentLine();
    if (line == nullptr) return 0;
    return effectiveChoiceCount(*line);
}

const DialogChoice* DialogRunner::choice(ChoiceId index) const {
    if (state_ != DialogState::ShowingChoices) return nullptr;
    const DialogLine* line = currentLine();
    if (line == nullptr) return nullptr;
    if (index >= effectiveChoiceCount(*line)) return nullptr;
    return &script_->choices[static_cast<uint16_t>(line->firstChoice) + index];
}

ChoiceId DialogRunner::selectedChoice() const {
    if (state_ != DialogState::ShowingChoices) return kNoChoice;
    const DialogLine* line = currentLine();
    if (line == nullptr || effectiveChoiceCount(*line) == 0) return kNoChoice;
    return selected_;
}

bool DialogRunner::select(ChoiceId index) {
    if (state_ != DialogState::ShowingChoices) return false;
    const DialogLine* line = currentLine();
    if (line == nullptr) return false;

    const uint8_t count = effectiveChoiceCount(*line);
    if (count == 0 || index >= count) return false;

    if (selected_ != index) {
        selected_ = index;
        ++revision_;
    }
    return true;
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
            // 0 when the line has at least one usable choice (the common
            // case); stays kNoChoice, already set above, when it has none
            // -- selectedChoice() must never report a real-looking index
            // for a line nothing can be confirmed on.
            selected_ = (effectiveChoiceCount(line) > 0) ? ChoiceId{0} : kNoChoice;
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
