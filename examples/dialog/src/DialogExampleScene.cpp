#include "DialogExampleScene.h"
#include <core/Engine.h>
#include <input/InputManager.h>
#include <platforms/EngineConfig.h>

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace dialogexample {

namespace gfx      = pr32::graphics;
namespace gameplay = pr32::gameplay;

namespace {

// Six buttons, InputManager order (see platforms/native.h / esp32_dev.h):
//   0 Up, 1 Down, 2 Left, 3 Right, 4 A (confirm/advance), 5 B (cancel).
constexpr uint8_t BTN_UP      = 0;
constexpr uint8_t BTN_DOWN    = 1;
constexpr uint8_t BTN_CONFIRM = 4;
constexpr uint8_t BTN_CANCEL  = 5;

constexpr const char* kNarrator = "Narrator";
constexpr const char* kGuide    = "Guide";

// The script: caller-owned, const, .rodata-resident. DialogRunner never
// copies it and must find it still alive for as long as it is started
// against it (StateMachine's table-ownership convention).
static const gameplay::DialogChoice kChoices[] = {
    {"Say hello",    5, 101},
    {"Ask a riddle", 6, 102},
    {"Walk away",    7, 103},
};

static const gameplay::DialogLine kLines[] = {
    // 0: auto-advances on its own after 1800ms.
    {"Welcome to the PixelRoot32 dialog demo.", kNarrator, 1, 0, 1800, 0, 0,
     gameplay::LineKind::Text, 0},
    // 1-3: a 3-line linear chain, advanced by the player (autoAdvanceMs == 0).
    {"This panel is a DialogBox, drawn from a DialogRunner.", kGuide, 2, 0, 0, 0, 0,
     gameplay::LineKind::Text, 0},
    {"The runner is headless: no Font, no Renderer, only states.", kGuide, 3, 0, 0, 0, 0,
     gameplay::LineKind::Text, 0},
    {"Now try a choice.", kGuide, 4, 0, 0, 0, 0, gameplay::LineKind::Text, 0},
    // 4: a Choice line with 3 options branching to 3 different endings.
    // Kept short -- a Choice line's prompt never pages.
    {"What do you do?", kGuide, gameplay::kNoLine, 0, 0, 0, 3, gameplay::LineKind::Choice, 0},
    // 5-7: one ending per choice, all rejoining at the End line below.
    {"Hello to you too!", kGuide, 8, 0, 0, 0, 0, gameplay::LineKind::Text, 0},
    {"Why did the chicken cross the road?", kGuide, 8, 0, 0, 0, 0, gameplay::LineKind::Text, 0},
    {"You walk away without a word.", kGuide, 8, 0, 0, 0, 0, gameplay::LineKind::Text, 0},
    // 8: End.
    {nullptr, nullptr, gameplay::kNoLine, 0, 0, 0, 0, gameplay::LineKind::End, 0},
};

static const gameplay::DialogScript kScript{
    kLines, kChoices, static_cast<uint16_t>(sizeof(kLines) / sizeof(kLines[0])),
    static_cast<uint16_t>(sizeof(kChoices) / sizeof(kChoices[0]))};

}  // namespace

void DialogExampleScene::onDialogEvent(void* owner, const gameplay::DialogEvent& event) {
    static_cast<DialogExampleScene*>(owner)->handleDialogEvent(event);
}

void DialogExampleScene::handleDialogEvent(const gameplay::DialogEvent& event) {
    if (event.type != gameplay::DialogEventType::ChoiceConfirmed) {
        return;
    }
    // ChoiceConfirmed is emitted BEFORE the runner follows the chosen
    // DialogChoice::next, so it is still on the ShowingChoices line here --
    // choice() is valid and addresses the option the player just confirmed.
    const gameplay::DialogChoice* choice = runner.choice(event.choice);
    lastChoiceText_ = (choice != nullptr) ? choice->text : nullptr;
}

void DialogExampleScene::init() {
    Scene::init();

    gfx::DialogBoxStyle style;
    style.x = 4;
    style.w = static_cast<int16_t>(pr32::platforms::config::LogicalWidth - 8);
    style.borderWidth = 1;
    style.padding = 4;
    style.textSize = 1;
    style.lineSpacing = 1;
    style.fixedPosition = true;

    // measureHeightPx() only reads style.w/font/padding/borderWidth/textSize/
    // lineSpacing, so it can be sized before style.h/y are known.
    style.h = gfx::DialogBox::measureHeightPx(kScript, style);
    style.y = static_cast<int16_t>(pr32::platforms::config::LogicalHeight - style.h - 4);
    box.setStyle(style);

    lastChoiceText_ = nullptr;
    runner.configure(this, &DialogExampleScene::onDialogEvent);
    runner.start(kScript, 0);
}

void DialogExampleScene::update(unsigned long deltaTime) {
    Scene::update(deltaTime);

    auto& input = engine.getInputManager();

    const gameplay::DialogState state = runner.state();
    const bool canRestart =
        state == gameplay::DialogState::Inactive || state == gameplay::DialogState::Finished;
    if (canRestart) {
        if (input.isButtonPressed(BTN_CONFIRM)) {
            lastChoiceText_ = nullptr;
            runner.start(kScript, 0);
        }
        return;
    }

    if (input.isButtonPressed(BTN_UP)) {
        runner.feed(gameplay::DialogAction::Up);
    }
    if (input.isButtonPressed(BTN_DOWN)) {
        runner.feed(gameplay::DialogAction::Down);
    }
    if (input.isButtonPressed(BTN_CONFIRM)) {
        // Confirm aliases Advance in the two text states, so the same
        // button both advances lines and confirms a choice.
        runner.feed(gameplay::DialogAction::Confirm);
    }
    if (input.isButtonPressed(BTN_CANCEL)) {
        runner.feed(gameplay::DialogAction::Cancel);
    }

    runner.update(deltaTime);
}

void DialogExampleScene::draw(gfx::Renderer& renderer) {
    renderer.drawFilledRectangle(0, 0, renderer.getLogicalWidth(), renderer.getLogicalHeight(),
                                  gfx::Color::Black);
    Scene::draw(renderer);

    box.draw(renderer, runner);
    drawHud(renderer);
}

void DialogExampleScene::drawHud(gfx::Renderer& renderer) {
    renderer.drawText("UP/DOWN choose  A confirm/advance", 4, 4, gfx::Color::Cyan, 1);

    const gameplay::DialogState state = runner.state();
    if (state == gameplay::DialogState::Inactive || state == gameplay::DialogState::Finished) {
        renderer.drawText("Dialog finished -- press A to replay", 4, 16, gfx::Color::Yellow, 1);
    }
    if (lastChoiceText_ != nullptr) {
        renderer.drawText("Last choice:", 4, 28, gfx::Color::Gray, 1);
        renderer.drawText(lastChoiceText_, 4, 38, gfx::Color::White, 1);
    }
}

}  // namespace dialogexample
