#include "StarfieldBackground.h"
#include "GameConstants.h"
#include "assets/Background.h"

namespace spaceinvaders {

namespace gfx = pixelroot32::graphics;
namespace math = pixelroot32::math;

StarfieldBackground::StarfieldBackground()
    : pixelroot32::core::Entity(math::Vector2::ZERO(), DISPLAY_WIDTH, DISPLAY_HEIGHT, pixelroot32::core::EntityType::GENERIC) {
    setRenderLayer(0);
}

void StarfieldBackground::update(unsigned long deltaTime) {
    (void)deltaTime;
}

void StarfieldBackground::draw(gfx::Renderer& renderer) {
    renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, gfx::Color::Black);
    
    const int starCount = background_assets::STAR_COUNT;
    const uint8_t* starX = background_assets::STAR_X;
    const uint8_t* starY = background_assets::STAR_Y;
    
    for (int i = 0; i < starCount; ++i) {
        renderer.drawPixel(static_cast<int>(starX[i]),
                           static_cast<int>(starY[i]),
                           gfx::Color::White);
    }
}

}