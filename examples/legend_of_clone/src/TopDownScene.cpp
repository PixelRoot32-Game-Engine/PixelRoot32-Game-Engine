#include "TopDownScene.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "core/Engine.h"
#include "graphics/Color.h"
#include "math/Scalar.h"

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace legend_of_clone {

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

TopDownScene::TopDownScene()
    : camera_(kPlayfieldWidth, kPlayfieldHeight)
    , player_(0, 0) {
}

void TopDownScene::init() {
    Scene::init();

    const Setup config = setup();

    // Dual palette mode: the tilemap resolves its pixel values against one
    // 16-entry table and the player against another. Without it both would
    // share a single table, which is how the previous version ended up spending
    // three of the world's slots on Link's skin, tunic and brown.
    //
    // The engine stores both pointers without copying, which is why callers
    // hand over namespace-scope constants from the asset export and not locals.
    gfx::enableDualPaletteMode(true);
    if (config.backgroundPalette != nullptr) {
        gfx::setBackgroundCustomPalette(config.backgroundPalette);
    }
    if (config.spritePalette != nullptr) {
        gfx::setSpriteCustomPalette(config.spritePalette);
    }

    // A snapshot taken under the previous scene's palette and tiles is worthless
    // to this one. Drop it, then take the buffer back at init time so the game
    // loop never reaches the heap.
    tilemapLayerCache_.clear();
    tilemapLayerCache_.invalidate();
    (void)tilemapLayerCache_.allocateForRenderer(engine.getRenderer());

    world_ = config.world;
    if (world_ == nullptr || !world_->isAttached() || config.roomLayer == nullptr) {
        worldReady_ = false;
        return;
    }

    // buildRoomGraph APPENDS. That is deliberate on the engine's side — it is
    // how several exported layers are stitched into one graph — but it means a
    // second call on a graph that is already full adds nothing and returns 0.
    //
    // The engine runs init() on every scene swap, so this is not a hypothetical:
    // without the reset, leaving the dungeon and coming back rebuilt nothing,
    // worldReady_ went false, and the overworld never drew again.
    rooms_ = gameplay::RoomGraph<kMaxRooms>{};

    // Camera rects and connections both come from the exported room layer, so
    // there are no hand-written addRoom/connect calls to drift out of sync.
    const uint16_t built = gameplay::buildRoomGraph(*config.roomLayer, rooms_);

    // A rejected layer builds nothing, and entering a room on an empty graph is
    // a silent no-op — the scene would run with no bounds and no current room.
    // Check the count rather than trusting the data.
    if (built != config.roomLayer->roomCount) {
        worldReady_ = false;
        return;
    }
    worldReady_ = true;
    transitionActive_ = false;
    lastFromRoom_ = -1;

    rooms_.setOnEnter(onRoomEnterCallback, this);
    setRoomGraph(&rooms_);

    rooms_.enterRoom(config.startRoom, &camera_);
    snapCameraToRoom(config.startRoom);

    player_.setWorld(world_);
    player_.setPixelPosition(config.startCol * kTileSize, config.startRow * kTileSize);
    player_.setFacing(config.startFacing);
    player_.setEnabled(true);
    addEntity(&player_);
}

void TopDownScene::snapCameraToRoom(uint16_t roomIdx) {
    const RoomRect rect = rectOf(rooms_.getRoom(roomIdx));
    camera_.setPosition(math::Vector2(rect.minX, rect.minY));
}

void TopDownScene::cameraSample(int& outX, int& outY) const {
    outX = math::roundToInt(camera_.getX());
    outY = math::roundToInt(camera_.getY());
}

gfx::TileMap4bppDrawSpec TopDownScene::terrainLayer() const {
    return { world_ != nullptr ? world_->layer() : nullptr, 0, 0 };
}

void TopDownScene::onRoomEnter(int fromIdx, int /*toIdx*/) {
    // The very first enterRoom() reports 0xFFFF as its origin — there was no
    // previous room. Recording it verbatim would put "FROM 65535" on screen.
    if (!rooms_.isValidIdx(static_cast<uint16_t>(fromIdx))) return;
    lastFromRoom_ = fromIdx;
}

void TopDownScene::onRoomEnterCallback(int fromIdx, int toIdx, void* userData) {
    if (userData == nullptr) return;
    static_cast<TopDownScene*>(userData)->onRoomEnter(fromIdx, toIdx);
}

void TopDownScene::playerCell(int& outCol, int& outRow) const {
    outCol = (player_.pixelX() + kPlayerSize / 2) / kTileSize;
    outRow = (player_.pixelY() + kPlayerSize / 2) / kTileSize;
}

void TopDownScene::update(unsigned long deltaTime) {
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

    // Only once the player is standing somewhere they chose to stand. During a
    // slide they are being interpolated across a seam and pass over tiles they
    // never stepped on — firing a cave entrance from one of those would be a
    // teleport the player did not ask for.
    if (!transitionActive_) {
        onPlayerSettled();
    }
}

void TopDownScene::checkRoomExit() {
    const uint16_t currentIdx = rooms_.currentRoomIndex();
    if (!rooms_.isValidIdx(currentIdx)) return;

    const gameplay::Room& room = rooms_.getRoom(currentIdx);
    const RoomRect rect = rectOf(room);

    const int playerX = player_.pixelX();
    const int playerY = player_.pixelY();

    // Trigger on the leading edge: the frame any pixel of the player enters the
    // neighbouring room, not once they are halfway across it.
    bool crossed = false;
    auto dir     = gameplay::RoomDir::Right;

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

void TopDownScene::beginTransition(gameplay::RoomDir dir, uint16_t targetIdx) {
    const RoomRect from = rectOf(rooms_.getRoom(rooms_.currentRoomIndex()));
    const RoomRect to   = rectOf(rooms_.getRoom(targetIdx));

    transitionTarget_  = targetIdx;
    transitionElapsed_ = 0;
    transitionActive_  = true;

    cameraFromX_ = math::roundToInt(camera_.getX());
    cameraFromY_ = math::roundToInt(camera_.getY());
    cameraToX_   = to.minX;
    cameraToY_   = to.minY;

    playerFromX_ = player_.pixelX();
    playerFromY_ = player_.pixelY();
    playerToX_   = playerFromX_;
    playerToY_   = playerFromY_;

    // Land the player just inside the far edge of the new room, on the same
    // line they left on. This is the only place the crossing direction matters.
    switch (dir) {
        case gameplay::RoomDir::Right: playerToX_ = to.minX;               break;
        case gameplay::RoomDir::Left:  playerToX_ = to.maxX - kPlayerSize; break;
        case gameplay::RoomDir::Down:  playerToY_ = to.minY;               break;
        case gameplay::RoomDir::Up:    playerToY_ = to.maxY - kPlayerSize; break;
    }

    // Camera2D clamps every setPosition to its bounds, and those bounds are
    // still the room being left. Without widening them to cover both rooms the
    // slide would pin to the old room's edge and nothing would appear to move.
    const int unionMinX = (from.minX < to.minX) ? from.minX : to.minX;
    const int unionMaxX = (from.maxX > to.maxX) ? from.maxX : to.maxX;
    const int unionMinY = (from.minY < to.minY) ? from.minY : to.minY;
    const int unionMaxY = (from.maxY > to.maxY) ? from.maxY : to.maxY;
    camera_.setBounds(math::toScalar(unionMinX), math::toScalar(unionMaxX));
    camera_.setVerticalBounds(math::toScalar(unionMinY), math::toScalar(unionMaxY));

    // Locks out input for the whole slide: Scene::update skips disabled entities.
    player_.setEnabled(false);
}

void TopDownScene::updateTransition(unsigned long deltaTime) {
    transitionElapsed_ += deltaTime;

    const bool done = transitionElapsed_ >= kTransitionDurationMs;
    const unsigned long elapsed = done ? kTransitionDurationMs : transitionElapsed_;

    camera_.setPosition(math::Vector2(
        lerpInt(cameraFromX_, cameraToX_, elapsed, kTransitionDurationMs),
        lerpInt(cameraFromY_, cameraToY_, elapsed, kTransitionDurationMs)));

    player_.setPixelPosition(
        lerpInt(playerFromX_, playerToX_, elapsed, kTransitionDurationMs),
        lerpInt(playerFromY_, playerToY_, elapsed, kTransitionDurationMs));

    if (!done) return;

    // enterRoom resets the widened bounds to the target room and fires onEnter.
    rooms_.enterRoom(transitionTarget_, &camera_);
    snapCameraToRoom(transitionTarget_);
    player_.setPixelPosition(playerToX_, playerToY_);
    player_.setEnabled(true);
    transitionActive_ = false;
}

void TopDownScene::adviseFramebufferBeforeBeginFrame(gfx::Renderer& renderer) {
#if PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
    if (!worldReady_) {
        (void)renderer;
        return;
    }

    // Tells beginFrame it can skip clearing: the next draw() is about to memcpy
    // the whole snapshot over the framebuffer anyway. Skipping this does not
    // break anything, it just pays for a clear nobody sees.
    int camX = 0;
    int camY = 0;
    cameraSample(camX, camY);
    const gfx::TileMap4bppDrawSpec staticLayers[] = { terrainLayer() };
    tilemapLayerCache_.adviseFramebufferBeforeBeginFrame(
        renderer, camX, camY, staticLayers, 1, nullptr, 0);
#else
    (void)renderer;
#endif
}

void TopDownScene::draw(gfx::Renderer& renderer) {
    if (!worldReady_) {
        // Screen space, and painted over. The camera offset left behind by
        // whichever scene drew last is still in the renderer at this point —
        // the first version of this branch drew the message in world space and
        // pushed it off the top of the screen, which turned a loud failure into
        // a black screen and cost an afternoon.
        const bool oldBypass = renderer.isOffsetBypassEnabled();
        renderer.setOffsetBypass(true);
        renderer.drawFilledRectangle(0, 0, kDisplayWidth, kDisplayHeight,
                                     gfx::Color::Black);
        renderer.drawTextCentered("MAP DATA REJECTED", kDisplayHeight / 2 - 4,
                                  gfx::Color::Red, 1);
        renderer.setOffsetBypass(oldBypass);
        return;
    }

    // Everything below this line is in world space until the status bar.
    camera_.apply(renderer);

    // One static layer. The cache replays it with a memcpy while the camera is
    // pinned and rebuilds it during a slide, which is the only time it moves.
    int camX = 0;
    int camY = 0;
    cameraSample(camX, camY);
    const gfx::TileMap4bppDrawSpec staticLayers[] = { terrainLayer() };
    tilemapLayerCache_.draw(renderer, camX, camY, staticLayers, 1, nullptr, 0);

    // Entities — the player, and whatever a later iteration adds.
    Scene::draw(renderer);

    // Drawn last and opaque: the camera viewport is 176 px tall but the panel is
    // 240, so world rows below the current room would otherwise bleed into the
    // strip the status bar occupies.
    drawStatusBar(renderer);
}

void TopDownScene::drawStatusBar(gfx::Renderer& renderer) {
    // Screen space, not world space — the bar does not scroll with the camera.
    const bool oldBypass = renderer.isOffsetBypassEnabled();
    renderer.setOffsetBypass(true);

    renderer.drawFilledRectangle(0, kStatusBarY, kDisplayWidth, kStatusBarHeight,
                                 gfx::Color::Black);

    renderer.setOffsetBypass(oldBypass);
}

} // namespace legend_of_clone

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
