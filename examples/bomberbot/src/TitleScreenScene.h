#pragma once
#include <cstdint>
#include "core/Scene.h"
#include "graphics/Renderer.h"
#include "platforms/EngineConfig.h"

namespace bomberbot {

/**
 * @class TitleScreenScene
 * @brief NES-style title screen: stacked "BOMBER" / "BOT" logo, a single
 *        "START" entry (CONTINUE is intentionally not shown — the save
 *        system is out of scope for this example), the copyright line, and
 *        a simple "PRESS A" / "PRESS ENTER" hint that transitions to the
 *        game scene on press.
 *
 * Logo rendering imitates the classic NES drop-shadow look without bitmap
 * assets: the same `drawText` is drawn three times per line (red shadow,
 * black outline, white body). The lines use the engine's default 5x7 font
 * scaled 2x so the title fits the 240x240 framebuffer comfortably.
 */
class TitleScreenScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /// Pointer to the game scene to transition into on START. Set by
    /// main.cpp (native) / esp32_dev.h (ESP32) before engine.run() begins.
    static void setNextScene(pixelroot32::core::Scene* next);

private:
    void drawLine(pixelroot32::graphics::Renderer& renderer,
                  const char* text, int y,
                  pixelroot32::graphics::Color body,
                  pixelroot32::graphics::Color outline,
                  pixelroot32::graphics::Color shadow) const;

    static inline pixelroot32::core::Scene* nextScene_ = nullptr;
    unsigned long elapsedMs_ = 0;
};

} // namespace bomberbot
