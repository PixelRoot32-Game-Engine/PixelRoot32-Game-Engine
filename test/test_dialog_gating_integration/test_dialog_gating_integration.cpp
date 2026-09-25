/**
 * @file test_dialog_gating_integration.cpp
 * @brief Integration tests for the dialog/gameplay gate: gameplay must NOT
 *        advance while a dialog is active, and must resume once it ends.
 *
 * This is the scene-level contract every unit suite leaves untested.
 * test_gameplay_dialog_runner proves the runner's own state machine in
 * isolation; nothing anywhere proves what a SCENE is supposed to do with
 * that state machine. A game that reads isActive() wrongly -- or reads it
 * after updating the world instead of before -- still passes every unit
 * test while the player walks around underneath an open dialog box.
 *
 * The harness below is the smallest thing that can be wrong in that way: a
 * mutable gameplay state (a position advanced by a fixed step per tick) and
 * a DialogRunner, wired together by the ONE rule a scene owes the runner --
 * skip the gameplay update while runner.isActive(). Every assertion here is
 * about that rule holding across real tick sequences, not about the runner's
 * internals.
 *
 * Headless by construction: no Renderer, no DialogBox, no InputManager. The
 * runner is renderer-free by design, so the gate it drives is testable
 * without a single pixel.
 *
 * KS-INTEGRATION-TEST-SUITE
 */

#include <unity.h>
#include "../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_DIALOG

#include "gameplay/DialogRunner.h"
#include "gameplay/DialogTypes.h"

#include <cstdint>

using namespace pixelroot32::gameplay;

namespace {

// =============================================================================
// Scene-like harness
//
// Stands in for a real Scene: it owns the runner and a trivial piece of
// mutable gameplay state, and its tick() applies the gating rule a scene is
// responsible for. Deliberately NOT a pixelroot32::core::Scene -- bringing
// in the entity/renderer machinery would test Scene, not the gate.
// =============================================================================

constexpr int32_t kStepPerTick = 4;  ///< Units the "player" moves per ungated tick.

struct DialogGatedScene {
    DialogRunner runner;

    int32_t playerX = 0;          ///< The gameplay state under test.
    int      gameplayTicks = 0;   ///< How many ticks actually ran gameplay.
    int      totalTicks = 0;      ///< How many ticks were requested.

    /// One frame. The dialog machine always ticks (it owns its own
    /// auto-advance timer); the world only ticks when no dialog is active.
    /// That ordering is the rule under test: read isActive() first, move
    /// second.
    void tick(unsigned long deltaTimeMs) {
        ++totalTicks;
        runner.update(deltaTimeMs);
        if (runner.isActive()) {
            return;  // Dialog owns the frame: gameplay is frozen.
        }
        updateGameplay();
    }

    /// Runs `count` frames.
    void tickTimes(int count, unsigned long deltaTimeMs = 16) {
        for (int i = 0; i < count; ++i) {
            tick(deltaTimeMs);
        }
    }

    /// Delivers a player action to the dialog. A scene routes input to the
    /// dialog INSTEAD of to the world while one is open, so this must never
    /// move the player by itself.
    void feedDialog(DialogAction action) {
        runner.feed(action);
    }

private:
    void updateGameplay() {
        ++gameplayTicks;
        playerX += kStepPerTick;
    }
};

// =============================================================================
// Script fixtures -- caller-owned, const; must outlive any runner started
// from them.
// =============================================================================

constexpr const char* kGreeting = "Hello there.";
constexpr const char* kFarewell = "Goodbye.";

/// Line 0 waits for the player (AwaitingAdvance), line 1 likewise, then ends.
static const DialogLine kTwoTextLines[] = {
    {kGreeting, nullptr, /*next*/ 1, /*tag*/ 1, /*autoAdvanceMs*/ 0, 0, 0, LineKind::Text, 0},
    {kFarewell, nullptr, /*next*/ kNoLine, /*tag*/ 2, /*autoAdvanceMs*/ 0, 0, 0, LineKind::Text, 0},
};
static const DialogScript kTwoTextScript{kTwoTextLines, nullptr, 2, 0};

/// Line 0 auto-advances after 2400 ms, so the runner sits in ShowingText and
/// the gate must hold across many ticks of the clock, not just across
/// player-driven frames.
static const DialogLine kAutoAdvanceLines[] = {
    {kGreeting, nullptr, /*next*/ 1, /*tag*/ 3, /*autoAdvanceMs*/ 2400, 0, 0, LineKind::Text, 0},
    {kFarewell, nullptr, /*next*/ kNoLine, /*tag*/ 4, /*autoAdvanceMs*/ 0, 0, 0, LineKind::Text, 0},
};
static const DialogScript kAutoAdvanceScript{kAutoAdvanceLines, nullptr, 2, 0};

/// A Choice line, so ShowingChoices gets its own gating coverage: choice 0
/// continues to a text line, choice 1 ends the dialog outright.
static const DialogChoice kChoices[] = {
    {"Stay", /*next*/ 1, /*tag*/ 501},
    {"Leave", /*next*/ kNoLine, /*tag*/ 502},
};
static const DialogLine kChoiceLines[] = {
    {nullptr, nullptr, /*next*/ kNoLine, /*tag*/ 5, 0, /*firstChoice*/ 0, /*choiceCount*/ 2,
     LineKind::Choice, 0},
    {kFarewell, nullptr, /*next*/ kNoLine, /*tag*/ 6, 0, 0, 0, LineKind::Text, 0},
};
static const DialogScript kChoiceScript{kChoiceLines, kChoices, 2, 2};

}  // namespace

// =============================================================================
// Baseline: without a dialog, the harness's gameplay really does advance.
// Every assertion below is "did NOT move" -- worthless unless moving is the
// harness's default.
// =============================================================================

void test_dialog_gating_gameplay_advances_with_no_dialog(void) {
    DialogGatedScene scene;

    scene.tickTimes(5);

    TEST_ASSERT_EQUAL_INT(5, scene.gameplayTicks);
    TEST_ASSERT_EQUAL_INT32(5 * kStepPerTick, scene.playerX);
    TEST_ASSERT_FALSE(scene.runner.isActive());
}

// =============================================================================
// Gate held: AwaitingAdvance
// =============================================================================

void test_dialog_gating_gameplay_frozen_while_awaiting_advance(void) {
    DialogGatedScene scene;
    scene.tickTimes(3);  // move first, so a frozen position is a real value
    const int32_t xBeforeDialog = scene.playerX;
    const int ticksBeforeDialog = scene.gameplayTicks;
    TEST_ASSERT_EQUAL_INT32(3 * kStepPerTick, xBeforeDialog);

    TEST_ASSERT_TRUE(scene.runner.start(kTwoTextScript, 0));
    TEST_ASSERT_TRUE(scene.runner.state() == DialogState::AwaitingAdvance);

    scene.tickTimes(10);

    TEST_ASSERT_EQUAL_INT32(xBeforeDialog, scene.playerX);
    TEST_ASSERT_EQUAL_INT(ticksBeforeDialog, scene.gameplayTicks);
    TEST_ASSERT_EQUAL_INT(13, scene.totalTicks);  // the frames ran; gameplay did not
}

// =============================================================================
// Gate held: ShowingText (auto-advance line), across the whole timer window
// =============================================================================

void test_dialog_gating_gameplay_frozen_while_showing_text(void) {
    DialogGatedScene scene;
    TEST_ASSERT_TRUE(scene.runner.start(kAutoAdvanceScript, 0));
    TEST_ASSERT_TRUE(scene.runner.state() == DialogState::ShowingText);

    // 100 frames of 16 ms = 1600 ms, short of the line's 2400 ms, so the
    // runner stays in ShowingText the whole way.
    scene.tickTimes(100, 16);

    TEST_ASSERT_TRUE(scene.runner.state() == DialogState::ShowingText);
    TEST_ASSERT_EQUAL_INT(0, scene.gameplayTicks);
    TEST_ASSERT_EQUAL_INT32(0, scene.playerX);

    // Past the threshold the runner moves on to line 1 -- still active, so
    // the gate must still hold.
    scene.tickTimes(60, 16);  // +960 ms, total 2560 ms
    TEST_ASSERT_TRUE(scene.runner.state() == DialogState::AwaitingAdvance);
    TEST_ASSERT_TRUE(scene.runner.isActive());
    TEST_ASSERT_EQUAL_INT(0, scene.gameplayTicks);
    TEST_ASSERT_EQUAL_INT32(0, scene.playerX);
}

// =============================================================================
// Gate held: ShowingChoices, including while the player moves the selection
// =============================================================================

void test_dialog_gating_gameplay_frozen_while_showing_choices(void) {
    DialogGatedScene scene;
    TEST_ASSERT_TRUE(scene.runner.start(kChoiceScript, 0));
    TEST_ASSERT_TRUE(scene.runner.state() == DialogState::ShowingChoices);

    scene.tickTimes(4);
    scene.feedDialog(DialogAction::Down);
    scene.tickTimes(4);
    scene.feedDialog(DialogAction::Up);
    scene.tickTimes(4);

    TEST_ASSERT_TRUE(scene.runner.state() == DialogState::ShowingChoices);
    TEST_ASSERT_EQUAL_UINT8(0, scene.runner.selectedChoice());
    TEST_ASSERT_EQUAL_INT(0, scene.gameplayTicks);
    TEST_ASSERT_EQUAL_INT32(0, scene.playerX);
}

// =============================================================================
// Feeding the dialog an action does not itself advance gameplay
//
// A scene that routed input to both the dialog and the world would move the
// player on the very press that turns the page.
// =============================================================================

void test_dialog_gating_feeding_an_action_does_not_advance_gameplay(void) {
    DialogGatedScene scene;
    TEST_ASSERT_TRUE(scene.runner.start(kTwoTextScript, 0));

    // No tick() at all between these -- every position change would have to
    // come from feed() itself.
    scene.feedDialog(DialogAction::Advance);   // line 0 -> line 1
    scene.feedDialog(DialogAction::Down);      // illegal here: ignored
    scene.feedDialog(DialogAction::Advance);   // line 1's next == kNoLine -> Finished

    TEST_ASSERT_EQUAL_INT(0, scene.gameplayTicks);
    TEST_ASSERT_EQUAL_INT32(0, scene.playerX);
    TEST_ASSERT_EQUAL_INT(0, scene.totalTicks);
    TEST_ASSERT_TRUE(scene.runner.state() == DialogState::Finished);
}

// =============================================================================
// Gate released: Finished
// =============================================================================

void test_dialog_gating_gameplay_resumes_once_the_dialog_finishes(void) {
    DialogGatedScene scene;
    TEST_ASSERT_TRUE(scene.runner.start(kTwoTextScript, 0));

    scene.tickTimes(5);
    TEST_ASSERT_EQUAL_INT32(0, scene.playerX);

    scene.feedDialog(DialogAction::Advance);  // -> line 1
    scene.tickTimes(5);
    TEST_ASSERT_EQUAL_INT32(0, scene.playerX);  // still gated mid-script

    scene.feedDialog(DialogAction::Advance);  // -> Finished
    TEST_ASSERT_TRUE(scene.runner.state() == DialogState::Finished);
    TEST_ASSERT_FALSE(scene.runner.isActive());

    scene.tickTimes(5);

    TEST_ASSERT_EQUAL_INT(5, scene.gameplayTicks);
    TEST_ASSERT_EQUAL_INT32(5 * kStepPerTick, scene.playerX);
    TEST_ASSERT_EQUAL_INT(15, scene.totalTicks);  // 5 gated + 5 gated + 5 free
}

// =============================================================================
// Gate released: Inactive, via a choice that ends the dialog and via stop()
// =============================================================================

void test_dialog_gating_gameplay_resumes_after_a_choice_ends_the_dialog(void) {
    DialogGatedScene scene;
    TEST_ASSERT_TRUE(scene.runner.start(kChoiceScript, 0));

    scene.tickTimes(3);
    scene.feedDialog(DialogAction::Down);     // select "Leave"
    scene.feedDialog(DialogAction::Confirm);  // its next == kNoLine -> Finished
    TEST_ASSERT_TRUE(scene.runner.state() == DialogState::Finished);
    TEST_ASSERT_EQUAL_INT32(0, scene.playerX);

    scene.tickTimes(4);

    TEST_ASSERT_EQUAL_INT(4, scene.gameplayTicks);
    TEST_ASSERT_EQUAL_INT32(4 * kStepPerTick, scene.playerX);
}

void test_dialog_gating_gameplay_resumes_after_stop(void) {
    DialogGatedScene scene;
    TEST_ASSERT_TRUE(scene.runner.start(kTwoTextScript, 0));

    scene.tickTimes(6);
    TEST_ASSERT_EQUAL_INT32(0, scene.playerX);

    // The "player cancelled out of the conversation" path: a scene closing
    // the dialog mid-script must release the gate just as a finished one does.
    scene.runner.stop();
    TEST_ASSERT_TRUE(scene.runner.state() == DialogState::Inactive);

    scene.tickTimes(3);

    TEST_ASSERT_EQUAL_INT(3, scene.gameplayTicks);
    TEST_ASSERT_EQUAL_INT32(3 * kStepPerTick, scene.playerX);
}

// =============================================================================
// A second dialog re-freezes gameplay -- the gate is a state read, not a
// one-shot latch consumed by the first conversation.
// =============================================================================

void test_dialog_gating_a_second_dialog_freezes_gameplay_again(void) {
    DialogGatedScene scene;

    TEST_ASSERT_TRUE(scene.runner.start(kTwoTextScript, 0));
    scene.feedDialog(DialogAction::Advance);
    scene.feedDialog(DialogAction::Advance);  // -> Finished
    scene.tickTimes(5);
    const int32_t xAfterFirstDialog = scene.playerX;
    TEST_ASSERT_EQUAL_INT32(5 * kStepPerTick, xAfterFirstDialog);

    TEST_ASSERT_TRUE(scene.runner.start(kTwoTextScript, 0));
    scene.tickTimes(7);

    TEST_ASSERT_EQUAL_INT32(xAfterFirstDialog, scene.playerX);
    TEST_ASSERT_EQUAL_INT(5, scene.gameplayTicks);
}

#else  // !PIXELROOT32_ENABLE_DIALOG

void test_dialog_gating_flag_off_stub_compiles(void) {
    // With the flag off, DialogRunner does not exist and there is no gate to
    // integrate with: a scene simply never freezes gameplay for a dialog.
    // This translation unit compiling and passing without referencing a
    // single dialog symbol is the flag-off half of that contract, matching
    // the convention in test/unit/test_dialog_types/test_dialog_types.cpp.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_DIALOG=0: DialogRunner is not compiled, no gameplay gate exists.");
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
    RUN_TEST(test_dialog_gating_gameplay_advances_with_no_dialog);
    RUN_TEST(test_dialog_gating_gameplay_frozen_while_awaiting_advance);
    RUN_TEST(test_dialog_gating_gameplay_frozen_while_showing_text);
    RUN_TEST(test_dialog_gating_gameplay_frozen_while_showing_choices);
    RUN_TEST(test_dialog_gating_feeding_an_action_does_not_advance_gameplay);
    RUN_TEST(test_dialog_gating_gameplay_resumes_once_the_dialog_finishes);
    RUN_TEST(test_dialog_gating_gameplay_resumes_after_a_choice_ends_the_dialog);
    RUN_TEST(test_dialog_gating_gameplay_resumes_after_stop);
    RUN_TEST(test_dialog_gating_a_second_dialog_freezes_gameplay_again);
#else
    RUN_TEST(test_dialog_gating_flag_off_stub_compiles);
#endif

    return UNITY_END();
}
