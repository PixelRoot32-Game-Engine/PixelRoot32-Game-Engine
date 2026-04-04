#include "CameraDemoScene.h"
#include "core/Engine.h"
#include "platforms/EngineConfig.h"
#include "input/InputManager.h"
#include "graphics/Renderer.h"
#include "physics/StaticActor.h"
#include "GameLayers.h"
#include "GameConstants.h"
#include "PlayerCube.h"

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace camerademo {

namespace core = pr32::core;
namespace gfx = pr32::graphics;
namespace math = pr32::math;
namespace physics = pr32::physics;

using gfx::Sprite;
using gfx::TileMap;
using gfx::Color;
using math::Scalar;
using math::toScalar;
using physics::StaticActor;

static const uint16_t TILE_EMPTY_BITS[] = {
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000
};

static const uint16_t TILE_GROUND_BITS[] = {
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0xFFFF
};

static const uint16_t TILE_PLATFORM_BITS[] = {
    0x0000,
    0x0000,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0x0000,
    0x0000
};

static const uint16_t TILE_SOLID_PLATFORM_BITS[] = {
    0xAAAA,
    0x5555,
    0xAAAA,
    0x5555,
    0xAAAA,
    0x5555,
    0xAAAA,
    0x5555
};

static const Sprite PLATFORMER_TILES[] = {
    { TILE_EMPTY_BITS,          TILE_SIZE, TILE_SIZE },
    { TILE_GROUND_BITS,         TILE_SIZE, TILE_SIZE },
    { TILE_PLATFORM_BITS,       TILE_SIZE, TILE_SIZE },
    { TILE_SOLID_PLATFORM_BITS, TILE_SIZE, TILE_SIZE }
};

static uint8_t PLATFORMER_INDICES[TILEMAP_WIDTH * TILEMAP_HEIGHT];

static TileMap PLATFORMER_MAP = {
    PLATFORMER_INDICES,
    static_cast<uint8_t>(TILEMAP_WIDTH),
    static_cast<uint8_t>(TILEMAP_HEIGHT),
    PLATFORMER_TILES,
    TILE_SIZE,
    TILE_SIZE,
    static_cast<uint16_t>(sizeof(PLATFORMER_TILES) / sizeof(Sprite))
};

// Build the scrolling platformer tilemap
static void initPlatformerTilemap() {
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;

    int total = TILEMAP_WIDTH * TILEMAP_HEIGHT;
    for (int i = 0; i < total; ++i) {
        PLATFORMER_INDICES[i] = 0;
    }

    int groundRow1 = TILEMAP_HEIGHT - 2;
    int groundRow2 = TILEMAP_HEIGHT - 1;

    for (int x = 0; x < TILEMAP_WIDTH; ++x) {
        PLATFORMER_INDICES[groundRow1 * TILEMAP_WIDTH + x] = 1;
        PLATFORMER_INDICES[groundRow2 * TILEMAP_WIDTH + x] = 1;
    }

    int p1Start = 10;
    int p1End = 18;
    int p1Row = groundRow1 - 3;
    for (int x = p1Start; x < p1End; ++x) {
        PLATFORMER_INDICES[p1Row * TILEMAP_WIDTH + x] = 3;
    }

    // Platform 2
    int p2Start = 28;
    int p2End = 36;
    int p2Row = groundRow1 - 5;
    for (int x = p2Start; x < p2End; ++x) {
        PLATFORMER_INDICES[p2Row * TILEMAP_WIDTH + x] = 2;
    }

    // Platform 3
    int p3Start = 50;
    int p3End = 58;
    int p3Row = groundRow1 - 4;
    for (int x = p3Start; x < p3End; ++x) {
        PLATFORMER_INDICES[p3Row * TILEMAP_WIDTH + x] = 2;
    }
}

CameraDemoScene::CameraDemoScene()
    : camera(DISPLAY_WIDTH, DISPLAY_HEIGHT)
    , player(nullptr)
    , levelWidth(static_cast<float>(TILEMAP_WIDTH * TILE_SIZE)) {
}

CameraDemoScene::~CameraDemoScene() {}

void CameraDemoScene::init() {
    gfx::setPalette(gfx::PaletteType::PR32);
    initPlatformerTilemap();
    jumpInputReady = false;

    int groundRow1 = TILEMAP_HEIGHT - 2;
    {
        int w = TILEMAP_WIDTH * TILE_SIZE;
        int h = 2 * TILE_SIZE;
        float x = 0.0f;
        float y = static_cast<float>(groundRow1 * TILE_SIZE);

        auto ground = std::make_unique<StaticActor>(toScalar(x), toScalar(y), w, h);
        ground->setShape(core::CollisionShape::AABB);
        ground->setCollisionLayer(Layers::GROUND);
        ground->setCollisionMask(Layers::PLAYER);
        addEntity(ground.get());
        ownedEntities.push_back(std::move(ground));
    }

    struct PlatDef {
        int startX;
        int endX;
        int rowY;
    };
    
    PlatDef defs[] = {
        {10, 18, groundRow1 - 3},
        {28, 36, groundRow1 - 5},
        {50, 58, groundRow1 - 4}
    };
    
    for (size_t i = 0; i < sizeof(defs)/sizeof(PlatDef); ++i) {
        const auto& def = defs[i];
        int w = (def.endX - def.startX) * TILE_SIZE;
        int h = TILE_SIZE;
        float x = static_cast<float>(def.startX * TILE_SIZE);
        float y = static_cast<float>(def.rowY * TILE_SIZE + PLATFORM_VISUAL_OFFSET);
        
        auto platform = std::make_unique<StaticActor>(toScalar(x), toScalar(y), w, h);
        platform->setShape(pr32::core::CollisionShape::AABB);
        
        if (i == 0) {
            platform->setCollisionLayer(Layers::GROUND);
            platform->setCollisionMask(Layers::PLAYER);
        } else {
            platform->setCollisionLayer(Layers::PLATFORM);
            platform->setCollisionMask(0);
        }
        addEntity(platform.get());
        ownedEntities.push_back(std::move(platform));
    }

    float startX = PLAYER_START_X;
    int playerWidth = static_cast<int>(PLAYER_WIDTH);
    int playerHeight = static_cast<int>(PLAYER_HEIGHT);
    float startY = PLAYER_START_Y;

    player = std::make_unique<PlayerCube>(math::Vector2(startX, startY),
                             playerWidth,
                             playerHeight);
    int worldWidthPixels = TILEMAP_WIDTH * TILE_SIZE;
    int worldHeightPixels = (TILEMAP_HEIGHT - 2) * TILE_SIZE;
    player->setWorldSize(worldWidthPixels, worldHeightPixels);
    player->setShape(pr32::core::CollisionShape::AABB);
    player->setCollisionLayer(Layers::PLAYER);
    player->setCollisionMask(Layers::GROUND | Layers::PLATFORM);
    addEntity(player.get());
    float maxCameraX = levelWidth - DISPLAY_WIDTH;
    if (maxCameraX < 0.0f) {
        maxCameraX = 0.0f;
    }
    camera.setBounds(toScalar(0.0f), toScalar(maxCameraX));
    camera.setVerticalBounds(toScalar(0.0f), toScalar(0.0f)); // Lock vertical movement
    camera.setPosition(math::Vector2::ZERO());
}

void CameraDemoScene::update(unsigned long deltaTime) {
    auto& input = engine.getInputManager();

    float moveDir = 0.0f;
    if (input.isButtonDown(3)) {
        moveDir += 1.0f;
    }
    if (input.isButtonDown(2)) {
        moveDir -= 1.0f;
    }

    bool rawJumpDown = input.isButtonDown(4);

    if (!jumpInputReady) {
        if (!rawJumpDown) {
            jumpInputReady = true;
        }
    }

    bool jumpPressed = false;
    if (jumpInputReady && input.isButtonPressed(4)) {
        jumpPressed = true;
    }

    if (player) {
        player->setInput(moveDir, jumpPressed);
    }
    Scene::update(deltaTime);

    if (player) {
        Scalar centerX = player->position.x + toScalar(player->width) * toScalar(0.5f);
        Scalar centerY = player->position.y + toScalar(player->height) * toScalar(0.5f);
        camera.followTarget(math::Vector2(centerX, centerY));
    }
}

void CameraDemoScene::draw(gfx::Renderer& renderer) {
    Scalar camX = camera.getX();
    Scalar farFactor = toScalar(0.4f);
    int farOffset = static_cast<int>(-camX * farFactor);
    renderer.setDisplayOffset(farOffset, 0);

    int horizonY = DISPLAY_HEIGHT / 3;
    int hillHeight = DISPLAY_HEIGHT / 4;

    renderer.drawFilledRectangle(-40, horizonY, DISPLAY_WIDTH + 80, hillHeight, Color::DarkBlue);
    renderer.drawFilledRectangle(DISPLAY_WIDTH / 2, horizonY + 10, DISPLAY_WIDTH, hillHeight + 10, Color::DarkGray);

    int midOffset = static_cast<int>(-camX * toScalar(0.7f));
    renderer.setDisplayOffset(midOffset, 0);

    int midY = (DISPLAY_HEIGHT * 2) / 3;
    renderer.drawFilledRectangle(-20, midY, DISPLAY_WIDTH + 40, 10, Color::DarkGreen);

    int mainOffset = static_cast<int>(-camX);
    renderer.setDisplayOffset(mainOffset, 0);

    renderer.drawTileMap(PLATFORMER_MAP, 0, 0, Color::Brown);

    if (player) {
        player->draw(renderer);
    }
    for (const auto& entity : ownedEntities) {
        if (entity->isVisible) {
            entity->draw(renderer);
        }
    }
}

}
