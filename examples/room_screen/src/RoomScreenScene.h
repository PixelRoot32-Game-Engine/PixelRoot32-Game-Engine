#pragma once
#include "core/Scene.h"
#include "graphics/Camera2D.h"
#include "graphics/Renderer.h"
#include "gameplay/RoomGraph.h"
#include "Player.h"

namespace room_screen {

/**
 * @brief Room/screen navigation example over the exported main scene.
 *
 * The asset (`assets/roomscreen_main_scene.h` / `.cpp`) exports a 30x30 tile
 * map at 16 px/tile split into four 15x15 rooms laid out in a 2x2 grid. The
 * player walks the world freely and collides with the Items layer (the
 * collision layer). When the player crosses a room boundary, the scene
 * follows the room connection — snapping the camera to the target room and
 * placing the player on its entry edge — or pushes them back if there is no
 * connection that way.
 */
class RoomScreenScene : public pixelroot32::core::Scene {
public:
    RoomScreenScene();
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    /// Camera viewport (one room = one screen).
    pixelroot32::graphics::Camera2D camera;
    /// Fixed-capacity 4-room graph.
    pixelroot32::gameplay::RoomGraph<4> rooms_;
    /// The player (red circle for now).
    Player player;

    /// Enter a room and snap the camera to its top-left corner.
    void enterRoom(uint16_t idx);

    /// After the player walks, detect a room-boundary crossing and transition.
    void handleRoomTransition();

    /// Clamp the player back inside a room (walls / unconnected edges).
    void clampPlayerToRoom(const pixelroot32::gameplay::Room& room);

    /// RoomGraph onEnter callback. Camera bounds are already updated when this fires.
    static void onRoomEnterCallback(int fromIdx, int toIdx, void* userData);
};

} // namespace room_screen
