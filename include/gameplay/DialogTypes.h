/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "platforms/PlatformDefaults.h"
#if PIXELROOT32_ENABLE_DIALOG
#include <cstdint>

namespace pixelroot32 {
// Opaque portrait handle. graphics::Sprite's definition lives in
// graphics/Renderer.h; this header must NOT include it -- DialogRunner is
// headless by design (no Font, no Renderer, no pixels) and DialogTypes.h is
// the layer that keeps it so. A pointer needs no definition, only a name:
// DialogBox.cpp includes Renderer.h and reads width/height there. The same
// forward-declaration precedent already exists in graphics/Font.h.
namespace graphics {
struct Sprite;
struct Sprite2bpp;
struct Sprite4bpp;
}  // namespace graphics

namespace gameplay {
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
///
/// kLineFlagAllowCancel lets DialogAction::Cancel act on a LineKind::Choice
/// line, and that action only emits DialogEventType::Cancelled with the
/// line's tag. The runner stays on the line; call DialogRunner::stop() to
/// close the dialog, which is legal from inside the DialogEventFn.
inline constexpr uint8_t kLineFlagAllowCancel = 0x01;

/// kLineFlagPortraitRight draws the line's DialogLine::portrait at the
/// top-RIGHT of the panel's content area instead of the default top-left,
/// letting two speakers face each other across alternating lines. Read by
/// graphics::DialogBox ONLY -- DialogRunner never masks this bit, so it is
/// covered by the "silently ignored" rule above as far as the runner is
/// concerned, exactly like a future reserved bit on an older build. The
/// vertical placement never changes (top, always); only the side mirrors.
inline constexpr uint8_t kLineFlagPortraitRight = 0x02;

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

struct DialogChoice;  // Defined below; forward-declared for ChoiceFilterFn.

/**
 * @typedef ChoiceFilterFn
 * @brief Optional per-choice visibility predicate for a ShowingChoices line.
 *
 * A game supplies this through DialogRunner::setChoiceFilter; the runner
 * calls it lazily -- on every choiceCount()/choice()/select() query and on
 * every ShowingChoices feed() -- so the visible set always reflects current
 * game state (e.g. hiding a "Buy" option the player can no longer afford)
 * with no explicit invalidation call. Returning false HIDES the choice:
 * navigation skips it, indices the runner reports are compacted over the
 * visible choices only, and a line with every choice hidden behaves exactly
 * like a line with zero usable choices. A null filter (the default) shows
 * every choice, preserving pre-filter behaviour bit for bit.
 *
 * Must be pure: it must not call back into the runner. The runner invokes
 * it while evaluating its own accessors, so a reentrant runner call from
 * inside the filter would recurse; a fail-open in-filter guard shows every
 * choice rather than recursing, but well-behaved filters never rely on it.
 *
 * @param owner Opaque pointer bound by DialogRunner::setChoiceFilter.
 * @param line Id of the ShowingChoices line being presented.
 * @param scriptIndex Offset into DialogScript::choices of the candidate
 *        choice (line.firstChoice + visible index is NOT valid under a
 *        filter; the runner translates).
 * @param choice Pointer to that candidate choice; never null.
 * @return true to show the choice, false to hide it.
 */
using ChoiceFilterFn = bool (*)(void* owner, LineId line, ChoiceId scriptIndex,
                                 const DialogChoice* choice);

/**
 * @struct DialogChoice
 * @brief One selectable option on a DialogState::ShowingChoices line.
 *
 * 12 bytes on ESP32 (two 4-byte pointers), 24 on 64-bit native -- this exact
 * figure is the regression guard `test_dialog_types_dialog_choice_size_guard`
 * pins, so growing this struct is a conscious, reviewed change rather
 * than silent drift in a game's flash budget. Grew from 8/16 when the
 * optional second-column `detail` literal was added (multi-column option
 * rows, second post-MVP dialog item).
 */
struct DialogChoice {
    const char* text;            ///< Main label, left-aligned. Flash literal. Never copied.
    const char* detail = nullptr;  ///< Optional second column (e.g. a price), right-aligned.
                                   ///< Flash literal. nullptr (the default, so existing
                                   ///< 3-value initializers keep compiling) draws the
                                   ///< classic single-column row with identical geometry.
                                   ///< Never wrapped, like `text`: keep `text + detail`
                                   ///< within the panel's content width (see DialogBoxStyle).
    LineId      next;   ///< kNoLine ends the dialog.
    uint16_t    tag;    ///< Opaque game code.
};

/**
 * @struct DialogLine
 * @brief One line of a DialogScript: either shown text or a choice prompt.
 *
 * 36 bytes on ESP32, 64 on 64-bit native. The pre-portrait fields sum to 18
 * on ESP32 with no padding between them -- next, tag and autoAdvanceMs land
 * on offsets 8, 10 and 12 and are already aligned, plus 2 trailing padding
 * bytes rounding to the 4-byte alignment the two leading pointers impose
 * (20). The three portrait pointers add 12/24 for 32/56, plus the 1-byte
 * portraitPaletteSlot (33/57) rounded up to the pointer alignment (36/64).
 * This exact figure is the regression guard
 * `test_dialog_types_dialog_line_size_guard` pins, so growing this struct
 * is a conscious, reviewed change rather than silent drift in a game's
 * flash budget. Grew from 20/32 when the optional speaker portrait was
 * added (third post-MVP dialog item), and from 24/40 when 2bpp/4bpp
 * portraits joined it.
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
    uint8_t     flags;          ///< kLineFlagAllowCancel, kLineFlagPortraitRight; see above.
    /// Optional speaker portrait, drawn 1:1 at the top of the content area
    /// (left by default, right with kLineFlagPortraitRight). Exactly one of
    /// the three pointers should be set: a 1bpp graphics::Sprite drawn in
    /// DialogBoxStyle::portraitInk, a graphics::Sprite2bpp, or a
    /// graphics::Sprite4bpp -- the two multi-color formats draw through
    /// portraitPaletteSlot instead of a tint, so one line can carry a
    /// higher-detail face than 1bpp allows. When more than one pointer is
    /// set, 4bpp wins, then 2bpp, then 1bpp. All three nullptr (the default,
    /// so existing 9-value initializers keep compiling) draws no portrait
    /// with identical geometry, as does DialogBoxStyle::portraitsEnabled
    /// set to false. Flash-resident asset data; never copied, never owned.
    /// The runner never reads these fields.
    const graphics::Sprite* portrait = nullptr;
    /// 4-color face; same position and priority rules as `portrait` above.
    const graphics::Sprite2bpp* portrait2bpp = nullptr;
    /// 16-color face; same position and priority rules as `portrait` above.
    const graphics::Sprite4bpp* portrait4bpp = nullptr;
    /// Sprite palette slot (0..7) resolving 2bpp/4bpp portrait colors.
    /// Ignored for a 1bpp portrait, which uses portraitInk instead.
    uint8_t portraitPaletteSlot = 0;
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

static_assert(sizeof(DialogChoice) <= 3 * sizeof(void*), "DialogChoice grew");
// Trivially destructible: the script is flash data, never destroyed.

}  // namespace gameplay
}  // namespace pixelroot32
#endif // PIXELROOT32_ENABLE_DIALOG
