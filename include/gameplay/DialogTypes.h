#pragma once
#include "platforms/PlatformDefaults.h"
#if PIXELROOT32_ENABLE_DIALOG
#include <cstdint>

namespace pixelroot32::gameplay {

using LineId   = uint16_t;
using ChoiceId = uint8_t;

inline constexpr LineId   kNoLine   = 0xFFFF;
inline constexpr ChoiceId kNoChoice = 0xFF;

/// DialogLine::flags bits. Reserved bits MUST be 0.
inline constexpr uint8_t kLineFlagAllowCancel = 0x01;

enum class LineKind : uint8_t { Text = 0, Choice = 1, End = 2 };

enum class DialogState : uint8_t {
    Inactive = 0,        ///< Not started, or stopped.
    ShowingText,         ///< Text line with autoAdvanceMs > 0; update(dt) ticks.
    AwaitingAdvance,     ///< Text line with autoAdvanceMs == 0; waits for the player.
    ShowingChoices,      ///< Choice line; Up/Down/Confirm/Cancel apply.
    Finished             ///< Ended. Distinct from Inactive so the result survives a frame.
};

enum class DialogAction : uint8_t { None = 0, Advance, Up, Down, Confirm, Cancel };

enum class DialogEventType : uint8_t { LineEnter = 0, ChoiceConfirmed, Cancelled, Ended };

struct DialogEvent {
    DialogEventType type;
    LineId          line;    ///< Line the event concerns; kNoLine on Ended after a bad id.
    ChoiceId        choice;  ///< kNoChoice except on ChoiceConfirmed.
    uint16_t        tag;     ///< Line tag, or choice tag on ChoiceConfirmed. Never interpreted.
};

using DialogEventFn = void (*)(void* owner, const DialogEvent& event);

/// 8 bytes on ESP32 (4-byte pointer), 16 on 64-bit native.
struct DialogChoice {
    const char* text;   ///< Flash literal. Never copied.
    LineId      next;   ///< kNoLine ends the dialog.
    uint16_t    tag;    ///< Opaque game code.
};

/// 20 bytes on ESP32 (not 16 -- see section 6), 32 on 64-bit native.
struct DialogLine {
    const char* text;           ///< nullptr for a choice-only line.
    const char* speaker;        ///< nullptr means no speaker.
    LineId      next;           ///< LineKind::Text only.
    uint16_t    tag;            ///< Carried on LineEnter; 0 is a legal tag.
    uint16_t    autoAdvanceMs;  ///< 0 waits for the player (AwaitingAdvance).
    ChoiceId    firstChoice;    ///< Index into DialogScript::choices.
    uint8_t     choiceCount;    ///< Clamped to config::DialogMaxChoices at runtime.
    LineKind    kind;
    uint8_t     flags;          ///< kLineFlagAllowCancel; honored from slice 2b.
};

/// Caller-owned, const, .rodata-resident. NOT copied; must outlive the runner.
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
