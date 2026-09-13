/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "platforms/PlatformDefaults.h"
#if PIXELROOT32_ENABLE_DIALOG
#include <cstdint>

namespace pixelroot32::gameplay {

using LineId   = uint16_t;
using ChoiceId = uint8_t;

inline constexpr LineId   kNoLine   = 0xFFFF;
inline constexpr ChoiceId kNoChoice = 0xFF;

/// DialogLine::flags bits. DialogRunner reads this field through an
/// explicit mask of each bit it knows about (e.g. `flags &
/// kLineFlagAllowCancel`), never by testing `flags` for truthiness. Unknown
/// or future bits are therefore silently IGNORED, not rejected -- a line
/// carrying a reserved bit the runner does not understand still runs
/// normally, it just has no effect from that bit. This keeps a script
/// forward-compatible with an older runner build instead of failing closed
/// on it.
inline constexpr uint8_t kLineFlagAllowCancel = 0x01;

/**
 * @enum LineKind
 * @brief Distinguishes what a DialogLine presents when the runner enters it.
 *
 * DialogRunner::enterLine() switches on this to pick the entry state: a
 * `Text` line becomes ShowingText or AwaitingAdvance depending on
 * DialogLine::autoAdvanceMs, a `Choice` line goes straight to
 * ShowingChoices, and `End` finishes the dialog. Keeping this a plain
 * three-value enum -- rather than inferring the kind from which fields
 * happen to be populated -- makes a malformed script fail loudly instead
 * of guessing.
 */
enum class LineKind : uint8_t { Text = 0, Choice = 1, End = 2 };

/**
 * @enum DialogState
 * @brief The five states DialogRunner can be in; feed() is total over State x DialogAction.
 *
 * ShowingText and AwaitingAdvance both mean "on a text line" but are kept
 * distinct so a per-line auto-advance timer (ShowingText) and a
 * player-driven wait (AwaitingAdvance) are each independently observable
 * and testable, rather than folding both into one state with a hidden
 * autoAdvanceMs branch. Finished is kept separate from Inactive so a game
 * can read the dialog's result on the frame after it ends without racing
 * a reset back to Inactive.
 */
enum class DialogState : uint8_t {
    Inactive = 0,        ///< Not started, or stopped.
    ShowingText,         ///< Text line with autoAdvanceMs > 0; update(dt) ticks.
    AwaitingAdvance,     ///< Text line with autoAdvanceMs == 0; waits for the player.
    ShowingChoices,      ///< Choice line; Up/Down/Confirm/Cancel apply.
    Finished             ///< Ended. Distinct from Inactive so the result survives a frame.
};

/**
 * @enum DialogAction
 * @brief The semantic input vocabulary DialogRunner::feed() accepts.
 *
 * Deliberately abstracted away from any physical input (touch tap, D-pad,
 * button) so the same runner works unmodified whether a game drives it
 * from touch (Chess) or buttons (Top Down City); the game translates its
 * own input into one of these actions before calling feed().
 */
enum class DialogAction : uint8_t { None = 0, Advance, Up, Down, Confirm, Cancel };

/**
 * @enum DialogEventType
 * @brief Identifies which fields of a DialogEvent are meaningful.
 *
 * Delivered through the single DialogEventFn callback rather than one
 * callback per kind, so a game's dialog handler is one switch statement
 * instead of four separate registrations.
 */
enum class DialogEventType : uint8_t { LineEnter = 0, ChoiceConfirmed, Cancelled, Ended };

/**
 * @struct DialogEvent
 * @brief The single payload type delivered to DialogEventFn for every kind of dialog event.
 *
 * One shared shape for all four DialogEventType values, rather than a
 * union or a type per event, keeps the runner's synchronous callback
 * interface a single function pointer with no std::function and no
 * heap-allocated event objects.
 */
struct DialogEvent {
    DialogEventType type;    ///< Which event this is; decides which fields below apply.
    LineId          line;    ///< Line the event concerns; kNoLine on Ended after a bad id.
    ChoiceId        choice;  ///< kNoChoice except on ChoiceConfirmed.
    uint16_t        tag;     ///< Line tag, or choice tag on ChoiceConfirmed. Never interpreted.
};

using DialogEventFn = void (*)(void* owner, const DialogEvent& event);

/**
 * @struct DialogChoice
 * @brief One selectable option on a DialogState::ShowingChoices line.
 *
 * 8 bytes on ESP32 (4-byte pointer), 16 on 64-bit native -- this exact
 * figure is the regression guard `test_dialog_types_dialog_choice_size_guard`
 * pins, so growing this struct is a conscious, reviewed change rather
 * than silent drift in a game's flash budget.
 */
struct DialogChoice {
    const char* text;   ///< Flash literal. Never copied.
    LineId      next;   ///< kNoLine ends the dialog.
    uint16_t    tag;    ///< Opaque game code.
};

/**
 * @struct DialogLine
 * @brief One line of a DialogScript: either shown text or a choice prompt.
 *
 * 20 bytes on ESP32, 32 on 64-bit native. The fields sum to 18 on ESP32
 * with no padding between them -- next, tag and autoAdvanceMs land on
 * offsets 8, 10 and 12 and are already aligned. The extra 2 bytes are
 * trailing padding, rounding the struct to the 4-byte alignment its two
 * leading pointers impose. This exact figure is
 * the regression guard
 * `test_dialog_types_dialog_line_size_guard` pins, so growing this struct
 * is a conscious, reviewed change rather than silent drift in a game's
 * flash budget.
 */
struct DialogLine {
    const char* text;           ///< nullptr for a choice-only line.
    const char* speaker;        ///< nullptr means no speaker.
    LineId      next;           ///< LineKind::Text only.
    uint16_t    tag;            ///< Carried on LineEnter; 0 is a legal tag.
    uint16_t    autoAdvanceMs;  ///< 0 waits for the player (AwaitingAdvance).
    // A script may hold more than 255 choices in total, but no single line can ADDRESS
    // one at or past index 255: ChoiceId is uint8_t and kNoChoice (0xFF) is its sentinel,
    // so an index that reached it would be indistinguishable from "no choice".
    // DialogRunner::choiceCount() clamps any pair that would reach it, or that overruns
    // DialogScript::choiceCount, rather than reading out of bounds.
    ChoiceId    firstChoice;    ///< Index into DialogScript::choices; must stay below 255.
    uint8_t     choiceCount;    ///< Clamped to DialogMaxChoices, to the table, and below 255.
    LineKind    kind;           ///< Discriminator; decides which fields above apply.
    uint8_t     flags;          ///< kLineFlagAllowCancel; unknown bits are ignored, not rejected.
};

/**
 * @struct DialogScript
 * @brief The caller-owned, immutable table a DialogRunner is started with.
 *
 * Lives in .rodata as flash data, never copied and never owned by the
 * runner: this is what keeps DialogRunner headless and heap-free, since
 * the runner only ever holds a pointer to a script the game already
 * allocated statically. Must outlive every DialogRunner started from it.
 */
struct DialogScript {
    const DialogLine*   lines;
    const DialogChoice* choices;
    uint16_t            lineCount;
    uint16_t            choiceCount;
};

static_assert(sizeof(DialogChoice) <= 2 * sizeof(void*), "DialogChoice grew");
// Trivially destructible: the script is flash data, never destroyed.

} // namespace pixelroot32::gameplay
#endif // PIXELROOT32_ENABLE_DIALOG
