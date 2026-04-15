#include "SnakeBackground.h"
#include "math/Vector2.h"

namespace pr32 = pixelroot32;

namespace snake {

namespace gfx = pr32::graphics;
namespace core = pr32::core;
namespace math = pr32::math;

SnakeBackground::SnakeBackground()
    : core::Entity(math::Vector2::ZERO(), DISPLAY_WIDTH, DISPLAY_HEIGHT, core::EntityType::GENERIC) {
    setRenderLayer(0);
}

void SnakeBackground::update(unsigned long deltaTime) {
    (void)deltaTime;
}

void SnakeBackground::draw(gfx::Renderer& renderer) {
    renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, gfx::Color::Black);

    int playAreaX = 0;
    int playAreaY = TOP_UI_GRID_ROWS * CELL_SIZE;
    int playAreaW = GRID_WIDTH * CELL_SIZE - 1;
    int playAreaH = (GRID_HEIGHT - TOP_UI_GRID_ROWS) * CELL_SIZE - 1;
    renderer.drawRectangle(playAreaX, playAreaY, playAreaW, playAreaH, gfx::Color::Red);
}

}