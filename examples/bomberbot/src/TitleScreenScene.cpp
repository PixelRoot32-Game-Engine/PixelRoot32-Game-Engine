#include "TitleScreenScene.h"
#include "core/Engine.h"
#include "BomberbotConstants.h"
#include "BomberbotScene.h"
#include "audio/AudioDirector.h"

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace bomberbot {

namespace gfx = pr32::graphics;

void TitleScreenScene::setNextScene(pr32::core::Scene* next) {
    nextScene_ = next;
}

void TitleScreenScene::init() {
    elapsedMs_ = 0;
    AudioDirector::instance().setEnabled(true);
}

void TitleScreenScene::update(unsigned long deltaTime) {
    AudioDirector::instance().update(deltaTime);
    elapsedMs_ += deltaTime;

    auto& input = engine.getInputManager();
    // Enter (native) or BTN_BOMB / BTN_RESTART on ESP32 — same game-side
    // mapping: a press on either the bomb or restart action is treated as
    // "start" from the title screen, mirroring how the Game Over overlay
    // consumes either key to restart.
    if (input.isButtonPressed(BTN_BOMB) || input.isButtonPressed(BTN_RESTART)) {
        if (nextScene_ != nullptr) {
#if PIXELROOT32_ENABLE_AUDIO
            AudioDirector::instance().playSfx(SfxId::MenuBlip);
#endif
            engine.setScene(nextScene_);
        }
    }
}

void TitleScreenScene::drawLine(gfx::Renderer& renderer,
                                const char* text, int y,
                                gfx::Color body, gfx::Color outline,
                                gfx::Color shadow) const {
    // Approximate text width to centre it. The default 5x7 font uses
    // 5 pixels per glyph + 1 px spacing; size=2 doubles both. 6 px per
    // glyph is the right runtime value, but we draw the text twice (once
    // offset by +1, +1 for the shadow) and let `drawTextCentered` recompute
    // the centre each call, so we do not need its exact width.
    const int size = 2;

    // Drop shadow: a +1, +1 offset duplicate in shadow colour gives the
    // classic NES title-screen look. Drawn first so the outline overlays it.
    renderer.drawTextCentered(text, y + 1, shadow, size);
    renderer.drawTextCentered(text, y, shadow, size);
    renderer.drawTextCentered(text, y + 1, shadow, size);

    // Black outline: 4-corner duplicate of the body, drawn UNDER the body
    // and OVER the shadow. With size=2 the body block is ~10 px tall and
    // wide, so a +/-1 px outline already reads clearly.
    renderer.drawTextCentered(text, y - 1, outline, size);
    renderer.drawTextCentered(text, y + 1, outline, size);

    // Body, on top.
    renderer.drawTextCentered(text, y, body, size);
}

void TitleScreenScene::draw(gfx::Renderer& renderer) {
    // Background fill (matches Bomberbot board floor green, so the
    // title-screen "feels" like the same game world).
    renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, gfx::Color::DarkGreen);

    // Logo: "BOMBER" centred around y=66, "BOT" centred around y=100. The
    // shadow/outline/body layering comes from drawLine() above.
    drawLine(renderer, "BOMBER", 66, gfx::Color::White, gfx::Color::Black, gfx::Color::Red);
    drawLine(renderer, "BOT",   100, gfx::Color::White, gfx::Color::Black, gfx::Color::Red);

    // "TM" mark next to the "T" of BOT (right-shifted from centre). Using
    // size=1 keeps it small; the red shadow matches the logo's body.
    renderer.drawTextCentered("TM", 122, gfx::Color::Red, 1);

    // Single menu entry, intentionally without CONTINUE.
    renderer.drawTextCentered("START", 156, gfx::Color::White, 1);

    // Copyright block: the 1987 line is a nod to the reference, but the
    // rights holder is PIXELROOT32 GAME ENGINE, not Nintendo of America.
    renderer.drawTextCentered("(C) 1987 PIXELROOT32 GAME ENGINE", 184, gfx::Color::White, 1);
    renderer.drawTextCentered("LICENSED BY",                       196, gfx::Color::White, 1);
    renderer.drawTextCentered("PIXELROOT32 GAME ENGINE",           208, gfx::Color::White, 1);

    // Blinking "PRESS A" hint — ~0.5 s on / 0.5 s off, square wave.
    const bool hintVisible = ((elapsedMs_ / 500U) % 2U) == 0U;
    if (hintVisible) {
        renderer.drawTextCentered("PRESS A OR ENTER", 228, gfx::Color::Yellow, 1);
    }
}

} // namespace bomberbot
