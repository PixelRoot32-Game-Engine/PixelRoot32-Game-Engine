/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "PhysicsDemoScene.h"
#include <memory>
#include <core/Engine.h>
#include <graphics/Color.h>
#include <input/TouchEventTypes.h>
#if PIXELROOT32_ENABLE_UI_SYSTEM
#include <graphics/ui/UITouchButton.h>
#include <graphics/ui/UITouchSlider.h>
#include <graphics/ui/UITouchCheckbox.h>
#include <graphics/ui/UIHorizontalLayout.h>
#include <graphics/ui/UIVerticalLayout.h>
#endif

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace physicsdemo {

namespace math = pr32::math;
namespace input = pr32::input;
namespace gfx = pr32::graphics;

using pr32::core::arenaNew;
using math::Vector2;
using math::toScalar;
using math::Scalar;
using input::TouchEvent;
using input::TouchEventType;
using gfx::Renderer;
using gfx::Color;
using gfx::ui::UIVerticalLayout;
using gfx::ui::UIHorizontalLayout;
using gfx::ui::UITouchButton;
using gfx::ui::UITouchSlider;
using gfx::ui::UITouchCheckbox;


#if PIXELROOT32_ENABLE_UI_SYSTEM
PhysicsDemoScene* PhysicsDemoScene::sUiTarget = nullptr;
#endif

#if PIXELROOT32_ENABLE_DEBUG_OVERLAY
namespace {
struct TouchDebugOverlayState {
    bool active = false;
    int16_t x = 0;
    int16_t y = 0;
};
TouchDebugOverlayState gTouchDebugOverlay;
} // namespace
#endif

#if PIXELROOT32_ENABLE_UI_SYSTEM
namespace {
void onPhysicsDemoTouchButtonClick() {
    if (PhysicsDemoScene::sUiTarget != nullptr) {
        PhysicsDemoScene::sUiTarget->init();
    }
}
void onPhysicsDemoTouchSliderValue(uint8_t v) {
    if (PhysicsDemoScene::sUiTarget != nullptr) {
        PhysicsDemoScene::sUiTarget->applyDynamicSpawnSlider(v);
    }
}
void onPhysicsDemoTouchCheckboxChanged(bool checked) {
    if (PhysicsDemoScene::sUiTarget != nullptr) {
        PhysicsDemoScene::sUiTarget->setPlayerVisible(checked);
    }
}
} // namespace
#endif

void PhysicsDemoScene::init() {


#if PIXELROOT32_ENABLE_UI_SYSTEM
    sUiTarget = this;
#endif

    touchController.reset();

    // Room for static level + player + 2 * kMaxSpawnPerType rigid bodies (pre-pooled for the slider).
    static uint8_t sceneBuffer[12288];
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

    spawnPoolCount = 0;
    for (int i = 0; i < kMaxSpawnPerType; ++i) {
        trackedBoxes[i] = nullptr;
        trackedCircles[i] = nullptr;
    }
    trackedSpawnPerType = 0;
    int pooled = 0;
    for (int i = 0; i < kMaxSpawnPerType; ++i) {
        BoxActor* box = arenaNew<BoxActor>(arena, 20 + i * 15, 10, 10, 10);
        CircleActor* circle = arenaNew<CircleActor>(arena, 60 + i * 20, 20, 12);
        if (box == nullptr || circle == nullptr) {
            break;
        }
        box->setCollisionLayer(1);
        box->setCollisionMask(1);
        box->setMass(1.0f);
        box->bounce = false;
        circle->setCollisionLayer(1);
        circle->setCollisionMask(1);
        circle->setMass(1.0f);
        circle->bounce = true;
        trackedBoxes[i] = box;
        trackedCircles[i] = circle;
        ++pooled;
    }
    spawnPoolCount = pooled;

    const int nSpawn = spawnCountPerType();
    int toAdd = nSpawn;
    if (toAdd > kMaxSpawnPerType) {
        toAdd = kMaxSpawnPerType;
    }
    if (toAdd > pooled) {
        toAdd = pooled;
    }
    for (int i = 0; i < toAdd; ++i) {
        if (trackedBoxes[i] != nullptr && trackedCircles[i] != nullptr) {
            addEntity(trackedBoxes[i]);
            addEntity(trackedCircles[i]);
        }
    }
    trackedSpawnPerType = toAdd;

    touchController.registerActor(player);
    // Resistive stack: calibrated coords can sit tens of pixels off valid hitboxes; slop restores
    // picking until RAW-range calibration (XPT2046_GPIO_USE_RAW_RANGE) is tuned per panel.
    touchController.setTouchHitSlop(48);

#if PIXELROOT32_ENABLE_UI_SYSTEM
    initUI();
#endif
}

#if PIXELROOT32_ENABLE_UI_SYSTEM
void PhysicsDemoScene::initUI() {
    auto& ui = getUIManager();
    ui.clear();
    demoTouchButton.reset();
    demoTouchSlider.reset();
    demoTouchCheckbox.reset();
    demoHorizontalLayout.reset();
    demoVerticalLayout.reset();

    auto& renderer = engine.getRenderer();
    const int sw = renderer.getLogicalWidth();
    const int sh = renderer.getLogicalHeight();
    constexpr int margin = 6;
    constexpr int barH = 36;
    constexpr int btnW = 72;
    constexpr int padding = 8;
    constexpr int gap = 8;
    constexpr int checkboxH = 30;
    const int totalHeight = checkboxH + barH + gap;
    const int barY = sh - totalHeight - margin;
    int sliderW = sw - margin * 2 - btnW - gap;
    if (sliderW < 48) {
        sliderW = 48;
    }

    demoVerticalLayout = std::make_unique<UIVerticalLayout>(
        toScalar(margin),
        toScalar(barY),
        sw - margin * 2,
        totalHeight
    );
    demoVerticalLayout->setSpacing(toScalar(gap));
    demoVerticalLayout->setPadding(toScalar(padding));
    addEntity(demoVerticalLayout.get());

    demoHorizontalLayout = std::make_unique<UIHorizontalLayout>(
        toScalar(margin),
        toScalar(barY),
        sw - margin * 2,
        barH
    );
    demoHorizontalLayout->setSpacing(toScalar(gap));
    demoHorizontalLayout->setPadding(toScalar(0));

    demoTouchButton = std::make_unique<UITouchButton>("Spawn", 0, 0,
        static_cast<uint16_t>(btnW), static_cast<uint16_t>(barH));
    demoTouchSlider = std::make_unique<UITouchSlider>(0, 0,
        static_cast<uint16_t>(sliderW), static_cast<uint16_t>(barH),
        dynamicSpawnSliderValue);

    ui.addElement(demoTouchButton.get());
    ui.addElement(demoTouchSlider.get());

    if (demoTouchButton) {
        demoHorizontalLayout->addElement(demoTouchButton.get());
        demoTouchButton->setColors(Color::Blue, Color::Navy, Color::DarkGray);
        demoTouchButton->setOnClick(onPhysicsDemoTouchButtonClick);
    }
    if (demoTouchSlider) {
        demoHorizontalLayout->addElement(demoTouchSlider.get());
        demoTouchSlider->setColors(Color::Navy, Color::LightGreen);
        demoTouchSlider->setOnValueChanged(onPhysicsDemoTouchSliderValue);
    }

    demoVerticalLayout->addElement(demoHorizontalLayout.get());

    demoTouchCheckbox = std::make_unique<UITouchCheckbox>("Show Player", 0, 0,
        static_cast<uint16_t>(sw - margin * 2), static_cast<uint16_t>(checkboxH), true);
    ui.addElement(demoTouchCheckbox.get());
    if (demoTouchCheckbox) {
        demoVerticalLayout->addElement(demoTouchCheckbox.get());
        demoTouchCheckbox->setColors(Color::White, Color::Cyan, Color::Gray);
        demoTouchCheckbox->setOnChanged(onPhysicsDemoTouchCheckboxChanged);
    }
}

void PhysicsDemoScene::applyDynamicSpawnSlider(uint8_t sliderValue) {
    dynamicSpawnSliderValue = sliderValue;
    int targetN = spawnCountPerType();
    if (targetN > spawnPoolCount) {
        targetN = spawnPoolCount;
    }
    if (targetN == trackedSpawnPerType) {
        return;
    }

    if (targetN > trackedSpawnPerType) {
        for (int i = trackedSpawnPerType; i < targetN; ++i) {
            BoxActor* box = trackedBoxes[i];
            CircleActor* circle = trackedCircles[i];
            if (box != nullptr && circle != nullptr) {
                addEntity(box);
                addEntity(circle);
            }
        }
    } else {
        for (int i = trackedSpawnPerType - 1; i >= targetN; --i) {
            if (trackedBoxes[i] != nullptr) {
                removeEntity(trackedBoxes[i]);
            }
            if (trackedCircles[i] != nullptr) {
                removeEntity(trackedCircles[i]);
            }
        }
    }
    trackedSpawnPerType = targetN;
}

void PhysicsDemoScene::setPlayerVisible(bool visible) {
    if (player != nullptr) {
        player->setVisible(visible);
    }
}
#endif

void PhysicsDemoScene::processTouchEvents(TouchEvent* events, uint8_t count) {
#if PIXELROOT32_ENABLE_DEBUG_OVERLAY
    if (events != nullptr) {
        using T = TouchEventType;
        for (uint8_t i = 0; i < count; ++i) {
            const auto ty = static_cast<TouchEventType>(events[i].type);
            if (ty == T::TouchUp || ty == T::DragEnd) {
                gTouchDebugOverlay.active = false;
            } else if (ty == T::TouchDown || ty == T::DragStart || ty == T::DragMove) {
                gTouchDebugOverlay.x = events[i].x;
                gTouchDebugOverlay.y = events[i].y;
                gTouchDebugOverlay.active = true;
            }
        }
    }
#endif
    Scene::processTouchEvents(events, count);
}

void PhysicsDemoScene::draw(Renderer& renderer) {
    Scene::draw(renderer);

#if PIXELROOT32_ENABLE_DEBUG_OVERLAY
    if (gTouchDebugOverlay.active) {
        constexpr int kMarkW = 12;
        constexpr int kMarkH = 12;
        const int x = static_cast<int>(gTouchDebugOverlay.x) - kMarkW / 2;
        const int y = static_cast<int>(gTouchDebugOverlay.y) - kMarkH / 2;
        renderer.drawFilledRectangle(x, y, kMarkW, kMarkH, pixelroot32::graphics::Color::Red);
    }
#endif
}

void PhysicsDemoScene::onUnconsumedTouchEvent(const TouchEvent& event) {
    touchController.handleTouch(event);
}

void PhysicsDemoScene::update(unsigned long deltaTime) {
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
