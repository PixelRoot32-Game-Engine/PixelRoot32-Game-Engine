#pragma once

#include <core/Scene.h>
#include <graphics/Renderer.h>
#include <platforms/EngineConfig.h>

#include <cstdint>

namespace mono_oled {

/** @brief Index of the single button declared in both platform headers. */
inline constexpr std::uint8_t BTN_NEXT = 0;

/** @brief What the scene is currently drawing. Advanced by the one button. */
enum class Page : std::uint8_t {
    OUTLINE = 0,  ///< Outline primitives: rectangle, circle, line.
    FILLED,       ///< Filled primitives: rectangle, circle.
    GEOMETRY,     ///< The logical size and the offset into the panel.
    COUNT
};

/**
 * @class MonoOledScene
 * @brief Everything a 1-bit OLED can draw, on a 72x40 logical screen.
 *
 * Three things this example exists to show, none of which any other
 * example in this repository covers:
 *
 * 1. **Which half of the renderer survives on a monochrome panel.**
 *    `U8G2_Drawer` implements `BaseDrawSurface`, but `drawTileDirect()` and
 *    `getSpriteBuffer()` are deliberate no-ops — its own header says tile
 *    rendering has "no benefit" on a monochrome display. So the tilemap
 *    fast path, the sprite blit path and the palette system are all
 *    unavailable, and what remains is the primitives that the drawer
 *    overrides with native U8G2 calls: line, rectangle, circle, their
 *    filled variants, and text. This scene uses only those.
 *
 * 2. **Logical size is not physical size.** The panel exposes a 72x40
 *    window into a 128x64 controller framebuffer, at offset (28, 24). The
 *    GEOMETRY page prints both so the distinction is visible rather than
 *    described.
 *
 * 3. **Designing for one button.** There is no D-pad and no touch panel, so
 *    a single press has to carry the whole interaction. Here it advances
 *    the page and wraps.
 *
 * A marker sweeps across the bottom on every page. It is not decoration: on
 * a static screen there is no way to tell a running program from a frozen
 * one, and a frozen U8G2 buffer looks exactly like a correct still frame.
 */
class MonoOledScene : public pixelroot32::core::Scene {
public:
    MonoOledScene();

    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    void drawOutlinePage(pixelroot32::graphics::Renderer& renderer);
    void drawFilledPage(pixelroot32::graphics::Renderer& renderer);
    void drawGeometryPage(pixelroot32::graphics::Renderer& renderer);
    void drawHeartbeat(pixelroot32::graphics::Renderer& renderer);

    Page page;              ///< Current page, advanced by BTN_NEXT.
    int  markerX;           ///< Heartbeat marker position, in logical pixels.
    int  markerStep;        ///< Marker direction: +1 or -1.
    unsigned long elapsed;  ///< Milliseconds accumulated since the last step.
};

} // namespace mono_oled
