#include "AnimatedTilemapScene.h"

#ifdef PIXELROOT32_ENABLE_TILE_ANIMATIONS

#include <core/Engine.h>
#include <core/Log.h>

#include "assets/AnimatedTiles.h"

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace animatedtilemap {

namespace gfx = pr32::graphics;
namespace logging = pr32::core::logging;

using gfx::Color;
using pr32::math::Vector2;
using logging::log;

// --- TileMapLayerEntityNew Implementation ---

TileMapLayerEntity::TileMapLayerEntity(const gfx::TileMap4bpp* tileMap, 
                                           int mapWidth, int mapHeight, int tileSize, int layer)
    : pr32::core::Entity(pr32::math::toScalar(0.0f), pr32::math::toScalar(0.0f),
                        pr32::math::toScalar(mapWidth * tileSize),
                        pr32::math::toScalar(mapHeight * tileSize),
                        pr32::core::EntityType::GENERIC),
      tileMap(tileMap) {
    setRenderLayer(layer);
    position = pr32::math::Vector2(pr32::math::toScalar(0.0f), pr32::math::toScalar(0.0f));
}

void TileMapLayerEntity::update(unsigned long) {
    // Tilemap layers don't need per-frame updates
}

void TileMapLayerEntity::draw(gfx::Renderer& renderer) {
    if (tileMap) {
        renderer.drawTileMap(*tileMap, static_cast<int>(position.x), static_cast<int>(position.y));
    }
}

// --- BaseLevelSceneNew Implementation ---

AnimatedTilemapScene::AnimatedTilemapScene()
    : levelWidth(pr32::math::toScalar(0.0f)),
     colorPaletteManager() {
    levelData = {nullptr, nullptr, nullptr, 0, 0, 0};
}

void AnimatedTilemapScene::init() {
    animatedtiles::init();

    tilemapLayerCache.clear();

    #ifdef PIXELROOT32_ENABLE_SCENE_ARENA
    static uint8_t sceneArenaBuffer[8192];
    if (arena.capacity == 0) {
        arena.init(sceneArenaBuffer, sizeof(sceneArenaBuffer));
    }
    arena.reset();
#endif
    setupLevelData();
    
    // Calculate level width for camera bounds
    levelWidth = pixelroot32::math::toScalar(levelData.mapWidth * levelData.tileSize);

    colorPaletteManager.initalizeGamePalettes();

    tilemapLayerCache.invalidate();
    (void)tilemapLayerCache.allocateForRenderer(engine.getRenderer());

    setupTilemapLayers();
}

void AnimatedTilemapScene::setupLevelData() {
    levelData = {
        &animatedtiles::background,
        &animatedtiles::ground,
        &animatedtiles::details,
        animatedtiles::MAP_WIDTH,
        animatedtiles::MAP_HEIGHT,
        animatedtiles::TILE_SIZE
    };
}

void AnimatedTilemapScene::setupTilemapLayers() {
    // Tilemaps are drawn in draw() via StaticTilemapLayerCache (background + ground snapshotted;
    // details redrawn each frame when fast path applies). Adjust TileMap4bppDrawSpec lists in draw()
    // for other layer splits. Use TileMapLayerEntity + Scene::draw for the generic entity path.
}

void AnimatedTilemapScene::update(unsigned long deltaTime) {

    // Update animated tiles — if you enable stepping, also call invalidateStaticLayerCache()
    // when the stepped manager affects background or ground layers (details-only fast path).
    animatedtiles::getGroundAnimManager().step();
    animatedtiles::getDetailsAnimManager().step();

    invalidateStaticLayerCache();
    
    // Update scene entities
    Scene::update(deltaTime);
}

void AnimatedTilemapScene::invalidateStaticLayerCache() {
    tilemapLayerCache.invalidate();
}

void AnimatedTilemapScene::draw(gfx::Renderer& renderer) {
    const int camX = -renderer.getXOffset();
    const int camY = -renderer.getYOffset();

    const gfx::TileMap4bppDrawSpec staticLayers[] = {
        {levelData.background, 0, 0},
        {levelData.ground, 0, 0},
    };
    const gfx::TileMap4bppDrawSpec dynamicLayers[] = {
        {levelData.details, 0, 0},
    };

    tilemapLayerCache.draw(renderer, camX, camY,
        staticLayers, sizeof(staticLayers) / sizeof(staticLayers[0]),
        dynamicLayers, sizeof(dynamicLayers) / sizeof(dynamicLayers[0]));

    Scene::draw(renderer);
}

} // namespace animatedtilemap

#endif // PIXELROOT32_ENABLE_TILE_ANIMATIONS
