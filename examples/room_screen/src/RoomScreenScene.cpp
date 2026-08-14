#include "RoomScreenScene.h"
#include "core/Engine.h"
#include "graphics/Color.h"
#include "math/Scalar.h"
#include "gameplay/RoomLayout.h"
#include "assets/main_scene.h"
#include "GameConstants.h"

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace room_screen {

namespace gfx = pr32::graphics;
namespace math = pr32::math;
namespace gameplay = pr32::gameplay;
namespace scene = roomscreen::main_scene;

RoomScreenScene::RoomScreenScene()
    : camera(kDisplaySize, kDisplaySize) {
}

void RoomScreenScene::init() {
    Scene::init();

    // Populate the three exported tilemap layers (background, items, details).
    scene::init();

    // Build the 4-room graph straight from the exported room layer. Rects and
    // connections both come from the data — no hand-written addRoom/connect
    // calls to drift out of sync with the map.
    const uint16_t built = gameplay::buildRoomGraph(scene::ROOMSCENE_MAIN_SCENE_ROOM_LAYER, rooms_);

    // A rejected layer builds nothing, and entering a room on an empty graph is
    // a silent no-op — the scene would render with no bounds and no current
    // room. Check the count instead of trusting the data blindly.
    if (built == 0) {
        return;
    }

    rooms_.setOnEnter(onRoomEnterCallback, this);
    setRoomGraph(&rooms_);

    // Start in room 0 (top-left of the 2x2 grid).
    enterRoom(0);
}

void RoomScreenScene::enterRoom(uint16_t idx) {
    if (!rooms_.isValidIdx(idx)) {
        return;
    }

    // Sets the camera bounds, then snap the camera onto the room's corner so
    // the tilemap scrolls to reveal it. enterRoom() alone only clamps bounds;
    // it does not move the camera, which would leave the previous room on screen.
    rooms_.enterRoom(idx, &camera);
    const gameplay::Room& room = rooms_.getRoom(idx);
    camera.setPosition(math::Vector2(room.cameraMinX, room.cameraMinY));
}

void RoomScreenScene::moveTowards(gameplay::RoomDir dir) {
    const uint16_t currentRoom = rooms_.currentRoomIndex();
    if (!rooms_.isValidIdx(currentRoom)) {
        return;
    }

    const int target = rooms_.getRoom(currentRoom).connections_[static_cast<int>(dir)];
    // An unconnected direction holds Room::INVALID_ROOM (0xFFFF), which
    // enterRoom() treats as out-of-range and ignores.
    enterRoom(static_cast<uint16_t>(target));
}

void RoomScreenScene::onRoomEnterCallback(int /*fromIdx*/, int /*toIdx*/, void* /*userData*/) {
    // Camera bounds and position are already handled by enterRoom().
}

void RoomScreenScene::update(unsigned long deltaTime) {
    (void)deltaTime;
    auto& input = engine.getInputManager();

    if (input.isButtonPressed(BTN_UP)) {
        moveTowards(gameplay::RoomDir::Up);
    } else if (input.isButtonPressed(BTN_DOWN)) {
        moveTowards(gameplay::RoomDir::Down);
    } else if (input.isButtonPressed(BTN_LEFT)) {
        moveTowards(gameplay::RoomDir::Left);
    } else if (input.isButtonPressed(BTN_RIGHT)) {
        moveTowards(gameplay::RoomDir::Right);
    }
}

void RoomScreenScene::draw(pixelroot32::graphics::Renderer& renderer) {
    // World space below this point: the camera offset scrolls the tilemap.
    camera.apply(renderer);

    renderer.drawTileMap(scene::background, 0, 0, gfx::LayerType::Static);
    renderer.drawTileMap(scene::items, 0, 0, gfx::LayerType::Static);
    renderer.drawTileMap(scene::details, 0, 0, gfx::LayerType::Static);
}

} // namespace room_screen
