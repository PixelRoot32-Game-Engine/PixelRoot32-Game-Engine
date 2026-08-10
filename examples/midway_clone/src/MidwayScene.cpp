#include "MidwayScene.h"

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

#include "core/Engine.h"
#include "graphics/Color.h"

#include "assets/EffectSprites.h"
#include "assets/EnemySprites.h"
#include "assets/OceanTileMap.h"
#include "assets/SpritePalette.h"
#include "assets/TilemapPalette.h"

#include <cstdio>

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace midway_clone {

namespace gfx = pr32::graphics;
namespace math = pr32::math;

// ---------------------------------------------------------------------------
// Wave table
// ---------------------------------------------------------------------------
// Waves are keyed to the CAMERA, not to a clock. Scroll rate and stage length
// can both change without any of these numbers moving, and a wave always
// arrives over the same stretch of water — which is what makes a stage
// learnable. A timer would decouple the two and the same wave would meet the
// player somewhere different every run.
//
// The camera counts DOWN from kCameraStartY (1392) to 0, so these are in
// descending order and a wave fires when the camera has climbed past it.

namespace {

struct Wave {
    int     triggerCameraY;
    uint8_t count;
    uint8_t weaves;      ///< Non-zero: the formation sways as it descends.
    int     startCol;    ///< Leftmost member, in world tiles.
    int     stepXPx;     ///< Horizontal offset per member. 0 is a column.
    int     spacingPx;   ///< Vertical gap between members.
};

const Wave kWaves[] = {
    { 1300, 3, 0,  6,   0, 26 },
    { 1200, 3, 1, 14,   0, 26 },
    { 1100, 4, 0,  4,  14, 22 },
    { 1000, 3, 1, 18,   0, 26 },
    {  900, 4, 0, 10, -14, 22 },
    {  800, 4, 1,  6,   0, 24 },
    {  700, 5, 0, 12,  12, 20 },
    {  600, 4, 1,  3,   0, 26 },
    {  500, 5, 0, 16, -12, 20 },
    {  400, 5, 1,  8,   0, 22 },
    {  300, 5, 0,  5,  14, 24 },
    {  200, 6, 1, 11,   0, 20 },
};

constexpr uint16_t kWaveCount = static_cast<uint16_t>(sizeof(kWaves) / sizeof(kWaves[0]));

static_assert(kMaxEnemies >= 6,
              "The largest wave in kWaves is 6. A pool smaller than the biggest "
              "wave silently spawns a short formation instead of failing.");

/**
 * Quarter-amplitude sine, 16 steps, scaled to 64.
 *
 * A table rather than sinf(): this runs once per weaving enemy per frame on a
 * target where the non-FPU build would soften-float every call. Sixteen steps
 * is coarse, but the sway is 28 px peak to peak over 1.4 seconds — the steps
 * are well under a pixel apart.
 */
const int8_t kSway[16] = {
    0, 24, 45, 59, 64, 59, 45, 24, 0, -24, -45, -59, -64, -59, -45, -24,
};

int swayOffset(unsigned long phaseMs) {
    const unsigned long step = (phaseMs * 16u / kEnemySwayPeriodMs) & 15u;
    return (kEnemySwayAmplitudePx * kSway[step]) / 64;
}

int clampInt(int value, int low, int high) {
    if (value < low)  return low;
    if (value > high) return high;
    return value;
}

} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

MidwayScene::MidwayScene()
    : camera_(kPlayfieldWidth, kPlayfieldHeight) {
}

void MidwayScene::init() {
    // Base first: it runs resetState(), which clears the entity list. Resetting
    // the pools before that would destruct objects the scene still points at.
    Scene::init();

    playerBullets_.reset();
    enemyBullets_.reset();
    enemies_.reset();
    explosions_.reset();

    // Dual palette: the sea gets its own 16 slots and the aircraft get theirs,
    // instead of both competing for one table. Installed by pointer and never
    // copied, which is why these are namespace-scope constants from the export
    // and not locals.
    gfx::enableDualPaletteMode(true);
    gfx::setBackgroundCustomPalette(TILEMAP_PALETTE_DATA);
    gfx::setSpriteCustomPalette(SPRITE_PALETTE_DATA);

    ocean::init();

    // A snapshot taken under another scene's palette and tiles is worthless to
    // this one. Drop it, then take the buffer back at init time so the game
    // loop never reaches the heap.
    tilemapLayerCache_.clear();
    tilemapLayerCache_.invalidate();
    (void)tilemapLayerCache_.allocateForRenderer(engine.getRenderer());

    cameraY_ = kCameraStartY;
    scrollTravel_ = 0;
    nextWave_ = 0;
    score_ = 0;
    lives_ = kPlayerLives;
    stageComplete_ = false;

    // Viewport size is set by the constructor and never changes. Only the
    // bounds and the position are re-established here, because init() runs on
    // every scene swap.
    //
    // THE BOUNDS ARE NOT OPTIONAL. Camera2D starts with minX/maxX/minY/maxY all
    // zero (Camera2D.cpp:15-22) and setPosition() clamps against them silently
    // (Camera2D.cpp:34-41), so a camera that is never given bounds is pinned to
    // the origin no matter what it is told. It fails with no error and no
    // warning: the world renders row 0 forever while the game logic runs
    // correctly somewhere off screen, which looks like every sprite in the game
    // having vanished.
    //
    // legend_of_clone never has to do this because RoomGraph::enterRoom() sets
    // the bounds for it, once per room. There is no room graph here.
    camera_.setBounds(math::toScalar(0), math::toScalar(0));
    camera_.setVerticalBounds(math::toScalar(0), math::toScalar(kCameraStartY));
    camera_.setPosition(math::Vector2(math::toScalar(0), math::toScalar(cameraY_)));

    player_.setViewportTop(cameraY_);
    player_.respawn();
    player_.setEnabled(true);
    addEntity(&player_);
}

gfx::TileMap4bppDrawSpec MidwayScene::seaLayer() const {
    return { &ocean::sea, 0, 0 };
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void MidwayScene::updateScroll(unsigned long deltaTime) {
    if (stageComplete_) {
        return;
    }
    // Negative: the camera climbs toward row 0.
    cameraY_ += advancePixels(scrollTravel_, -kScrollSpeedPxPerSec, deltaTime);
    if (cameraY_ <= 0) {
        cameraY_ = 0;
        stageComplete_ = true;
    }
}

void MidwayScene::spawnDueWaves() {
    while (nextWave_ < kWaveCount && cameraY_ <= kWaves[nextWave_].triggerCameraY) {
        const Wave& wave = kWaves[nextWave_];
        ++nextWave_;

        for (uint8_t i = 0; i < wave.count; ++i) {
            Enemy* enemy = enemies_.acquire();
            if (enemy == nullptr) {
                // Pool full: the previous wave is still alive. Dropping the
                // rest of this formation is the right failure — the alternative
                // is stalling the wave queue behind a player who is not
                // shooting, which reads as the stage having ended.
                break;
            }

            const int x = clampInt(wave.startCol * kTileSize + i * wave.stepXPx,
                                   0, kPlayfieldWidth - kEnemySize);
            enemy->x = x;
            enemy->homeX = x;
            // Stacked above the top of the viewport so the formation flies in
            // rather than appearing.
            enemy->y = cameraY_ - kEnemySize - i * wave.spacingPx;
            enemy->travelY = 0;
            enemy->weaves = wave.weaves;
            // Staggered phase, so a weaving formation snakes instead of sliding
            // sideways as one block.
            enemy->swayMs = static_cast<unsigned long>(i) * (kEnemySwayPeriodMs / 6u);
            enemy->fireTimer = 0;
            enemy->frameTimer = 0;
            enemy->frame = 0;
        }
    }
}

void MidwayScene::updateEnemies(unsigned long deltaTime) {
    const int viewportBottom = cameraY_ + kPlayfieldHeight;

    for (uint16_t i = enemies_.nextLive(0); i != EnemyPool::kEnd; i = enemies_.nextLive(i + 1)) {
        Enemy* enemy = enemies_.at(i);

        enemy->y += advancePixels(enemy->travelY, kEnemySpeedPxPerSec, deltaTime);

        if (enemy->weaves != 0u) {
            enemy->swayMs += deltaTime;
            enemy->x = clampInt(enemy->homeX + swayOffset(enemy->swayMs),
                                0, kPlayfieldWidth - kEnemySize);
        }

        enemy->frameTimer += deltaTime;
        while (enemy->frameTimer >= kPropFrameMs) {
            enemy->frameTimer -= kPropFrameMs;
            enemy->frame ^= 1u;
        }

        // Only shoot once actually on screen. An enemy still above the viewport
        // would otherwise fire from off-screen, which is unreadable and unfair.
        if (enemy->y > cameraY_) {
            enemy->fireTimer += deltaTime;
            if (enemy->fireTimer >= kEnemyFireIntervalMs) {
                enemy->fireTimer -= kEnemyFireIntervalMs;
                Bullet* shot = enemyBullets_.acquire();
                if (shot != nullptr) {
                    shot->x = enemy->x + (kEnemySize - kBulletSize) / 2;
                    shot->y = enemy->y + kEnemySize;
                    shot->speedPxPerSec = kEnemyBulletSpeedPxPerSec;
                    shot->travel = 0;
                }
            }
        }

        if (enemy->y > viewportBottom) {
            enemies_.releaseAt(i);
        }
    }
}

void MidwayScene::updateBullets(unsigned long deltaTime) {
    const int top    = cameraY_ - kBulletSize;
    const int bottom = cameraY_ + kPlayfieldHeight;

    for (uint16_t i = playerBullets_.nextLive(0); i != BulletPool::kEnd;
         i = playerBullets_.nextLive(i + 1)) {
        Bullet* bullet = playerBullets_.at(i);
        bullet->y += advancePixels(bullet->travel, bullet->speedPxPerSec, deltaTime);
        if (bullet->y < top || bullet->y > bottom) {
            playerBullets_.releaseAt(i);
        }
    }

    for (uint16_t i = enemyBullets_.nextLive(0); i != EnemyBulletPool::kEnd;
         i = enemyBullets_.nextLive(i + 1)) {
        Bullet* bullet = enemyBullets_.at(i);
        bullet->y += advancePixels(bullet->travel, bullet->speedPxPerSec, deltaTime);
        if (bullet->y < top || bullet->y > bottom) {
            enemyBullets_.releaseAt(i);
        }
    }
}

void MidwayScene::updateExplosions(unsigned long deltaTime) {
    for (uint16_t i = explosions_.nextLive(0); i != ExplosionPool::kEnd;
         i = explosions_.nextLive(i + 1)) {
        Explosion* blast = explosions_.at(i);
        blast->timer += deltaTime;
        while (blast->timer >= kExplosionFrameMs) {
            blast->timer -= kExplosionFrameMs;
            ++blast->frame;
        }
        if (blast->frame >= EXPLOSION_FRAME_COUNT) {
            explosions_.releaseAt(i);
        }
    }
}

void MidwayScene::spawnExplosion(int worldX, int worldY) {
    Explosion* blast = explosions_.acquire();
    if (blast == nullptr) {
        // Six at once already fills the screen. Dropping the seventh costs a
        // puff of smoke; growing the pool costs storage on every frame forever.
        return;
    }
    blast->x = worldX;
    blast->y = worldY;
    blast->timer = 0;
    blast->frame = 0;
}

void MidwayScene::killPlayer() {
    if (player_.isInvulnerable()) {
        return;
    }
    spawnExplosion(player_.screenX(), cameraY_ + player_.screenY());
    if (lives_ > 0) {
        --lives_;
    }
    player_.respawn();
}

void MidwayScene::resolveCollisions() {
    // Player fire against enemies. Both loops are over live slots only, so the
    // worst case here is kMaxPlayerBullets * kMaxEnemies = 80 integer AABB
    // tests, and the realistic case is a fraction of that.
    for (uint16_t b = playerBullets_.nextLive(0); b != BulletPool::kEnd;
         b = playerBullets_.nextLive(b + 1)) {
        const Bullet* bullet = playerBullets_.at(b);
        const Box bulletBox{ bullet->x, bullet->y, kBulletSize, kBulletSize };

        for (uint16_t e = enemies_.nextLive(0); e != EnemyPool::kEnd;
             e = enemies_.nextLive(e + 1)) {
            const Enemy* enemy = enemies_.at(e);
            const Box enemyBox{ enemy->x, enemy->y, kEnemySize, kEnemySize };
            if (!overlaps(bulletBox, enemyBox)) {
                continue;
            }
            spawnExplosion(enemy->x, enemy->y);
            enemies_.releaseAt(e);
            playerBullets_.releaseAt(b);
            score_ += kEnemyScore;
            // This bullet is gone; stop testing it against further enemies.
            break;
        }
    }

    if (player_.isInvulnerable()) {
        return;
    }
    const Box playerBox = player_.hitbox();

    for (uint16_t b = enemyBullets_.nextLive(0); b != EnemyBulletPool::kEnd;
         b = enemyBullets_.nextLive(b + 1)) {
        const Bullet* bullet = enemyBullets_.at(b);
        const Box bulletBox{ bullet->x, bullet->y, kBulletSize, kBulletSize };
        if (overlaps(bulletBox, playerBox)) {
            enemyBullets_.releaseAt(b);
            killPlayer();
            return;
        }
    }

    for (uint16_t e = enemies_.nextLive(0); e != EnemyPool::kEnd;
         e = enemies_.nextLive(e + 1)) {
        const Enemy* enemy = enemies_.at(e);
        const Box enemyBox{ enemy->x, enemy->y, kEnemySize, kEnemySize };
        if (overlaps(enemyBox, playerBox)) {
            spawnExplosion(enemy->x, enemy->y);
            enemies_.releaseAt(e);
            killPlayer();
            return;
        }
    }
}

void MidwayScene::update(unsigned long deltaTime) {
    if (isTerminal()) {
        // Everything holds: the stage stops scrolling, nothing spawns, and the
        // last frame stays on screen under the HUD message. The only input
        // still read is restart.
        //
        // isButtonPressed is the press EDGE, not the held state, which is what
        // makes this safe without any extra latch: a player who happened to be
        // holding B when the run ended produces no edge until they release and
        // press again, so the game cannot restart itself out from under the
        // message it just showed.
        if (engine.getInputManager().isButtonPressed(BTN_B)) {
            // init() is idempotent by the Scene contract - it runs
            // resetState() first, which clears the entity list before the
            // player is added back. Calling it here is safe because nothing is
            // iterating that list at this point: Scene::update() has not been
            // reached this frame, and we return immediately afterwards.
            init();
        }
        return;
    }

    updateScroll(deltaTime);

    // The player stores a screen position and needs to know where the playfield
    // currently sits before anything reads its world coordinates. This must
    // come before Scene::update, which is what runs player_.update().
    player_.setViewportTop(cameraY_);
    Scene::update(deltaTime);

    if (player_.consumeShot()) {
        Bullet* shot = playerBullets_.acquire();
        if (shot != nullptr) {
            player_.muzzleWorldPosition(shot->x, shot->y);
            shot->speedPxPerSec = -kPlayerBulletSpeedPxPerSec;
            shot->travel = 0;
        }
    }

    spawnDueWaves();
    updateEnemies(deltaTime);
    updateBullets(deltaTime);
    updateExplosions(deltaTime);
    resolveCollisions();

    camera_.setPosition(math::Vector2(math::toScalar(0), math::toScalar(cameraY_)));
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void MidwayScene::adviseFramebufferBeforeBeginFrame(gfx::Renderer& renderer) {
#if PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE
    // Tells beginFrame it can skip clearing when the next draw() is about to
    // memcpy a snapshot over the whole framebuffer.
    //
    // Here it almost never can, and that is the point: the camera sample
    // changes every frame this stage is scrolling, so the cache misses and the
    // clear is paid. The call stays in because removing it would also remove
    // the one frame where it does help — the stage-complete hold, when the
    // camera stops.
    const gfx::TileMap4bppDrawSpec staticLayers[] = { seaLayer() };
    tilemapLayerCache_.adviseFramebufferBeforeBeginFrame(
        renderer, 0, cameraY_, staticLayers, 1, nullptr, 0);
#else
    (void)renderer;
#endif
}

void MidwayScene::draw(gfx::Renderer& renderer) {
    // Everything below this line is world space until drawHud.
    camera_.apply(renderer);

    // The camera sample handed to the cache is read from cameraY_, not from
    // renderer.getYOffset(). The offset is only set by camera_.apply() inside
    // draw(), so adviseFramebufferBeforeBeginFrame — which the engine runs
    // before beginFrame — would see the previous frame's value and disagree
    // with this call on the exact frame the camera moves. Both read cameraY_
    // and always agree.
    const gfx::TileMap4bppDrawSpec staticLayers[] = { seaLayer() };
    tilemapLayerCache_.draw(renderer, 0, cameraY_, staticLayers, 1, nullptr, 0);

    for (uint16_t i = enemies_.nextLive(0); i != EnemyPool::kEnd; i = enemies_.nextLive(i + 1)) {
        const Enemy* enemy = enemies_.at(i);
        renderer.drawSprite(ENEMY_FRAMES[enemy->frame], enemy->x, enemy->y);
    }

    for (uint16_t i = playerBullets_.nextLive(0); i != BulletPool::kEnd;
         i = playerBullets_.nextLive(i + 1)) {
        const Bullet* bullet = playerBullets_.at(i);
        renderer.drawSprite(BULLET_FRAMES[BULLET_PLAYER], bullet->x, bullet->y);
    }

    for (uint16_t i = enemyBullets_.nextLive(0); i != EnemyBulletPool::kEnd;
         i = enemyBullets_.nextLive(i + 1)) {
        const Bullet* bullet = enemyBullets_.at(i);
        renderer.drawSprite(BULLET_FRAMES[BULLET_ENEMY], bullet->x, bullet->y);
    }

    // Entities — the player. Drawn after the enemies so a pass-over reads as
    // the player being nearer the camera, which is what a top-down shooter
    // implies by having the player shoot past them.
    Scene::draw(renderer);

    // Last, so a fireball covers whatever it consumed.
    for (uint16_t i = explosions_.nextLive(0); i != ExplosionPool::kEnd;
         i = explosions_.nextLive(i + 1)) {
        const Explosion* blast = explosions_.at(i);
        renderer.drawSprite(EXPLOSION_FRAMES[blast->frame], blast->x, blast->y);
    }

    drawHud(renderer);
}

void MidwayScene::drawHud(gfx::Renderer& renderer) {
    // Screen space, not world space — the HUD does not scroll with the camera.
    const bool oldBypass = renderer.isOffsetBypassEnabled();
    renderer.setOffsetBypass(true);

    // Opaque, and drawn every frame: the camera viewport is 208 px tall but the
    // panel is 240, so world rows below the playfield would otherwise bleed
    // into the strip the HUD occupies.
    renderer.drawFilledRectangle(0, kHudY, kDisplayWidth, kHudHeight, gfx::Color::Black);

    char line[24];
    std::snprintf(line, sizeof(line), "SCORE %06d", score_);
    renderer.drawText(line, 4, kHudY + 6, gfx::Color::White, 1);

    std::snprintf(line, sizeof(line), "PLANES %d", lives_);
    renderer.drawText(line, 4, kHudY + 18, gfx::Color::White, 1);

    if (lives_ <= 0) {
        renderer.drawTextCentered("GAME OVER", kPlayfieldHeight / 2 - 12, gfx::Color::Red, 2);
    } else if (stageComplete_) {
        renderer.drawTextCentered("STAGE CLEAR", kPlayfieldHeight / 2 - 12, gfx::Color::White, 2);
    }
    if (isTerminal()) {
        // Named for the button, not the key, because the two platforms bind it
        // differently: RETURN on the SDL2 build, GPIO 12 on the ESP32 one.
        renderer.drawTextCentered("PRESS B", kPlayfieldHeight / 2 + 10, gfx::Color::White, 1);
    }

    renderer.setOffsetBypass(oldBypass);
}

} // namespace midway_clone

#endif // PIXELROOT32_ENABLE_4BPP_SPRITES
