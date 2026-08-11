#include "CameraDemoScene.h"
#include "CameraDemoScene2.h"
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
    , levelWidth(static_cast<float>(TILEMAP_WIDTH * TILE_SIZE))
    , scene2Ref_(nullptr)
    , endReached_(false) {
}

CameraDemoScene::~CameraDemoScene() {}

void CameraDemoScene::resetState() noexcept {
    // Release owned resources BEFORE clearing base state
    player.reset();
    for (int i = 0; i < entityCount; ++i) {
        ownedEntities[i].reset();
    }
    entityCount = 0;
    // Delegate to base — clears entities[], arena, and collision system
    Scene::resetState();
}

void CameraDemoScene::init() {
    Scene::init();  // Idempotent: resets prior state before re-initialising
    gfx::setPalette(gfx::PaletteType::PR32);
    initPlatformerTilemap();
    jumpInputReady = false;
    nextEffect = EffectStep::Shake;
    activeEffectName = nullptr;
    tourStage = TourStage::Idle;
    tourHoldElapsed = 0;
    wasOnFloor = false;

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
        ownedEntities[entityCount++] = std::move(ground);
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
        ownedEntities[entityCount++] = std::move(platform);
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
    if (input.isButtonDown(BTN_RIGHT)) {
        moveDir += 1.0f;
    }
    if (input.isButtonDown(BTN_LEFT)) {
        moveDir -= 1.0f;
    }

    bool rawJumpDown = input.isButtonDown(BTN_JUMP);

    if (!jumpInputReady) {
        if (!rawJumpDown) {
            jumpInputReady = true;
        }
    }

    bool jumpPressed = false;
    if (jumpInputReady && input.isButtonPressed(BTN_JUMP)) {
        jumpPressed = true;
    }

    if (player) {
        player->setInput(moveDir, jumpPressed);
    }

    // Camera effect controls. An effect is an offset applied at draw time, so
    // firing one never disturbs the follow logic below.
    if (input.isButtonPressed(BTN_EFFECT)) {
        fireNextEffect();
    }
    if (input.isButtonPressed(BTN_CANCEL)) {
        cameraEffects.cancelAll();
        activeEffectName = nullptr;
    }
    if (input.isButtonPressed(BTN_TWEEN) && tourStage == TourStage::Idle) {
        startCameraTour();
    }

    // End-of-level detection: player reaches the rightmost edge of the level
    if (!endReached_ && player && scene2Ref_) {
        float playerRightEdge = static_cast<float>(player->position.x) + PLAYER_WIDTH;
        if (playerRightEdge >= levelWidth) {
            endReached_ = true;
            // Trigger directional iris transition:
            //   Out closes from RIGHT edge of the screen
            //   In opens from LEFT edge of the screen
            engine.triggerTransition(
                static_cast<pr32::core::Scene*>(scene2Ref_),
                gfx::TransitionType::Iris,
                500,
                DISPLAY_WIDTH, DISPLAY_HEIGHT / 2,  // Out center: RIGHT
                0, DISPLAY_HEIGHT / 2                // In center: LEFT
            );
        }
    }

    Scene::update(deltaTime);

    // Punch the camera on landing. The engine reports floor contact per frame,
    // so the landing edge is "on the floor now, airborne last frame".
    if (player) {
        bool onFloor = player->is_on_floor();
        if (onFloor && !wasOnFloor) {
            cameraEffects.triggerPunch(toScalar(LANDING_PUNCH_AMPLITUDE),
                                       LANDING_PUNCH_DURATION_MS,
                                       math::Vector2::DOWN());
        }
        wasOnFloor = onFloor;
    }

    updateCameraTour(deltaTime);

    // A tween owns the camera position while it runs; following it at the same
    // time would overwrite the interpolated value every frame.
    if (tourStage == TourStage::Idle && player) {
        Scalar centerX = player->position.x + toScalar(player->width) * toScalar(0.5f);
        Scalar centerY = player->position.y + toScalar(player->height) * toScalar(0.5f);
        camera.followTarget(math::Vector2(centerX, centerY));
    }
}

// ---------------------------------------------------------------------------
// Camera effects
// ---------------------------------------------------------------------------

void CameraDemoScene::fireNextEffect() {
    switch (nextEffect) {
        case EffectStep::Shake:
            cameraEffects.triggerShake(toScalar(SHAKE_AMPLITUDE), SHAKE_DURATION_MS);
            activeEffectName = "Shake";
            break;
        case EffectStep::PunchUp:
            cameraEffects.triggerPunch(toScalar(PUNCH_AMPLITUDE), PUNCH_DURATION_MS,
                                       math::Vector2::UP());
            activeEffectName = "Punch Up";
            break;
        case EffectStep::PunchDown:
            cameraEffects.triggerPunch(toScalar(PUNCH_AMPLITUDE), PUNCH_DURATION_MS,
                                       math::Vector2::DOWN());
            activeEffectName = "Punch Down";
            break;
        case EffectStep::PunchLeft:
            cameraEffects.triggerPunch(toScalar(PUNCH_AMPLITUDE), PUNCH_DURATION_MS,
                                       math::Vector2::LEFT());
            activeEffectName = "Punch Left";
            break;
        case EffectStep::PunchRight:
            cameraEffects.triggerPunch(toScalar(PUNCH_AMPLITUDE), PUNCH_DURATION_MS,
                                       math::Vector2::RIGHT());
            activeEffectName = "Punch Right";
            break;
        case EffectStep::Offset:
            cameraEffects.triggerOffset(toScalar(OFFSET_AMPLITUDE), OFFSET_DURATION_MS);
            activeEffectName = "Offset";
            break;
        default:
            return;
    }

    uint8_t step = static_cast<uint8_t>(nextEffect) + 1;
    if (step >= static_cast<uint8_t>(EffectStep::COUNT)) {
        step = 0;
    }
    nextEffect = static_cast<EffectStep>(step);
}

// ---------------------------------------------------------------------------
// Camera tween
// ---------------------------------------------------------------------------

void CameraDemoScene::startCameraTour() {
    Scalar targetX = toScalar(static_cast<float>(TWEEN_TARGET_TILE_X * TILE_SIZE
                                                 - DISPLAY_WIDTH / 2));
    if (targetX < toScalar(0.0f)) {
        targetX = toScalar(0.0f);
    }
    Scalar maxX = toScalar(levelWidth - static_cast<float>(DISPLAY_WIDTH));
    if (maxX < toScalar(0.0f)) {
        maxX = toScalar(0.0f);
    }
    if (targetX > maxX) {
        targetX = maxX;
    }

    // Vertical bounds are locked at 0 in init(), so the tour is horizontal.
    tweens.startTween(camera.getPosition(),
                      math::Vector2(targetX, toScalar(0.0f)),
                      TWEEN_DURATION_MS,
                      gfx::TweenEasing::EaseInOutQuad);
    tourStage = TourStage::Out;
    tourHoldElapsed = 0;
}

void CameraDemoScene::updateCameraTour(unsigned long deltaTime) {
    if (tourStage == TourStage::Idle) {
        return;
    }

    // deltaTime is milliseconds; the tween pool takes a 16-bit millisecond step.
    uint16_t stepMs = deltaTime > 0xFFFFu ? 0xFFFFu
                                          : static_cast<uint16_t>(deltaTime);
    tweens.update(stepMs, &camera);

    if (tweens.activeCount() > 0) {
        return;
    }

    switch (tourStage) {
        case TourStage::Out:
            tourStage = TourStage::Hold;
            tourHoldElapsed = 0;
            break;

        case TourStage::Hold:
            tourHoldElapsed += deltaTime;
            if (tourHoldElapsed >= TWEEN_HOLD_MS) {
                // Pan back to wherever the player is NOW, not to where the
                // camera started — the player is free to move during the tour.
                math::Vector2 back = camera.getPosition();
                if (player) {
                    Scalar centerX = player->position.x
                                   + toScalar(player->width) * toScalar(0.5f);
                    Scalar backX = centerX - toScalar(DISPLAY_WIDTH / 2);
                    if (backX < toScalar(0.0f)) {
                        backX = toScalar(0.0f);
                    }
                    Scalar maxX = toScalar(levelWidth - static_cast<float>(DISPLAY_WIDTH));
                    if (maxX < toScalar(0.0f)) {
                        maxX = toScalar(0.0f);
                    }
                    if (backX > maxX) {
                        backX = maxX;
                    }
                    back = math::Vector2(backX, toScalar(0.0f));
                }
                tweens.startTween(camera.getPosition(), back,
                                  TWEEN_DURATION_MS,
                                  gfx::TweenEasing::EaseInOutQuad);
                tourStage = TourStage::Back;
            }
            break;

        case TourStage::Back:
            tourStage = TourStage::Idle;
            break;

        default:
            break;
    }
}

void CameraDemoScene::draw(gfx::Renderer& renderer) {
    Scalar camX = camera.getX();

    // The active effect resolves to a single offset for this frame. Adding it
    // to every layer's display offset shakes the whole world at once; the HUD
    // below is drawn after the offset is cleared, so it stays rock steady.
    math::Vector2 fx = getCameraEffectOffset();
    int fxX = static_cast<int>(fx.x);
    int fxY = static_cast<int>(fx.y);

    Scalar farFactor = toScalar(0.4f);
    int farOffset = static_cast<int>(-camX * farFactor);
    renderer.setDisplayOffset(farOffset + fxX, fxY);

    int horizonY = DISPLAY_HEIGHT / 3;
    int hillHeight = DISPLAY_HEIGHT / 4;

    renderer.drawFilledRectangle(-40, horizonY, DISPLAY_WIDTH + 80, hillHeight, Color::DarkBlue);
    renderer.drawFilledRectangle(DISPLAY_WIDTH / 2, horizonY + 10, DISPLAY_WIDTH, hillHeight + 10, Color::DarkGray);

    int midOffset = static_cast<int>(-camX * toScalar(0.7f));
    renderer.setDisplayOffset(midOffset + fxX, fxY);

    int midY = (DISPLAY_HEIGHT * 2) / 3;
    renderer.drawFilledRectangle(-20, midY, DISPLAY_WIDTH + 40, 10, Color::DarkGreen);

    int mainOffset = static_cast<int>(-camX);
    renderer.setDisplayOffset(mainOffset + fxX, fxY);

    renderer.drawTileMap(PLATFORMER_MAP, 0, 0, Color::Brown);

    if (player) {
        player->draw(renderer);
    }
    for (int i = 0; i < entityCount; ++i) {
        if (ownedEntities[i]->isVisible) {
            ownedEntities[i]->draw(renderer);
        }
    }

    renderer.setDisplayOffset(0, 0);
    drawHud(renderer);
}

void CameraDemoScene::drawHud(gfx::Renderer& renderer) {
    renderer.drawText("B:effect  UP:pan  DOWN:cancel", 4, 4, Color::Cyan, 1);

    if (tourStage != TourStage::Idle) {
        renderer.drawText("Camera tween", 4, 14, Color::Yellow, 1);
    } else if (activeEffectName != nullptr) {
        renderer.drawText(activeEffectName, 4, 14, Color::Yellow, 1);
    }
}

}
