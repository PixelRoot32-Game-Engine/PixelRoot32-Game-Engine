#pragma once

#include "EffectDemoConstants.h"
#include "core/Scene.h"
#include "core/Log.h"
#include "graphics/Camera2D.h"
#include "graphics/ui/UILabel.h"
#include "graphics/ui/UIButton.h"
#include "graphics/ui/UIVerticalLayout.h"

#include <memory>
#include <cstdint>

namespace effectdemo {

class EffectDemoScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    // Public trigger methods for callbacks
    void triggerShake();
    void triggerPunchUp();
    void triggerPunchDown();
    void triggerPunchLeft();
    void triggerPunchRight();
    void triggerOffset();
    void cancelAll();

private:
    EffectDemoState currentState = EffectDemoState::MAIN;
    unsigned long effectElapsed = 0;
    const char* activeEffectName = nullptr;

    // Camera2D at dead-center
    pixelroot32::graphics::Camera2D camera{0, 0};

    // UI Elements
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> titleLabel;
    std::unique_ptr<pixelroot32::graphics::ui::UIVerticalLayout> buttonLayout;

    // Effect buttons (7 total)
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> btnShake;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> btnPunchUp;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> btnPunchDown;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> btnPunchLeft;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> btnPunchRight;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> btnOffset;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> btnCancelAll;

    // Active effect label
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> lblActiveEffect;

    // Navigation labels
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> lblNavigate;
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> lblSelect;
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> lblBack;

    // Helper methods
    void setupMenu();
    void showMenu();
    void activateEffect(const char* name);
};

} // namespace effectdemo
