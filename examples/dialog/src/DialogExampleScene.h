#pragma once
#include <core/Scene.h>
#include <gameplay/DialogRunner.h>
#include <gameplay/DialogTypes.h>
#include <graphics/DialogBox.h>
#include <graphics/Renderer.h>

namespace dialogexample {

/**
 * @class DialogExampleScene
 * @brief Minimal DialogRunner + DialogBox demo.
 *
 * Shows the three MVP behaviours the dialog system exists for: a line that
 * auto-advances on its own timer, a 3-line chain the player advances by
 * hand, and a 3-option Choice line whose branches rejoin at one ending. The
 * scene only translates buttons into semantic DialogAction values and feeds
 * them to a headless DialogRunner; DialogBox draws whatever the runner is
 * currently showing, and the scene reacts to ChoiceConfirmed to report which
 * branch the player took.
 */
class DialogExampleScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    /// Trampoline required by DialogEventFn's plain function-pointer shape
    /// (no std::function, no capturing lambda); forwards to the instance.
    static void onDialogEvent(void* owner, const pixelroot32::gameplay::DialogEvent& event);
    void handleDialogEvent(const pixelroot32::gameplay::DialogEvent& event);
    void drawHud(pixelroot32::graphics::Renderer& renderer);

    pixelroot32::gameplay::DialogRunner runner;
    pixelroot32::graphics::DialogBox    box;
    /// Flash literal from the confirmed DialogChoice; nullptr until the
    /// player has confirmed one. Never owned, never copied.
    const char* lastChoiceText_ = nullptr;
};

}  // namespace dialogexample
