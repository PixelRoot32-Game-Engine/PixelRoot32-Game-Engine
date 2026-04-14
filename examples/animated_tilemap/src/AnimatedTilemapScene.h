#pragma once
#include <platforms/PlatformDefaults.h>

// This example requires tile animation support
#if PIXELROOT32_ENABLE_TILE_ANIMATIONS

#include <core/Scene.h>
#include <graphics/Renderer.h>
#include <graphics/Color.h>
#include <graphics/TileAnimation.h>
#include <graphics/StaticTilemapLayerCache.h>
#include <platforms/EngineConfig.h>

#include "assets/ColorPaletteManager.h"

namespace animatedtilemap {

/**
 * @file AnimatedTilemapScene.h
 * @class AnimatedTilemapScene
 * @brief Demonstration of the tile animation system with water, fire, and static tiles.
 * 
 * This example showcases:
 * - Water tiles (8 frames, slow animation)
 * - Fire tiles (4 frames, fast animation)
 * - Static tiles (grass, stone - no animation)
 * - Animation speed control
 * - Multiple animation managers
 * 
 * The scene demonstrates how to:
 * 1. Define tile animations in PROGMEM
 * 2. Create and configure TileAnimationManager
 * 3. Link animation manager to tilemap
 * 4. Control animation speed (pause, slow, normal, fast)
 * 5. Use multiple layers with independent animations
 */
class AnimatedTilemapScene : public pixelroot32::core::Scene {
public:
    AnimatedTilemapScene();
    ~AnimatedTilemapScene() override = default;

    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * When the static draw group (background only in this example) changes — tile data,
     * palette, mask, or stepping an animator bound to background — call this so the
     * framebuffer snapshot is rebuilt.
     */
    void invalidateStaticLayerCache();

protected:
    struct LevelMapData {
        const pixelroot32::graphics::TileMap4bpp* background;
        const pixelroot32::graphics::TileMap4bpp* ground;
        const pixelroot32::graphics::TileMap4bpp* details;

        int mapWidth;
        int mapHeight;
        int tileSize;
    };

    LevelMapData levelData;
    pixelroot32::math::Scalar levelWidth;
    ColorPaletteManager colorPaletteManager;

private:
    void setupLevelData();
    void setupTilemapLayers();

    pixelroot32::graphics::StaticTilemapLayerCache tilemapLayerCache;
};

/**
 * @brief Tilemap layer entity for rendering background layers.
 */
class TileMapLayerEntity : public pixelroot32::core::Entity {
public:
    TileMapLayerEntity(const pixelroot32::graphics::TileMap4bpp* tileMap, 
                         int mapWidth, int mapHeight, int tileSize, int layer);
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    const pixelroot32::graphics::TileMap4bpp* tileMap;
};

} // namespace animatedtilemap

#endif // PIXELROOT32_ENABLE_TILE_ANIMATIONS
