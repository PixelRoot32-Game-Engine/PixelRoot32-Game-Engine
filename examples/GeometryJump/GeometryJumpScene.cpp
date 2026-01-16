#include "GeometryJumpScene.h"
#include "core/Engine.h"
#include "graphics/particles/ParticlePresets.h"
#include "audio/AudioTypes.h"
#include <cstdio>
#include <cstring>

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace geometryjump {

using Color = pr32::graphics::Color;
using pr32::graphics::particles::ParticleEmitter;

void GeometryJumpScene::init() {
    int screenWidth = engine.getRenderer().getWidth();
    int screenHeight = engine.getRenderer().getHeight();

    score = 0;
    scoreLabel = new pr32::graphics::ui::UILabel("0", 0.0f, 8.0f, Color::White, 2);
    float initialWidth = 1.0f * (6.0f * 2.0f);
    scoreLabel->centerX(screenWidth);

    lblGameOver = new pr32::graphics::ui::UILabel("GAME OVER", 0, 50, Color::White, 2);
    lblGameOver->centerX(screenWidth);
    lblGameOver->setVisible(false);

    lblRetry = new pr32::graphics::ui::UILabel("PRESS A TO RETRY", 0, 70, Color::White, 1);
    lblRetry->centerX(screenWidth);
    lblRetry->setVisible(false);

    playAreaBottom = screenHeight - GROUND_HEIGHT;

    playerStartX = screenWidth * 0.25f;
    playerStartY = static_cast<float>(playAreaBottom - PLAYER_HEIGHT - 4);

    player = new PlayerActor(playerStartX, playerStartY, PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_SPEED_X);
    player->setWorldSize(screenWidth, playAreaBottom);

    scrollSpeed = SCROLL_SPEED;
    levelDistance = 0.0f;
    nextObstacleIndex = 0;

    addEntity(scoreLabel);
    addEntity(lblGameOver);
    addEntity(lblRetry);
    addEntity(player);

    auto dustConfig = pr32::graphics::particles::ParticlePresets::Dust;
    dustConfig.minAngleDeg = 170.0f;
    dustConfig.maxAngleDeg = 190.0f;
    dustConfig.gravity = 0.02f;
    deathEmitter = new ParticleEmitter(0.0f, 0.0f, pr32::graphics::particles::ParticlePresets::Explosion);
    addEntity(deathEmitter);

    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        obstacles[i] = new ObstacleActor(0.0f, 0.0f, OBSTACLE_WIDTH, OBSTACLE_HEIGHT);
        obstacles[i]->isEnabled = false;
        obstacles[i]->isVisible = false;
        obstacleScored[i] = false;
        addEntity(obstacles[i]);
    }
}

void GeometryJumpScene::update(unsigned long deltaTime) {
    auto& input = engine.getInputManager();
    if (!gameOver) {
        if (input.isButtonPressed(BTN_JUMP) && player->isGrounded()) {
            player->jump();
        }
    } else {
        if (input.isButtonPressed(BTN_JUMP)) {
            player->reset(playerStartX, playerStartY);

            levelDistance = 0.0f;
            nextObstacleIndex = 0;

            for (int i = 0; i < MAX_OBSTACLES; ++i) {
                obstacles[i]->isEnabled = false;
                obstacles[i]->isVisible = false;
                obstacleScored[i] = false;
            }

            gameOver = false;
            scrollSpeed = SCROLL_SPEED;
            lblGameOver->setVisible(false);
            lblRetry->setVisible(false);

            score = 0;
            scoreLabel->x = (engine.getRenderer().getWidth() - (1.0f * (6.0f * 2.0f))) * 0.5f;
            scoreLabel->setText("0");
        }
    }

    float dt = deltaTime * 0.001f;

    if (!gameOver) {
        int screenWidth = engine.getRenderer().getWidth();

        // Use constant SCROLL_SPEED (base speed) for level progression
        // This ensures obstacles spawn at fixed time intervals regardless of visual speed,
        // maintaining the same time window for jumps/falls even as the game speeds up.
        levelDistance += SCROLL_SPEED * dt;

        if (levelDistance >= LEVEL1_TOTAL_DISTANCE) {
            levelDistance -= LEVEL1_TOTAL_DISTANCE;
            nextObstacleIndex = 0;
        }

        float playerX = player->x;

        for (int i = 0; i < MAX_OBSTACLES; ++i) {
            if (obstacles[i]->isEnabled) {
                obstacles[i]->x -= scrollSpeed * dt;

                if (!obstacleScored[i] && obstacles[i]->x + obstacles[i]->width < playerX) {
                    obstacleScored[i] = true;
                    score++;

                    pr32::audio::AudioEvent coinEvent{};
                    coinEvent.type = pr32::audio::WaveType::PULSE;
                    coinEvent.frequency = 1500.0f;
                    coinEvent.duration = 0.12f;
                    coinEvent.volume = 0.8f;
                    coinEvent.duty = 0.5f;
                    engine.getAudioEngine().playEvent(coinEvent);

                    if (scrollSpeed < MAX_SCROLL_SPEED) {
                        scrollSpeed += SPEED_INCREMENT;
                    }

                    char scoreBuffer[16];
                    snprintf(scoreBuffer, sizeof(scoreBuffer), "%d", score);
                    float textWidth = static_cast<float>(strlen(scoreBuffer) * 12);
                    scoreLabel->x = (engine.getRenderer().getWidth() - textWidth) * 0.5f;
                    scoreLabel->setText(scoreBuffer);
                }

                if (obstacles[i]->x + obstacles[i]->width < 0) {
                    obstacles[i]->isEnabled = false;
                    obstacles[i]->isVisible = false;
                }
            }
        }

        while (nextObstacleIndex < LEVEL1_OBSTACLE_COUNT &&
               levelDistance >= LEVEL1_OBSTACLES[nextObstacleIndex].distance) {
            for (int i = 0; i < MAX_OBSTACLES; ++i) {
                if (!obstacles[i]->isEnabled) {
                    const auto& def = LEVEL1_OBSTACLES[nextObstacleIndex];
                    float spawnX = static_cast<float>(screenWidth + def.width);
                    float baseY = static_cast<float>(playAreaBottom - def.height - OBSTACLE_GROUND_OFFSET);
                    float spawnY = baseY - static_cast<float>(def.heightOffset);

                    obstacles[i]->x = spawnX;
                    obstacles[i]->y = spawnY;
                    obstacles[i]->width = def.width;
                    obstacles[i]->height = def.height;
                    obstacles[i]->isEnabled = true;
                    obstacles[i]->isVisible = true;
                    obstacleScored[i] = false;
                    break;
                }
            }
            ++nextObstacleIndex;
        }
    }

    pr32::core::Scene::update(deltaTime);

    if (!gameOver && player->isEnabled) {
        for (int i = 0; i < MAX_OBSTACLES; ++i) {
            if (obstacles[i]->isEnabled) {
                if (player->getHitBox().intersects(obstacles[i]->getHitBox())) {
                    pr32::audio::AudioEvent hitEvent{};
                    hitEvent.type = pr32::audio::WaveType::NOISE;
                    hitEvent.frequency = 1200.0f;
                    hitEvent.duration = 0.25f;
                    hitEvent.volume = 0.9f;
                    hitEvent.duty = 0.5f;
                    engine.getAudioEngine().playEvent(hitEvent);
                    gameOver = true;
                    player->isEnabled = false;
                    float cx = player->x + player->width * 0.5f;
                    float cy = player->y + player->height * 0.5f;
                    deathEmitter->burst(cx, cy, 20);
                    lblGameOver->setVisible(true);
                    lblRetry->setVisible(true);
                    break;
                }
            }
        }
    }
}

void GeometryJumpScene::draw(pr32::graphics::Renderer& renderer) {
    int screenWidth = engine.getRenderer().getWidth();
    int screenHeight = engine.getRenderer().getHeight();

    int groundY = playAreaBottom;
    int groundHeight = screenHeight - groundY;
    if (groundHeight > 0) {
        renderer.drawFilledRectangle(0, groundY, screenWidth, groundHeight, Color::White);
    }

    pr32::core::Scene::draw(renderer);
}

}

