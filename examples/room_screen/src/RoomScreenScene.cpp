#include "RoomScreenScene.h"
#include "core/Engine.h"
#include "graphics/Color.h"
#include "math/Scalar.h"
#include "gameplay/RoomLayout.h"
#include "assets/RoomScreenRooms.h"
#include "GameConstants.h"

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace room_screen {

namespace gfx = pr32::graphics;
namespace math = pr32::math;
namespace gameplay = pr32::gameplay;

RoomScreenScene::RoomScreenScene()
    : camera(kDisplaySize, kDisplaySize) {
}

void RoomScreenScene::init() {
    Scene::init();

    // Build the 2-room horizontal graph straight from the exported room layer.
    // Rects and connections both come from the data — no hand-written addRoom/
    // connect calls to drift out of sync with the map.
    const uint16_t built = gameplay::buildRoomGraph(ROOM_SCREEN_ROOM_LAYER, rooms_);

    // A rejected layer builds nothing, and entering a room on an empty graph is
    // a silent no-op — the scene would render with no bounds and no current
    // room. Check the count instead of trusting the data blindly.
    if (built == 0) {
        return;
    }

    rooms_.setOnEnter(onRoomEnterCallback, this);
    setRoomGraph(&rooms_);

    // Start in room 0 (sets initial camera bounds).
    rooms_.enterRoom(0, &camera);
}

void RoomScreenScene::onRoomEnterCallback(int /*fromIdx*/, int /*toIdx*/, void* /*userData*/) {
    // Camera bounds already clamped by RoomGraph::enterRoom.
}

void RoomScreenScene::update(unsigned long deltaTime) {
    (void)deltaTime;
    auto& input = engine.getInputManager();
    uint16_t currentRoom = rooms_.currentRoomIndex();

    if (currentRoom == 0 && input.isButtonPressed(BTN_RIGHT)) {
        rooms_.enterRoom(1, &camera);
    } else if (currentRoom == 1 && input.isButtonPressed(BTN_LEFT)) {
        rooms_.enterRoom(0, &camera);
    }
}

void RoomScreenScene::draw(pixelroot32::graphics::Renderer& renderer) {
    uint16_t currentRoom = rooms_.currentRoomIndex();

    // Per-room background color.
    gfx::Color bg = (currentRoom == 0) ? gfx::Color::DarkGreen : gfx::Color::Navy;
    renderer.drawFilledRectangle(0, 0, kDisplaySize, kDisplaySize, bg);

    // Centered room label.
    const char* label = (currentRoom == 0) ? "Room 0" : "Room 1";
    renderer.drawTextCentered(label, kDisplaySize / 2 - 10, gfx::Color::White, 2);
}

} // namespace room_screen
