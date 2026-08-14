#include "RoomScreenScene.h"
#include "core/Engine.h"
#include "graphics/Color.h"
#include "math/Scalar.h"
#include "gameplay/RoomLayout.h"
#include "assets/roomscreen_main_scene.h"
#include "assets/PlayerPalette.h"
#include "GameConstants.h"

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace room_screen {

namespace gfx = pr32::graphics;
namespace math = pr32::math;
namespace gameplay = pr32::gameplay;
namespace scene = roomscreen::main_scene;

RoomScreenScene::RoomScreenScene()
    : camera(kDisplaySize, kDisplaySize)
    , player(scene::SPAWN_TILE_X * scene::TILE_SIZE,
             scene::SPAWN_TILE_Y * scene::TILE_SIZE) {
}

void RoomScreenScene::init() {
    Scene::init();

    // Populate the three exported tilemap layers (background, items, details).
    scene::init();

    // The hero resolves its pixel values against sprite palette slot 0, which
    // keeps its colors out of the tilemap's background slots entirely.
    gfx::setSpriteCustomPaletteSlot(0, PLAYER_SPRITE_PALETTE_RGB565);

    // Build the 4-room graph straight from the exported room layer. Rects and
    // connections both come from the data — no hand-written addRoom/connect
    // calls to drift out of sync with the map.
    const uint16_t built = gameplay::buildRoomGraph(scene::ROOMSCREEN_MAIN_SCENE_ROOM_LAYER, rooms_);

    // A rejected layer builds nothing, and entering a room on an empty graph is
    // a silent no-op — the scene would render with no bounds and no current
    // room. Check the count instead of trusting the data blindly.
    if (built == 0) {
        return;
    }

    rooms_.setOnEnter(onRoomEnterCallback, this);
    setRoomGraph(&rooms_);

    addEntity(&player);

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

void RoomScreenScene::onRoomEnterCallback(int /*fromIdx*/, int /*toIdx*/, void* /*userData*/) {
    // Camera bounds and position are already handled by enterRoom().
}

void RoomScreenScene::update(unsigned long deltaTime) {
    // Updates the player (and any future entities) first.
    Scene::update(deltaTime);

    // Then decide whether the walk crossed into another room.
    handleRoomTransition();
}

void RoomScreenScene::handleRoomTransition() {
    const uint16_t currentRoom = rooms_.currentRoomIndex();
    if (!rooms_.isValidIdx(currentRoom)) {
        return;
    }

    const gameplay::Room& room = rooms_.getRoom(currentRoom);
    const int minX = math::roundToInt(room.cameraMinX);
    const int maxX = math::roundToInt(room.cameraMaxX);
    const int minY = math::roundToInt(room.cameraMinY);
    const int maxY = math::roundToInt(room.cameraMaxY);

    const int px = player.pixelX();
    const int py = player.pixelY();

    gameplay::RoomDir dir;
    bool crossed = false;

    if (px < minX) {
        dir = gameplay::RoomDir::Left;
        crossed = true;
    } else if (px + kPlayerSize > maxX) {
        dir = gameplay::RoomDir::Right;
        crossed = true;
    } else if (py < minY) {
        dir = gameplay::RoomDir::Up;
        crossed = true;
    } else if (py + kPlayerSize > maxY) {
        dir = gameplay::RoomDir::Down;
        crossed = true;
    }

    if (!crossed) {
        return;
    }

    const int target = room.connections_[static_cast<int>(dir)];
    if (target == gameplay::Room::INVALID_ROOM || !rooms_.isValidIdx(static_cast<uint16_t>(target))) {
        // No room that way: treat it as a wall and push the player back inside.
        clampPlayerToRoom(room);
        return;
    }

    enterRoom(static_cast<uint16_t>(target));
    const gameplay::Room& dest = rooms_.getRoom(static_cast<uint16_t>(target));

    // Land just inside the entry edge, carrying the perpendicular coordinate
    // over so the crossing reads as continuous.
    int nx = player.pixelX();
    int ny = player.pixelY();
    switch (dir) {
        case gameplay::RoomDir::Left:
            nx = math::roundToInt(dest.cameraMaxX) - kPlayerSize;
            break;
        case gameplay::RoomDir::Right:
            nx = math::roundToInt(dest.cameraMinX);
            break;
        case gameplay::RoomDir::Up:
            ny = math::roundToInt(dest.cameraMaxY) - kPlayerSize;
            break;
        case gameplay::RoomDir::Down:
            ny = math::roundToInt(dest.cameraMinY);
            break;
    }
    player.setPixelPosition(nx, ny);
}

void RoomScreenScene::clampPlayerToRoom(const gameplay::Room& room) {
    const int minX = math::roundToInt(room.cameraMinX);
    const int maxX = math::roundToInt(room.cameraMaxX) - kPlayerSize;
    const int minY = math::roundToInt(room.cameraMinY);
    const int maxY = math::roundToInt(room.cameraMaxY) - kPlayerSize;

    int nx = player.pixelX();
    int ny = player.pixelY();
    if (nx < minX) nx = minX;
    if (nx > maxX) nx = maxX;
    if (ny < minY) ny = minY;
    if (ny > maxY) ny = maxY;
    player.setPixelPosition(nx, ny);
}

void RoomScreenScene::draw(pixelroot32::graphics::Renderer& renderer) {
    // World space below this point: the camera offset scrolls the tilemap.
    camera.apply(renderer);

    renderer.drawTileMap(scene::background, 0, 0, gfx::LayerType::Static);
    renderer.drawTileMap(scene::items, 0, 0, gfx::LayerType::Static);
    renderer.drawTileMap(scene::details, 0, 0, gfx::LayerType::Static);

    // Entities (the player) on top of the tilemaps.
    Scene::draw(renderer);
}

} // namespace room_screen
