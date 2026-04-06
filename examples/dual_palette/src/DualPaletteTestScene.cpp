#include "DualPaletteTestScene.h"
#include <core/Engine.h>
#include <graphics/Color.h>
#include <graphics/Renderer.h>
#include <cstdio>

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace dualpalettetest {

namespace gfx = pr32::graphics;

using Entity = pr32::core::Entity;
using EntityType = pr32::core::EntityType;
using Renderer = gfx::Renderer;
using Sprite = gfx::Sprite;
using Color = gfx::Color;

// Test sprites for demonstration
static const uint16_t SPRITE_SQUARE_DATA[] = {
    0b11111111,  // Row 0: all pixels on
    0b10000001,  // Row 1: border
    0b10111101,  // Row 2: border with inner pattern
    0b10111101,  // Row 3: border with inner pattern
    0b10111101,  // Row 4: border with inner pattern
    0b10111101,  // Row 5: border with inner pattern
    0b10000001,  // Row 6: border
    0b11111111   // Row 7: all pixels on
};

static const uint16_t SPRITE_CIRCLE_DATA[] = {
    0b00111100,  // Row 0
    0b01111110,  // Row 1
    0b11111111,  // Row 2
    0b11111111,  // Row 3
    0b11111111,  // Row 4
    0b11111111,  // Row 5
    0b01111110,  // Row 6
    0b00111100   // Row 7
};

static const uint16_t SPRITE_TRIANGLE_DATA[] = {
    0b00010000,  // Row 0
    0b00111000,  // Row 1
    0b01111100,  // Row 2
    0b11111110,  // Row 3
    0b11111110,  // Row 4
    0b01111100,  // Row 5
    0b00111000,  // Row 6
    0b00010000   // Row 7
};


class DualPaletteTestScene::TestBackground : public Entity {
public:
    TestBackground()
        : Entity(0.0f, 0.0f, DISPLAY_WIDTH, DISPLAY_HEIGHT, EntityType::GENERIC) {
        setRenderLayer(0);
    }

    void update(unsigned long) override {
    }

    void draw(Renderer& renderer) override {
        renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, Color::Black);
        renderer.drawFilledRectangle(10, 10, 30, 30, Color::Navy);
        renderer.drawFilledRectangle(50, 10, 30, 30, Color::Blue);
        renderer.drawFilledRectangle(90, 10, 30, 30, Color::Cyan);
        renderer.drawFilledRectangle(130, 10, 30, 30, Color::DarkGreen);
        renderer.drawFilledRectangle(10, 50, 30, 30, Color::Green);
        renderer.drawFilledRectangle(50, 50, 30, 30, Color::LightGreen);
        renderer.drawFilledRectangle(90, 50, 30, 30, Color::Yellow);
        renderer.drawFilledRectangle(130, 50, 30, 30, Color::Orange);
        renderer.drawFilledRectangle(10, 90, 30, 30, Color::LightRed);
        renderer.drawFilledRectangle(50, 90, 30, 30, Color::Red);
        renderer.drawFilledRectangle(90, 90, 30, 30, Color::DarkRed);
        renderer.drawFilledRectangle(130, 90, 30, 30, Color::Purple);
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if ((i + j) % 2 == 0) {
                    renderer.drawFilledRectangle(170 + i * 8, 130 + j * 8, 8, 8, Color::Cyan);
                }
            }
        }
        renderer.drawText("BG Palette", 10, DISPLAY_HEIGHT - 30, Color::White, 1);
        renderer.drawText("(NES)", 10, DISPLAY_HEIGHT - 20, Color::Gray, 1);
    }
};

class DualPaletteTestScene::TestSprite : public Entity {
public:
    TestSprite()
        : Entity(0.0f, 0.0f, DISPLAY_WIDTH, DISPLAY_HEIGHT, EntityType::GENERIC) {
        setRenderLayer(1);
    }

    void update(unsigned long) override {
    }

    void draw(Renderer& renderer) override {
        int startX = 170;
        int startY = 10;
        Sprite squareSprite;
        squareSprite.data = SPRITE_SQUARE_DATA;
        squareSprite.width = 8;
        squareSprite.height = 8;
        
        Sprite circleSprite;
        circleSprite.data = SPRITE_CIRCLE_DATA;
        circleSprite.width = 8;
        circleSprite.height = 8;
        
        Sprite triangleSprite;
        triangleSprite.data = SPRITE_TRIANGLE_DATA;
        triangleSprite.width = 8;
        triangleSprite.height = 8;
        renderer.drawSprite(squareSprite, startX, startY, Color::DarkGreen, false);
        renderer.drawSprite(squareSprite, startX + 20, startY, Color::Green, false);
        renderer.drawSprite(squareSprite, startX + 40, startY, Color::LightGreen, false);
        renderer.drawSprite(circleSprite, startX, startY + 30, Color::Yellow, false);
        renderer.drawSprite(circleSprite, startX + 20, startY + 30, Color::Orange, false);
        renderer.drawSprite(circleSprite, startX + 40, startY + 30, Color::LightRed, false);
        renderer.drawSprite(triangleSprite, startX, startY + 60, Color::Cyan, false);
        renderer.drawSprite(triangleSprite, startX + 20, startY + 60, Color::Blue, false);
        renderer.drawSprite(triangleSprite, startX + 40, startY + 60, Color::Navy, false);
        renderer.drawSprite(squareSprite, startX + 70, startY, 2.0f, 2.0f, Color::Green, false);
        renderer.drawSprite(circleSprite, startX + 90, startY + 30, 1.5f, 1.5f, Color::Yellow, false);
        renderer.drawText("Sprite Palette", startX, startY + 100, Color::White, 1);
        renderer.drawText("(GB)", startX, startY + 110, Color::Gray, 1);
        renderer.drawText("Compare colors:", 10, 130, Color::White, 1);
        renderer.drawText("Same index =", 10, 145, Color::White, 1);
        renderer.drawText("Different RGB!", 10, 160, Color::White, 1);
    }
};

DualPaletteTestScene::DualPaletteTestScene()
    : background(nullptr), testSprite(nullptr) {
    background = std::make_unique<TestBackground>();
    testSprite = std::make_unique<TestSprite>();
    
    addEntity(background.get());
    addEntity(testSprite.get());
}

DualPaletteTestScene::~DualPaletteTestScene() {
}

void DualPaletteTestScene::init() {
    gfx::enableDualPaletteMode(true);
    gfx::setBackgroundPalette(gfx::PaletteType::NES);
    gfx::setSpritePalette(gfx::PaletteType::GB);
}

void DualPaletteTestScene::update(unsigned long deltaTime) {
    Scene::update(deltaTime);
}

void DualPaletteTestScene::draw(Renderer& renderer) {
    Scene::draw(renderer);
}

}
