#pragma once

#include "core/Entity.h"
#include "graphics/Renderer.h"
#include "graphics/StaticLayerSnapshot.h"

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

    /**
     * @brief Reserves the snapshot buffer. Call from the scene's init().
     *
     * Separate from the constructor because it allocates and the renderer's
     * logical size is not known until the engine has initialised. A false
     * return is not an error the game has to handle: the room simply draws its
     * 49 tiles every frame, exactly as it did before the snapshot existed.
     */
    bool reserveSnapshot(pixelroot32::graphics::Renderer& renderer);

    /// Forwards the scene's pre-beginFrame hook to the snapshot.
    void adviseFramebufferBeforeBeginFrame(pixelroot32::graphics::Renderer& renderer) const;

private:
    /// Draws the 49 floor and wall tiles. The slow path the snapshot replaces.
    void drawTiles(pixelroot32::graphics::Renderer& renderer);

    pixelroot32::graphics::StaticLayerSnapshot snapshot_;
};

}  // namespace iso_dungeon
