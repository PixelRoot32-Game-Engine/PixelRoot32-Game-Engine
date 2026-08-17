#pragma once

#include "core/Entity.h"
#include "graphics/Renderer.h"

#include "IsoDungeonConstants.h"

namespace iso_dungeon {

/**
 * @class RoomRenderer
 * @brief Draws the static room -- backdrop, floor diamonds and back walls.
 *
 * Lives on render layer 0, below every actor and prop. Its whole job is a
 * single row-major sweep over the layout, which is already the correct
 * isometric paint order (see the implementation for why).
 *
 * The walls are drawn here rather than as depth-sorted entities on purpose,
 * and it is not a shortcut: both walls hug the two BACK edges of the room, so
 * their screen depth is lower than that of any reachable tile. Nothing the
 * player can stand on is ever behind them, so sorting them would be work with
 * no possible effect on the image.
 */
class RoomRenderer : public pixelroot32::core::Entity {
public:
    RoomRenderer();

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
};

}  // namespace iso_dungeon
