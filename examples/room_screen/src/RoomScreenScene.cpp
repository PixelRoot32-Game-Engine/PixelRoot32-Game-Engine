#include "RoomScreenScene.h"
#include "core/Engine.h"
#include "graphics/Color.h"
#include "math/Scalar.h"
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

    // Build a 2-room horizontal graph. Room 0 = left, Room 1 = right.
    rooms_.addRoom(math::toScalar(0), math::toScalar(0),
                   math::toScalar(kRoomWidth), math::toScalar(kRoomHeight));
    rooms_.addRoom(math::toScalar(kRoomWidth), math::toScalar(0),
                   math::toScalar(kRoomWidth * 2), math::toScalar(kRoomHeight));

    rooms_.connect(0, 1, gameplay::RoomDir::Right);
    rooms_.connect(1, 0, gameplay::RoomDir::Left);

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
