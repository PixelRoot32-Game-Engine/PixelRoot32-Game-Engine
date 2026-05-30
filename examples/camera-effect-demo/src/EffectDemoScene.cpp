#include "EffectDemoConstants.h"
#include "EffectDemoScene.h"

#include <core/Engine.h>
#include <cstdio>

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace effectdemo {

using Color = pr32::graphics::Color;
using Vector2 = pr32::math::Vector2;
using Scalar = pr32::math::Scalar;
using pr32::math::toScalar;

static EffectDemoScene* sSceneInstance = nullptr;

// -----------------------------------------------------------------------
// Static callback wrappers
// -----------------------------------------------------------------------
static void onShake() { if (sSceneInstance) sSceneInstance->triggerShake(); }
static void onPunchUp() { if (sSceneInstance) sSceneInstance->triggerPunchUp(); }
static void onPunchDown() { if (sSceneInstance) sSceneInstance->triggerPunchDown(); }
static void onPunchLeft() { if (sSceneInstance) sSceneInstance->triggerPunchLeft(); }
static void onPunchRight() { if (sSceneInstance) sSceneInstance->triggerPunchRight(); }
static void onOffset() { if (sSceneInstance) sSceneInstance->triggerOffset(); }
static void onCancelAll() { if (sSceneInstance) sSceneInstance->cancelAll(); }

// =============================================================================
// init()
// =============================================================================

void EffectDemoScene::init() {
    clearEntities();
    pr32::graphics::setPalette(pr32::graphics::PaletteType::PR32);

    int sw = engine.getRenderer().getLogicalWidth();
    int sh = engine.getRenderer().getLogicalHeight();
    sSceneInstance = this;

    // Title
    titleLabel = std::make_unique<pr32::graphics::ui::UILabel>(
        "Camera Effects", Vector2(0, static_cast<int>(TITLE_Y)),
        Color::White, TITLE_FONT_SIZE);
    titleLabel->centerX(sw);
    titleLabel->setRenderLayer(2);
    addEntity(titleLabel.get());

    // Button layout
    buttonLayout = std::make_unique<pr32::graphics::ui::UIVerticalLayout>(
        static_cast<int>((sw - BTN_WIDTH) / 2),
        static_cast<int>(BTN_START_Y),
        static_cast<int>(BTN_WIDTH),
        static_cast<int>(sh - BTN_START_Y - NAV_INSTR_Y_OFFSET - 10));
    buttonLayout->setPadding(0);
    buttonLayout->setSpacing(static_cast<int>(BTN_GAP));
    buttonLayout->setScrollEnabled(true);
    buttonLayout->setNavigationButtons(BTN_NAV_UP, BTN_NAV_DOWN);
    buttonLayout->setButtonStyle(Color::White, Color::Cyan, Color::White, Color::Black);
    buttonLayout->setRenderLayer(2);
    addEntity(buttonLayout.get());

    // Create 7 effect buttons
    Vector2 btnSize{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)};

    btnShake = std::make_unique<pr32::graphics::ui::UIButton>(
        "Shake", BTN_SELECT, Vector2::ZERO(), btnSize,
        onShake, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);

    btnPunchUp = std::make_unique<pr32::graphics::ui::UIButton>(
        "Punch Up", BTN_SELECT, Vector2::ZERO(), btnSize,
        onPunchUp, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);

    btnPunchDown = std::make_unique<pr32::graphics::ui::UIButton>(
        "Punch Down", BTN_SELECT, Vector2::ZERO(), btnSize,
        onPunchDown, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);

    btnPunchLeft = std::make_unique<pr32::graphics::ui::UIButton>(
        "Punch Left", BTN_SELECT, Vector2::ZERO(), btnSize,
        onPunchLeft, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);

    btnPunchRight = std::make_unique<pr32::graphics::ui::UIButton>(
        "Punch Right", BTN_SELECT, Vector2::ZERO(), btnSize,
        onPunchRight, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);

    btnOffset = std::make_unique<pr32::graphics::ui::UIButton>(
        "Offset", BTN_SELECT, Vector2::ZERO(), btnSize,
        onOffset, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);

    btnCancelAll = std::make_unique<pr32::graphics::ui::UIButton>(
        "Cancel All", BTN_SELECT, Vector2::ZERO(), btnSize,
        onCancelAll, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);

    // Active effect label (shown during EFFECT_ACTIVE state)
    lblActiveEffect = std::make_unique<pr32::graphics::ui::UILabel>(
        "", Vector2(0, static_cast<int>(TITLE_Y) + 14),
        Color::Yellow, TITLE_FONT_SIZE);
    lblActiveEffect->centerX(sw);
    lblActiveEffect->setRenderLayer(2);
    addEntity(lblActiveEffect.get());

    // Navigation labels
    lblNavigate = std::make_unique<pr32::graphics::ui::UILabel>(
        "UP/DOWN: Navigate", Vector2(0, sh - static_cast<int>(NAV_INSTR_Y_OFFSET)),
        Color::Cyan, INSTRUCTION_FONT_SIZE);
    lblNavigate->centerX(sw);
    lblNavigate->setRenderLayer(2);
    addEntity(lblNavigate.get());

    lblSelect = std::make_unique<pr32::graphics::ui::UILabel>(
        "A: Select", Vector2(0, sh - static_cast<int>(SEL_INSTR_Y_OFFSET)),
        Color::Cyan, INSTRUCTION_FONT_SIZE);
    lblSelect->centerX(sw);
    lblSelect->setRenderLayer(2);
    addEntity(lblSelect.get());

    lblBack = std::make_unique<pr32::graphics::ui::UILabel>(
        "B: Back", Vector2(0, sh - 15),
        Color::Cyan, INSTRUCTION_FONT_SIZE);
    lblBack->centerX(sw);
    lblBack->setRenderLayer(2);
    addEntity(lblBack.get());

    currentState = EffectDemoState::MAIN;
    activeEffectName = nullptr;
    showMenu();
}

// =============================================================================
// update()
// =============================================================================

void EffectDemoScene::update(unsigned long dt) {
    Scene::update(dt);

    auto& input = engine.getInputManager();

    if (currentState == EffectDemoState::EFFECT_ACTIVE) {
        effectElapsed += dt;
        if (effectElapsed >= AUTO_CANCEL_MS) {
            cancelAll();
        }
        return;
    }

    // MAIN state: handle menu navigation
    static bool wasBack = false;
    bool isBackPressed = input.isButtonPressed(BTN_BACK);
    if (isBackPressed && !wasBack) {
        // Back on main menu is a no-op (no submenus)
    }
    wasBack = isBackPressed;

    buttonLayout->handleInput(input);
}

// =============================================================================
// draw()
// =============================================================================

void EffectDemoScene::draw(pr32::graphics::Renderer& renderer) {
    int cx = renderer.getLogicalWidth() / 2;
    int cy = renderer.getLogicalHeight() / 2;

    // Step 1: Static reference shapes — drawn BEFORE camera effect offset
    // Crosshair at center
    renderer.drawLine(cx - 30, cy, cx + 30, cy, Color::DarkGray);
    renderer.drawLine(cx, cy - 30, cx, cy + 30, Color::DarkGray);
    // Center marker
    renderer.drawPixel(cx, cy, Color::White);

    // Step 2: Apply camera + effect offset (pushes transform into renderer)
    camera.apply(renderer, getCameraEffectOffset());

    // Step 3: Dynamic shapes — drawn AFTER effect offset, will jitter/shift
    // Filled rect around center (will displace with effect)
    renderer.drawRectangle(cx - 20, cy - 20, 40, 40, Color::Red);
    renderer.drawFilledRectangle(cx - 15, cy - 15, 30, 30, Color::LightRed);
    // Inner marker that shows the displacement clearly
    renderer.drawFilledRectangle(cx - 4, cy - 4, 8, 8, Color::Yellow);

    // Step 4: Reset display offset for UI
    renderer.setDisplayOffset(0, 0);

    // Step 5: Draw UI (labels, buttons) — unaffected by camera/effects
    Scene::draw(renderer);
}

// =============================================================================
// showMenu()
// =============================================================================

void EffectDemoScene::showMenu() {
    buttonLayout->clearElements();
    int sw = engine.getRenderer().getLogicalWidth();
    titleLabel->setText("Camera Effects");

    buttonLayout->addElement(btnShake.get());
    buttonLayout->addElement(btnPunchUp.get());
    buttonLayout->addElement(btnPunchDown.get());
    buttonLayout->addElement(btnPunchLeft.get());
    buttonLayout->addElement(btnPunchRight.get());
    buttonLayout->addElement(btnOffset.get());
    buttonLayout->addElement(btnCancelAll.get());

    titleLabel->centerX(sw);
}

// =============================================================================
// activateEffect()
// =============================================================================

void EffectDemoScene::activateEffect(const char* name) {
    currentState = EffectDemoState::EFFECT_ACTIVE;
    effectElapsed = 0;
    activeEffectName = name;

    if (lblActiveEffect) {
        int sw = engine.getRenderer().getLogicalWidth();
        static char buf[48];
        std::snprintf(buf, sizeof(buf), "Active: %s", name);
        lblActiveEffect->setText(buf);
        lblActiveEffect->centerX(sw);
    }
}

// =============================================================================
// Effect trigger methods
// =============================================================================

void EffectDemoScene::triggerShake() {
    cameraEffects.triggerShake(toScalar(PRESET_SHAKE_AMP), PRESET_SHAKE_DUR);
    activateEffect("Shake");
}

void EffectDemoScene::triggerPunchUp() {
    cameraEffects.triggerPunch(toScalar(PRESET_PUNCH_AMP), PRESET_PUNCH_DUR, Vector2::UP());
    activateEffect("Punch Up");
}

void EffectDemoScene::triggerPunchDown() {
    cameraEffects.triggerPunch(toScalar(PRESET_PUNCH_AMP), PRESET_PUNCH_DUR, Vector2::DOWN());
    activateEffect("Punch Down");
}

void EffectDemoScene::triggerPunchLeft() {
    cameraEffects.triggerPunch(toScalar(PRESET_PUNCH_AMP), PRESET_PUNCH_DUR, Vector2::LEFT());
    activateEffect("Punch Left");
}

void EffectDemoScene::triggerPunchRight() {
    cameraEffects.triggerPunch(toScalar(PRESET_PUNCH_AMP), PRESET_PUNCH_DUR, Vector2::RIGHT());
    activateEffect("Punch Right");
}

void EffectDemoScene::triggerOffset() {
    cameraEffects.triggerOffset(toScalar(PRESET_OFFSET_AMP), PRESET_OFFSET_DUR);
    activateEffect("Offset");
}

void EffectDemoScene::cancelAll() {
    cameraEffects.cancelAll();
    currentState = EffectDemoState::MAIN;
    effectElapsed = 0;
    activeEffectName = nullptr;

    if (lblActiveEffect) {
        lblActiveEffect->setText("");
    }
    showMenu();
}

} // namespace effectdemo
