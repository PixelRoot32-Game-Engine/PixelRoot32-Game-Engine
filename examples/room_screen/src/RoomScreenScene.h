#pragma once
#include "core/Scene.h"
#include "graphics/Camera2D.h"
#include "graphics/Renderer.h"
#include "gameplay/RoomGraph.h"

namespace room_screen {

/**
 * @brief Minimal 2-room example demonstrating RoomGraph.
 *
 * Two rooms laid out horizontally (Room 0 → Room 1). Arrow-key
 * input triggers an enterRoom transition, which clamps the camera
 * to the target room's bounds and fires an onEnter callback.
 */
class RoomScreenScene : public pixelroot32::core::Scene {
public:
    RoomScreenScene();
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    /// Camera viewport (one screen).
    pixelroot32::graphics::Camera2D camera;
    /// Fixed-capacity 2-room graph.
    pixelroot32::gameplay::RoomGraph<2> rooms_;

    /// RoomGraph onEnter callback. Camera bounds are already updated when this fires.
    static void onRoomEnterCallback(int fromIdx, int toIdx, void* userData);
};

} // namespace room_screen
