#include <core/Engine.h>

#include "MonoOledScene.h"

#include <cstdio>

extern pixelroot32::core::Engine engine;

namespace mono_oled {

using pixelroot32::graphics::Color;
using pixelroot32::graphics::Renderer;

namespace cfg = pixelroot32::platforms::config;

namespace {

/// Logical screen, from the -D flags in platformio.ini.
constexpr int kWidth  = cfg::LogicalWidth;
constexpr int kHeight = cfg::LogicalHeight;

/// The built-in font at size 1 is 6x8 pixels per glyph, so a 72-pixel line
/// holds 12 characters and the 40-pixel screen holds 5 rows. Every string
/// below is written to that budget; there is no wrapping and no ellipsis,
/// an overlong line is simply drawn off the edge.
constexpr int kGlyphHeight = 8;

/// Milliseconds between heartbeat marker steps.
constexpr unsigned long kMarkerIntervalMs = 40;

/// On a 1-bit panel there are two colours. Every draw call uses White on
/// the cleared (Black) buffer; anything else collapses to one of these two
/// on hardware, which is exactly the constraint this example is about.
constexpr Color kInk = Color::White;

} // namespace

MonoOledScene::MonoOledScene()
    : page(Page::OUTLINE), markerX(0), markerStep(1), elapsed(0) {}

void MonoOledScene::init() {
    page = Page::OUTLINE;
    markerX = 0;
    markerStep = 1;
    elapsed = 0;
}

void MonoOledScene::update(unsigned long deltaTime) {
    auto& input = engine.getInputManager();

    // One button, one job: advance the page and wrap. isButtonPressed is
    // true for exactly one frame, so a held button does not spin the pages.
    if (input.isButtonPressed(BTN_NEXT)) {
        const auto next = static_cast<std::uint8_t>(page) + 1;
        page = static_cast<Page>(next % static_cast<std::uint8_t>(Page::COUNT));
    }

    elapsed += deltaTime;
    while (elapsed >= kMarkerIntervalMs) {
        elapsed -= kMarkerIntervalMs;
        markerX += markerStep;
        if (markerX >= kWidth - 1) {
            markerX = kWidth - 1;
            markerStep = -1;
        } else if (markerX <= 0) {
            markerX = 0;
            markerStep = 1;
        }
    }
}

void MonoOledScene::draw(Renderer& renderer) {
    switch (page) {
        case Page::OUTLINE:  drawOutlinePage(renderer);  break;
        case Page::FILLED:   drawFilledPage(renderer);   break;
        case Page::GEOMETRY: drawGeometryPage(renderer); break;
        case Page::COUNT:                                break;
    }
    drawHeartbeat(renderer);
}

void MonoOledScene::drawOutlinePage(Renderer& renderer) {
    renderer.drawText("OUTLINE", 0, 0, kInk, 1);

    // A border on the logical bounds. If the offsets in platformio.ini are
    // wrong, this rectangle is the first thing to lose an edge — which is
    // why it is drawn at exactly (0,0)-(kWidth-1, kHeight-1) and not inset.
    renderer.drawRectangle(0, 0, kWidth, kHeight, kInk);

    renderer.drawRectangle(4, 12, 20, 16, kInk);
    renderer.drawCircle(40, 20, 8, kInk);
    renderer.drawLine(52, 28, 68, 12, kInk);
}

void MonoOledScene::drawFilledPage(Renderer& renderer) {
    renderer.drawText("FILLED", 0, 0, kInk, 1);
    renderer.drawRectangle(0, 0, kWidth, kHeight, kInk);

    renderer.drawFilledRectangle(4, 12, 20, 16, kInk);
    renderer.drawFilledCircle(40, 20, 8, kInk);
}

void MonoOledScene::drawGeometryPage(Renderer& renderer) {
    char line[16];

    renderer.drawText("GEOMETRY", 0, 0, kInk, 1);

    // Printed from the same constants the driver uses, so this readout
    // cannot drift from the build. Compare it against the panel: if the
    // text says 72x40 but a character is missing at the right edge, the
    // offsets are wrong, not the font.
    std::snprintf(line, sizeof(line), "LOG %dx%d", kWidth, kHeight);
    renderer.drawText(line, 0, kGlyphHeight + 2, kInk, 1);

    std::snprintf(line, sizeof(line), "PHY %dx%d",
                  cfg::PhysicalDisplayWidth, cfg::PhysicalDisplayHeight);
    renderer.drawText(line, 0, 2 * (kGlyphHeight + 2), kInk, 1);

    std::snprintf(line, sizeof(line), "OFF %d,%d", cfg::XOffset, cfg::YOffset);
    renderer.drawText(line, 0, 3 * (kGlyphHeight + 2), kInk, 1);
}

void MonoOledScene::drawHeartbeat(Renderer& renderer) {
    renderer.drawFilledRectangle(markerX, kHeight - 2, 2, 2, kInk);
}

} // namespace mono_oled
