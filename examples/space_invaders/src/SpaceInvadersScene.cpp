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

using pixelroot32::graphics::Sprite;
using pixelroot32::graphics::SpriteAnimationFrame;
using pixelroot32::audio::AudioEvent;
using pixelroot32::audio::WaveType;
using pixelroot32::audio::MusicNote;
using pixelroot32::audio::MusicTrack;
using pixelroot32::audio::InstrumentPreset;
using pixelroot32::audio::INSTR_PULSE_BASS;
using pixelroot32::audio::Note;
using pixelroot32::math::Scalar;
using pixelroot32::math::Vector2;

#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
using pixelroot32::core::SceneArena;
using pixelroot32::core::arenaNew;

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
        : pr32::core::Entity(Vector2::ZERO(), DISPLAY_WIDTH, DISPLAY_HEIGHT, pr32::core::EntityType::GENERIC) {
        setRenderLayer(0);
    }

    void update(unsigned long) override {
    }

    void draw(pr32::graphics::Renderer& renderer) override {
        renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, pr32::graphics::Color::Black);
        for (int i = 0; i < background_assets::STAR_COUNT; ++i) {
            renderer.drawPixel(static_cast<int>(background_assets::STAR_X[i]),
                               static_cast<int>(background_assets::STAR_Y[i]),
                               pr32::graphics::Color::White);
        }
    }
};

// Base four-note bass pattern: "tu tu tu tu"
static const InstrumentPreset BASS_INSTRUMENT = INSTR_PULSE_BASS;

static const MusicNote BGM_SLOW_NOTES[] = {
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.21f),
    pixelroot32::audio::makeRest(0.207f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.21f),
    pixelroot32::audio::makeRest(0.207f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.21f),
    pixelroot32::audio::makeRest(0.207f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.21f),
    pixelroot32::audio::makeRest(0.207f),
};

static const MusicNote BGM_MEDIUM_NOTES[] = {
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.12f),
    pixelroot32::audio::makeRest(0.06f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.12f),
    pixelroot32::audio::makeRest(0.06f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.12f),
    pixelroot32::audio::makeRest(0.06f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.12f),
    pixelroot32::audio::makeRest(0.06f),
};

static const MusicNote BGM_FAST_NOTES[] = {
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.08f),
    pixelroot32::audio::makeRest(0.04f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.08f),
    pixelroot32::audio::makeRest(0.04f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.08f),
    pixelroot32::audio::makeRest(0.04f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.08f),
    pixelroot32::audio::makeRest(0.04f),
};

static const MusicTrack BGM_SLOW_TRACK = {
    BGM_SLOW_NOTES,
    sizeof(BGM_SLOW_NOTES) / sizeof(MusicNote),
    true,
    WaveType::PULSE,
    BASS_INSTRUMENT.duty
};

static const MusicTrack BGM_MEDIUM_TRACK = {
    BGM_MEDIUM_NOTES,
    sizeof(BGM_MEDIUM_NOTES) / sizeof(MusicNote),
    true,
    WaveType::PULSE,
    BASS_INSTRUMENT.duty
};

static const MusicTrack BGM_FAST_TRACK = {
    BGM_FAST_NOTES,
    sizeof(BGM_FAST_NOTES) / sizeof(MusicNote),
    true,
    WaveType::PULSE,
    BASS_INSTRUMENT.duty
};

// --- WIN / GAME OVER MUSIC ---

static const MusicNote WIN_NOTES[] = {
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.15f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::E, 0.15f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::G, 0.15f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.4f), // C High ideally, but using C for safety if C_High undefined
    pixelroot32::audio::makeRest(0.1f)
};

static const MusicTrack WIN_TRACK = {
    WIN_NOTES,
    sizeof(WIN_NOTES) / sizeof(MusicNote),
    false, // No loop
    WaveType::PULSE,
    BASS_INSTRUMENT.duty
};

static const MusicNote GAME_OVER_NOTES[] = {
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::G, 0.2f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::E, 0.2f), // Using E instead of Eb if Eb undefined, checking later
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, Note::C, 0.4f),
    pixelroot32::audio::makeRest(0.1f)
};

static const MusicTrack GAME_OVER_TRACK = {
    GAME_OVER_NOTES,
    sizeof(GAME_OVER_NOTES) / sizeof(MusicNote),
    false, // No loop
    WaveType::PULSE,
    BASS_INSTRUMENT.duty
};


ExplosionAnimation::ExplosionAnimation()
    : active(false), position(pr32::math::toScalar(0), pr32::math::toScalar(0)), timeAccumulator(0), stepsDone(0) {
    animation.frames = PLAYER_EXPLOSION_FRAMES;
    animation.frameCount = static_cast<uint8_t>(sizeof(PLAYER_EXPLOSION_FRAMES) / sizeof(SpriteAnimationFrame));
    animation.current = 0;
}

void ExplosionAnimation::start(pr32::math::Vector2 pos) {
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

void ExplosionAnimation::draw(pr32::graphics::Renderer& renderer) {
    if (!active) {
        return;
    }

    const Sprite* sprite = animation.getCurrentSprite();
    if (!sprite) {
        return;
    }

    const int drawX = static_cast<int>(position.x);
    const int drawY = static_cast<int>(position.y);

    renderer.drawSprite(*sprite, drawX, drawY, pr32::graphics::Color::White);
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
      stepTimer(0.0f),
      stepDelay(BASE_STEP_DELAY),
      moveDirection(1),
      isPaused(false),
      fireInputReady(false),
      lastFireTime(0),
      currentMusicTempoFactor(1.0f) {

#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
    background = nullptr;
#else
    background = std::make_unique<StarfieldBackground>();
    addEntity(background.get());
#endif

    for (int i = 0; i < MaxEnemyExplosions; ++i) {
        enemyExplosions[i].active = false;
        enemyExplosions[i].position = pr32::math::Vector2(0, 0);
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
    pr32::graphics::setSpritePalette(pr32::graphics::PaletteType::NES);
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
    background = arenaNew<StarfieldBackground>(arena);
    addEntity(background);

    player = arenaNew<PlayerActor>(arena, pr32::math::Vector2(PLAYER_START_X, PLAYER_START_Y));
    addEntity(player);
#else
    if (background) {
        addEntity(background.get());
    }
    player = std::make_unique<PlayerActor>(pr32::math::Vector2(PLAYER_START_X, PLAYER_START_Y));
    addEntity(player.get());
#endif

    spawnAliens();
    spawnBunkers();

    projectiles.reserve(MaxProjectiles);
    for (int i = 0; i < MaxProjectiles; ++i) {
#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
        ProjectileActor* projectile = arenaNew<ProjectileActor>(arena, pr32::math::Vector2(0, -PROJECTILE_HEIGHT), ProjectileType::PLAYER_BULLET);
        if (!projectile) {
            continue;
        }
        projectile->deactivate();
        projectiles.push_back(projectile);
        addEntity(projectile);
#else
        auto projectile = std::make_unique<ProjectileActor>(pr32::math::Vector2(0, -PROJECTILE_HEIGHT), ProjectileType::PLAYER_BULLET);
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
            AlienActor* alien = arenaNew<AlienActor>(arena, pr32::math::Vector2(x, y), type);
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
        BunkerActor* bunker = arenaNew<BunkerActor>(arena, pr32::math::Vector2(x, y), BUNKER_WIDTH, BUNKER_HEIGHT, 4);
        if (!bunker) {
            continue;
        }
        bunkers.push_back(bunker);
        addEntity(bunker);
#else
        auto bunker = std::make_unique<BunkerActor>(pr32::math::Vector2(x, y), BUNKER_WIDTH, BUNKER_HEIGHT, 4);
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

    if (player) {
        if (!fireInputReady) {
            if (!player->isFireDown()) {
                fireInputReady = true;
            }
        }

        if (fireInputReady && player->wantsToShoot()) {
            // Count active player bullets
            int playerBulletCount = 0;
            for (const auto& proj : projectiles) {
                if (proj->isActive() && proj->getType() == ProjectileType::PLAYER_BULLET) {
                    playerBulletCount++;
                }
            }

            // Check cooldown and bullet limit
            unsigned long now = engine.getMillis();
            if (playerBulletCount < MAX_PLAYER_BULLETS && 
                (now - lastFireTime) >= PLAYER_FIRE_COOLDOWN) {
                pr32::math::Scalar px = player->position.x + pr32::math::toScalar(PLAYER_WIDTH - PROJECTILE_WIDTH) * pr32::math::toScalar(0.5f);
                pr32::math::Scalar py = player->position.y - pr32::math::toScalar(PROJECTILE_HEIGHT);

                for (auto& proj : projectiles) {
                    if (!proj->isActive()) {
                        proj->reset(pr32::math::Vector2(px, py), ProjectileType::PLAYER_BULLET);

                        AudioEvent event{};
                        event.type = WaveType::PULSE;
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

    handleCollisions();
    updateEnemyExplosions(deltaTime);

    updateMusicTempo();
}

void SpaceInvadersScene::updateAliens(unsigned long deltaTime) {
    float scaledDelta = static_cast<float>(deltaTime) * currentMusicTempoFactor;
    stepTimer += scaledDelta;
    
    if (stepTimer >= stepDelay) {
        stepTimer = 0.0f;
        
        bool edgeHit = false;

        for (auto& alien : aliens) {
            if (!alien->isActive()) continue;
            
            if (moveDirection == 1) { // Moving Right
                if (alien->position.x + alien->width >= DISPLAY_WIDTH - 2) {
                    edgeHit = true;
                    break;
                }
            } else { // Moving Left
                if (alien->position.x <= 2) {
                    edgeHit = true;
                    break;
                }
            }
        }

        if (edgeHit) {
            moveDirection *= -1;
            for (auto& alien : aliens) {
                if (alien->isActive()) {
                    alien->move(0, ALIEN_DROP_AMOUNT);
                }
            }
        } else {
            pr32::math::Scalar dx = pr32::math::toScalar(moveDirection) * pr32::math::toScalar(ALIEN_STEP_AMOUNT_X);
            for (auto& alien : aliens) {
                if (alien->isActive()) {
                    alien->move(dx, pr32::math::toScalar(0));
                }
            }
        }

        enemyShoot();

        if (!gameOver && player) {
            for (auto& alien : aliens) {
                if (!alien->isActive()) {
                    continue;
                }
                pr32::math::Scalar bottom = alien->position.y + pr32::math::toScalar(alien->height);
                if (bottom >= player->position.y) {
                    lives = 0;
                    gameOver = true;
                    engine.getMusicPlayer().setTempoFactor(1.0f);
                    engine.getMusicPlayer().play(GAME_OVER_TRACK);
                    break;
                }
            }
        }
    }
}

void SpaceInvadersScene::handleCollisions() {
    using pixelroot32::physics::Circle;
    using pixelroot32::physics::sweepCircleVsRect;

    for (auto& proj : projectiles) {
        if (!proj->isActive()) {
            continue;
        }
        if (proj->getType() == ProjectileType::PLAYER_BULLET) {
            pr32::math::Scalar radius = pr32::math::toScalar(PROJECTILE_WIDTH * 0.5f);
            pr32::math::Scalar halfHeight = pr32::math::toScalar(PROJECTILE_HEIGHT * 0.5f);

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
                pr32::math::Scalar tHit = pr32::math::toScalar(0);
                pixelroot32::core::Rect targetBox = alien->getHitBox();
                if (sweepCircleVsRect(startCircle, endCircle, targetBox, tHit) ||
                    proj->getHitBox().intersects(targetBox)) {
                    proj->deactivate();
                    alien->kill();
                    score += alien->getScoreValue();

                    pr32::math::Scalar ex = alien->position.x + pr32::math::toScalar(alien->width) * pr32::math::toScalar(0.5f);
                    pr32::math::Scalar ey = alien->position.y + pr32::math::toScalar(alien->height) * pr32::math::toScalar(0.5f);
                    spawnEnemyExplosion(ex, ey);

                    AudioEvent event{};
                    event.type = WaveType::NOISE;
                    event.frequency = 600.0f;
                    event.duration = 0.12f;
                    event.volume = 0.6f;
                    event.duty = 0.5f;
                    engine.getAudioEngine().playEvent(event);

                    if (getActiveAlienCount() == 0) {
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
                    pr32::math::Scalar tHit = pr32::math::toScalar(0);
                    pixelroot32::core::Rect bunkerBox = bunker->getHitBox();
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

    pixelroot32::core::Rect playerBox = player->getHitBox();
    for (auto& proj : projectiles) {
        if (!proj->isActive()) {
            continue;
        }
        if (proj->getType() == ProjectileType::ENEMY_BULLET) {
            pr32::math::Scalar radius = pr32::math::toScalar(PROJECTILE_WIDTH * 0.5f);
            pr32::math::Scalar halfHeight = pr32::math::toScalar(PROJECTILE_HEIGHT * 0.5f);

            using pixelroot32::physics::Circle;
            Circle startCircle;
            startCircle.x = proj->getPreviousX() + radius;
            startCircle.y = proj->getPreviousY() + halfHeight;
            startCircle.radius = radius;

            Circle endCircle;
            endCircle.x = proj->position.x + radius;
            endCircle.y = proj->position.y + halfHeight;
            endCircle.radius = radius;

            pixelroot32::core::Rect eBox = proj->getHitBox();
            bool handled = false;
            for (auto& bunker : bunkers) {
                if (bunker->isDestroyed()) {
                    continue;
                }
                pixelroot32::core::Rect bunkerBox = bunker->getHitBox();
                pr32::math::Scalar tHit = pr32::math::toScalar(0);
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
            pr32::math::Scalar tHitPlayer = pr32::math::toScalar(0);
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
    std::vector<AlienActor*> potentialShooters;

    for (int col = 0; col < ALIEN_COLS; ++col) {
        AlienActor* lowestAlien = nullptr;
        float maxY = -1.0f;
        
        for (auto& alien : aliens) {
            if (!alien->isActive()) continue;

            float ax = static_cast<float>(alien->position.x);
            float colX = ALIEN_START_X + (col * ALIEN_SPACING_X);
            
            if (std::abs(ax - colX) < 5.0f) {
                if (static_cast<float>(alien->position.y) > maxY) {
                    maxY = static_cast<float>(alien->position.y);
                    lowestAlien = &*alien;
                }
            }
        }
        
        if (lowestAlien) {
            potentialShooters.push_back(lowestAlien);
        }
    }
    
    if (potentialShooters.empty()) return;

    int idx = std::rand() % potentialShooters.size();
    AlienActor* shooter = potentialShooters[idx];

    for (auto& proj : projectiles) {
        if (!proj->isActive()) {
            pr32::math::Scalar sx = shooter->position.x + pr32::math::toScalar(shooter->width) * pr32::math::toScalar(0.5f);
            pr32::math::Scalar sy = shooter->position.y + pr32::math::toScalar(shooter->height);
            
            proj->reset(Vector2(sx, sy), ProjectileType::ENEMY_BULLET);
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

void SpaceInvadersScene::draw(pr32::graphics::Renderer& renderer) {
    renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, pr32::graphics::Color::Black);
    Scene::draw(renderer);
    drawEnemyExplosions(renderer);
    playerExplosion.draw(renderer);

    char buffer[32];

    std::snprintf(buffer, sizeof(buffer), "SCORE %04d", score);
    renderer.drawText(buffer, 4, 4, pr32::graphics::Color::White, 1);

    std::snprintf(buffer, sizeof(buffer), "LIVES %d", lives);
    renderer.drawText(buffer, DISPLAY_WIDTH - 70, 4, pr32::graphics::Color::White, 1);

    if (gameOver) {
        if (gameWon) {
            std::snprintf(buffer, sizeof(buffer), "YOU WIN!");
            int textY = DISPLAY_HEIGHT / 2 - 8;
            renderer.drawTextCentered(buffer, textY, pr32::graphics::Color::Green, 2);
        } else {
            std::snprintf(buffer, sizeof(buffer), "GAME OVER");
            int textY = DISPLAY_HEIGHT / 2 - 8;
            renderer.drawTextCentered(buffer, textY, pr32::graphics::Color::Red, 2);
        }

        std::snprintf(buffer, sizeof(buffer), "PRESS FIRE");
        int textY = DISPLAY_HEIGHT / 2 - 8;
        renderer.drawTextCentered(buffer, textY + 20, pr32::graphics::Color::White, 1);
    }
}

void SpaceInvadersScene::updateMusicTempo() {
    if (gameOver) return;

    float lowestY = ALIEN_START_Y;
    bool found = false;

    for (const auto& alien : aliens) {
        if (alien->isActive()) {
            float y = static_cast<float>(alien->position.y + pr32::math::toScalar(alien->height));
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

void SpaceInvadersScene::drawEnemyExplosions(pr32::graphics::Renderer& renderer) {
    using Color = pr32::graphics::Color;

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
                renderer.drawFilledRectangle(hx, hy, hw, 1, Color::White);
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
                renderer.drawFilledRectangle(vx, vy, 1, vh, Color::White);
            }
        }
    }
}

void SpaceInvadersScene::spawnEnemyExplosion(pr32::math::Scalar x, pr32::math::Scalar y) {
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

    AudioEvent event{};
    event.type = WaveType::NOISE;
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

    pr32::math::Scalar newX = pr32::math::toScalar(PLAYER_START_X);

    if (targetBunker) {
        newX = targetBunker->position.x + (pr32::math::toScalar(targetBunker->width - PLAYER_WIDTH) * pr32::math::toScalar(0.5f));
    }

    player->position.x = newX;
    player->setVisible(true);
}

}
