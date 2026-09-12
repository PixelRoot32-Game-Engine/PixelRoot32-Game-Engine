/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "gameplay/DialogRunner.h"

#if PIXELROOT32_ENABLE_DIALOG

#include <cstdint>

namespace pixelroot32::gameplay {

void DialogRunner::configure(void* owner, DialogEventFn onEvent) {
    owner_ = owner;
    onEvent_ = onEvent;
}

bool DialogRunner::start(const DialogScript& script, LineId first) {
    if (script.lines == nullptr || script.lineCount == 0 || first >= script.lineCount) {
        state_ = DialogState::Inactive;
        return false;
    }

    script_ = &script;
    enterLine(first);
    return true;
}

void DialogRunner::stop() {
    // Teardown only (mirrors StateMachine::reset): fires nothing, so it is
    // safe to call even when the owner is already gone.
    state_ = DialogState::Inactive;
    current_ = kNoLine;
    selected_ = kNoChoice;
    page_ = 0;
    pageCount_ = 1;
    ++revision_;
}

void DialogRunner::feed(DialogAction action) {
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
    if (state_ != DialogState::ShowingText) return;

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

    current_ = id;
    page_ = 0;
    pageCount_ = 1;
    timeInLineMs_ = 0;
    selected_ = kNoChoice;
    ++revision_;

    const DialogLine& line = script_->lines[id];
    emit(DialogEventType::LineEnter, id, kNoChoice, line.tag);

    switch (line.kind) {
        case LineKind::End:
            finish(id);
            break;
        case LineKind::Choice:
            state_ = DialogState::ShowingChoices;
            break;
        case LineKind::Text:
        default:
            state_ = (line.autoAdvanceMs > 0) ? DialogState::ShowingText
                                               : DialogState::AwaitingAdvance;
            break;
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
