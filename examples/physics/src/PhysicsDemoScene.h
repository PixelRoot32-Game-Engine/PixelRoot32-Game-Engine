/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include <core/Scene.h>
#include <physics/StaticActor.h>
#include <physics/KinematicActor.h>
#include <physics/RigidActor.h>

/**
 * @file PhysicsDemoScene.h
 * @brief Physics demo: RigidActor, KinematicActor, StaticActor.
 */

namespace physicsdemo {

using namespace pixelroot32::core;
using namespace pixelroot32::physics;
using namespace pixelroot32::math;

/** @brief Dynamic box (restitution 0.5, friction 0.2). */
class BoxActor : public RigidActor {
public:
    BoxActor(Scalar x, Scalar y, int w, int h) : RigidActor(x, y, w, h) {
        setRestitution(0.5f);
        setFriction(0.2f);
    }
    
    Rect getHitBox() override {
        return {position, width, height};
    }

    void draw(pixelroot32::graphics::Renderer& renderer) override {
        renderer.drawRectangle(static_cast<int>(position.x), static_cast<int>(position.y), width, height, Color::Yellow);
    }
};

/** @brief Dynamic circle with radius-based collision. */
class CircleActor : public RigidActor {
public:
    CircleActor(Scalar x, Scalar y, int diameter) : RigidActor(x, y, diameter, diameter) {
        setShape(CollisionShape::CIRCLE);
        setRadius(diameter / 2.0f);
        setRestitution(0.5f);
        setFriction(0.2f);
    }
    
    Rect getHitBox() override {
        return {position, width, height};
    }

    void draw(pixelroot32::graphics::Renderer& renderer) override {
        Scalar r = getRadius();
        renderer.drawCircle(static_cast<int>(position.x + r), static_cast<int>(position.y + r), static_cast<int>(r), Color::Cyan);
    }
};

/** @brief Player controlled by input; pushes others but not vice versa. */
class PlayerActor : public KinematicActor {
public:
    PlayerActor(Scalar x, Scalar y, int w, int h) : KinematicActor(x, y, w, h) {}

    Rect getHitBox() override {
        return {position, width, height};
    }

    void draw(pixelroot32::graphics::Renderer& renderer) override {
        renderer.drawFilledRectangle(static_cast<int>(position.x), static_cast<int>(position.y), width, height, Color::White);
    }
};

/** @brief Static wall/floor (immovable, infinite mass). */
class WallActor : public StaticActor {
public:
    WallActor(Scalar x, Scalar y, int w, int h) : StaticActor(x, y, w, h) {
        bounce = true;
    }

    Rect getHitBox() override {
        return {position, width, height};
    }

    void draw(pixelroot32::graphics::Renderer& renderer) override {
        renderer.drawRectangle(static_cast<int>(position.x), static_cast<int>(position.y), width, height, Color::DarkGreen);
    }
};

/** @brief Physics demo scene (player, boxes, circles, walls). */
class PhysicsDemoScene : public Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;

private:
    PlayerActor* player = nullptr;  ///< Player entity
    WallActor* floor = nullptr;     ///< Floor reference
};

} // namespace physicsdemo
