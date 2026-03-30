/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "PhysicsDemoScene.h"
#include <core/Engine.h>

extern pixelroot32::core::Engine engine;

namespace physicsdemo {

void PhysicsDemoScene::init() {
    using namespace pixelroot32::math;

    static uint8_t sceneBuffer[4096];
    arena.init(sceneBuffer, sizeof(sceneBuffer));

    clearEntities();

    floor = arenaNew<WallActor>(arena, 0, 110, 240, 130);
    floor->setCollisionLayer(1);
    floor->setCollisionMask(1);
    addEntity(floor);

    // 2. Create Left Wall — just at screen edge
    WallActor* leftWall = arenaNew<WallActor>(arena, -10, 0, 10, 240);
    leftWall->setCollisionLayer(1);
    leftWall->setCollisionMask(1);
    addEntity(leftWall);

    WallActor* rightWall = arenaNew<WallActor>(arena, 240, 0, 10, 240);
    rightWall->setCollisionLayer(1);
    rightWall->setCollisionMask(1);
    addEntity(rightWall);

    WallActor* ceiling = arenaNew<WallActor>(arena, 0, -10, 240, 10);
    ceiling->setCollisionLayer(1);
    ceiling->setCollisionMask(1);
    addEntity(ceiling);

    player = arenaNew<PlayerActor>(arena, 140, 30, 10, 10);
    player->setCollisionLayer(1);
    player->setCollisionMask(1);
    addEntity(player);

    for (int i = 0; i < 6; ++i) {
        BoxActor* box = arenaNew<BoxActor>(arena, 20 + i * 15, 10, 10, 10);
        box->setCollisionLayer(1);
        box->setCollisionMask(1);
        box->setMass(1.0f);
        box->bounce = false;
        addEntity(box);
    }

    for (int i = 0; i < 6; ++i) {
        CircleActor* circle = arenaNew<CircleActor>(arena, 60 + i * 20, 20, 12);
        circle->setCollisionLayer(1);
        circle->setCollisionMask(1);
        circle->setMass(1.0f);
        circle->bounce = true;
        addEntity(circle);
    }
}

void PhysicsDemoScene::update(unsigned long deltaTime) {
    using namespace pixelroot32::math;

    auto& input = engine.getInputManager();

    if (input.isButtonPressed(4)) {
        init();
        return;
    }

    Vector2 move = {toScalar(0), toScalar(0)};
    Scalar speed = toScalar(100.0f); 

    if (input.isButtonDown(0)) move.y -= toScalar(1.0f);
    if (input.isButtonDown(1)) move.y += toScalar(1.0f);
    if (input.isButtonDown(2)) move.x -= toScalar(1.0f);
    if (input.isButtonDown(3)) move.x += toScalar(1.0f);

    if (move.length() > toScalar(0)) {
        move.normalize();
        Scalar dt = toScalar(static_cast<float>(deltaTime) * 0.001f);
        player->moveAndSlide(move * speed * dt);
    }

    Scene::update(deltaTime);
}

} // namespace physicsdemo
