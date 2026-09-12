/**
 * @file test_gameplay_dialog_runner.cpp
 * @brief Unit tests for gameplay/DialogRunner
 *
 * Covers the dialog-runner capability:
 * - Five-state machine and legal transitions
 * - Linear line chains (DialogLine::next)
 * - Per-line auto-advance
 * - Paging when text exceeds DialogMaxWrappedLines (setPageCount)
 * - Bad LineId clamps instead of indexing out of range
 * - Zero heap, trivially destructible data (heap side, across a full session)
 * - sizeof(DialogRunner) RAM regression guard
 * - The four choice accessors and ShowingChoices' action handling
 *
 * Zero-byte reservation when the flag is disabled is exercised by the #else
 * stub below compiling and passing without referencing DialogRunner at all.
 *
 * The four choice accessors (choiceCount()/choice()/selectedChoice()/
 * select()), ShowingChoices' Up/Down/Confirm/Cancel handling, and
 * DialogEventType::{ChoiceConfirmed,Cancelled} are covered here too. None
 * of the earlier no-op assertions in this file change: a Choice line still
 * reaches ShowingChoices the same way, and Advance/None there are still
 * no-ops -- only Up/Down/Confirm/Cancel gained real behavior.
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_DIALOG is
 * enabled, since DialogRunner is entirely guarded behind that flag (see
 * include/gameplay/DialogRunner.h). This file therefore compiles cleanly in
 * BOTH the default (flag off) and opt-in (flag on) configurations, matching
 * the flags-off / flags-on CI matrix (platformio.ini's native_test vs
 * native_test_gameplay).
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_DIALOG

#include "gameplay/DialogRunner.h"
#include "gameplay/DialogTypes.h"

#include <cstdint>
#include <cstdlib>
#include <new>

using namespace pixelroot32::gameplay;

// =============================================================================
// Static-layout regression guard: 28 B ESP32 (32-bit
// pointer), 40 B on 64-bit native. Mirrors test_dialog_types.cpp's pattern.
// ESP32 moved from 24 to 28 when the dispatching_ reentrancy-guard field was
// added -- see the RAM regression guard comment above DialogRunner's
// static_assert in DialogRunner.h for the full byte-by-byte reconciliation.
// =============================================================================

#ifdef ESP32
static_assert(sizeof(DialogRunner) == 28,
              "DialogRunner must be 28 bytes on ESP32; see the RAM regression "
              "guard comment above DialogRunner's static_assert in "
              "DialogRunner.h if this changed intentionally.");
#else
static_assert(sizeof(DialogRunner) == 40,
              "DialogRunner must be 40 bytes on 64-bit native; see the RAM "
              "regression guard comment above DialogRunner's static_assert "
              "in DialogRunner.h if this changed intentionally.");
#endif

namespace {

// =============================================================================
// Event capture -- fixed-size log, mirrors test_gameplay_state_machine.cpp's
// MockOwner rather than reaching for std::vector, keeping the test harness in
// the same idiom as the rest of the gameplay-framework suites.
// =============================================================================

struct LoggedEvent {
    DialogEventType type;
    LineId           line;
    ChoiceId         choice;
    uint16_t         tag;
};

struct MockOwner {
    static constexpr int kMaxLog = 16;
    LoggedEvent log[kMaxLog]{};
    int         logCount = 0;

    void record(const DialogEvent& event) {
        if (logCount < kMaxLog) {
            log[logCount++] = LoggedEvent{event.type, event.line, event.choice, event.tag};
        }
    }
};

void onDialogEvent(void* owner, const DialogEvent& event) {
    static_cast<MockOwner*>(owner)->record(event);
}

// =============================================================================
// Script fixtures -- caller-owned, const, .rodata-resident; must outlive
// any DialogRunner started from them.
// =============================================================================

constexpr const char* kLineAText = "Line A";
constexpr const char* kLineBText = "Line B";

// Two-line linear chain, both player-driven (autoAdvanceMs == 0).
static const DialogLine kLinearLines[] = {
    {kLineAText, nullptr, /*next*/ 1, /*tag*/ 111, /*autoAdvanceMs*/ 0, 0, 0, LineKind::Text, 0},
    {kLineBText, nullptr, /*next*/ kNoLine, /*tag*/ 222, /*autoAdvanceMs*/ 0, 0, 0, LineKind::Text, 0},
};
static const DialogScript kLinearScript{kLinearLines, nullptr, 2, 0};

// Single line, auto-advances after 2400ms, then ends (next == kNoLine).
static const DialogLine kAutoAdvanceLines[] = {
    {kLineAText, nullptr, /*next*/ kNoLine, /*tag*/ 5, /*autoAdvanceMs*/ 2400, 0, 0, LineKind::Text, 0},
};
static const DialogScript kAutoAdvanceScript{kAutoAdvanceLines, nullptr, 1, 0};

// Auto-advancing line 0 followed by a player-driven line 1 -- lets a test
// observe the auto-advance transition without the session also finishing.
static const DialogLine kAutoAdvanceThenAwaitLines[] = {
    {kLineAText, nullptr, /*next*/ 1, /*tag*/ 1, /*autoAdvanceMs*/ 2400, 0, 0, LineKind::Text, 0},
    {kLineBText, nullptr, /*next*/ kNoLine, /*tag*/ 2, /*autoAdvanceMs*/ 0, 0, 0, LineKind::Text, 0},
};
static const DialogScript kAutoAdvanceThenAwaitScript{kAutoAdvanceThenAwaitLines, nullptr, 2, 0};

// Single line whose tag is 0 -- LineEnter must still fire; tag filtering,
// including a tag of 0, is the game's responsibility, not the runner's.
static const DialogLine kZeroTagLines[] = {
    {kLineAText, nullptr, /*next*/ kNoLine, /*tag*/ 0, 0, 0, 0, LineKind::Text, 0},
};
static const DialogScript kZeroTagScript{kZeroTagLines, nullptr, 1, 0};

// next references a line id past lineCount (not kNoLine) -- must clamp to
// Finished rather than indexing out of range.
static const DialogLine kBadNextLines[] = {
    {kLineAText, nullptr, /*next*/ 99, /*tag*/ 7, 0, 0, 0, LineKind::Text, 0},
};
static const DialogScript kBadNextScript{kBadNextLines, nullptr, 1, 0};

// Single Choice line. choices is null / choiceCount is 0: DialogRunner does
// not read the choices table yet, so this stays valid until it does.
static const DialogLine kChoiceLines[] = {
    {nullptr, nullptr, /*next*/ kNoLine, /*tag*/ 9, 0, /*firstChoice*/ 0, /*choiceCount*/ 2,
     LineKind::Choice, 0},
};
static const DialogScript kChoiceScript{kChoiceLines, nullptr, 1, 0};

// Single End-kind line -- must finish immediately on entry.
static const DialogLine kEndLines[] = {
    {nullptr, nullptr, kNoLine, /*tag*/ 3, 0, 0, 0, LineKind::End, 0},
};
static const DialogScript kEndScript{kEndLines, nullptr, 1, 0};

// Five lines, only used to exercise start() at a non-zero valid index (4) in
// the start()-failure-reset test below.
static const DialogLine kFiveLineLines[] = {
    {kLineAText, nullptr, kNoLine, 1, 0, 0, 0, LineKind::Text, 0},
    {kLineAText, nullptr, kNoLine, 2, 0, 0, 0, LineKind::Text, 0},
    {kLineAText, nullptr, kNoLine, 3, 0, 0, 0, LineKind::Text, 0},
    {kLineAText, nullptr, kNoLine, 4, 0, 0, 0, LineKind::Text, 0},
    {kLineBText, nullptr, kNoLine, 5, 0, 0, 0, LineKind::Text, 0},
};
static const DialogScript kFiveLineScript{kFiveLineLines, nullptr, 5, 0};

// Two-line cycle (0 -> 1 -> 0 -> ...), used only by the reentrancy test to
// prove a callback that always re-feeds Advance on LineEnter cannot recurse
// without bound.
static const DialogLine kCyclicLines[] = {
    {kLineAText, nullptr, /*next*/ 1, /*tag*/ 10, 0, 0, 0, LineKind::Text, 0},
    {kLineBText, nullptr, /*next*/ 0, /*tag*/ 20, 0, 0, 0, LineKind::Text, 0},
};
static const DialogScript kCyclicScript{kCyclicLines, nullptr, 2, 0};

/// Owner used only by the reentrancy test. Counts LineEnter events, checks
/// that currentLineId()/state() are already consistent with the entering
/// line WHEN the callback observes them (the ordering fix), and reacts to
/// every LineEnter by feeding another Advance straight back into the same
/// runner -- the exact pattern that recurses without bound on an unguarded
/// runner given a cyclic script.
struct ReentrantOwner {
    DialogRunner* runner = nullptr;
    int lineEnterCount = 0;
    bool allConsistent = true;
};

void onReentrantLineEnter(void* ownerPtr, const DialogEvent& event) {
    auto* owner = static_cast<ReentrantOwner*>(ownerPtr);
    if (event.type != DialogEventType::LineEnter) return;

    ++owner->lineEnterCount;
    if (owner->runner->currentLineId() != event.line ||
        owner->runner->state() != DialogState::AwaitingAdvance) {
        owner->allConsistent = false;
    }

    owner->runner->feed(DialogAction::Advance);  // reentrant: must be a no-op
}

// A player-driven text line (0) followed by an End-kind line (1) -- lets a
// test reach an End line via resolveAdvance()'s enterLine(line.next) path,
// not just start()'s enterLine(first) path.
static const DialogLine kTextThenEndLines[] = {
    {kLineAText, nullptr, /*next*/ 1, /*tag*/ 1, 0, 0, 0, LineKind::Text, 0},
    {nullptr, nullptr, kNoLine, /*tag*/ 2, 0, 0, 0, LineKind::End, 0},
};
static const DialogScript kTextThenEndScript{kTextThenEndLines, nullptr, 2, 0};

/// Owner used by the two stop()-from-callback tests. Records every event
/// and calls stop() back into the runner when LineEnter fires for
/// `stopOnLine`.
struct StopOnLineOwner {
    DialogRunner* runner = nullptr;
    LineId stopOnLine = kNoLine;
    LoggedEvent log[8]{};
    int logCount = 0;

    void record(const DialogEvent& event) {
        if (logCount < 8) {
            log[logCount++] = LoggedEvent{event.type, event.line, event.choice, event.tag};
        }
    }
};

void onLineEnterStopsOnTargetLine(void* ownerPtr, const DialogEvent& event) {
    auto* owner = static_cast<StopOnLineOwner*>(ownerPtr);
    owner->record(event);
    if (event.type == DialogEventType::LineEnter && event.line == owner->stopOnLine) {
        owner->runner->stop();
    }
}

/// Owner used by the reentrant-start() test. Attempts exactly one reentrant
/// start() against a DIFFERENT script the first time LineEnter fires, and
/// records that call's own return value.
struct ReentrantStartOwner {
    DialogRunner* runner = nullptr;
    const DialogScript* reentrantScript = nullptr;
    bool attempted = false;
    bool reentrantResult = true;
};

void onLineEnterAttemptsReentrantStart(void* ownerPtr, const DialogEvent& event) {
    auto* owner = static_cast<ReentrantStartOwner*>(ownerPtr);
    if (event.type == DialogEventType::LineEnter && !owner->attempted) {
        owner->attempted = true;
        owner->reentrantResult = owner->runner->start(*owner->reentrantScript, 0);
    }
}

}  // namespace

// =============================================================================
// Requirement: start() validation
// =============================================================================

void test_dialog_runner_start_fails_with_null_lines(void) {
    DialogRunner runner;
    DialogScript badScript{nullptr, nullptr, 0, 0};

    TEST_ASSERT_FALSE(runner.start(badScript, 0));
    TEST_ASSERT_TRUE(runner.state() == DialogState::Inactive);
    TEST_ASSERT_FALSE(runner.isActive());
}

void test_dialog_runner_start_fails_with_zero_line_count(void) {
    DialogRunner runner;
    DialogScript badScript{kLinearLines, nullptr, 0, 0};

    TEST_ASSERT_FALSE(runner.start(badScript, 0));
    TEST_ASSERT_TRUE(runner.state() == DialogState::Inactive);
}

void test_dialog_runner_start_fails_with_out_of_range_first(void) {
    DialogRunner runner;

    TEST_ASSERT_FALSE(runner.start(kLinearScript, 99));
    TEST_ASSERT_TRUE(runner.state() == DialogState::Inactive);
}

void test_dialog_runner_start_failure_resets_a_previously_active_session(void) {
    // Exact sequence from review: a successful start() at a non-zero line,
    // then a start() that fails, must not leave currentLineId()/
    // currentLine() pointing at the first script -- a caller reading
    // `false` as "detached" and then freeing/reusing that script's storage
    // would otherwise turn this into a stale-pointer read.
    DialogRunner runner;
    TEST_ASSERT_TRUE(runner.start(kFiveLineScript, 4));
    TEST_ASSERT_EQUAL_UINT16(4, runner.currentLineId());
    const uint16_t revBefore = runner.revision();

    TEST_ASSERT_FALSE(runner.start(kLinearScript, 99));  // rejected: out of range

    TEST_ASSERT_TRUE(runner.state() == DialogState::Inactive);
    TEST_ASSERT_FALSE(runner.isActive());
    TEST_ASSERT_EQUAL_HEX16(kNoLine, runner.currentLineId());
    TEST_ASSERT_NULL(runner.currentLine());
    TEST_ASSERT_NOT_EQUAL(revBefore, runner.revision());  // detaching a live session IS visible
}

void test_dialog_runner_start_failure_on_an_already_inactive_runner_does_not_bump_revision(void) {
    DialogRunner runner;
    const uint16_t revBefore = runner.revision();

    TEST_ASSERT_FALSE(runner.start(kLinearScript, 99));

    TEST_ASSERT_TRUE(runner.state() == DialogState::Inactive);
    TEST_ASSERT_EQUAL_UINT16(revBefore, runner.revision());  // nothing was attached to detach
}

// =============================================================================
// Requirement: LineEnter fires unconditionally, including tag 0
// =============================================================================

void test_dialog_runner_line_enter_fires_with_line_tag(void) {
    MockOwner owner;
    DialogRunner runner;
    runner.configure(&owner, onDialogEvent);

    TEST_ASSERT_TRUE(runner.start(kLinearScript, 0));

    TEST_ASSERT_EQUAL_INT(1, owner.logCount);
    TEST_ASSERT_TRUE(owner.log[0].type == DialogEventType::LineEnter);
    TEST_ASSERT_EQUAL_UINT16(0, owner.log[0].line);
    TEST_ASSERT_EQUAL_HEX8(kNoChoice, owner.log[0].choice);
    TEST_ASSERT_EQUAL_UINT16(111, owner.log[0].tag);
}

void test_dialog_runner_line_enter_fires_with_tag_zero(void) {
    MockOwner owner;
    DialogRunner runner;
    runner.configure(&owner, onDialogEvent);

    TEST_ASSERT_TRUE(runner.start(kZeroTagScript, 0));

    TEST_ASSERT_EQUAL_INT(1, owner.logCount);
    TEST_ASSERT_TRUE(owner.log[0].type == DialogEventType::LineEnter);
    TEST_ASSERT_EQUAL_UINT16(0, owner.log[0].tag);
}

// =============================================================================
// Requirement: Five-state machine -- start() branches on autoAdvanceMs
// =============================================================================

void test_dialog_runner_text_line_with_auto_advance_enters_showing_text(void) {
    DialogRunner runner;

    TEST_ASSERT_TRUE(runner.start(kAutoAdvanceScript, 0));

    TEST_ASSERT_TRUE(runner.state() == DialogState::ShowingText);
    TEST_ASSERT_TRUE(runner.isActive());
}

void test_dialog_runner_text_line_without_auto_advance_enters_awaiting_advance(void) {
    DialogRunner runner;

    TEST_ASSERT_TRUE(runner.start(kLinearScript, 0));

    TEST_ASSERT_TRUE(runner.state() == DialogState::AwaitingAdvance);
    TEST_ASSERT_TRUE(runner.isActive());
}

// =============================================================================
// Requirement: Linear line chains -- Advance/Confirm alias, follows `next`
// =============================================================================

void test_dialog_runner_advance_moves_to_next_line(void) {
    DialogRunner runner;
    runner.start(kLinearScript, 0);

    runner.feed(DialogAction::Advance);

    TEST_ASSERT_EQUAL_UINT16(1, runner.currentLineId());
    TEST_ASSERT_TRUE(runner.state() == DialogState::AwaitingAdvance);
}

void test_dialog_runner_line_enter_fires_on_a_mid_session_transition(void) {
    // The prior test proves the transition; this one proves LineEnter is
    // unconditional beyond just the FIRST line -- start()'s own LineEnter
    // is not the only one exercised across a `next` link.
    MockOwner owner;
    DialogRunner runner;
    runner.configure(&owner, onDialogEvent);
    runner.start(kLinearScript, 0);
    owner.logCount = 0;

    runner.feed(DialogAction::Advance);  // line 0 -> line 1 via `next`

    TEST_ASSERT_EQUAL_INT(1, owner.logCount);
    TEST_ASSERT_TRUE(owner.log[0].type == DialogEventType::LineEnter);
    TEST_ASSERT_EQUAL_UINT16(1, owner.log[0].line);
    TEST_ASSERT_EQUAL_HEX8(kNoChoice, owner.log[0].choice);
    TEST_ASSERT_EQUAL_UINT16(222, owner.log[0].tag);  // line 1's own tag
}

void test_dialog_runner_confirm_aliases_advance(void) {
    DialogRunner runner;
    runner.start(kLinearScript, 0);

    runner.feed(DialogAction::Confirm);

    TEST_ASSERT_EQUAL_UINT16(1, runner.currentLineId());
}

void test_dialog_runner_confirm_aliases_advance_in_showing_text(void) {
    DialogRunner runner;
    runner.start(kAutoAdvanceScript, 0);
    TEST_ASSERT_TRUE(runner.state() == DialogState::ShowingText);

    runner.feed(DialogAction::Confirm);

    TEST_ASSERT_TRUE(runner.state() == DialogState::Finished);  // next == kNoLine
}

// =============================================================================
// Requirement: Per-line auto-advance -- fires at exactly autoAdvanceMs
// =============================================================================

void test_dialog_runner_auto_advance_fires_at_exact_ms(void) {
    DialogRunner runner;
    runner.start(kAutoAdvanceThenAwaitScript, 0);

    runner.update(2399);
    TEST_ASSERT_EQUAL_UINT16(0, runner.currentLineId());
    TEST_ASSERT_TRUE(runner.state() == DialogState::ShowingText);

    runner.update(1);  // accumulates to exactly 2400
    TEST_ASSERT_EQUAL_UINT16(1, runner.currentLineId());
    TEST_ASSERT_TRUE(runner.state() == DialogState::AwaitingAdvance);
}

void test_dialog_runner_auto_advance_fires_on_a_single_call_at_and_past_threshold(void) {
    // The previous test only proves the accumulated-delta path (2399 then
    // +1). A caller ticking with one large deltaTimeMs per frame must
    // converge on the same behavior in a single update() call, both at the
    // exact threshold and past it.
    DialogRunner exact;
    exact.start(kAutoAdvanceThenAwaitScript, 0);
    exact.update(2400);  // single call, lands exactly on autoAdvanceMs
    TEST_ASSERT_EQUAL_UINT16(1, exact.currentLineId());
    TEST_ASSERT_TRUE(exact.state() == DialogState::AwaitingAdvance);

    DialogRunner past;
    past.start(kAutoAdvanceThenAwaitScript, 0);
    past.update(5000);  // single call, well past autoAdvanceMs
    TEST_ASSERT_EQUAL_UINT16(1, past.currentLineId());
    TEST_ASSERT_TRUE(past.state() == DialogState::AwaitingAdvance);
}

void test_dialog_runner_update_is_noop_outside_showing_text(void) {
    DialogRunner runner;
    runner.start(kLinearScript, 0);  // AwaitingAdvance
    const uint16_t revBefore = runner.revision();

    runner.update(999999);

    TEST_ASSERT_TRUE(runner.state() == DialogState::AwaitingAdvance);
    TEST_ASSERT_EQUAL_UINT16(0, runner.currentLineId());
    TEST_ASSERT_EQUAL_UINT16(revBefore, runner.revision());
}

// =============================================================================
// Requirement: Paging via setPageCount -- advance, clamp, reset per line
// =============================================================================

void test_dialog_runner_set_page_count_advance_before_next_line(void) {
    DialogRunner runner;
    runner.start(kLinearScript, 0);

    runner.setPageCount(3);
    TEST_ASSERT_EQUAL_UINT8(3, runner.pageCount());
    TEST_ASSERT_EQUAL_UINT8(0, runner.page());

    runner.feed(DialogAction::Advance);  // page 0 -> 1, stays on line 0
    TEST_ASSERT_EQUAL_UINT16(0, runner.currentLineId());
    TEST_ASSERT_EQUAL_UINT8(1, runner.page());

    runner.feed(DialogAction::Advance);  // page 1 -> 2, stays on line 0
    TEST_ASSERT_EQUAL_UINT16(0, runner.currentLineId());
    TEST_ASSERT_EQUAL_UINT8(2, runner.page());

    runner.feed(DialogAction::Advance);  // final page consumed -> moves to next line
    TEST_ASSERT_EQUAL_UINT16(1, runner.currentLineId());
    TEST_ASSERT_EQUAL_UINT8(0, runner.page());
    TEST_ASSERT_EQUAL_UINT8(1, runner.pageCount());  // reset per line
}

void test_dialog_runner_set_page_count_clamps_current_page_on_shrink(void) {
    DialogRunner runner;
    runner.start(kLinearScript, 0);
    runner.setPageCount(3);
    runner.feed(DialogAction::Advance);
    runner.feed(DialogAction::Advance);
    TEST_ASSERT_EQUAL_UINT8(2, runner.page());

    runner.setPageCount(1);  // shrink: current page must clamp into range

    TEST_ASSERT_EQUAL_UINT8(1, runner.pageCount());
    TEST_ASSERT_EQUAL_UINT8(0, runner.page());
}

void test_dialog_runner_set_page_count_zero_is_treated_as_one(void) {
    DialogRunner runner;
    runner.start(kLinearScript, 0);

    runner.setPageCount(0);

    TEST_ASSERT_EQUAL_UINT8(1, runner.pageCount());
}

void test_dialog_runner_set_page_count_is_noop_when_inactive(void) {
    // The realistic trigger: an async text-wrap result lands a frame after
    // the runner was never started, or after the player already advanced
    // past the last line. Nothing player-visible changes, so revision()
    // must not bump either.
    DialogRunner runner;
    const uint16_t revBefore = runner.revision();

    runner.setPageCount(5);

    TEST_ASSERT_EQUAL_UINT8(1, runner.pageCount());
    TEST_ASSERT_EQUAL_UINT16(revBefore, runner.revision());
}

void test_dialog_runner_set_page_count_is_noop_when_finished(void) {
    DialogRunner runner;
    runner.start(kLinearScript, 1);
    runner.feed(DialogAction::Advance);  // -> Finished (next == kNoLine)
    TEST_ASSERT_TRUE(runner.state() == DialogState::Finished);
    const uint16_t revBefore = runner.revision();

    runner.setPageCount(5);

    TEST_ASSERT_EQUAL_UINT8(1, runner.pageCount());
    TEST_ASSERT_EQUAL_UINT16(revBefore, runner.revision());
}

// =============================================================================
// Requirement: next == kNoLine finishes; out-of-range next clamps to Finished
// =============================================================================

void test_dialog_runner_next_kNoLine_finishes_dialog(void) {
    MockOwner owner;
    DialogRunner runner;
    runner.configure(&owner, onDialogEvent);
    runner.start(kLinearScript, 1);  // line 1: next == kNoLine
    owner.logCount = 0;

    runner.feed(DialogAction::Advance);

    TEST_ASSERT_TRUE(runner.state() == DialogState::Finished);
    TEST_ASSERT_FALSE(runner.isActive());
    TEST_ASSERT_EQUAL_HEX16(kNoLine, runner.currentLineId());
    TEST_ASSERT_EQUAL_INT(1, owner.logCount);
    TEST_ASSERT_TRUE(owner.log[0].type == DialogEventType::Ended);
    TEST_ASSERT_EQUAL_UINT16(1, owner.log[0].line);   // "at" == the line that finished
    TEST_ASSERT_EQUAL_UINT16(222, owner.log[0].tag);  // that line's own tag
}

void test_dialog_runner_out_of_range_next_finishes_without_oob(void) {
    MockOwner owner;
    DialogRunner runner;
    runner.configure(&owner, onDialogEvent);
    runner.start(kBadNextScript, 0);
    owner.logCount = 0;

    runner.feed(DialogAction::Advance);  // next == 99, lineCount == 1

    TEST_ASSERT_TRUE(runner.state() == DialogState::Finished);
    TEST_ASSERT_EQUAL_HEX16(kNoLine, runner.currentLineId());
    TEST_ASSERT_EQUAL_INT(1, owner.logCount);
    TEST_ASSERT_TRUE(owner.log[0].type == DialogEventType::Ended);
    TEST_ASSERT_EQUAL_HEX16(kNoLine, owner.log[0].line);  // no valid line to report
    TEST_ASSERT_EQUAL_UINT16(0, owner.log[0].tag);
}

void test_dialog_runner_end_kind_line_finishes_immediately(void) {
    MockOwner owner;
    DialogRunner runner;
    runner.configure(&owner, onDialogEvent);

    TEST_ASSERT_TRUE(runner.start(kEndScript, 0));

    TEST_ASSERT_TRUE(runner.state() == DialogState::Finished);
    TEST_ASSERT_EQUAL_INT(2, owner.logCount);  // LineEnter, then Ended
    TEST_ASSERT_TRUE(owner.log[0].type == DialogEventType::LineEnter);
    TEST_ASSERT_TRUE(owner.log[1].type == DialogEventType::Ended);
    TEST_ASSERT_EQUAL_UINT16(0, owner.log[1].line);
    TEST_ASSERT_EQUAL_UINT16(3, owner.log[1].tag);
}

void test_dialog_runner_stop_from_callback_on_end_kind_line_via_start_is_respected(void) {
    // Reached via start()'s enterLine(first) path. Without the fix,
    // enterLine()'s trailing finish() call would run unconditionally after
    // the callback returns, silently undoing stop() and firing an Ended
    // event stop()'s own contract promises will never happen.
    StopOnLineOwner owner;
    DialogRunner runner;
    owner.runner = &runner;
    owner.stopOnLine = 0;
    runner.configure(&owner, onLineEnterStopsOnTargetLine);

    const bool started = runner.start(kEndScript, 0);

    // start() itself succeeded -- it validly entered line 0 and dispatched
    // LineEnter. Whether the SESSION survives past that point is a
    // separate question, answered by what the callback did with it.
    TEST_ASSERT_TRUE(started);

    // Full observable consequence, not just state(): exactly one event
    // (LineEnter; no Ended), fully Inactive, no current line, revision()
    // reflects stop()'s own bump and nothing more.
    TEST_ASSERT_EQUAL_INT(1, owner.logCount);
    TEST_ASSERT_TRUE(owner.log[0].type == DialogEventType::LineEnter);
    TEST_ASSERT_TRUE(runner.state() == DialogState::Inactive);
    TEST_ASSERT_EQUAL_HEX16(kNoLine, runner.currentLineId());
    TEST_ASSERT_FALSE(runner.isActive());
    TEST_ASSERT_EQUAL_UINT16(2, runner.revision());  // enterLine's bump, then stop()'s
}

void test_dialog_runner_stop_from_callback_on_end_kind_line_via_advance_is_respected(void) {
    // Reached via resolveAdvance()'s enterLine(line.next) path -- the other
    // route into an End-kind line named explicitly in review.
    StopOnLineOwner owner;
    DialogRunner runner;
    owner.runner = &runner;
    owner.stopOnLine = 1;  // the End line, reached only after advancing
    runner.configure(&owner, onLineEnterStopsOnTargetLine);
    runner.start(kTextThenEndScript, 0);
    TEST_ASSERT_TRUE(runner.state() == DialogState::AwaitingAdvance);
    owner.logCount = 0;
    const uint16_t revBefore = runner.revision();

    runner.feed(DialogAction::Advance);  // line 0 -> line 1 (End) -> callback stops

    TEST_ASSERT_EQUAL_INT(1, owner.logCount);  // LineEnter only; no Ended
    TEST_ASSERT_TRUE(owner.log[0].type == DialogEventType::LineEnter);
    TEST_ASSERT_EQUAL_UINT16(1, owner.log[0].line);
    TEST_ASSERT_TRUE(runner.state() == DialogState::Inactive);
    TEST_ASSERT_EQUAL_HEX16(kNoLine, runner.currentLineId());
    TEST_ASSERT_FALSE(runner.isActive());
    TEST_ASSERT_NOT_EQUAL(revBefore, runner.revision());  // stop() itself is a visible change
}

// =============================================================================
// Requirement: feed() is total -- every illegal (state, action) cell is a
// no-op, including no revision() bump.
// =============================================================================

void test_dialog_runner_illegal_actions_are_noop_when_inactive(void) {
    DialogRunner runner;
    const uint16_t revBefore = runner.revision();

    runner.feed(DialogAction::Advance);
    runner.feed(DialogAction::Up);
    runner.feed(DialogAction::Down);
    runner.feed(DialogAction::Confirm);
    runner.feed(DialogAction::Cancel);
    runner.feed(DialogAction::None);

    TEST_ASSERT_TRUE(runner.state() == DialogState::Inactive);
    TEST_ASSERT_EQUAL_UINT16(revBefore, runner.revision());
}

void test_dialog_runner_illegal_actions_are_noop_in_showing_text(void) {
    DialogRunner runner;
    runner.start(kAutoAdvanceScript, 0);
    const uint16_t revBefore = runner.revision();
    const LineId lineBefore = runner.currentLineId();

    runner.feed(DialogAction::Up);
    runner.feed(DialogAction::Down);
    runner.feed(DialogAction::Cancel);
    runner.feed(DialogAction::None);

    TEST_ASSERT_TRUE(runner.state() == DialogState::ShowingText);
    TEST_ASSERT_EQUAL_UINT16(lineBefore, runner.currentLineId());
    TEST_ASSERT_EQUAL_UINT16(revBefore, runner.revision());
}

void test_dialog_runner_illegal_actions_are_noop_in_awaiting_advance(void) {
    DialogRunner runner;
    runner.start(kLinearScript, 0);
    const uint16_t revBefore = runner.revision();

    runner.feed(DialogAction::Up);
    runner.feed(DialogAction::Down);
    runner.feed(DialogAction::Cancel);
    runner.feed(DialogAction::None);

    TEST_ASSERT_TRUE(runner.state() == DialogState::AwaitingAdvance);
    TEST_ASSERT_EQUAL_UINT16(revBefore, runner.revision());
}

void test_dialog_runner_illegal_actions_are_noop_when_finished(void) {
    DialogRunner runner;
    runner.start(kLinearScript, 1);
    runner.feed(DialogAction::Advance);  // -> Finished
    TEST_ASSERT_TRUE(runner.state() == DialogState::Finished);
    const uint16_t revBefore = runner.revision();

    runner.feed(DialogAction::Advance);
    runner.feed(DialogAction::Confirm);
    runner.feed(DialogAction::Up);
    runner.feed(DialogAction::Down);
    runner.feed(DialogAction::Cancel);
    runner.feed(DialogAction::None);

    TEST_ASSERT_TRUE(runner.state() == DialogState::Finished);
    TEST_ASSERT_EQUAL_UINT16(revBefore, runner.revision());
}

// =============================================================================
// Requirement: stop() returns to Inactive and fires no event
// =============================================================================

void test_dialog_runner_stop_fires_no_event(void) {
    MockOwner owner;
    DialogRunner runner;
    runner.configure(&owner, onDialogEvent);
    runner.start(kLinearScript, 0);
    owner.logCount = 0;

    runner.stop();

    TEST_ASSERT_EQUAL_INT(0, owner.logCount);
    TEST_ASSERT_TRUE(runner.state() == DialogState::Inactive);
    TEST_ASSERT_EQUAL_HEX16(kNoLine, runner.currentLineId());
    TEST_ASSERT_FALSE(runner.isActive());
}

// =============================================================================
// Requirement: a Choice line reaches ShowingChoices; every action there is a
// deliberate no-op until ShowingChoices' action handling is implemented
// =============================================================================

void test_dialog_runner_choice_line_reaches_showing_choices_and_ignores_advance(void) {
    MockOwner owner;
    DialogRunner runner;
    runner.configure(&owner, onDialogEvent);

    TEST_ASSERT_TRUE(runner.start(kChoiceScript, 0));

    TEST_ASSERT_TRUE(runner.state() == DialogState::ShowingChoices);
    TEST_ASSERT_TRUE(runner.isActive());
    const uint16_t revBefore = runner.revision();
    const int logBefore = owner.logCount;

    runner.feed(DialogAction::Advance);
    runner.feed(DialogAction::Up);
    runner.feed(DialogAction::Down);
    runner.feed(DialogAction::Confirm);
    runner.feed(DialogAction::Cancel);
    runner.feed(DialogAction::None);  // Match the other three illegal-action
                                       // tests: all six actions, so the four
                                       // are only meaningful together as full
                                       // coverage of the action product.

    TEST_ASSERT_TRUE(runner.state() == DialogState::ShowingChoices);
    TEST_ASSERT_EQUAL_UINT16(revBefore, runner.revision());
    TEST_ASSERT_EQUAL_INT(logBefore, owner.logCount);
}

// =============================================================================
// currentLine() reflects state: null when not on a line, valid otherwise
// =============================================================================

void test_dialog_runner_current_line_reflects_state(void) {
    DialogRunner runner;
    TEST_ASSERT_NULL(runner.currentLine());  // Inactive

    runner.start(kLinearScript, 0);
    TEST_ASSERT_NOT_NULL(runner.currentLine());
    TEST_ASSERT_EQUAL_STRING(kLineAText, runner.currentLine()->text);

    runner.feed(DialogAction::Advance);  // -> line 1
    runner.feed(DialogAction::Advance);  // line 1's next == kNoLine -> Finished

    TEST_ASSERT_TRUE(runner.state() == DialogState::Finished);
    TEST_ASSERT_NULL(runner.currentLine());
}

// =============================================================================
// Requirement: reentrant feed()/update()/start() calls made from within the
// configured DialogEventFn are ignored, not recursive -- and state()/
// currentLineId() are already consistent with the entering line by the time
// the callback observes them.
// =============================================================================

void test_dialog_runner_reentrant_feed_from_line_enter_is_ignored_not_recursive(void) {
    ReentrantOwner owner;
    DialogRunner runner;
    owner.runner = &runner;
    runner.configure(&owner, onReentrantLineEnter);

    TEST_ASSERT_TRUE(runner.start(kCyclicScript, 0));

    // start()'s own LineEnter dispatch already tried one reentrant feed();
    // on an unguarded runner and this cyclic script, that recurses without
    // bound and overflows the stack on ESP32. Here it must simply be
    // dropped: exactly one LineEnter, no extra transition.
    TEST_ASSERT_EQUAL_INT(1, owner.lineEnterCount);
    TEST_ASSERT_TRUE(owner.allConsistent);
    TEST_ASSERT_EQUAL_UINT16(0, runner.currentLineId());
    TEST_ASSERT_TRUE(runner.state() == DialogState::AwaitingAdvance);

    runner.feed(DialogAction::Advance);  // top-level call: allowed to transition

    TEST_ASSERT_EQUAL_INT(2, owner.lineEnterCount);
    TEST_ASSERT_TRUE(owner.allConsistent);
    TEST_ASSERT_EQUAL_UINT16(1, runner.currentLineId());  // moved exactly once
    TEST_ASSERT_TRUE(runner.state() == DialogState::AwaitingAdvance);
}

void test_dialog_runner_reentrant_start_from_callback_is_a_pure_noop(void) {
    // A reentrant start() must not reset anything (unlike a REJECTED
    // start()): the outer call is still executing and owns the session.
    // Resetting here would tear that session down out from under it.
    ReentrantStartOwner owner;
    DialogRunner runner;
    owner.runner = &runner;
    owner.reentrantScript = &kEndScript;  // deliberately a different script
    runner.configure(&owner, onLineEnterAttemptsReentrantStart);

    const bool outerResult = runner.start(kLinearScript, 0);

    TEST_ASSERT_TRUE(outerResult);            // the OUTER call succeeded
    TEST_ASSERT_TRUE(owner.attempted);
    TEST_ASSERT_FALSE(owner.reentrantResult);  // the REENTRANT call was rejected

    // Zero effect: the runner is still exactly where the outer start() left
    // it -- kLinearScript's line 0, not torn down or redirected toward
    // kEndScript by the reentrant attempt.
    TEST_ASSERT_EQUAL_UINT16(0, runner.currentLineId());
    TEST_ASSERT_TRUE(runner.state() == DialogState::AwaitingAdvance);
    TEST_ASSERT_NOT_NULL(runner.currentLine());
    TEST_ASSERT_EQUAL_STRING(kLineAText, runner.currentLine()->text);
}

// =============================================================================
// Requirement: zero heap allocation across a full session
// (start -> paging -> linear advance -> auto-advance no-op -> finish -> stop)
// =============================================================================

namespace {
size_t g_heapAllocCount = 0;
}

void* operator new(std::size_t size) {
    ++g_heapAllocCount;
    return std::malloc(size);
}
void operator delete(void* ptr) noexcept {
    std::free(ptr);
}
void operator delete(void* ptr, std::size_t) noexcept {
    std::free(ptr);
}
void* operator new[](std::size_t size) {
    ++g_heapAllocCount;
    return std::malloc(size);
}
void operator delete[](void* ptr) noexcept {
    std::free(ptr);
}
void operator delete[](void* ptr, std::size_t) noexcept {
    std::free(ptr);
}

void test_dialog_runner_zero_heap_allocation_across_full_session(void) {
    MockOwner owner;
    DialogRunner runner;
    runner.configure(&owner, onDialogEvent);

    g_heapAllocCount = 0;

    runner.start(kLinearScript, 0);
    runner.setPageCount(2);
    runner.feed(DialogAction::Advance);  // page 0 -> 1, still line 0
    runner.feed(DialogAction::Advance);  // final page consumed -> line 1
    runner.update(16);                   // no-op: AwaitingAdvance ignores update()
    runner.feed(DialogAction::Confirm);  // line 1's next == kNoLine -> Finished
    runner.stop();

    TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(g_heapAllocCount));
}

// =============================================================================
// Requirement: sizeof(DialogRunner) RAM regression guard
// =============================================================================

void test_dialog_runner_sizeof_guard(void) {
#ifdef ESP32
    TEST_ASSERT_EQUAL_UINT32(28u, static_cast<uint32_t>(sizeof(DialogRunner)));
#else
    TEST_ASSERT_EQUAL_UINT32(40u, static_cast<uint32_t>(sizeof(DialogRunner)));
#endif
    TEST_ASSERT_LESS_OR_EQUAL(3 * sizeof(void*) + 16, sizeof(DialogRunner));
}

#else  // !PIXELROOT32_ENABLE_DIALOG

void test_dialog_runner_flag_off_stub_compiles(void) {
    // With the flag off, every DialogRunner.h symbol lives entirely inside
    // the #if PIXELROOT32_ENABLE_DIALOG guard. This translation unit
    // compiling and passing without referencing any of them IS the
    // "zero bytes reserved" property required by the "Flat header placement
    // and flag wiring" requirement's flag-off scenario.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_DIALOG=0: DialogRunner is not compiled, zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_DIALOG

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_DIALOG
    RUN_TEST(test_dialog_runner_start_fails_with_null_lines);
    RUN_TEST(test_dialog_runner_start_fails_with_zero_line_count);
    RUN_TEST(test_dialog_runner_start_fails_with_out_of_range_first);
    RUN_TEST(test_dialog_runner_start_failure_resets_a_previously_active_session);
    RUN_TEST(test_dialog_runner_start_failure_on_an_already_inactive_runner_does_not_bump_revision);
    RUN_TEST(test_dialog_runner_line_enter_fires_with_line_tag);
    RUN_TEST(test_dialog_runner_line_enter_fires_with_tag_zero);
    RUN_TEST(test_dialog_runner_text_line_with_auto_advance_enters_showing_text);
    RUN_TEST(test_dialog_runner_text_line_without_auto_advance_enters_awaiting_advance);
    RUN_TEST(test_dialog_runner_advance_moves_to_next_line);
    RUN_TEST(test_dialog_runner_line_enter_fires_on_a_mid_session_transition);
    RUN_TEST(test_dialog_runner_confirm_aliases_advance);
    RUN_TEST(test_dialog_runner_confirm_aliases_advance_in_showing_text);
    RUN_TEST(test_dialog_runner_auto_advance_fires_at_exact_ms);
    RUN_TEST(test_dialog_runner_auto_advance_fires_on_a_single_call_at_and_past_threshold);
    RUN_TEST(test_dialog_runner_update_is_noop_outside_showing_text);
    RUN_TEST(test_dialog_runner_set_page_count_advance_before_next_line);
    RUN_TEST(test_dialog_runner_set_page_count_clamps_current_page_on_shrink);
    RUN_TEST(test_dialog_runner_set_page_count_zero_is_treated_as_one);
    RUN_TEST(test_dialog_runner_set_page_count_is_noop_when_inactive);
    RUN_TEST(test_dialog_runner_set_page_count_is_noop_when_finished);
    RUN_TEST(test_dialog_runner_next_kNoLine_finishes_dialog);
    RUN_TEST(test_dialog_runner_out_of_range_next_finishes_without_oob);
    RUN_TEST(test_dialog_runner_end_kind_line_finishes_immediately);
    RUN_TEST(test_dialog_runner_stop_from_callback_on_end_kind_line_via_start_is_respected);
    RUN_TEST(test_dialog_runner_stop_from_callback_on_end_kind_line_via_advance_is_respected);
    RUN_TEST(test_dialog_runner_illegal_actions_are_noop_when_inactive);
    RUN_TEST(test_dialog_runner_illegal_actions_are_noop_in_showing_text);
    RUN_TEST(test_dialog_runner_illegal_actions_are_noop_in_awaiting_advance);
    RUN_TEST(test_dialog_runner_illegal_actions_are_noop_when_finished);
    RUN_TEST(test_dialog_runner_stop_fires_no_event);
    RUN_TEST(test_dialog_runner_choice_line_reaches_showing_choices_and_ignores_advance);
    RUN_TEST(test_dialog_runner_current_line_reflects_state);
    RUN_TEST(test_dialog_runner_reentrant_feed_from_line_enter_is_ignored_not_recursive);
    RUN_TEST(test_dialog_runner_reentrant_start_from_callback_is_a_pure_noop);
    RUN_TEST(test_dialog_runner_zero_heap_allocation_across_full_session);
    RUN_TEST(test_dialog_runner_sizeof_guard);
#else
    RUN_TEST(test_dialog_runner_flag_off_stub_compiles);
#endif

    return UNITY_END();
}
