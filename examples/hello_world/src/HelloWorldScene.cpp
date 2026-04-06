#include "HelloWorldScene.h"

#include <core/Engine.h>
#include <core/Log.h>

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace helloworld {

namespace gfx = pr32::graphics;
namespace logging = pr32::core::logging;
using logging::LogLevel;
using logging::log;
using gfx::Color;

static constexpr uint8_t PR32_PALETTE_SIZE = 16;

static constexpr uint16_t PR32_PALETTE_RGB565[PR32_PALETTE_SIZE] = {
    0x0000, 0xFFFF, 0x1907, 0x025F,
    0x061F, 0x13C2, 0x3648, 0xA7F3,
    0xFEA0, 0xFCE3, 0xC3FF, 0xB884,
    0x6822, 0x7977, 0xCE79, 0x8C71
};

void HelloWorldScene::init() {
    log("HelloWorldScene: initializing...");

    gfx::setPalette(gfx::PaletteType::PR32);

    int screenWidth = engine.getRenderer().getLogicalWidth();
    int screenHeight = engine.getRenderer().getLogicalHeight();

    textLabel = std::make_unique<gfx::ui::UILabel>(
        "Hello PixelRoot32!",
        pr32::math::Vector2(0, screenHeight / 2 - 10),
        Color::White,
        1
    );
    textLabel->centerX(screenWidth);
    textLabel->setRenderLayer(2);
    addEntity(textLabel.get());

    buttonPressLabel = std::make_unique<gfx::ui::UILabel>(
        "",
        pr32::math::Vector2(0, screenHeight / 2 + 10),
        Color::White,
        1
    );
    buttonPressLabel->centerX(screenWidth);
    buttonPressLabel->setRenderLayer(2);
    addEntity(buttonPressLabel.get());

    snprintf(buttonPressText, sizeof(buttonPressText), "Press: -");

    changeBackground();

    log(LogLevel::Info, "HelloWorldScene: initialized");
}

void HelloWorldScene::update(unsigned long deltaTime) {
    Scene::update(deltaTime);

    checkButtonPress();

    frameCounter++;

    if (frameCounter >= COLOR_CHANGE_INTERVAL) {
        frameCounter = 0;
        changeBackground();
    }
}

void HelloWorldScene::draw(pixelroot32::graphics::Renderer& renderer) {
    renderer.drawFilledRectangle(
        0, 0,
        renderer.getLogicalWidth(),
        renderer.getLogicalHeight(),
        backgroundColor
    );

    Scene::draw(renderer);
}

void HelloWorldScene::changeBackground() {
    static uint8_t colorIndex = 0;
    static const uint8_t fixedRange[] = {0, 2, 5, 7, 10, 12, 14};
    colorIndex = (colorIndex + 1) % 7;
    backgroundColor = static_cast<Color>(fixedRange[colorIndex]);
}

void HelloWorldScene::checkButtonPress() {
    auto& input = engine.getInputManager();

    if (input.isButtonPressed(0)) {
        snprintf(buttonPressText, sizeof(buttonPressText), "Press: U");
    } else if (input.isButtonPressed(1)) {
        snprintf(buttonPressText, sizeof(buttonPressText), "Press: D");
    } else if (input.isButtonPressed(2)) {
        snprintf(buttonPressText, sizeof(buttonPressText), "Press: L");
    } else if (input.isButtonPressed(3)) {
        snprintf(buttonPressText, sizeof(buttonPressText), "Press: R");
    } else if (input.isButtonPressed(4)) {
        snprintf(buttonPressText, sizeof(buttonPressText), "Press: A");
    } else if (input.isButtonPressed(5)) {
        snprintf(buttonPressText, sizeof(buttonPressText), "Press: B");
    }

    buttonPressLabel->setText(buttonPressText);
}


} // namespace helloworld
