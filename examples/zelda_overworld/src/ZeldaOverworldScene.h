#pragma once

#include "core/Scene.h"
#include "graphics/Camera2D.h"
#include "graphics/Renderer.h"
#include "gameplay/RoomGraph.h"

#include "GameConstants.h"
#include "PlayerActor.h"

namespace zelda_overworld {

/**
 * @class ZeldaOverworldScene
 * @brief Four adjacent NES-style overworld screens and the scrolling
 *        transition between them.
 *
 * Iteration 1 exists to prove one thing: that a player walking off the edge of
 * one screen lands correctly on the next, with the camera, the room graph and
 * the collision map all agreeing about where the seam is. There are no enemies,
 * no items and no caves — anything that would obscure that question is absent
 * on purpose.
 *
 * ### How a screen change works
 *
 * The camera never follows the player. It sits pinned at the current room's
 * origin, which is what makes the overworld read as a grid of fixed screens
 * rather than a scrolling field. When the player's box crosses a room edge and
 * that edge has a connection, the scene:
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
 */
class ZeldaOverworldScene : public pixelroot32::core::Scene {
public:
    ZeldaOverworldScene();

    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Fired by RoomGraph once a screen change completes.
     *
     * Where a full game would swap the music track, spawn the new screen's
     * enemies or roll a secret. Iteration 1 has none of those, so this only
     * records the arrival for the debug readout.
     */
    void onRoomEnter(int fromIdx, int toIdx) override;

private:
    /// Camera viewport is the playfield, not the panel — the status bar is not scrolled.
    pixelroot32::graphics::Camera2D camera;

    /// Four screens, wired 2x2. Fixed capacity, no allocation.
    pixelroot32::gameplay::RoomGraph<4> rooms_;

    PlayerActor player_;

    /// False when buildRoomGraph() rejected the layer; draw() then reports it.
    bool worldReady_ = false;

    // --- Screen transition state ---------------------------------------------
    bool          transitionActive_ = false;
    unsigned long transitionElapsed_ = 0;
    uint16_t      transitionTarget_ = 0;
    int           cameraFromX_ = 0, cameraFromY_ = 0;
    int           cameraToX_   = 0, cameraToY_   = 0;
    int           playerFromX_ = 0, playerFromY_ = 0;
    int           playerToX_   = 0, playerToY_   = 0;

    /// Last completed transition, for the debug readout.
    int lastFromRoom_ = -1;

    /// Pins the camera at a room's origin and places nothing else.
    void snapCameraToRoom(uint16_t roomIdx);

    /// Detects a room edge crossing and either starts a transition or blocks it.
    void checkRoomExit();

    /// Sets up the slide described in the class comment.
    void beginTransition(pixelroot32::gameplay::RoomDir dir, uint16_t targetIdx);

    /// Advances the slide; completes it on the final frame.
    void updateTransition(unsigned long deltaTime);

    /// Draws the reserved status strip below the playfield.
    void drawStatusBar(pixelroot32::graphics::Renderer& renderer);

    /// Bridges RoomGraph's C callback to the Scene::onRoomEnter virtual.
    static void onRoomEnterCallback(int fromIdx, int toIdx, void* userData);
};

} // namespace zelda_overworld
