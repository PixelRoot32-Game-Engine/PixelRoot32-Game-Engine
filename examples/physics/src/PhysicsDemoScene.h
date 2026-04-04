/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include <cstdint>
#include <memory>
#include <core/Scene.h>
#include <input/ActorTouchController.h>
#include <physics/StaticActor.h>
#include <physics/KinematicActor.h>
#include <physics/RigidActor.h>
#if PIXELROOT32_ENABLE_UI_SYSTEM
#include <graphics/ui/UITouchButton.h>
#include <graphics/ui/UITouchSlider.h>
#include <graphics/ui/UITouchCheckbox.h>
#include <graphics/ui/UIHorizontalLayout.h>
#include <graphics/ui/UIVerticalLayout.h>
#endif

/**
 * @file PhysicsDemoScene.h
 * @brief Physics demo: RigidActor, KinematicActor, StaticActor.
 */

namespace physicsdemo {

/** @brief Dynamic box (restitution 0.5, friction 0.2). */
class BoxActor : public pixelroot32::physics::RigidActor {
public:
    BoxActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h) : pixelroot32::physics::RigidActor(x, y, w, h) {
        setRestitution(0.5f);
        setFriction(0.2f);
    }
    
    pixelroot32::core::Rect getHitBox() override {
        return {position, width, height};
    }

    void draw(pixelroot32::graphics::Renderer& renderer) override {
        renderer.drawRectangle(static_cast<int>(position.x), static_cast<int>(position.y), width, height, pixelroot32::graphics::Color::Yellow);
    }
};

/** @brief Dynamic circle with radius-based collision. */
class CircleActor : public pixelroot32::physics::RigidActor {
public:
    CircleActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int diameter) : pixelroot32::physics::RigidActor(x, y, diameter, diameter) {
        setShape(pixelroot32::core::CollisionShape::CIRCLE);
        setRadius(diameter / 2.0f);
        setRestitution(0.5f);
        setFriction(0.2f);
    }
    
    pixelroot32::core::Rect getHitBox() override {
        return {position, width, height};
    }

    void draw(pixelroot32::graphics::Renderer& renderer) override {
        pixelroot32::math::Scalar r = getRadius();
        renderer.drawCircle(static_cast<int>(position.x + r), static_cast<int>(position.y + r), static_cast<int>(r), pixelroot32::graphics::Color::Cyan);
    }
};

/** @brief Player controlled by input; pushes others but not vice versa. */
class PlayerActor : public pixelroot32::physics::KinematicActor {
public:
    PlayerActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h) : pixelroot32::physics::KinematicActor(x, y, w, h) {}

    pixelroot32::core::Rect getHitBox() override {
        return {position, width, height};
    }

    void draw(pixelroot32::graphics::Renderer& renderer) override {
        renderer.drawFilledRectangle(static_cast<int>(position.x), static_cast<int>(position.y), width, height, pixelroot32::graphics::Color::White);
    }
};

/** @brief Static wall/floor (immovable, infinite mass). */
class WallActor : public pixelroot32::physics::StaticActor {
public:
    WallActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h) : pixelroot32::physics::StaticActor(x, y, w, h) {
        setBounce(true);
    }

    pixelroot32::core::Rect getHitBox() override {
        return {position, width, height};
    }

    void draw(pixelroot32::graphics::Renderer& renderer) override {
        renderer.drawRectangle(static_cast<int>(position.x), static_cast<int>(position.y), width, height, pixelroot32::graphics::Color::DarkGreen);
    }
};

/** @brief Physics demo scene (player, boxes, circles, walls). */
class PhysicsDemoScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;

    void processTouchEvents(pixelroot32::input::TouchEvent* events, uint8_t count) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    void onUnconsumedTouchEvent(const pixelroot32::input::TouchEvent& event) override;

#if PIXELROOT32_ENABLE_UI_SYSTEM
    void initUI() override;

    /**
     * @brief Updates spawn load from the touch slider (0–100).
     *
     * Adds or removes boxes/circles incrementally without resetting the scene (player and layout stay).
     * Use init() (touch button or keyboard) for a full reset.
     */
    void applyDynamicSpawnSlider(uint8_t sliderValue);

    /**
     * @brief Sets player visibility for the checkbox callback.
     * @param visible True to show player, false to hide
     */
    void setPlayerVisible(bool visible);

    /** Target for C-style widget callbacks; set at the start of init(). */
    static PhysicsDemoScene* sUiTarget;
#endif

private:
    PlayerActor* player = nullptr;  ///< Player entity
    WallActor* floor = nullptr;     ///< Floor reference
    pixelroot32::input::ActorTouchController touchController;

    /** Slider is 0–100; spawnCountPerType() maps it to [0, kMaxSpawnPerType] per type (boxes and circles). */
    static constexpr int kMaxSpawnPerType = 10;
    /** Default 30 → three boxes and three circles (rounded): (30 * kMaxSpawnPerType + 50) / 100 == 3 */
    uint8_t dynamicSpawnSliderValue = 30;

    /** @return Number of BoxActor and CircleActor instances to spawn from dynamicSpawnSliderValue. */
    int spawnCountPerType() const {
        return static_cast<int>(
            (static_cast<unsigned>(dynamicSpawnSliderValue) * static_cast<unsigned>(kMaxSpawnPerType) + 50u) / 100u);
    }

    /**
     * Pre-allocated in init() up to kMaxSpawnPerType each; objects stay in the scene arena.
     * Indices [0, trackedSpawnPerType) are registered in the Scene (addEntity); the slider only
     * addEntity/removeEntity — no extra arenaNew — so the arena does not run out after many cycles.
     */
    BoxActor* trackedBoxes[kMaxSpawnPerType]{};
    CircleActor* trackedCircles[kMaxSpawnPerType]{};
    /** Pairs successfully arena-allocated (may be < kMaxSpawnPerType if the arena is too small). */
    int spawnPoolCount = 0;
    int trackedSpawnPerType = 0;

#if PIXELROOT32_ENABLE_UI_SYSTEM
    std::unique_ptr<pixelroot32::graphics::ui::UITouchButton> demoTouchButton;
    std::unique_ptr<pixelroot32::graphics::ui::UITouchSlider> demoTouchSlider;
    std::unique_ptr<pixelroot32::graphics::ui::UITouchCheckbox> demoTouchCheckbox;
    std::unique_ptr<pixelroot32::graphics::ui::UIHorizontalLayout> demoHorizontalLayout;
    std::unique_ptr<pixelroot32::graphics::ui::UIVerticalLayout> demoVerticalLayout;
#endif
};

} // namespace physicsdemo
