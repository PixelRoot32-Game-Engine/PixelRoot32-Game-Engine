#pragma once
#include "core/Scene.h"
#include "graphics/Camera2D.h"
#include "graphics/Renderer.h"
#include "gameplay/RoomGraph.h"

namespace room_screen {

/**
 * @brief Room/screen navigation example over the exported main scene.
 *
 * The asset (`assets/main_scene.h` / `main_scene.cpp`) exports a 30x30 tile
 * map at 16 px/tile split into four 15x15 rooms laid out in a 2x2 grid.
 * Arrow-key input walks the room connections (Up/Down/Left/Right); entering
 * a room clamps the camera to its bounds and repositions it on that room's
 * top-left corner. The three exported layers (background, items, details)
 * are drawn every frame in world space.
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

    /// Enter a room and snap the camera to its top-left corner.
    void enterRoom(uint16_t idx);

    /// Walk a connection in the given direction (no-op on a wall).
    void moveTowards(pixelroot32::gameplay::RoomDir dir);

    /// RoomGraph onEnter callback. Camera bounds are already updated when this fires.
    static void onRoomEnterCallback(int fromIdx, int toIdx, void* userData);
};

} // namespace room_screen
