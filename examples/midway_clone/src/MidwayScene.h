#pragma once

#include "platforms/PlatformDefaults.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "core/Scene.h"
#include "gameplay/ObjectPool.h"
#include "graphics/Camera2D.h"
#include "graphics/Renderer.h"
#include "graphics/StaticTilemapLayerCache.h"

#include "GameConstants.h"
#include "PlayerActor.h"

namespace midway_clone {

/**
 * @struct Bullet
 * @brief One tracer. Travels on Y only, in world pixels.
 *
 * Player fire and enemy fire share this type and differ only in the sign of
 * `speedPxPerSec` and which pool they live in. Two structs would be two copies
 * of the same three lines of motion.
 */
struct Bullet {
    int x = 0;
    int y = 0;
    int speedPxPerSec = 0;
    int travel = 0;   ///< Carried sub-pixel travel, px*ms. See advancePixels.
};

/**
 * @struct Enemy
 * @brief One enemy aircraft, descending and optionally weaving.
 */
struct Enemy {
    int x = 0;
    int y = 0;
    int homeX = 0;            ///< Centre a weaving enemy sways about.
    int travelY = 0;
    unsigned long swayMs = 0; ///< Phase into the sway cycle.
    unsigned long fireTimer = 0;
    unsigned long frameTimer = 0;
    uint8_t frame = 0;        ///< Propeller frame, 0 or 1.
    uint8_t weaves = 0;       ///< Non-zero: sway horizontally while descending.
};

/**
 * @struct Explosion
 * @brief A three-frame fireball at a fixed world position.
 *
 * It does not ride the scroll. An explosion lasts 210 ms, over which the world
 * moves under it by seven pixels — not enough to read as the fireball sliding
 * off the wreck, and pinning it to the world would cost a per-explosion
 * accumulator for that.
 */
struct Explosion {
    int x = 0;
    int y = 0;
    unsigned long timer = 0;
    uint8_t frame = 0;
};

/**
 * @class MidwayScene
 * @brief One continuously scrolling stage: the sea moves, the player holds
 *        station, enemies arrive on a schedule keyed to the scroll.
 *
 * ### What this example is actually for
 *
 * Every other tilemap example in this tree keeps its camera still and lets
 * StaticTilemapLayerCache replay the terrain with a memcpy. This one moves the
 * camera every frame, so the cache misses every frame, and the terrain is
 * rebuilt from tiles 30 times a second. The cache is still wired up — the same
 * three calls metroidvania makes — precisely so the cost of missing it can be
 * measured rather than argued about.
 *
 * The result is not what it looks like. Scrolling costs CPU redraw time and no
 * transmit time at all: the ESP32 driver pushes the entire framebuffer on every
 * present regardless of what changed (TFT_eSPI_Drawer.cpp:120,
 * sendBufferScaled), and at 240x240 RGB565 over 40 MHz SPI that push alone is
 * 23.0 ms. The scroll adds a few milliseconds on top of a cost every example
 * here already pays. See the README.
 *
 * ### Why the pools are not entities
 *
 * The player is an Entity and goes through Scene::draw. Bullets, enemies and
 * explosions do not: they live in ObjectPools this scene owns and draws
 * directly. Scene::draw sorts its entity list and viewport-culls each member
 * every frame, which is worth paying for a handful of long-lived objects and
 * not for thirty projectiles that are already known to be on screen.
 */
class MidwayScene : public pixelroot32::core::Scene {
public:
    using BulletPool    = pixelroot32::gameplay::ObjectPool<Bullet, kMaxPlayerBullets>;
    using EnemyBulletPool = pixelroot32::gameplay::ObjectPool<Bullet, kMaxEnemyBullets>;
    using EnemyPool     = pixelroot32::gameplay::ObjectPool<Enemy, kMaxEnemies>;
    using ExplosionPool = pixelroot32::gameplay::ObjectPool<Explosion, kMaxExplosions>;

    /**
     * @brief Sizes the camera viewport.
     *
     * Camera2D has no default constructor — it takes its viewport up front —
     * so this cannot be `= default`. The viewport is the playfield, not the
     * panel: the HUD strip below is drawn in screen space afterwards and must
     * not be part of what the camera considers visible.
     */
    MidwayScene();

    void init() override;
    void update(unsigned long deltaTime) override;
    void adviseFramebufferBeforeBeginFrame(pixelroot32::graphics::Renderer& renderer) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    pixelroot32::graphics::Camera2D camera_;
    pixelroot32::graphics::StaticTilemapLayerCache tilemapLayerCache_;
    PlayerActor player_;

    BulletPool      playerBullets_;
    EnemyBulletPool enemyBullets_;
    EnemyPool       enemies_;
    ExplosionPool   explosions_;

    /// World y of the top of the playfield. Counts DOWN from kCameraStartY.
    int cameraY_ = kCameraStartY;
    /// Carried sub-pixel scroll, px*ms. See advancePixels.
    int scrollTravel_ = 0;

    /// Index into the wave table of the next wave not yet spawned.
    uint16_t nextWave_ = 0;

    int score_ = 0;
    int lives_ = kPlayerLives;

    /// True once the camera reaches the top of the map.
    bool stageComplete_ = false;

    /**
     * @brief True when the stage has ended, either way.
     *
     * Both endings behave identically: everything freezes on the last frame and
     * the only input read is restart. An arcade cabinet holds STAGE CLEAR on
     * screen the same way it holds GAME OVER — the run is over and the machine
     * is waiting, and which of the two it was is the HUD's business, not the
     * update loop's.
     */
    bool isTerminal() const { return lives_ <= 0 || stageComplete_; }

    void updateScroll(unsigned long deltaTime);
    void spawnDueWaves();
    void updateBullets(unsigned long deltaTime);
    void updateEnemies(unsigned long deltaTime);
    void updateExplosions(unsigned long deltaTime);
    void resolveCollisions();

    void spawnExplosion(int worldX, int worldY);
    void killPlayer();

    void drawHud(pixelroot32::graphics::Renderer& renderer);

    /// The one static layer, in the shape StaticTilemapLayerCache takes.
    pixelroot32::graphics::TileMap4bppDrawSpec seaLayer() const;
};

} // namespace midway_clone

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
