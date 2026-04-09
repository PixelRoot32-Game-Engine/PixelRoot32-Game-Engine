#include "SpaceInvadersScene.h"
#include "actors/PlayerActor.h"
#include "actors/AlienActor.h"
#include "actors/ProjectileActor.h"
#include "actors/BunkerActor.h"
#include "GameConstants.h"
#include "assets/Background.h"
#include "assets/PlayerExplosionSprites.h"

#include <platforms/EngineConfig.h>
#include <core/Engine.h>
#include <audio/AudioTypes.h>
#include <audio/AudioMusicTypes.h>
#include <cstdlib>
#include <cstdio>

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace spaceinvaders {

namespace core = pr32::core;
namespace gfx = pr32::graphics;
namespace physics = pr32::physics;
namespace audio = pr32::audio;
namespace math = pr32::math;

#ifdef PIXELROOT32_ENABLE_SCENE_ARENA

/** Arena for all scene entities (background, player, aliens, bunkers, projectiles).
 *  Before the tile-attributes changes, PhysicsActor was smaller (no userData, sensor, oneWay, etc.),
 *  so 2048 bytes were enough. With the current engine, each actor is larger and ~50 entities need
 *  more space; 4096 is safe. If you need 2048 again (e.g. tight RAM), build this target without
 *  PIXELROOT32_ENABLE_SCENE_ARENA so the scene uses heap instead of this fixed buffer. */
static unsigned char SPACE_INVADERS_SCENE_ARENA_BUFFER[4096];
#endif


class StarfieldBackground : public pr32::core::Entity {
public:
    StarfieldBackground()
        : pr32::core::Entity(math::Vector2::ZERO(), DISPLAY_WIDTH, DISPLAY_HEIGHT, core::EntityType::GENERIC) {
        setRenderLayer(0);
    }

    void update(unsigned long) override {
    }

    void draw(gfx::Renderer& renderer) override {
        renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, gfx::Color::Black);
        for (int i = 0; i < background_assets::STAR_COUNT; ++i) {
            renderer.drawPixel(static_cast<int>(background_assets::STAR_X[i]),
                               static_cast<int>(background_assets::STAR_Y[i]),
                               gfx::Color::White);
        }
    }
    activeAlienCount = ALIEN_ROWS * ALIEN_COLS;
}

void ExplosionAnimation::start(math::Vector2 pos) {
    position = pos;
    timeAccumulator = 0;
    stepsDone = 0;
    animation.reset();
    active = true;
}

void ExplosionAnimation::update(unsigned long deltaTime) {
    if (!active) {
        return;
    }

    const unsigned long frameTimeMs = 60;
    timeAccumulator += deltaTime;

    while (timeAccumulator >= frameTimeMs && active) {
        timeAccumulator -= frameTimeMs;

        if (stepsDone + 1 < animation.frameCount) {
            animation.step();
            stepsDone++;
        } else {
            active = false;
        }
    }
}

void ExplosionAnimation::draw(gfx::Renderer& renderer) {
    if (!active) {
        return;
    }

    const gfx::Sprite* sprite = animation.getCurrentSprite();
    if (!sprite) {
        return;
    }

    const int drawX = static_cast<int>(position.x);
    const int drawY = static_cast<int>(position.y);

    renderer.drawSprite(*sprite, drawX, drawY, gfx::Color::White);
}

bool ExplosionAnimation::isActive() const {
    return active;
}

SpaceInvadersScene::SpaceInvadersScene()
    : background(nullptr),
      player(nullptr),
      score(0),
      lives(3),
      gameOver(false),
      gameWon(false),
      activeAlienCount(0),
      stepTimer(0.0f),
      stepDelay(BASE_STEP_DELAY),
      moveDirection(1),
      isPaused(false),
      fireInputReady(false),
      lastFireTime(0),
      activePlayerBulletCount(0),
      currentMusicTempoFactor(1.0f) {

#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
    background = nullptr;
#else
    background = std::make_unique<StarfieldBackground>();
    addEntity(background.get());
#endif

    for (int i = 0; i < MaxEnemyExplosions; ++i) {
        enemyExplosions[i].active = false;
        enemyExplosions[i].position = math::Vector2(0, 0);
        enemyExplosions[i].remainingMs = 0;
    }
}

SpaceInvadersScene::~SpaceInvadersScene() {
    cleanup();
#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
    background = nullptr;
#endif
}

void SpaceInvadersScene::init() {
#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
    arena.init(SPACE_INVADERS_SCENE_ARENA_BUFFER, sizeof(SPACE_INVADERS_SCENE_ARENA_BUFFER));
#endif
    gfx::setSpritePalette(gfx::PaletteType::NES);
    background_assets::init();
    resetGame();

    engine.getMusicPlayer().play(BGM_SLOW_TRACK);
    currentMusicTempoFactor = 1.0f;
    engine.getMusicPlayer().setTempoFactor(currentMusicTempoFactor);
}

void SpaceInvadersScene::cleanup() {
    clearEntities();

#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
    player = nullptr;
    aliens.clear();
    projectiles.clear();
    bunkers.clear();
#else
    player.reset();
    aliens.clear();
    projectiles.clear();
    bunkers.clear();
#endif
}

void SpaceInvadersScene::resetGame() {
    cleanup();

#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
    arena.reset();
#endif

#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
    background = core::arenaNew<StarfieldBackground>(arena);
    addEntity(background);

    player = core::arenaNew<PlayerActor>(arena, math::Vector2(PLAYER_START_X, PLAYER_START_Y));
    addEntity(player);
#else
    if (background) {
        addEntity(background.get());
    }
    player = std::make_unique<PlayerActor>(math::Vector2(PLAYER_START_X, PLAYER_START_Y));
    addEntity(player.get());
#endif

    spawnAliens();
    spawnBunkers();

    projectiles.reserve(MaxProjectiles);
    for (int i = 0; i < MaxProjectiles; ++i) {
#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
        ProjectileActor* projectile = core::arenaNew<ProjectileActor>(arena, pr32::math::Vector2(0, -PROJECTILE_HEIGHT), ProjectileType::PLAYER_BULLET);
        if (!projectile) {
            continue;
        }
        projectile->deactivate();
        projectiles.push_back(projectile);
        addEntity(projectile);
#else
        auto projectile = std::make_unique<ProjectileActor>(math::Vector2(0, -PROJECTILE_HEIGHT), ProjectileType::PLAYER_BULLET);
        projectile->deactivate();
        addEntity(projectile.get());
        projectiles.push_back(std::move(projectile));
#endif
    }

    score = 0;
    lives = 3;
    gameOver = false;
    gameWon = false;
    isPaused = false;
    fireInputReady = false;
    activePlayerBulletCount = 0;

    for (int i = 0; i < MaxEnemyExplosions; ++i) {
        enemyExplosions[i].active = false;
        enemyExplosions[i].remainingMs = 0;
    }
    
    stepTimer = 0.0f;
    moveDirection = 1;
    stepDelay = BASE_STEP_DELAY;
}

void SpaceInvadersScene::spawnAliens() {
    for (int row = 0; row < ALIEN_ROWS; ++row) {
        AlienType type;
        if (row == 0) type = AlienType::SQUID;
        else if (row < 3) type = AlienType::CRAB;
        else type = AlienType::OCTOPUS;

        for (int col = 0; col < ALIEN_COLS; ++col) {
            float x = ALIEN_START_X + (col * ALIEN_SPACING_X);
            float y = ALIEN_START_Y + (row * ALIEN_SPACING_Y);
            
#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
            AlienActor* alien = core::arenaNew<AlienActor>(arena, pr32::math::Vector2(x, y), type);
            if (!alien) {
                continue;
            }
            aliens.push_back(alien);
            addEntity(alien);
#else
            auto alien = std::make_unique<AlienActor>(pr32::math::Vector2(x, y), type);
            addEntity(alien.get());
            aliens.push_back(std::move(alien));
#endif
        }
    }
    activeAlienCount = ALIEN_ROWS * ALIEN_COLS;

    for (int col = 0; col < ALIEN_COLS; ++col) {
        lowestAlienInColumn[col] = -1;
    }
    for (int i = 0; i < static_cast<int>(aliens.size()); ++i) {
        if (!aliens[i]->isActive()) continue;
        int col = static_cast<int>((aliens[i]->position.x - ALIEN_START_X) / ALIEN_SPACING_X + 0.5f);
        if (col >= 0 && col < ALIEN_COLS) {
            lowestAlienInColumn[col] = i;
        }
    }
}

void SpaceInvadersScene::spawnBunkers() {
    if (BUNKER_COUNT <= 0) {
        return;
    }

    float totalBunkersWidth = BUNKER_COUNT * BUNKER_WIDTH;
    float gap = (DISPLAY_WIDTH - totalBunkersWidth) / (BUNKER_COUNT + 1);

    for (int i = 0; i < BUNKER_COUNT; ++i) {
        float x = gap + i * (BUNKER_WIDTH + gap);
        float y = BUNKER_Y - BUNKER_HEIGHT;
#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
        BunkerActor* bunker = core::arenaNew<BunkerActor>(arena, math::Vector2(x, y), BUNKER_WIDTH, BUNKER_HEIGHT, 4);
        if (!bunker) {
            continue;
        }
        bunkers.push_back(bunker);
        addEntity(bunker);
#else
        auto bunker = std::make_unique<BunkerActor>(math::Vector2(x, y), BUNKER_WIDTH, BUNKER_HEIGHT, 4);
        addEntity(bunker.get());
        bunkers.push_back(std::move(bunker));
#endif
    }
}

void SpaceInvadersScene::update(unsigned long deltaTime) {
    if (gameOver) {
        if (engine.getInputManager().isButtonPressed(BTN_FIRE)) {
            resetGame();
            engine.getMusicPlayer().play(BGM_SLOW_TRACK);
            currentMusicTempoFactor = 1.0f;
            engine.getMusicPlayer().setTempoFactor(currentMusicTempoFactor);
        }
        return;
    }

    if (isPaused) {
        playerExplosion.update(deltaTime);
        updateEnemyExplosions(deltaTime);

        if (!playerExplosion.isActive()) {
            respawnPlayerUnderBunker();
            isPaused = false;
        }
        return;
    }

    Scene::update(deltaTime);

    activePlayerBulletCount = 0;
    for (const auto& proj : projectiles) {
        if (proj->isActive() && proj->getType() == ProjectileType::PLAYER_BULLET) {
            activePlayerBulletCount++;
        }
    }

    if (player) {
        if (!fireInputReady) {
            if (!player->isFireDown()) {
                fireInputReady = true;
            }
        }

        if (fireInputReady && player->wantsToShoot()) {
            // Use cached active bullet count instead of O(n) scan
            unsigned long now = engine.getMillis();
            if (activePlayerBulletCount < MAX_PLAYER_BULLETS && 
                (now - lastFireTime) >= PLAYER_FIRE_COOLDOWN) {
                math::Scalar px = player->position.x + math::toScalar(PLAYER_WIDTH - PROJECTILE_WIDTH) * math::toScalar(0.5f);
                math::Scalar py = player->position.y - math::toScalar(PROJECTILE_HEIGHT);

                for (auto& proj : projectiles) {
                    if (!proj->isActive()) {
                        proj->reset(math::Vector2(px, py), ProjectileType::PLAYER_BULLET);

                        audio::AudioEvent event{};
                        event.type = audio::WaveType::PULSE;
                        event.frequency = 880.0f;
                        event.duration = 0.08f;
                        event.volume = 0.4f;
                        event.duty = 0.5f;
                        engine.getAudioEngine().playEvent(event);

                        lastFireTime = now;
                        break;
                    }
                }
            }
        }
    }

    updateAliens(deltaTime);

    // Rebuild shooter lookup after alien movement
    for (int col = 0; col < ALIEN_COLS; ++col) {
        lowestAlienInColumn[col] = -1;
    }
    for (int i = 0; i < static_cast<int>(aliens.size()); ++i) {
        if (!aliens[i]->isActive()) continue;
        int col = static_cast<int>((aliens[i]->position.x - ALIEN_START_X) / ALIEN_SPACING_X + 0.5f);
        if (col >= 0 && col < ALIEN_COLS) {
            if (lowestAlienInColumn[col] == -1 ||
                aliens[i]->position.y > aliens[lowestAlienInColumn[col]]->position.y) {
                lowestAlienInColumn[col] = i;
            }
        }
    }

    handleCollisions();
    updateEnemyExplosions(deltaTime);

    updateMusicTempo();
}

void SpaceInvadersScene::updateAliens(unsigned long deltaTime) {
    float scaledDelta = static_cast<float>(deltaTime) * currentMusicTempoFactor;
    stepTimer += scaledDelta;
    
    if (stepTimer < stepDelay) {
        return;
    }
    
    stepTimer = 0.0f;
    
    bool edgeHit = false;
    bool alienReachedPlayer = false;
    
    // Single pass: check edges and game over condition
    for (const auto& alien : aliens) {
        if (!alien->isActive()) continue;
        
        // Edge detection
        if (moveDirection == 1) {
            if (alien->position.x + alien->width >= DISPLAY_WIDTH - 2) {
                edgeHit = true;
            }
        } else {
            if (alien->position.x <= 2) {
                edgeHit = true;
            }
        }
        
        // Game over check
        if (player) {
            math::Scalar alienBottom = alien->position.y + math::toScalar(alien->height);
            if (alienBottom >= player->position.y) {
                alienReachedPlayer = true;
            }
        }
    }
    
    // Handle direction change and drop
    if (edgeHit) {
        moveDirection *= -1;
    }
    
    math::Scalar dx = math::toScalar(moveDirection) * math::toScalar(ALIEN_STEP_AMOUNT_X);
    math::Scalar dy = edgeHit ? math::toScalar(ALIEN_DROP_AMOUNT) : math::toScalar(0);
    
    // Move all aliens in single pass
    for (const auto& alien : aliens) {
        if (alien->isActive()) {
            alien->move(dx, dy);
        }
    }
    
    // Handle game over
    if (alienReachedPlayer) {
        lives = 0;
        gameOver = true;
        engine.getMusicPlayer().setTempoFactor(1.0f);
        engine.getMusicPlayer().play(GAME_OVER_TRACK);
        return;
    }
    
    enemyShoot();
}

void SpaceInvadersScene::handleCollisions() {
    using physics::Circle;
    using physics::sweepCircleVsRect;

    for (auto& proj : projectiles) {
        if (!proj->isActive()) {
            continue;
        }
        if (proj->getType() == ProjectileType::PLAYER_BULLET) {
            math::Scalar radius = math::toScalar(PROJECTILE_WIDTH * 0.5f);
            math::Scalar halfHeight = math::toScalar(PROJECTILE_HEIGHT * 0.5f);

            Circle startCircle;
            startCircle.x = proj->getPreviousX() + radius;
            startCircle.y = proj->getPreviousY() + halfHeight;
            startCircle.radius = radius;

            Circle endCircle;
            endCircle.x = proj->position.x + radius;
            endCircle.y = proj->position.y + halfHeight;
            endCircle.radius = radius;

            bool hitResolved = false;

            for (auto& alien : aliens) {
                if (!alien->isActive()) {
                    continue;
                }
                math::Scalar tHit = math::toScalar(0);
                core::Rect targetBox = alien->getHitBox();
                if (sweepCircleVsRect(startCircle, endCircle, targetBox, tHit) ||
                    proj->getHitBox().intersects(targetBox)) {
                    proj->deactivate();
                    alien->kill();
                    score += alien->getScoreValue();

                    math::Scalar ex = alien->position.x + math::toScalar(alien->width) * math::toScalar(0.5f);
                    math::Scalar ey = alien->position.y + math::toScalar(alien->height) * math::toScalar(0.5f);
                    spawnEnemyExplosion(ex, ey);

                    audio::AudioEvent event{};
                    event.type = audio::WaveType::NOISE;
                    event.frequency = 600.0f;
                    event.duration = 0.12f;
                    event.volume = 0.6f;
                    event.duty = 0.5f;
                    engine.getAudioEngine().playEvent(event);

                    activeAlienCount--;
                    if (activeAlienCount == 0) {
                        gameOver = true;
                        gameWon = true;
                        engine.getMusicPlayer().setTempoFactor(1.0f);
                        engine.getMusicPlayer().play(WIN_TRACK);
                    }

                    hitResolved = true;
                    break;
                }
            }
            if (!hitResolved && proj->isActive()) {
                for (auto& bunker : bunkers) {
                    if (bunker->isDestroyed()) {
                        continue;
                    }
                    math::Scalar tHit = math::toScalar(0);
                    core::Rect bunkerBox = bunker->getHitBox();
                    if (sweepCircleVsRect(startCircle, endCircle, bunkerBox, tHit) ||
                        proj->getHitBox().intersects(bunkerBox)) {
                        proj->deactivate();
                        bunker->applyDamage(1);
                        break;
                    }
                }
            }
        }
    }

    if (!player) {
        return;
    }

    core::Rect playerBox = player->getHitBox();
    for (auto& proj : projectiles) {
        if (!proj->isActive()) {
            continue;
        }
        if (proj->getType() == ProjectileType::ENEMY_BULLET) {
            math::Scalar radius = math::toScalar(PROJECTILE_WIDTH * 0.5f);
            math::Scalar halfHeight = math::toScalar(PROJECTILE_HEIGHT * 0.5f);

            using physics::Circle;
            Circle startCircle;
            startCircle.x = proj->getPreviousX() + radius;
            startCircle.y = proj->getPreviousY() + halfHeight;
            startCircle.radius = radius;

            Circle endCircle;
            endCircle.x = proj->position.x + radius;
            endCircle.y = proj->position.y + halfHeight;
            endCircle.radius = radius;

            core::Rect eBox = proj->getHitBox();
            bool handled = false;
            for (auto& bunker : bunkers) {
                if (bunker->isDestroyed()) {
                    continue;
                }
                core::Rect bunkerBox = bunker->getHitBox();
                math::Scalar tHit = math::toScalar(0);
                if (sweepCircleVsRect(startCircle, endCircle, bunkerBox, tHit) ||
                    eBox.intersects(bunkerBox)) {
                    proj->deactivate();
                    bunker->applyDamage(1);
                    handled = true;
                    break;
                }
            }
            if (handled) {
                continue;
            }
            math::Scalar tHitPlayer = math::toScalar(0);
            if (sweepCircleVsRect(startCircle, endCircle, playerBox, tHitPlayer) ||
                eBox.intersects(playerBox)) {
                proj->deactivate();
                handlePlayerHit();
                break;
            }
        }
    }
}

void SpaceInvadersScene::enemyShoot() {
    AlienActor* potentialShooters[ALIEN_COLS];
    int shooterCount = 0;

    for (int col = 0; col < ALIEN_COLS; ++col) {
        int alienIdx = lowestAlienInColumn[col];
        if (alienIdx >= 0 && alienIdx < static_cast<int>(aliens.size())) {
            potentialShooters[shooterCount++] = &*aliens[alienIdx];
        }
    }
    
    if (shooterCount == 0) return;

    int idx = std::rand() % shooterCount;
    AlienActor* shooter = potentialShooters[idx];

    for (auto& proj : projectiles) {
        if (!proj->isActive()) {
            math::Scalar sx = shooter->position.x + math::toScalar(shooter->width) * math::toScalar(0.5f);
            math::Scalar sy = shooter->position.y + math::toScalar(shooter->height);
            
            proj->reset(math::Vector2(sx, sy), ProjectileType::ENEMY_BULLET);
            break;
        }
    }
}

int SpaceInvadersScene::getActiveAlienCount() const {
    int count = 0;
    for (const auto& alien : aliens) {
        if (alien->isActive()) count++;
    }
    return count;
}

void SpaceInvadersScene::draw(gfx::Renderer& renderer) {
    renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, gfx::Color::Black);
    Scene::draw(renderer);
    drawEnemyExplosions(renderer);
    playerExplosion.draw(renderer);

    char buffer[32];

    std::snprintf(buffer, sizeof(buffer), "SCORE %04d", score);
    renderer.drawText(buffer, 4, 4, gfx::Color::White, 1);

    std::snprintf(buffer, sizeof(buffer), "LIVES %d", lives);
    renderer.drawText(buffer, DISPLAY_WIDTH - 70, 4, gfx::Color::White, 1);

    if (gameOver) {
        if (gameWon) {
            std::snprintf(buffer, sizeof(buffer), "YOU WIN!");
            int textY = DISPLAY_HEIGHT / 2 - 8;
            renderer.drawTextCentered(buffer, textY, gfx::Color::Green, 2);
        } else {
            std::snprintf(buffer, sizeof(buffer), "GAME OVER");
            int textY = DISPLAY_HEIGHT / 2 - 8;
            renderer.drawTextCentered(buffer, textY, gfx::Color::Red, 2);
        }

        std::snprintf(buffer, sizeof(buffer), "PRESS FIRE");
        int textY = DISPLAY_HEIGHT / 2 - 8;
        renderer.drawTextCentered(buffer, textY + 20, gfx::Color::White, 1);
    }
}

void SpaceInvadersScene::updateMusicTempo() {
    if (gameOver) return;

    float lowestY = ALIEN_START_Y;
    bool found = false;

    for (const auto& alien : aliens) {
        if (alien->isActive()) {
            float y = static_cast<float>(alien->position.y + math::toScalar(alien->height));
            if (y > lowestY) {
                lowestY = y;
                found = true;
            }
        }
    }

    if (!found) return;

    float threatFactor = (lowestY - ALIEN_START_Y) * INV_Y_RANGE;
    if (threatFactor < 0.0f) threatFactor = 0.0f;
    if (threatFactor > 1.0f) threatFactor = 1.0f;

    float targetTempo = 1.0f + (threatFactor * 0.9f);
    currentMusicTempoFactor += (targetTempo - currentMusicTempoFactor) * 0.05f;
    engine.getMusicPlayer().setTempoFactor(currentMusicTempoFactor);
}

void SpaceInvadersScene::updateEnemyExplosions(unsigned long deltaTime) {
    for (int i = 0; i < MaxEnemyExplosions; ++i) {
        EnemyExplosion& e = enemyExplosions[i];
        if (!e.active) {
            continue;
        }

        if (deltaTime >= e.remainingMs) {
            e.active = false;
            e.remainingMs = 0;
        } else {
            e.remainingMs -= deltaTime;
        }
    }
}

void SpaceInvadersScene::drawEnemyExplosions(gfx::Renderer& renderer) {
    for (int i = 0; i < MaxEnemyExplosions; ++i) {
        const EnemyExplosion& e = enemyExplosions[i];
        if (!e.active) {
            continue;
        }

        int cx = static_cast<int>(e.position.x);
        int cy = static_cast<int>(e.position.y);

        int hx = cx - 2;
        int hw = 5;
        int hy = cy;

        if (hx < 0) {
            hw += hx;
            hx = 0;
        }
        if (hx < DISPLAY_WIDTH && hw > 0) {
            if (hx + hw > DISPLAY_WIDTH) {
                hw = DISPLAY_WIDTH - hx;
            }
            if (hw > 0 && hy >= 0 && hy < DISPLAY_HEIGHT) {
                renderer.drawFilledRectangle(hx, hy, hw, 1, gfx::Color::White);
            }
        }

        int vx = cx;
        int vy = cy - 2;
        int vh = 5;

        if (vy < 0) {
            vh += vy;
            vy = 0;
        }
        if (vy < DISPLAY_HEIGHT && vh > 0) {
            if (vy + vh > DISPLAY_HEIGHT) {
                vh = DISPLAY_HEIGHT - vy;
            }
            if (vh > 0 && vx >= 0 && vx < DISPLAY_WIDTH) {
                renderer.drawFilledRectangle(vx, vy, 1, vh, gfx::Color::White);
            }
        }
    }
}

void SpaceInvadersScene::spawnEnemyExplosion(math::Scalar x, math::Scalar y) {
    for (int i = 0; i < MaxEnemyExplosions; ++i) {
        EnemyExplosion& e = enemyExplosions[i];
        if (!e.active) {
            e.active = true;
            e.position.x = x;
            e.position.y = y;
            e.remainingMs = 200;
            break;
        }
    }
}

void SpaceInvadersScene::handlePlayerHit() {
    if (lives > 0) {
        lives -= 1;
    }

    audio::AudioEvent event{};
    event.type = audio::WaveType::NOISE;
    event.frequency = 400.0f;
    event.duration = 0.18f;
    event.volume = 0.7f;
    event.duty = 0.5f;
    engine.getAudioEngine().playEvent(event);

    if (lives <= 0) {
        gameOver = true;
        engine.getMusicPlayer().setTempoFactor(1.0f);
        engine.getMusicPlayer().play(GAME_OVER_TRACK);
        return;
    }

    if (!player) {
        return;
    }

    playerExplosion.start(player->position);
    player->setVisible(false);
    isPaused = true;
}

void SpaceInvadersScene::respawnPlayerUnderBunker() {
    if (!player) {
        return;
    }

    BunkerActor* targetBunker = nullptr;
    for (const auto& bunker : bunkers) {
        if (!bunker->isDestroyed()) {
            targetBunker = &*bunker;
            break;
        }
    }

    math::Scalar newX = math::toScalar(PLAYER_START_X);

    if (targetBunker) {
        newX = targetBunker->position.x + (math::toScalar(targetBunker->width - PLAYER_WIDTH) * math::toScalar(0.5f));
    }

    player->position.x = newX;
    player->setVisible(true);
}

}
