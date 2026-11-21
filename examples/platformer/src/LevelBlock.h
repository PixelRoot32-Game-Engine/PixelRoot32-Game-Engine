#pragma once
#include "graphics/Color.h"
#include "graphics/Renderer.h"
#include "physics/StaticActor.h"

namespace platformerdemo {

/**
 * @class LevelBlock
 * @brief `StaticActor` scenery: floor, walls, the crate shelf, one-way platforms.
 *
 * The only thing this subclass adds is drawing and the one-way flag: physics,
 * mass and the fact that nothing can move it come from `StaticActor`.
 */
class LevelBlock : public pixelroot32::physics::StaticActor {
public:
    /**
     * @param x Left edge in world pixels.
     * @param y Top edge in world pixels.
     * @param w Width in pixels.
     * @param h Height in pixels.
     * @param fill Colour the block is drawn in.
     * @param oneWay true for a platform that only blocks from above.
     */
    LevelBlock(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h,
               pixelroot32::graphics::Color fill, bool oneWay)
        : pixelroot32::physics::StaticActor(x, y, w, h)
        , fillColor(fill) {
        setShape(pixelroot32::core::CollisionShape::AABB);
        setBounce(false);
        setOneWay(oneWay);
    }

    void draw(pixelroot32::graphics::Renderer& renderer) override {
        const int x = static_cast<int>(position.x);
        const int y = static_cast<int>(position.y);
        renderer.drawFilledRectangle(x, y, width, height, fillColor);

        if (isOneWay()) {
            // The bright lip marks the one surface that blocks: a one-way
            // platform is landed on from above and passed through from below.
            renderer.drawFilledRectangle(x, y, width, 2, pixelroot32::graphics::Color::LightGreen);
        }
    }

private:
    pixelroot32::graphics::Color fillColor;
};

}
