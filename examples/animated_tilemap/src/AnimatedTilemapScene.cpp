#include "AnimatedTilemapScene.h"

#ifdef PIXELROOT32_ENABLE_TILE_ANIMATIONS

#include <core/Engine.h>
#include <core/Log.h>

#include "assets/AnimatedTiles.h"

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace animatedtilemap {

using namespace pr32::core::logging;
using pr32::graphics::Color;
using pr32::math::Vector2;

// --- TileMapLayerEntityNew Implementation ---

TileMapLayerEntity::TileMapLayerEntity(const pixelroot32::graphics::TileMap4bpp* tileMap, 
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

void TileMapLayerEntity::draw(pr32::graphics::Renderer& renderer) {
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

AnimatedTilemapScene::~AnimatedTilemapScene() {
    // Cleanup is handled by the scene arena system
}

void AnimatedTilemapScene::init() {
    animatedtiles::init();

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
    if (levelData.background) {
        addEntity(pr32::core::arenaNew<TileMapLayerEntity>(
            arena, levelData.background, levelData.mapWidth, levelData.mapHeight, 
            levelData.tileSize, 0));
    }
    
    if (levelData.ground) {
        addEntity(pr32::core::arenaNew<TileMapLayerEntity>(
            arena, levelData.ground, levelData.mapWidth, levelData.mapHeight, 
            levelData.tileSize, 1));
    }

    if(levelData.details) {
        addEntity(pr32::core::arenaNew<TileMapLayerEntity>(
            arena, levelData.details, levelData.mapWidth, levelData.mapHeight, 
            levelData.tileSize, 2));
    }
}

void AnimatedTilemapScene::update(unsigned long deltaTime) {

    // Update animated tiles
    animatedtiles::getGroundAnimManager().step();
    animatedtiles::getDetailsAnimManager().step();
    
    // Update scene entities
    Scene::update(deltaTime);
}

void AnimatedTilemapScene::draw(pr32::graphics::Renderer& renderer) {
    // Draw world entities (tilemaps, actors, etc.)
    Scene::draw(renderer);
}

} // namespace animatedtilemap

#endif // PIXELROOT32_ENABLE_TILE_ANIMATIONS
