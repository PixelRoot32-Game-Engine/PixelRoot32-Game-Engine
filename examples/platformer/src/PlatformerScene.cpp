#include "PlatformerScene.h"

#include "core/Engine.h"
#include "graphics/Color.h"
#include "graphics/Renderer.h"
#include "input/InputManager.h"
#include "platforms/EngineConfig.h"

#include "CrateActor.h"
#include "GameLayers.h"
#include "LevelBlock.h"
#include "PlayerCharacter.h"

#include <memory>

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace platformerdemo {

namespace gfx = pr32::graphics;
namespace math = pr32::math;

using gfx::Color;
using math::Vector2;
using math::toScalar;

namespace {

struct BlockDef {
    int x;
    int y;
    int w;
    int h;
    bool oneWay;
};

/**
 * The level as data, on an 8 px grid.
 *
 * One `StaticActor` per run of tiles, not per tile: `MAX_ENTITIES` and
 * `PHYSICS_MAX_ENTITIES` are 64, and a 30x30 tile level spent one body per cell
 * would exhaust both before the crates are even spawned.
 *
 * Vertical gaps are 24 px, 32 px from the ground to the first one-way platform
 * and 56 px from the ground up to platform B, against the 62 px the jump
 * actually reaches (v^2 / 2g = 60.5 px plus the discrete step), so every
 * platform is reachable from the one below and the 56 px hop is the tightest.
 */
const BlockDef LEVEL_BLOCKS[] = {
    {   0, 208, 240,  32, false },  // ground strip
    {   0,   0,   8, 208, false },  // left wall
    { 232,   0,   8, 208, false },  // right wall
    {  32, 176,  64,   8, true  },  // one-way platform A
    { 112, 152,  64,   8, true  },  // one-way platform B
    {  32, 128,  56,   8, true  },  // one-way platform C
    { 112, 104,  56,   8, true  },  // one-way platform D
    { 176, 176,  56,   8, false }   // solid shelf the crates rest on
};

constexpr int LEVEL_BLOCK_COUNT =
    static_cast<int>(sizeof(LEVEL_BLOCKS) / sizeof(LEVEL_BLOCKS[0]));

static_assert(LEVEL_BLOCK_COUNT <= MAX_LEVEL_BLOCKS,
              "LEVEL_BLOCKS does not fit in PlatformerScene::blocks");

/// Crates start above the shelf and clear of every one-way platform, so they
/// fall straight onto it.
const int CRATE_START_X[] = { 184, 200, 216 };
constexpr float CRATE_START_Y = 120.0f;

constexpr int CRATE_COUNT = static_cast<int>(sizeof(CRATE_START_X) / sizeof(CRATE_START_X[0]));

static_assert(CRATE_COUNT <= MAX_CRATES, "CRATE_START_X does not fit in PlatformerScene::crates");

} // namespace

PlatformerScene::PlatformerScene() = default;
PlatformerScene::~PlatformerScene() = default;

void PlatformerScene::resetState() noexcept {
    // Release owned resources BEFORE clearing base state, or the base entity
    // array keeps pointers to objects that no longer exist.
    player.reset();
    for (int i = 0; i < crateCount; ++i) {
        crates[i].reset();
    }
    for (int i = 0; i < blockCount; ++i) {
        blocks[i].reset();
    }
    crateCount = 0;
    blockCount = 0;

    Scene::resetState();
}

void PlatformerScene::init() {
    Scene::init();  // Idempotent: resets prior state before re-initialising

    gfx::setPalette(gfx::PaletteType::PR32);

    spawnLevel();
    spawnCrates();

    player = std::make_unique<PlayerCharacter>(
        Vector2(toScalar(PLAYER_START_X), toScalar(PLAYER_START_Y)),
        PLAYER_WIDTH,
        PLAYER_HEIGHT);
    player->setCollisionLayer(Layers::PLAYER);
    player->setCollisionMask(Layers::PLAYER_MASK);
    addEntity(player.get());
}

void PlatformerScene::spawnLevel() {
    blockCount = 0;

    for (int i = 0; i < LEVEL_BLOCK_COUNT; ++i) {
        const BlockDef& def = LEVEL_BLOCKS[i];
        const Color fill = def.oneWay ? Color::DarkGreen : Color::Green;

        auto block = std::make_unique<LevelBlock>(
            toScalar(static_cast<float>(def.x)),
            toScalar(static_cast<float>(def.y)),
            def.w,
            def.h,
            fill,
            def.oneWay);

        block->setCollisionLayer(def.oneWay ? Layers::PLATFORM : Layers::GROUND);
        block->setCollisionMask(def.oneWay ? Layers::PLATFORM_MASK : Layers::GROUND_MASK);

        addEntity(block.get());
        blocks[blockCount++] = std::move(block);
    }
}

void PlatformerScene::spawnCrates() {
    crateCount = 0;

    for (int i = 0; i < CRATE_COUNT; ++i) {
        auto crate = std::make_unique<CrateActor>(
            toScalar(static_cast<float>(CRATE_START_X[i])),
            toScalar(CRATE_START_Y),
            CRATE_SIZE);

        crate->setCollisionLayer(Layers::PROP);
        crate->setCollisionMask(Layers::PROP_MASK);

        addEntity(crate.get());
        crates[crateCount++] = std::move(crate);
    }
}

void PlatformerScene::update(unsigned long deltaTime) {
    auto& input = engine.getInputManager();

    // A full re-init is the reset: the same path runs on boot and when the
    // player presses B, so there is only one way the level is built.
    if (input.isButtonPressed(BTN_RESET)) {
        init();
        return;
    }

    float moveDir = 0.0f;
    if (input.isButtonDown(BTN_RIGHT)) {
        moveDir += 1.0f;
    }
    if (input.isButtonDown(BTN_LEFT)) {
        moveDir -= 1.0f;
    }

    // isButtonPressed() is the edge of the button, so holding jump does not
    // repeat it. is_on_floor() is last frame's contact, which the engine
    // refreshes inside moveAndSlide().
    if (player) {
        player->setInput(moveDir, input.isButtonPressed(BTN_JUMP));
    }

    Scene::update(deltaTime);
}

void PlatformerScene::draw(gfx::Renderer& renderer) {
    renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, Color::Navy);

    // Statics, then crates, then the character: addEntity() order within the
    // same render layer is the draw order.
    Scene::draw(renderer);

    drawHud(renderer);
}

void PlatformerScene::drawHud(gfx::Renderer& renderer) {
    // One swatch per actor type, in the colour that type is drawn in.
    renderer.drawFilledRectangle(12, 6, 4, 4, Color::Cyan);
    renderer.drawText("KINEMATIC", 20, 4, Color::Cyan, 1);

    renderer.drawFilledRectangle(96, 6, 4, 4, Color::Orange);
    renderer.drawText("RIGID", 104, 4, Color::Orange, 1);

    renderer.drawFilledRectangle(152, 6, 4, 4, Color::Green);
    renderer.drawText("STATIC", 160, 4, Color::Green, 1);

    renderer.drawText("Arrows move  A jump  B reset", 12, 14, Color::White, 1);
}

}
