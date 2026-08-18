#pragma once

#include "core/Entity.h"
#include "graphics/Renderer.h"
#include "graphics/StaticLayerSnapshot.h"

#include "IsoDungeonConstants.h"
#include "RoomCatalog.h"

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
     *
     * `[[nodiscard]]` because that fallback is invisible from the outside --
     * the room still looks right, it just costs what it used to. A caller that
     * genuinely does not care has to write the cast and say why, which is the
     * only difference between an accepted trade-off and an unnoticed
     * regression. StaticLayerSnapshot logs the byte count it could not get.
     */
    [[nodiscard]] bool reserveSnapshot(pixelroot32::graphics::Renderer& renderer);

    /// Forwards the scene's pre-beginFrame hook to the snapshot.
    void adviseFramebufferBeforeBeginFrame(pixelroot32::graphics::Renderer& renderer) const;

    /**
     * @brief Switches which room this renderer draws.
     *
     * Invalidates the snapshot, because the snapshot holds a picture of the
     * room that was on screen a moment ago and every pixel of it is now wrong.
     * Skipping this would not merely draw the old room -- it would keep drawing
     * it forever, since restore() succeeds and drawTiles() never runs again.
     *
     * Reclaiming the framebuffer is the caller's job and is NOT done here: see
     * IsoDungeonScene::onRoomEnter, the only caller, for why forceFullRedraw()
     * has to accompany this call.
     *
     * @param room Room to draw from now on. Must outlive this renderer, which
     *             every `kRooms` entry does -- they are constexpr statics.
     */
    void setRoom(const RoomSpec& room);

private:
    /// Draws the 49 floor and wall tiles. The slow path the snapshot replaces.
    void drawTiles(pixelroot32::graphics::Renderer& renderer);

    /// The room currently on screen. Never null after the scene's init().
    const RoomSpec* room_ = nullptr;

    pixelroot32::graphics::StaticLayerSnapshot snapshot_;
};

}  // namespace iso_dungeon
