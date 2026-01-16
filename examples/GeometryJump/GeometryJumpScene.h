#pragma once

#include "core/Scene.h"
#include "graphics/ui/UILabel.h"
#include "graphics/Color.h"
#include "Config.h"
#include "GameConstants.h"
#include "PlayerActor.h"
#include "ObstacleActor.h"
#include "graphics/particles/ParticleEmitter.h"

namespace geometryjump {

class GeometryJumpScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    pixelroot32::graphics::ui::UILabel* scoreLabel;
    pixelroot32::graphics::ui::UILabel* lblGameOver;
    pixelroot32::graphics::ui::UILabel* lblRetry;
    pixelroot32::graphics::particles::ParticleEmitter* deathEmitter;
    PlayerActor* player;
    ObstacleActor* obstacles[MAX_OBSTACLES];
    bool gameOver = false;
    float scrollSpeed = 0.0f;
    float levelDistance = 0.0f;
    int nextObstacleIndex = 0;
    int playAreaBottom = 0;
    float playerStartX = 0.0f;
    float playerStartY = 0.0f;
    int score = 0;
    bool obstacleScored[MAX_OBSTACLES];
};

}
