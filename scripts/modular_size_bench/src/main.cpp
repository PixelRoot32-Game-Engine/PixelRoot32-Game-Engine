#include <Arduino.h>
#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <core/Engine.h>
#include <platforms/EngineConfig.h>

#include "BenchScene.h"

#if PIXELROOT32_SIZEOF_PROBE
#include <drivers/esp32/ESP32AudioScheduler.h>
#include <physics/CollisionSystem.h>
#if PIXELROOT32_ENABLE_PARTICLES
#include <graphics/particles/ParticleEmitter.h>
#endif
#if PIXELROOT32_ENABLE_UI_SYSTEM
#include <graphics/ui/UIManager.h>
#include <graphics/ui/UILabel.h>
#include <graphics/ui/UIButton.h>
#endif
#endif

namespace pr32 = pixelroot32;

const int BTN_UP = 32;
const int BTN_DOWN = 27;
const int BTN_LEFT = 33;
const int BTN_RIGHT = 14;
const int BTN_A = 13;
const int BTN_B = 12;

pr32::graphics::DisplayConfig config(
    pr32::graphics::DisplayType::ST7735,
    DISPLAY_ROTATION,
    PHYSICAL_DISPLAY_WIDTH,
    PHYSICAL_DISPLAY_HEIGHT,
    LOGICAL_WIDTH,
    LOGICAL_HEIGHT,
    X_OFF_SET,
    Y_OFF_SET
);

pr32::input::InputConfig inputConfig(BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B);
pr32::core::Engine engine(config, inputConfig);
modular_size_bench::BenchScene benchScene;

#if PIXELROOT32_SIZEOF_PROBE
// Intentionally incomplete — compiler error prints the size as template argument.
template <size_t N>
struct PixelRoot32SizeofProbe;

void forceSizeofProbe() {
    PixelRoot32SizeofProbe<sizeof(pr32::audio::ESP32AudioScheduler)> audioSchedulerSize;
    PixelRoot32SizeofProbe<sizeof(pr32::physics::CollisionSystem)> collisionSystemSize;
#if PIXELROOT32_ENABLE_PARTICLES
    PixelRoot32SizeofProbe<sizeof(pr32::graphics::particles::ParticleEmitter)> particleEmitterSize;
#endif
#if PIXELROOT32_ENABLE_UI_SYSTEM
    PixelRoot32SizeofProbe<sizeof(pr32::graphics::ui::UIManager)> uiManagerSize;
    PixelRoot32SizeofProbe<sizeof(pr32::graphics::ui::UILabel)> uiLabelSize;
    PixelRoot32SizeofProbe<sizeof(pr32::graphics::ui::UIButton)> uiButtonSize;
#endif
    (void)audioSchedulerSize;
    (void)collisionSystemSize;
}
#endif

void setup() {
#if PIXELROOT32_SIZEOF_PROBE
    forceSizeofProbe();
#endif
    engine.init();
    engine.setScene(&benchScene);
}

void loop() {
    engine.run();
}
