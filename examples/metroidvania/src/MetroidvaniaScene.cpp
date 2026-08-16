#include "platforms/PlatformDefaults.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "MetroidvaniaScene.h"
#include "PlayerActor.h"
#include "GameConstants.h"
#include "core/Engine.h"
#include "input/InputManager.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "assets/MetroidvaniaSceneOneTileMap.h"
#include "assets/MetroidvaniaSceneOneTileMapPalette.h"
#include "assets/PlayerPalette.h"
#include "physics/StaticActor.h"
#include "GameLayers.h"
#include <cstdint>
#include <cstdio>

#include "platforms/PlatformMemory.h"

extern pixelroot32::core::Engine engine;

namespace pr32 = pixelroot32;

namespace metroidvania {

using pr32::graphics::Color;

MetroidvaniaScene::MetroidvaniaScene() = default;
MetroidvaniaScene::~MetroidvaniaScene() = default;

/**
 * @brief Main scene for the Metroidvania example
 * 
 * Architecture:
 * - Tilemap-based Level: Background, Platforms, and Stairs are drawn via
 *   StaticTilemapLayerCache (ESP32 fast path: snapshot + memcpy when camera stable).
 * - Optimized Collision: Instead of creating thousands of StaticActors for
 *   tiles, the player checks collisions directly against the tilemap data.
 * - 4BPP Sprites: Demonstrates memory-efficient sprite rendering.
 */
void MetroidvaniaScene::init() {
    metroidvania::init();

    tilemapLayerCache.clear();
    
    // Dual palette mode: tilemaps use Background palette, player uses Sprite palette.
    // Without this, both would use the default PR32 palette (white background, wrong colors).
    pr32::graphics::enableDualPaletteMode(true);
    pr32::graphics::setBackgroundCustomPalette(metroidvania::TILEMAP_PALETTE_DATA);
    pr32::graphics::setSpriteCustomPalette(metroidvania::PLAYER_SPRITE_PALETTE_RGB565);

    tilemapLayerCache.invalidate();
    (void)tilemapLayerCache.allocateForRenderer(engine.getRenderer());

    // Create and add the player.
    player = std::make_unique<PlayerActor>(pixelroot32::math::Vector2(PLAYER_START_X, PLAYER_START_Y));
    addEntity(player.get());

#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
    // The collision system holds a NON-owning pointer; the tracker is a member
    // of this scene, so its lifetime already covers every frame the system
    // runs. It is also reset from CollisionSystem::clear(), which is what
    // keeps stale contact pairs from dispatching through dangling pointers
    // after a scene reset.
    collisionSystem.setInteractionTracker(&interactionTracker);
#if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS
    // Optional second output. With no bus attached the component callbacks
    // still fire; attaching one ALSO publishes a TriggerEnter/TriggerExit per
    // edge, which is what lets unrelated systems observe contact without
    // knowing about PickupActor at all.
    interactionTracker.setEventBus(&engine.getGameplayEventBus());
    lastTriggerEnterCount = 0;
    lastTriggerExitCount = 0;
#endif
    spawnPickups();
#endif

    // Pass stairs layer data to the player for internal logic.
    player->setStairs(metroidvania::stairs.indices, 
                      metroidvania::MAP_WIDTH, 
                      metroidvania::MAP_HEIGHT, 
                      metroidvania::TILE_SIZE);
    player->buildStairsCache();  // RAM cache to avoid repeated PROGMEM reads on ESP32

    // Generate StaticActors for platforms using a horizontal greedy meshing algorithm
    const uint8_t* pIndices = metroidvania::platforms.indices;
    int pWidth = metroidvania::MAP_WIDTH;
    int pHeight = metroidvania::MAP_HEIGHT;
    int pSize = metroidvania::TILE_SIZE;

    for (int y = 0; y < pHeight; ++y) {
        int x = 0;
        while (x < pWidth) {
            uint8_t tile = PIXELROOT32_READ_BYTE_P(pIndices + (y * pWidth + x));
            if (tile > 0) {
                int startX = x;
                while (x < pWidth) {
                    uint8_t nextTile = PIXELROOT32_READ_BYTE_P(pIndices + (y * pWidth + x));
                    if (nextTile > 0) x++;
                    else break;
                }
                
                int w = (x - startX) * pSize;
                int h = pSize;
                float px = static_cast<float>(startX * pSize);
                float py = static_cast<float>(y * pSize);

                auto platform = std::make_unique<pixelroot32::physics::StaticActor>(
                    pixelroot32::math::toScalar(px),
                    pixelroot32::math::toScalar(py),
                    w, h);
                platform->setShape(pixelroot32::core::CollisionShape::AABB);
                platform->setCollisionLayer(Layers::GROUND);
                platform->setCollisionMask(0); // Passive platforms, the dynamic collider (Player) will decide whether to apply collision or not.
                
                addEntity(platform.get());
                layers.push_back(std::move(platform));
            } else {
                x++;
            }
        }
    }
}

void MetroidvaniaScene::update(unsigned long deltaTime) {
    auto& input = engine.getInputManager();
    
    // Input management: map buttons to directions.
    float moveDir = 0.0f;
    if (input.isButtonDown(BTN_RIGHT)) moveDir += 1.0f;  // Right
    if (input.isButtonDown(BTN_LEFT)) moveDir -= 1.0f;  // Left

    float vDir = 0.0f;
    if (input.isButtonDown(BTN_UP)) vDir -= 1.0f;     // Up
    if (input.isButtonDown(BTN_DOWN)) vDir += 1.0f;     // Down

    bool jumpPressed = input.isButtonPressed(BTN_JUMP);  // Jump (A / Space)
    
    // Send processed inputs to the player actor.
    if (player) player->setInput(moveDir, vDir, jumpPressed);

    // Update all entities in the scene.
    pixelroot32::core::Scene::update(deltaTime);

    // Check if player just fell into the void — trigger camera shake.
    if (player && player->consumeRespawnFlag()) {
        cameraEffects.triggerShake(pixelroot32::math::toScalar(4.0f), 200);
    }

#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
    // Read the collected count off the orbs themselves. The callback already
    // ran during the physics step inside Scene::update() above.
    pickupsCollected = 0;
    for (int i = 0; i < kPickupCount; ++i) {
        if (pickups[i] && pickups[i]->isCollected()) {
            ++pickupsCollected;
        }
    }
#endif

#if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS
    drainGameplayEvents();
#endif
}

#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
void MetroidvaniaScene::spawnPickups() {
    // Three orbs along the player's path, spaced far enough apart that each
    // one produces a distinct enter edge.
    static const float kOffsetsX[kPickupCount] = { 60.0f, 100.0f, 140.0f };

    for (int i = 0; i < kPickupCount; ++i) {
        pickups[i] = std::make_unique<PickupActor>(
            pixelroot32::math::toScalar(kOffsetsX[i]),
            pixelroot32::math::toScalar(PLAYER_START_Y));
        addEntity(pickups[i].get());
        // MUST come after addEntity: registerActor() keys on entityId, which
        // the collision system assigns during registration. Registering first
        // would silently store id 0 and the orb would never dispatch.
        interactionTracker.registerActor(pickups[i].get(), &pickups[i]->interaction);
    }
    pickupsCollected = 0;
}
#endif

#if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS
void MetroidvaniaScene::drainGameplayEvents() {
    lastTriggerEnterCount = 0;
    lastTriggerExitCount = 0;

    // The bus is a fixed-capacity FIFO with a drop-newest overflow policy, so
    // a consumer that skips a frame loses the NEWEST events, not the oldest —
    // draining every frame is the contract, not an optimisation.
    pr32::gameplay::GameplayEvent event;
    while (engine.getGameplayEventBus().consume(event)) {
        switch (event.type) {
            case pr32::gameplay::GameplayEventType::TriggerEnter:
                ++lastTriggerEnterCount;
                break;
            case pr32::gameplay::GameplayEventType::TriggerExit:
                ++lastTriggerExitCount;
                break;
            default:
                break;
        }
    }
}
#endif

void MetroidvaniaScene::adviseFramebufferBeforeBeginFrame(pr32::graphics::Renderer& renderer) {
#if PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
    const int camX = -renderer.getXOffset();
    const int camY = -renderer.getYOffset();
    const pr32::graphics::TileMap4bppDrawSpec staticLayers[] = {
        {&metroidvania::background, 0, 0},
        {&metroidvania::platforms, 0, 0},
        {&metroidvania::stairs, 0, 0},
    };
    tilemapLayerCache.adviseFramebufferBeforeBeginFrame(
        renderer,
        camX,
        camY,
        staticLayers,
        sizeof(staticLayers) / sizeof(staticLayers[0]),
        nullptr,
        0);
#else
    (void)renderer;
#endif
}

void MetroidvaniaScene::draw(pr32::graphics::Renderer& renderer) {
    // Apply camera effect offset (shake/punch) BEFORE tilemaps so everything shakes.
    // The tilemap cache auto-invalidates because camX/Y change each shake frame.
    auto effectOffset = getCameraEffectOffset();
    renderer.setDisplayOffset(
        static_cast<int>(effectOffset.x),
        static_cast<int>(effectOffset.y));

    const int camX = -renderer.getXOffset();
    const int camY = -renderer.getYOffset();

    const pr32::graphics::TileMap4bppDrawSpec staticLayers[] = {
        {&metroidvania::background, 0, 0},
        {&metroidvania::platforms, 0, 0},
        {&metroidvania::stairs, 0, 0},
    };

    tilemapLayerCache.draw(renderer, camX, camY,
        staticLayers, sizeof(staticLayers) / sizeof(staticLayers[0]),
        nullptr, 0);

    // Entities drawn with shake offset already set
    pixelroot32::core::Scene::draw(renderer);

    // Reset offset
    renderer.setDisplayOffset(0, 0);

#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
    char hud[24];
    std::snprintf(hud, sizeof(hud), "ORBS %d/%d", pickupsCollected, kPickupCount);
    renderer.drawText(hud, 4, 4, Color::Yellow, 1);
#endif
}

} // namespace metroidvania

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
