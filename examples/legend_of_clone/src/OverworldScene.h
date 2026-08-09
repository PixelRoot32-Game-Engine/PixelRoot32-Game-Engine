#pragma once

#include "TopDownScene.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

namespace legend_of_clone {

/**
 * @class OverworldScene
 * @brief Four adjacent NES-style overworld screens, with a cave that leads
 *        into the dungeon.
 *
 * Everything about moving between screens lives in TopDownScene. What is left
 * here is what makes this map the overworld rather than the dungeon: its data,
 * its status readout, and the one tile that changes scene.
 */
class OverworldScene : public TopDownScene {
public:
    /**
     * @brief Puts the player at the cave mouth on the next init(), facing away
     *        from it.
     *
     * Called by the dungeon just before it hands control back. The engine runs
     * init() on every scene swap, so without this the player would be returned
     * to the start of the game each time they climbed a staircase.
     */
    void spawnAtCaveMouth();

protected:
    Setup setup() override;
    void drawStatusBar(pixelroot32::graphics::Renderer& renderer) override;
    void onPlayerSettled() override;

private:
    /// Points at the exported overworld map. Three pointers, no pixel data.
    TileWorld world_;

    /// Where the next init() drops the player. Defaults to the game's start.
    int    spawnCol_ = kPlayerStartCol;
    int    spawnRow_ = kPlayerStartRow;
    Facing spawnFacing_ = Facing::Down;
};

} // namespace legend_of_clone

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
