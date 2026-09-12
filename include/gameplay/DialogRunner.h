/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "platforms/PlatformDefaults.h"
#if PIXELROOT32_ENABLE_DIALOG
#include "gameplay/DialogTypes.h"

namespace pixelroot32::gameplay {

/**
 * @class DialogRunner
 * @brief Headless five-state dialog machine over a caller-owned, const DialogScript.
 *
 * No Renderer, no InputManager, no Font: it consumes semantic DialogActions
 * and knows nothing about pixels, which is what lets Top Down City adopt it
 * alone, without DialogBox, and keeps the RAM guard below meaningful without
 * a graphics include in this header. Zero heap allocation in
 * every path -- feed()/update()/start() only ever mutate this object's own
 * fixed fields. Table-ownership, function-pointer and packing conventions
 * are copied from gameplay/StateMachine.h: the caller-owned
 * script is bound, not copied, and must outlive the runner.
 *
 * This implementation covers Inactive, ShowingText,
 * AwaitingAdvance and Finished fully, plus entry only into ShowingChoices --
 * a Choice line reaches that state and every DialogAction fed there is a
 * deliberate no-op. The four choice accessors (choiceCount(), choice(),
 * selectedChoice(), select()) and ShowingChoices' action handling are
 * additive and land later; nothing declared here changes shape or meaning
 * when they do.
 */
class DialogRunner {
public:
    /**
     * @brief Binds the event sink invoked for every DialogEvent.
     * @param owner Opaque pointer forwarded uncast to every callback; may be null.
     * @param onEvent Callback invoked synchronously from feed()/update()/start();
     *        a null callback silently drops every event.
     */
    void configure(void* owner, DialogEventFn onEvent);

    /**
     * @brief Binds `script` and enters `first`.
     * @param script Caller-owned, const, .rodata-resident script table. NOT
     *        copied; must outlive this runner.
     * @param first Line to enter first. Defaults to 0.
     * @return false, leaving the runner Inactive, when `script.lines` is
     *         null, `script.lineCount` is 0, or `first` is out of range;
     *         true otherwise, after entering `first` (which itself may
     *         finish immediately if `first`'s kind is LineKind::End).
     */
    bool start(const DialogScript& script, LineId first = 0);

    /**
     * @brief Returns to Inactive. Fires no event.
     *
     * Teardown only -- dispatching an event here could run through an owner
     * that is already gone, the same reasoning as StateMachine::reset().
     */
    void stop();

    /**
     * @brief Applies one semantic action.
     * @param action The action to apply.
     *
     * Total over DialogState x DialogAction: an
     * action illegal in the current state is silently ignored -- no state
     * change, no revision() bump, no event, no crash. Confirm aliases
     * Advance in ShowingText and AwaitingAdvance.
     */
    void feed(DialogAction action);

    /**
     * @brief Ticks the per-line auto-advance timer.
     * @param deltaTimeMs Milliseconds elapsed since the previous update()
     *        call; accumulates saturating at UINT32_MAX.
     *
     * No-op unless state() == ShowingText -- AwaitingAdvance and
     * ShowingChoices wait for feed(), never the clock.
     */
    void update(unsigned long deltaTimeMs);

    /**
     * @brief Declares how many pages the CURRENT line's text occupies.
     * @param pageCount Total pages for the current line; 0 is treated as 1.
     *
     * The runner is headless and cannot derive this itself:
     * the presenter (DialogBox, or the game) supplies it after wrapping.
     * Resets to 1 on every line entry, so a runner with no presenter
     * behaves as exactly one page per line. Clamps the current page into
     * range and bumps revision() only when something actually changed.
     */
    void setPageCount(uint8_t pageCount);

    /**
     * @brief Whether the runner is on an active line.
     * @return true in ShowingText, AwaitingAdvance or ShowingChoices; false
     *         in Inactive or Finished.
     */
    [[nodiscard]] bool isActive() const;

    /**
     * @brief The runner's current state.
     * @return The current DialogState.
     */
    [[nodiscard]] DialogState state() const { return state_; }

    /**
     * @brief The current line's data.
     * @return Pointer to the current DialogLine, or nullptr when the
     *         runner is not on one (Inactive, Finished, or a bad id).
     */
    [[nodiscard]] const DialogLine* currentLine() const;

    /**
     * @brief The current line's id.
     * @return The current LineId, or kNoLine when not on a line.
     */
    [[nodiscard]] LineId currentLineId() const { return current_; }

    /**
     * @brief The current page of the current line.
     * @return The zero-based page index.
     */
    [[nodiscard]] uint8_t page() const { return page_; }

    /**
     * @brief The total page count of the current line.
     * @return The value last supplied via setPageCount(), or 1 by default.
     */
    [[nodiscard]] uint8_t pageCount() const { return pageCount_; }

    /**
     * @brief A change counter, incremented whenever anything player-visible
     *        changes (line, page, or -- once choice selection is
     *        implemented -- the selected choice).
     * @return The counter's current value.
     *
     * WRAPS: uint16_t, roughly 18 minutes of per-frame bumps at 60 FPS.
     * COMPARE BY INEQUALITY ONLY (`a != b`); never order it (`<`, `>`,
     * subtraction) -- after a wrap the ordering is meaningless.
     */
    [[nodiscard]] uint16_t revision() const { return revision_; }

private:
    void enterLine(LineId id);       ///< Emits LineEnter, sets state, resets page/timer.
    void resolveAdvance();           ///< page+1, else follow `next`, else finish.
    void finish(LineId at);          ///< Emits Ended, state_ = Finished.
    void emit(DialogEventType type, LineId line, ChoiceId choice, uint16_t tag) const;

    const DialogScript* script_ = nullptr;       // 4 / 8
    void* owner_ = nullptr;                      // 4 / 8
    DialogEventFn onEvent_ = nullptr;            // 4 / 8
    uint32_t timeInLineMs_ = 0;                  // 4
    uint16_t revision_ = 0;                      // 2
    LineId current_ = kNoLine;                   // 2
    // Declared and initialized here, deliberately unused until choice
    // selection is implemented -- keeps sizeof(DialogRunner) and the
    // static_assert below stable in the meantime, so a future diff adding
    // selection support carries no layout change to review.
    ChoiceId selected_ = kNoChoice;              // 1
    uint8_t page_ = 0;                           // 1
    uint8_t pageCount_ = 1;                      // 1
    DialogState state_ = DialogState::Inactive;  // 1
};

/// RAM regression guard. Actual: 24 B on ESP32, 40 B on 64-bit native. Zero
/// native slack is deliberate -- adding a field must be a conscious bump of
/// this constant with a note, not a silent drift.
static_assert(sizeof(DialogRunner) <= 3 * sizeof(void*) + 16,
              "DialogRunner exceeds its RAM budget (3*sizeof(void*)+16 bytes); "
              "if this growth is intentional, raise the threshold above and "
              "update the size comment");

} // namespace pixelroot32::gameplay
#endif // PIXELROOT32_ENABLE_DIALOG
