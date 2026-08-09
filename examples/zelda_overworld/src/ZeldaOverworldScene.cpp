#include "ZeldaOverworldScene.h"

#include "core/Engine.h"
#include "graphics/Color.h"
#include "math/Scalar.h"
#include "gameplay/RoomLayout.h"

#include "TileFormat.h"
#include "assets/OverworldMap.h"
#include "assets/OverworldRooms.h"

#include <cstdio>

namespace pr32 = pixelroot32;

namespace zelda_overworld {

namespace gfx = pr32::graphics;
namespace math = pr32::math;
namespace gameplay = pr32::gameplay;

namespace {

/// A room's world-space rect in whole pixels, which is the space the player
/// and the camera both work in.
struct RoomRect {
    int minX;
    int minY;
    int maxX;
    int maxY;
};

RoomRect rectOf(const gameplay::Room& room) {
    return {
        math::roundToInt(room.cameraMinX),
        math::roundToInt(room.cameraMinY),
        math::roundToInt(room.cameraMaxX),
        math::roundToInt(room.cameraMaxY),
    };
}

/// Linear interpolation on integers, exact at both ends.
int lerpInt(int from, int to, unsigned long elapsed, unsigned long duration) {
    if (duration == 0 || elapsed >= duration) return to;
    return from + static_cast<int>(
        static_cast<long long>(to - from) * static_cast<long long>(elapsed) /
        static_cast<long long>(duration));
}

} // namespace

ZeldaOverworldScene::ZeldaOverworldScene()
    : camera(kPlayfieldWidth, kPlayfieldHeight)
    , player_(kPlayerStartCol, kPlayerStartRow) {
}

void ZeldaOverworldScene::init() {
    Scene::init();

    // Expand the character map into layer indices and the collision map before
    // anything queries either.
    buildOverworld();

    // Camera rects and connections both come from the exported room layer, so
    // there are no hand-written addRoom/connect calls to drift out of sync.
    const uint16_t built = gameplay::buildRoomGraph(OVERWORLD_ROOM_LAYER, rooms_);

    // A rejected layer builds nothing, and entering a room on an empty graph is
    // a silent no-op — the scene would run with no bounds and no current room.
    // Check the count rather than trusting the data.
    if (built != OVERWORLD_ROOM_LAYER.roomCount) {
        worldReady_ = false;
        return;
    }
    worldReady_ = true;

    rooms_.setOnEnter(onRoomEnterCallback, this);
    setRoomGraph(&rooms_);

    rooms_.enterRoom(kRoomSouthWest, &camera);
    snapCameraToRoom(kRoomSouthWest);

    player_.setPixelPosition(kPlayerStartCol * kTileSize, kPlayerStartRow * kTileSize);
    addEntity(&player_);
}

void ZeldaOverworldScene::snapCameraToRoom(uint16_t roomIdx) {
    const RoomRect rect = rectOf(rooms_.getRoom(roomIdx));
    camera.setPosition(math::Vector2(rect.minX, rect.minY));
}

void ZeldaOverworldScene::onRoomEnter(int fromIdx, int /*toIdx*/) {
    // The very first enterRoom() reports 0xFFFF as its origin — there was no
    // previous room. Recording it verbatim would put "FROM 65535" on screen.
    if (!rooms_.isValidIdx(static_cast<uint16_t>(fromIdx))) return;
    lastFromRoom_ = fromIdx;
}

void ZeldaOverworldScene::onRoomEnterCallback(int fromIdx, int toIdx, void* userData) {
    if (userData == nullptr) return;
    static_cast<ZeldaOverworldScene*>(userData)->onRoomEnter(fromIdx, toIdx);
}

void ZeldaOverworldScene::update(unsigned long deltaTime) {
    if (!worldReady_) return;

    if (transitionActive_) {
        // The player is disabled for the duration, so Scene::update would have
        // nothing to advance. Skipping it also guarantees no other system moves
        // anything mid-slide.
        updateTransition(deltaTime);
        return;
    }

    Scene::update(deltaTime);
    checkRoomExit();
}

void ZeldaOverworldScene::checkRoomExit() {
    const uint16_t currentIdx = rooms_.currentRoomIndex();
    if (!rooms_.isValidIdx(currentIdx)) return;

    const gameplay::Room& room = rooms_.getRoom(currentIdx);
    const RoomRect rect = rectOf(room);

    const int playerX = player_.pixelX();
    const int playerY = player_.pixelY();

    // Trigger on the leading edge: the frame any pixel of the player enters the
    // neighbouring screen, not once they are halfway across it.
    bool  crossed = false;
    auto  dir     = gameplay::RoomDir::Right;

    if (playerX + kPlayerSize > rect.maxX) {
        dir = gameplay::RoomDir::Right;
        crossed = true;
    } else if (playerX < rect.minX) {
        dir = gameplay::RoomDir::Left;
        crossed = true;
    } else if (playerY + kPlayerSize > rect.maxY) {
        dir = gameplay::RoomDir::Down;
        crossed = true;
    } else if (playerY < rect.minY) {
        dir = gameplay::RoomDir::Up;
        crossed = true;
    }

    if (!crossed) return;

    const uint16_t target = room.connections_[static_cast<int>(dir)];
    if (target == gameplay::kNoRoomConnection || !rooms_.isValidIdx(target)) {
        // An opening in the tiles with no connection behind it. Push the player
        // back inside rather than letting them walk into nothing — a map bug
        // should look wrong, not crash.
        int clampedX = playerX;
        int clampedY = playerY;
        if (clampedX + kPlayerSize > rect.maxX) clampedX = rect.maxX - kPlayerSize;
        if (clampedX < rect.minX)               clampedX = rect.minX;
        if (clampedY + kPlayerSize > rect.maxY) clampedY = rect.maxY - kPlayerSize;
        if (clampedY < rect.minY)               clampedY = rect.minY;
        player_.setPixelPosition(clampedX, clampedY);
        return;
    }

    beginTransition(dir, target);
}

void ZeldaOverworldScene::beginTransition(gameplay::RoomDir dir, uint16_t targetIdx) {
    const RoomRect from = rectOf(rooms_.getRoom(rooms_.currentRoomIndex()));
    const RoomRect to   = rectOf(rooms_.getRoom(targetIdx));

    transitionTarget_  = targetIdx;
    transitionElapsed_ = 0;
    transitionActive_  = true;

    cameraFromX_ = math::roundToInt(camera.getX());
    cameraFromY_ = math::roundToInt(camera.getY());
    cameraToX_   = to.minX;
    cameraToY_   = to.minY;

    playerFromX_ = player_.pixelX();
    playerFromY_ = player_.pixelY();
    playerToX_   = playerFromX_;
    playerToY_   = playerFromY_;

    // Land the player just inside the far edge of the new screen, on the same
    // line they left on. This is the only place the crossing direction matters.
    switch (dir) {
        case gameplay::RoomDir::Right: playerToX_ = to.minX;                 break;
        case gameplay::RoomDir::Left:  playerToX_ = to.maxX - kPlayerSize;   break;
        case gameplay::RoomDir::Down:  playerToY_ = to.minY;                 break;
        case gameplay::RoomDir::Up:    playerToY_ = to.maxY - kPlayerSize;   break;
    }

    // Camera2D clamps every setPosition to its bounds, and those bounds are
    // still the room being left. Without widening them to cover both rooms the
    // slide would pin to the old room's edge and nothing would appear to move.
    const int unionMinX = (from.minX < to.minX) ? from.minX : to.minX;
    const int unionMaxX = (from.maxX > to.maxX) ? from.maxX : to.maxX;
    const int unionMinY = (from.minY < to.minY) ? from.minY : to.minY;
    const int unionMaxY = (from.maxY > to.maxY) ? from.maxY : to.maxY;
    camera.setBounds(math::toScalar(unionMinX), math::toScalar(unionMaxX));
    camera.setVerticalBounds(math::toScalar(unionMinY), math::toScalar(unionMaxY));

    // Locks out input for the whole slide: Scene::update skips disabled entities.
    player_.setEnabled(false);
}

void ZeldaOverworldScene::updateTransition(unsigned long deltaTime) {
    transitionElapsed_ += deltaTime;

    const bool done = transitionElapsed_ >= kTransitionDurationMs;
    const unsigned long elapsed = done ? kTransitionDurationMs : transitionElapsed_;

    camera.setPosition(math::Vector2(
        lerpInt(cameraFromX_, cameraToX_, elapsed, kTransitionDurationMs),
        lerpInt(cameraFromY_, cameraToY_, elapsed, kTransitionDurationMs)));

    player_.setPixelPosition(
        lerpInt(playerFromX_, playerToX_, elapsed, kTransitionDurationMs),
        lerpInt(playerFromY_, playerToY_, elapsed, kTransitionDurationMs));

    if (!done) return;

    // enterRoom resets the widened bounds to the target room and fires onEnter.
    rooms_.enterRoom(transitionTarget_, &camera);
    snapCameraToRoom(transitionTarget_);
    player_.setPixelPosition(playerToX_, playerToY_);
    player_.setEnabled(true);
    transitionActive_ = false;
}

void ZeldaOverworldScene::draw(gfx::Renderer& renderer) {
    if (!worldReady_) {
        renderer.drawTextCentered("ROOM LAYER REJECTED", kDisplayHeight / 2 - 4,
                                  gfx::Color::Red, 1);
        return;
    }

    // Everything below this line is in world space until the status bar.
    camera.apply(renderer);

    for (int i = 0; i < OVERWORLD_LAYER_COUNT; ++i) {
        const WorldLayer& layer = OVERWORLD_LAYERS[i];
        drawSceneTileMap(renderer, layer.map, 0, 0, layer.color);
    }

    // Entities — the player, and whatever iteration 2 adds.
    Scene::draw(renderer);

    // Drawn last and opaque: the camera viewport is 176 px tall but the panel is
    // 240, so world rows below the current room would otherwise bleed into the
    // strip the status bar occupies.
    drawStatusBar(renderer);
}

void ZeldaOverworldScene::drawStatusBar(gfx::Renderer& renderer) {
    // Screen space, not world space — the bar does not scroll with the camera.
    const bool oldBypass = renderer.isOffsetBypassEnabled();
    renderer.setOffsetBypass(true);

    renderer.drawFilledRectangle(0, kStatusBarY, kDisplayWidth, kStatusBarHeight,
                                 gfx::Color::Black);

    // Placeholder readout. The heart row and item slots belong here, and this is
    // where UISpriteRow lands once iteration 2 gives the player something to
    // lose.
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "SCREEN %u",
                  static_cast<unsigned>(rooms_.currentRoomIndex()));
    renderer.drawText(buffer, 8, kStatusBarY + 12, gfx::Color::White, 1);

    if (lastFromRoom_ >= 0) {
        std::snprintf(buffer, sizeof(buffer), "FROM %d", lastFromRoom_);
        renderer.drawText(buffer, 8, kStatusBarY + 30, gfx::Color::Gray, 1);
    }

    renderer.setOffsetBypass(oldBypass);
}

} // namespace zelda_overworld
