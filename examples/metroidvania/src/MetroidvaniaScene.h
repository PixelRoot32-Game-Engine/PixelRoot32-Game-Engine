#pragma once
#include "platforms/PlatformDefaults.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "core/Scene.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "graphics/StaticTilemapLayerCache.h"
#include "platforms/EngineConfig.h"
#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
#include "gameplay/InteractionTracker.h"
#include "PickupActor.h"
#endif
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

#if PIXELROOT32_ENABLE_INTERACTION_TRIGGERS
    /** Spawns the orbs and registers them with the tracker. */
    void spawnPickups();

    static constexpr int kPickupCount = 3;

    /**
     * Scene-owned, because CollisionSystem only holds a non-owning pointer to
     * it and Scene::resetState() resets the collision system underneath.
     */
    pixelroot32::gameplay::InteractionTracker interactionTracker;
    std::unique_ptr<PickupActor> pickups[kPickupCount];
    int pickupsCollected = 0;
#endif

#if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS
    /** Drains the Engine-owned bus and counts this frame's trigger edges. */
    void drainGameplayEvents();

    uint16_t lastTriggerEnterCount = 0;
    uint16_t lastTriggerExitCount = 0;
#endif
};

} // namespace metroidvania

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
