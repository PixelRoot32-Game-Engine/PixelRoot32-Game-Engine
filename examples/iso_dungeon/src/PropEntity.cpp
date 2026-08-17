#include "PropEntity.h"

#include "math/Vector2.h"
#include "IsoDraw.h"

namespace pr32 = pixelroot32;

namespace iso_dungeon {

namespace gfx = pr32::graphics;
namespace core = pr32::core;
namespace math = pr32::math;

PropEntity::PropEntity(const gfx::Sprite4bpp& sprite, int footY, int tileX, int tileY)
    : core::Entity(math::Vector2(gameplay::cellToScreenX(tileX, tileY, kTileProjection),
                                 gameplay::cellToScreenY(tileX, tileY, kTileProjection)),
                   sprite.width, sprite.height, core::EntityType::GENERIC),
      sprite_(sprite),
      footY_(footY),
      centreX_(gameplay::cellToScreenX(tileX, tileY, kTileProjection)),
      centreY_(gameplay::cellToScreenY(tileX, tileY, kTileProjection)) {
    setRenderLayer(1);

#if PIXELROOT32_ENABLE_DEPTH_SORT
    // A prop never moves, so its key is written once here rather than every
    // frame. Same expression the hero uses -- the projected screen Y of the
    // ground point -- because the sort only works if every participant
    // measures depth the same way.
    depthKey = static_cast<int16_t>(centreY_);
#endif
}

void PropEntity::update(unsigned long deltaTime) {
    (void)deltaTime;
}

void PropEntity::draw(gfx::Renderer& renderer) {
    drawAtCell(renderer, sprite_, footY_, centreX_, centreY_);
}

}  // namespace iso_dungeon
