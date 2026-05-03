#pragma once
#include "platforms/PlatformDefaults.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "core/Scene.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "graphics/StaticTilemapLayerCache.h"
#include "platforms/EngineConfig.h"
#include <memory>
#include <vector>

namespace metroidvania {

class PlayerActor;

/**
 * @class MetroidvaniaScene
 * @brief Platformer scene with tilemap layers and player movement.
 *
 * Requires PIXELROOT32_ENABLE_4BPP_SPRITES. Uses tilemaps for background,
 * platforms, and stairs; KinematicActor for player with custom collision.
 */
class MetroidvaniaScene : public pixelroot32::core::Scene {
public:
    MetroidvaniaScene();
    virtual ~MetroidvaniaScene();
    void init() override;
    void update(unsigned long deltaTime) override;
    void adviseFramebufferBeforeBeginFrame(pixelroot32::graphics::Renderer& renderer) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    std::unique_ptr<PlayerActor> player;
    /** Player + platform StaticActors (tilemap layers use StaticTilemapLayerCache in draw()). */
    std::vector<std::unique_ptr<pixelroot32::core::Entity>> layers;
    pixelroot32::graphics::StaticTilemapLayerCache tilemapLayerCache;
};

} // namespace metroidvania

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
