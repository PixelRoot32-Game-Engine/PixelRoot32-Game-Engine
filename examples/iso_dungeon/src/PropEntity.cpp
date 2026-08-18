#include "PropEntity.h"

#include "math/Vector2.h"
#include "IsoDraw.h"

namespace pr32 = pixelroot32;

namespace iso_dungeon {

namespace gfx = pr32::graphics;
namespace core = pr32::core;
namespace math = pr32::math;

PropEntity::PropEntity()
    : core::Entity(math::Vector2::ZERO(), 0, 0, core::EntityType::GENERIC) {
    setRenderLayer(1);
    release();
}

void PropEntity::configure(const PropSpec& spec) {
    sprite_ = spec.sprite;
    footY_ = spec.footY;
    centreX_ = gameplay::cellToScreenX(spec.tileX, spec.tileY, kTileProjection);
    centreY_ = gameplay::cellToScreenY(spec.tileX, spec.tileY, kTileProjection);

    position = math::Vector2(centreX_, centreY_);
    width = sprite_->width;
    height = sprite_->height;
    setVisible(true);

#if PIXELROOT32_ENABLE_DEPTH_SORT
    // A prop never moves WITHIN a room, so its key is written once here rather
    // than every frame. Same expression the hero uses -- the projected screen Y
    // of the ground point -- because the sort only works if every participant
    // measures depth the same way.
    depthKey = static_cast<int16_t>(centreY_);
#endif
}

void PropEntity::release() {
    sprite_ = nullptr;
    setVisible(false);

#if PIXELROOT32_ENABLE_DEPTH_SORT
    // An empty slot still sits in the scene's entity array and still gets
    // compared by the insertion sort. Pinning its key below every real one
    // keeps it at the front of the order instead of letting a stale key from
    // the previous room shuffle live props around for no reason. It draws
    // nothing either way; this only stops it from being a moving part.
    depthKey = INT16_MIN;
#endif
}

void PropEntity::update(unsigned long deltaTime) {
    (void)deltaTime;
}

void PropEntity::draw(gfx::Renderer& renderer) {
    // Scene::draw already skips invisible entities, so an empty slot never
    // reaches here. The check is for the direct-call path, where the cost of
    // being wrong is a null dereference rather than a missing sprite.
    if (sprite_ == nullptr) {
        return;
    }
    drawAtCell(renderer, *sprite_, footY_, centreX_, centreY_);
}

}  // namespace iso_dungeon
