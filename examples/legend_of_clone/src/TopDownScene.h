#pragma once

#include "platforms/PlatformDefaults.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "core/Scene.h"
#include "graphics/Camera2D.h"
#include "graphics/Renderer.h"
#include "graphics/StaticTilemapLayerCache.h"
#include "gameplay/RoomGraph.h"
#include "gameplay/RoomLayout.h"

#include "GameConstants.h"
#include "PlayerActor.h"
#include "TileWorld.h"

namespace legend_of_clone {

/**
 * @class TopDownScene
 * @brief A screen-by-screen room grid with a scrolling transition between
 *        rooms, a camera pinned per room, and one player.
 *
 * Everything the overworld and the dungeon do identically lives here. That is
 * not tidiness for its own sake: the transition below contains one step that is
 * easy to leave out and impossible to spot afterwards (see beginTransition),
 * and a second copy of it in the dungeon would be a second chance to get it
 * wrong.
 *
 * A subclass supplies data through setup() and, if it wants them, two hooks:
 * drawStatusBar() and onPlayerSettled().
 *
 * ### How a room change works
 *
 * The camera never follows the player. It sits pinned at the current room's
 * origin, which is what makes the world read as a grid of fixed screens rather
 * than a scrolling field. When the player's box crosses a room edge and that
 * edge has a connection, the scene:
 *
 *  1. widens the camera bounds to cover both rooms, because Camera2D clamps to
 *     its bounds and a slide that leaves the current room would otherwise be
 *     silently pinned to the edge;
 *  2. disables the player, which locks out input for the whole slide;
 *  3. interpolates camera and player together over kTransitionDurationMs;
 *  4. calls RoomGraph::enterRoom() on arrival, which resets the bounds to the
 *     new room and fires the onEnter hook.
 *
 * The player is interpolated along with the camera rather than teleported,
 * because the NES walks Link the last few pixels across the seam while the
 * screen scrolls. Teleporting looks like a cut; sliding looks like the original.
 *
 * ### Why the terrain goes through StaticTilemapLayerCache
 *
 * A pinned camera is the case the cache exists for. While the player walks
 * around a room the camera does not move, so the terrain is byte-identical
 * frame after frame and the cache replays it with one memcpy instead of
 * blitting 330 tiles. It rebuilds by itself during a slide, when the camera
 * sample changes every frame — which is exactly when a rebuild is correct.
 */
class TopDownScene : public pixelroot32::core::Scene {
public:
    /**
     * @brief Rooms any one map may have.
     *
     * Both maps are a 2x2 block, so one concrete RoomGraph serves both and the
     * base class stays free of templates — which matters here, because a
     * template would duplicate the graph's code into flash once per size on a
     * target that counts kilobytes.
     */
    static constexpr uint16_t kMaxRooms = 4;

    TopDownScene();

    void init() override;
    void update(unsigned long deltaTime) override;
    void adviseFramebufferBeforeBeginFrame(pixelroot32::graphics::Renderer& renderer) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Fired by RoomGraph once a room change completes.
     *
     * Where a full game would swap the music track or spawn the new room's
     * enemies. Here it only records the arrival for the debug readout.
     */
    void onRoomEnter(int fromIdx, int toIdx) override;

protected:
    /// Everything a concrete scene must supply for the base to run.
    struct Setup {
        /**
         * 16 RGB565 entries for the tilemap, from the asset export.
         *
         * Separate from the sprite table because the scene runs in dual palette
         * mode: the world and the player each get their own 16 slots instead of
         * sharing one crowded table. Both are installed by pointer and are
         * never copied, so both must outlive the scene.
         */
        const uint16_t* backgroundPalette = nullptr;
        /// 16 RGB565 entries for the player. See backgroundPalette.
        const uint16_t* spritePalette = nullptr;
        /// Exported room data. Rects and connections both come from here.
        const pixelroot32::gameplay::RoomLayer* roomLayer = nullptr;
        /// The map to draw and collide against, already attached to its export.
        const TileWorld* world = nullptr;
        uint16_t startRoom = 0;
        int startCol = 0;   ///< Spawn column, in world tiles.
        int startRow = 0;   ///< Spawn row, in world tiles.
        Facing startFacing = Facing::Down;
    };

    /**
     * @brief Attaches this scene's world and returns everything init() needs.
     *
     * Called once per init(), which the engine runs on every scene swap — so a
     * scene that is re-entered gets a fresh spawn, and one that wants to
     * remember where the player came from stores that itself before the swap.
     */
    virtual Setup setup() = 0;

    /// Draws the reserved strip below the playfield. Default paints it black.
    virtual void drawStatusBar(pixelroot32::graphics::Renderer& renderer);

    /**
     * @brief Called each frame after the player has moved, when no transition
     *        is running.
     *
     * Where a scene reacts to the tile the player is standing on — a cave
     * mouth, a staircase. Deliberately not called mid-slide: the player is
     * being interpolated across a seam then and is briefly on tiles they never
     * chose to step on.
     */
    virtual void onPlayerSettled() {}

    PlayerActor& player() { return player_; }
    const TileWorld& world() const { return *world_; }
    uint16_t currentRoom() const { return rooms_.currentRoomIndex(); }
    int lastFromRoom() const { return lastFromRoom_; }
    bool worldReady() const { return worldReady_; }

    /// World tile the player's centre is standing on.
    void playerCell(int& outCol, int& outRow) const;

private:
    pixelroot32::graphics::Camera2D camera_;
    pixelroot32::gameplay::RoomGraph<kMaxRooms> rooms_;
    pixelroot32::graphics::StaticTilemapLayerCache tilemapLayerCache_;
    PlayerActor player_;
    const TileWorld* world_ = nullptr;

    /// False when buildRoomGraph() rejected the layer; draw() then reports it.
    bool worldReady_ = false;

    // --- Room transition state -----------------------------------------------
    bool          transitionActive_ = false;
    unsigned long transitionElapsed_ = 0;
    uint16_t      transitionTarget_ = 0;
    int           cameraFromX_ = 0, cameraFromY_ = 0;
    int           cameraToX_   = 0, cameraToY_   = 0;
    int           playerFromX_ = 0, playerFromY_ = 0;
    int           playerToX_   = 0, playerToY_   = 0;

    /// Last completed room change, for the debug readout.
    int lastFromRoom_ = -1;

    void snapCameraToRoom(uint16_t roomIdx);
    void checkRoomExit();
    void beginTransition(pixelroot32::gameplay::RoomDir dir, uint16_t targetIdx);
    void updateTransition(unsigned long deltaTime);

    /**
     * @brief The camera position the tilemap cache keys its snapshot on.
     *
     * Read from Camera2D rather than from the renderer's offset, which is what
     * the cache's documentation suggests. The offset is only set inside draw(),
     * so adviseFramebufferBeforeBeginFrame — which the engine runs before
     * beginFrame — would see the *previous* frame's value and disagree with
     * draw() on the exact frame the camera moves. Both callers read the camera
     * itself and always agree.
     */
    void cameraSample(int& outX, int& outY) const;

    /// The one static layer, in the shape StaticTilemapLayerCache takes.
    pixelroot32::graphics::TileMap4bppDrawSpec terrainLayer() const;

    /// Bridges RoomGraph's C callback to the Scene::onRoomEnter virtual.
    static void onRoomEnterCallback(int fromIdx, int toIdx, void* userData);
};

} // namespace legend_of_clone

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
