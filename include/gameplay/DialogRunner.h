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
 * allow it. Advance and None remain no-ops in this state, and Up, Down and
 * Confirm are no-ops on a line with zero usable choices.
 *
 * Cancel does not leave the line. An allowed Cancel only emits
 * DialogEventType::Cancelled: the runner stays in ShowingChoices on the
 * same line, with the same selection. A game that wants Cancel to close
 * the dialog calls stop() itself, which is legal from inside the
 * DialogEventFn that receives Cancelled.
 *
 * Reentrancy: feed()/update()/start() may call the configured DialogEventFn
 * synchronously, and that callback is allowed to call back into this same
 * runner. See configure() below for the exact contract.
 *
 * Choosing the next line from game state after a choice: a start() made
 * from inside the DialogEventFn is dropped, so restart the runner from the
 * game's frame code instead, after feed() returns. Point the choice's
 * DialogChoice::next at kNoLine, read the highlighted choice BEFORE feeding
 * Confirm (choice() returns nullptr once the runner has left
 * ShowingChoices; the pointer addresses the caller-owned script, so it
 * stays valid afterwards), and start the chosen line if the runner
 * finished:
 *
 * @code
 * const DialogChoice* picked = runner.choice(runner.selectedChoice());
 * runner.feed(DialogAction::Confirm);
 * if (picked != nullptr && picked->tag == kTagBuy &&
 *     runner.state() == DialogState::Finished) {
 *     runner.start(kShopScript, canAfford() ? kLineThanks : kLineNoMoney);
 * }
 * @endcode
 *
 * Within that frame the events arrive in this order: ChoiceConfirmed and
 * Ended from the choice line, then LineEnter from the started line. The
 * runner is Finished only between feed() and start(), so a presenter that
 * draws after this code never sees a frame without a current line. If the
 * callback called stop() on ChoiceConfirmed, state() is Inactive and the
 * restart is skipped.
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
     *
     * Because a start() made from `onEvent` is dropped, a callback cannot
     * choose the next line from game state after a choice. The class
     * description shows the supported same-frame restart pattern.
     */
    void configure(void* owner, DialogEventFn onEvent);

    /**
     * @brief Binds the optional per-choice visibility filter.
     * @param owner Opaque pointer forwarded uncast to every filter call; may be null.
     * @param filter Predicate the runner calls lazily for each candidate
     *        choice whenever it evaluates choiceCount()/choice()/select(),
     *        ShowingChoices feed() input, or refreshChoices(). A null filter
     *        (the default) shows every choice.
     *
     * The filter MUST be pure: it must not call back into this runner
     * (feed()/update()/start()/stop()/select()/refreshChoices() or any
     * accessor). The runner invokes it while evaluating its own accessors,
     * so a reentrant call would recurse; an in-filter guard fails open
     * (shows every choice) rather than recursing, but well-behaved filters
     * never rely on it. Changing the binding is never itself a visible
     * change -- call refreshChoices() afterwards when the visible set may
     * have changed and the runner should renormalize its selection.
     *
     * Zero heap; stores two pointers only.
     */
    void setChoiceFilter(void* owner, ChoiceFilterFn filter);

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
     *         oversight. To pick the next line from game state after a
     *         choice, restart from frame code once feed() returns; the
     *         class description shows that pattern.
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
     * Advance/None stay no-ops. The selection clamps rather than wraps
     * because config::DialogMaxChoices caps a line at 4 options and
     * DialogBox draws them all at once: wrapping would buy no reach the
     * player does not already have, while clamping makes "I am at the end
     * of the list" unambiguous and keeps select()'s contract monotonic. An honored Cancel only emits Cancelled and
     * leaves the runner on the line; closing the dialog is the game's call
     * to stop(). Up, Down and Confirm are no-ops on a line with zero usable
     * choices -- there is nothing to move to or confirm -- while an allowed
     * Cancel still emits Cancelled there. A reentrant call made from within
     * the configured DialogEventFn is also ignored -- see configure()'s
     * reentrancy contract.
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
     *
     *         When a ChoiceFilterFn is bound (see setChoiceFilter), the
     *         count is instead the number of VISIBLE choices -- candidates
     *         the filter hides are compacted out, so every index below the
     *         returned count addresses a visible choice through choice().
     *         A line with every choice hidden reports 0, exactly like a
     *         line with zero usable choices.
     *
     *         WHY CLAMP RATHER THAN REJECT THE SCRIPT IN start(): a
     *         malformed choice range is a per-line authoring error, and
     *         start() may be asked to run a script long before the offending
     *         line is ever reached. Failing the whole script there shows the
     *         player nothing at all, on a device with no console to explain
     *         why; clamping degrades exactly that one line to "no usable
     *         choices" and leaves the rest of the script playable. Keeping
     *         all three clamps in this one function is also what stops the
     *         bounds policy from being split across two places that can
     *         disagree.
     */
    [[nodiscard]] uint8_t choiceCount() const;

    /**
     * @brief The choice at `index` on the current ShowingChoices line.
     * @param index Zero-based index, local to the current line (not an
     *        offset into DialogScript::choices). Under a ChoiceFilterFn
     *        this is an index into the VISIBLE choices, compacted over the
     *        hidden ones -- `line.firstChoice + index` is NOT valid then;
     *        the runner translates.
     * @return nullptr when state() != ShowingChoices or `index >=
     *         choiceCount()`. Never dereferences DialogScript::choices
     *         outside the range choiceCount() already bounds.
     */
    [[nodiscard]] const DialogChoice* choice(ChoiceId index) const;

    /**
     * @brief The currently selected choice on a ShowingChoices line.
     * @return selected_, or kNoChoice when state() != ShowingChoices or the
     *         current line has zero usable choices (choiceCount() == 0).
     *         Under a ChoiceFilterFn this is an index into the VISIBLE
     *         choices; kNoChoice is also returned when the stored selection
     *         no longer addresses a visible choice (the filter hid it since
     *         the selection was made) -- feed()/select()/refreshChoices()
     *         renormalize the stored value, const accessors only report.
     *         Reset to 0 on entering a ShowingChoices line with at least
     *         one usable choice, and to kNoChoice on entering any other
     *         line, or a ShowingChoices line with none.
     */
    [[nodiscard]] ChoiceId selectedChoice() const;

    /**
     * @brief Sets the selection directly, for touch hit-testing.
     * @param index Zero-based index, local to the current line. Under a
     *        ChoiceFilterFn this is an index into the VISIBLE choices.
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
     *         Callable from within the configured DialogEventFn -- it
     *         never calls emit(), so it cannot recurse -- and the
     *         selection it sets holds for exactly as long as the runner
     *         stays on the same line. Two things end that before the
     *         dispatching call (feed(), update() or start()) returns: a
     *         stop() the callback makes itself, from any event, and
     *         ChoiceConfirmed, which fires while the runner is still on
     *         the line it is about to leave -- the transition after it
     *         either enters the chosen next line, which resets the
     *         selection, or finishes, which leaves the state every choice
     *         accessor gates on. So drive a touch hit-test from LineEnter,
     *         not from ChoiceConfirmed.
     */
    bool select(ChoiceId index);

    /**
     * @brief Renormalizes the selection against the current filter result.
     *
     * Call this after game state the bound ChoiceFilterFn reads has
     * changed while a ShowingChoices line is open (e.g. the player's gold
     * changed, hiding the "Buy" option) and before the presenter draws:
     * the selection clamps into the visible range exactly as feed()'s
     * Up/Down would, and revision() bumps only when the selection actually
     * moved. No-op when state() != ShowingChoices; with no filter bound it
     * still clamps defensively but can never find anything to change.
     *
     * Draw paths that call draw() unconditionally do not strictly need
     * this -- draw() re-reads choiceCount()/choice() lazily every frame --
     * but anything polling needsRedraw() does: without a renormalizing
     * call, a filter-driven selection change is not observable through
     * revision().
     */
    void refreshChoices();

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

    /// Whether candidate script choice `scriptIndex` (an offset into
    /// DialogScript::choices, NOT a visible index) is shown. Null filter
    /// shows everything; while already inside a filter call (inFilter_)
    /// also shows everything rather than recursing -- filters must be
    /// pure and never call back into the runner, so this path is only
    /// reachable from a misbehaving filter.
    [[nodiscard]] bool isChoiceVisible(LineId lineId, ChoiceId scriptIndex,
                                       const DialogChoice* choice) const;

    /// Number of visible choices on `line`: the raw effectiveChoiceCount()
    /// range with hidden candidates compacted out. 0 with no filter change
    /// beyond effectiveChoiceCount() itself.
    [[nodiscard]] uint8_t visibleChoiceCount(const DialogLine& line) const;

    /// Translates a visible index (what choiceCount()/choice()/select()/
    /// selectedChoice() and ChoiceConfirmed traffic in) to the underlying
    /// DialogScript::choices offset. Returns kNoChoice when `visible` is
    /// past the visible end.
    [[nodiscard]] ChoiceId scriptIndexForVisible(const DialogLine& line,
                                                 ChoiceId visible) const;

    /// Clamps selected_ into [0, visibleChoiceCount()) -- or to kNoChoice
    /// when the line has no visible choice -- bumping revision() only when
    /// the selection actually moved. Called on entering a ShowingChoices
    /// line, from feed()'s ShowingChoices branch, from select()'s bounds
    /// path and from refreshChoices().
    void normalizeSelection(const DialogLine& line);

    const DialogScript* script_ = nullptr;       // 4 / 8
    void* owner_ = nullptr;                      // 4 / 8
    DialogEventFn onEvent_ = nullptr;            // 4 / 8
    void* filterOwner_ = nullptr;                // 4 / 8
    ChoiceFilterFn filter_ = nullptr;            // 4 / 8
    uint32_t timeInLineMs_ = 0;                  // 4
    uint16_t revision_ = 0;                      // 2
    LineId current_ = kNoLine;                   // 2
    // Index into the current line's choices, local to that line, NOT an
    // offset into DialogScript::choices. kNoChoice whenever the runner is
    // not on a choice line, or is on one with no usable choice. Held
    // within [0, choiceCount()) by enterLine()'s reset, by Up/Down's
    // clamp and by select()'s bounds check; feed()'s Confirm branch
    // depends on that.
    ChoiceId selected_ = kNoChoice;              // 1
    uint8_t page_ = 0;                           // 1
    uint8_t pageCount_ = 1;                      // 1
    DialogState state_ = DialogState::Inactive;  // 1
    // True while a ChoiceFilterFn call made by this runner is in flight.
    // Mutable so the const choice accessors can hold it: a filter that
    // calls back into the runner would otherwise recurse without a bound
    // (const accessors cannot set dispatching_, which only feed()/update()/
    // start() hold). The fail-open read in isChoiceVisible() is the backstop;
    // well-behaved filters never reach it. Deliberately NOT set by feed()/
    // update()/start() themselves -- dispatching_ already covers those, and
    // conflating the two would let a filter run unguarded outside dispatch.
    mutable bool inFilter_ = false;              // 1
    // True for the duration of any feed()/update()/start() call that is
    // still inside its own DialogEventFn dispatch. See configure()'s
    // reentrancy contract for why this exists and what it does. stop() is
    // deliberately NOT gated by this flag: it never calls emit() and so
    // cannot recurse.
    bool dispatching_ = false;                   // 1
};

/// RAM regression guard, re-derived by hand.
/// ESP32: fields sum to 34 (25 + 8 filter pointers + 1 in-filter flag), +2
/// padding to 4-byte alignment = 36 B. Was 28 (sum 25, +3 padding), so the
/// filter binding is real 8-byte growth plus 1 flag byte sharing the new
/// padding -- there was no slack left to reclaim.
/// Native: fields sum to 54 (37 + 16 + 1), +2 padding to 8-byte alignment =
/// 56 B, unchanged in shape (was sum 37 +3 padding; the new fields took the
/// slack and grew past it by the same 16 the pointers cost).
/// Both sit exactly at the threshold below, zero slack. A future pointer
/// field costs 4/8 B again; a future 1-2 byte field still fits ESP32's 2
/// padding bytes without tripping this assert or the sizeof test guard.
static_assert(sizeof(DialogRunner) <= 5 * sizeof(void*) + 16,
              "DialogRunner exceeds its RAM budget (5*sizeof(void*)+16 bytes); "
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
