/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "platforms/PlatformDefaults.h"
#if PIXELROOT32_ENABLE_DIALOG
#include "gameplay/DialogTypes.h"
#include "platforms/EngineConfig.h"

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
 * This implementation covers all five states fully, including
 * ShowingChoices: a Choice line reaches that state, Up/Down move the
 * selection (clamped, never wrapping), select() sets it directly for touch
 * hit-testing, Confirm emits ChoiceConfirmed and follows the chosen
 * DialogChoice::next, and Cancel is honored only when the line's flags
 * allow it. Advance and None remain no-ops in this state, and every action
 * is a no-op on a line with zero usable choices.
 *
 * Reentrancy: feed()/update()/start() may call the configured DialogEventFn
 * synchronously, and that callback is allowed to call back into this same
 * runner. See configure() below for the exact contract.
 */
class DialogRunner {
public:
    /**
     * @brief Binds the event sink invoked for every DialogEvent.
     * @param owner Opaque pointer forwarded uncast to every callback; may be null.
     * @param onEvent Callback invoked synchronously from feed()/update()/start();
     *        a null callback silently drops every event.
     *
     * Reentrancy contract: `onEvent` is free to read anything on this
     * runner, but a call it makes back into feed(), update() or start() on
     * this same runner is ignored -- not queued, not run after the
     * in-progress call finishes, simply dropped. Without this, a script
     * whose `DialogLine::next` values form a cycle, paired with a callback
     * that reacts to `LineEnter` by feeding another action, would recurse
     * without a bound and overflow the stack on ESP32. stop() is the one
     * exception: it never emits and cannot recurse, so it remains legal
     * (and useful, e.g. to abort a dialog from inside a tag handler) to
     * call from within `onEvent`.
     */
    void configure(void* owner, DialogEventFn onEvent);

    /**
     * @brief Binds `script` and enters `first`.
     * @param script Caller-owned, const, .rodata-resident script table. NOT
     *        copied; must outlive this runner.
     * @param first Line to enter first. Defaults to 0.
     * @return false when `script.lines` is null, `script.lineCount` is 0,
     *         or `first` is out of range -- the runner is left fully
     *         Inactive: state(), currentLineId() and currentLine() all
     *         report "not on a line", even if a PRIOR successful start()
     *         had it pointing at a different script.
     *
     *         ALSO false, but with NO effect on this runner at all, when
     *         called reentrantly from within the configured DialogEventFn
     *         (see configure()): the outer call in flight owns the session
     *         and must not be torn down under it. A bool cannot express
     *         three outcomes, so the return alone does not separate
     *         "rejected" from "ignored, still running" -- only the caller
     *         can, since a reentrant call is reachable only from code that
     *         already knows it is mid-dispatch. Deliberate limit, not an
     *         oversight.
     *
     *         Returns true otherwise, after entering `first` (which itself
     *         may finish immediately if `first`'s kind is LineKind::End).
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
     * Advance in ShowingText and AwaitingAdvance. In ShowingChoices: Up/Down
     * move selectedChoice() (clamped at the boundaries, never wrapping;
     * revision() bumps only when the selection actually changed), Confirm
     * emits ChoiceConfirmed and follows the chosen DialogChoice::next
     * (kNoLine finishes the dialog), Cancel is honored only when the line's
     * flags allow it (see DialogTypes.h's kLineFlagAllowCancel), and
     * Advance/None stay no-ops. Every ShowingChoices action is a no-op on a
     * line with zero usable choices -- there is nothing to move to or
     * confirm. A reentrant call made from within the configured
     * DialogEventFn is also ignored -- see configure()'s reentrancy
     * contract.
     */
    void feed(DialogAction action);

    /**
     * @brief Ticks the per-line auto-advance timer.
     * @param deltaTimeMs Milliseconds elapsed since the previous update()
     *        call; accumulates saturating at UINT32_MAX.
     *
     * No-op unless state() == ShowingText -- AwaitingAdvance and
     * ShowingChoices wait for feed(), never the clock. Also a no-op,
     * dropping `deltaTimeMs` entirely, when called reentrantly from within
     * the configured DialogEventFn (see configure()).
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
     *
     * No-op, entirely, when there is no current line (Inactive or
     * Finished) -- there is nothing to page and nothing player-visible
     * changes, so revision() correctly does not bump either. Calling this
     * on a fresh or a just-finished runner is otherwise easy to reach (an
     * async text-wrap result landing a frame after the player advances
     * past the last line is the realistic trigger) and must not look like
     * a redraw-worthy change to a presenter polling revision().
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
     *        changes (line, page, or the selected choice).
     * @return The counter's current value.
     *
     * WRAPS: uint16_t, roughly 18 minutes of per-frame bumps at 60 FPS.
     * COMPARE BY INEQUALITY ONLY (`a != b`); never order it (`<`, `>`,
     * subtraction) -- after a wrap the ordering is meaningless.
     */
    [[nodiscard]] uint16_t revision() const { return revision_; }

    /**
     * @brief The current line's effective choice count.
     * @return 0 unless state() == ShowingChoices. Otherwise the current
     *         line's DialogLine::choiceCount, clamped to
     *         config::DialogMaxChoices and further clamped so the
     *         addressed range never leaves DialogScript::choices and never
     *         reaches the kNoChoice sentinel value (see DialogTypes.h for
     *         why a line can never address a choice at or past index 255).
     *         0 whenever any of those clamps leaves nothing usable, e.g. a
     *         DialogLine::firstChoice that is itself out of range or equal
     *         to kNoChoice.
     */
    [[nodiscard]] uint8_t choiceCount() const;

    /**
     * @brief The choice at `index` on the current ShowingChoices line.
     * @param index Zero-based index, local to the current line (not an
     *        offset into DialogScript::choices).
     * @return nullptr when state() != ShowingChoices or `index >=
     *         choiceCount()`. Never dereferences DialogScript::choices
     *         outside the range choiceCount() already bounds.
     */
    [[nodiscard]] const DialogChoice* choice(ChoiceId index) const;

    /**
     * @brief The currently selected choice on a ShowingChoices line.
     * @return selected_, or kNoChoice when state() != ShowingChoices or the
     *         current line has zero usable choices (choiceCount() == 0).
     *         Reset to 0 on entering a ShowingChoices line with at least
     *         one usable choice, and to kNoChoice on entering any other
     *         line, or a ShowingChoices line with none.
     */
    [[nodiscard]] ChoiceId selectedChoice() const;

    /**
     * @brief Sets the selection directly, for touch hit-testing.
     * @param index Zero-based index, local to the current line.
     * @return false, changing nothing, when state() != ShowingChoices or
     *         `index >= choiceCount()`. Both rejection causes collapse to
     *         the same single postcondition -- "no effect" -- so this
     *         bool's false has exactly one meaning, unlike start()'s two
     *         reentrancy-dependent outcomes (see start() above).
     *
     *         Returns true otherwise, including when `index` already equals
     *         the current selection; revision() bumps only when the
     *         selection actually changed, not on every successful call.
     *
     *         Legal to call from within the configured DialogEventFn: it
     *         never calls emit() and so cannot recurse, and every other
     *         mutation it performs (selected_, revision_) has no trailing
     *         statement after it that a reentrant call could invalidate.
     */
    bool select(ChoiceId index);

private:
    void enterLine(LineId id);       ///< Emits LineEnter, sets state, resets page/timer.
    void resolveAdvance();           ///< page+1, else follow `next`, else finish.
    void finish(LineId at);          ///< Emits Ended, state_ = Finished.
    void emit(DialogEventType type, LineId line, ChoiceId choice, uint16_t tag) const;

    /// The shared clamp behind choiceCount()/choice()/selectedChoice()'s
    /// zero-usable-choices case: DialogLine::choiceCount bounded by
    /// config::DialogMaxChoices, by what DialogScript::choices actually
    /// holds from `line.firstChoice`, and by the kNoChoice collision guard
    /// (an addressed index may never reach 0xFF). Returns 0 whenever
    /// `line.firstChoice` is itself out of range or equal to kNoChoice.
    [[nodiscard]] uint8_t effectiveChoiceCount(const DialogLine& line) const;

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
    // True for the duration of any feed()/update()/start() call that is
    // still inside its own DialogEventFn dispatch. See configure()'s
    // reentrancy contract for why this exists and what it does. stop() is
    // deliberately NOT gated by this flag: it never calls emit() and so
    // cannot recurse.
    bool dispatching_ = false;                   // 1
};

/// RAM regression guard, re-derived by hand.
/// ESP32: fields sum to 25, +3 padding to 4-byte alignment = 28 B. Was
/// 24 (sum 24, zero padding), so `dispatching_` is real 4-byte growth,
/// not reclaimed slack -- there was none.
/// Native: fields sum to 37, +3 padding to 8-byte alignment = 40 B,
/// unchanged (was sum 36 +4 padding; the new field took 1 of those 4).
/// Both sit exactly at the threshold below, zero slack. A future 1-3
/// byte field still fits native's 3 padding bytes without tripping this
/// assert or the sizeof test guard.
static_assert(sizeof(DialogRunner) <= 3 * sizeof(void*) + 16,
              "DialogRunner exceeds its RAM budget (3*sizeof(void*)+16 bytes); "
              "if this growth is intentional, raise the threshold above and "
              "update the size comment");

/// ChoiceId is a uint8_t and kNoChoice (0xFF) is its "no choice" sentinel,
/// so any real choice index must stay strictly below it. This holds today
/// (DialogMaxChoices is 4) by a wide margin; it exists to fail the build
/// loudly if a future config change ever pushed DialogMaxChoices to 255,
/// rather than letting the runtime clamp in effectiveChoiceCount() paper
/// over a config that can no longer address its own last choice.
static_assert(pixelroot32::platforms::config::DialogMaxChoices < kNoChoice,
              "DialogMaxChoices must stay below the kNoChoice sentinel value");

} // namespace pixelroot32::gameplay
#endif // PIXELROOT32_ENABLE_DIALOG
