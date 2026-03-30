#include "SpritesDemoScene.h"

#if defined(PIXELROOT32_ENABLE_2BPP_SPRITES) || defined(PIXELROOT32_ENABLE_4BPP_SPRITES)

#include "graphics/Renderer.h"
#include <cstdint>

namespace pr32 = pixelroot32;

namespace spritesdemo {

using pr32::graphics::Color;
using pr32::graphics::Sprite2bpp;
using pr32::graphics::Sprite4bpp;

namespace {

static constexpr uint8_t SPRITE_WIDTH = 16;
static constexpr uint8_t SPRITE_HEIGHT = 32;

static const Color SPRITES_2BPP_PALETTE[] = {
    Color::Transparent,
    Color::Black,
    Color::LightBlue,
    Color::White
};

static const Sprite2bpp SPRITE_0_2BPP_STRUCT = { reinterpret_cast<const uint8_t*>(SPRITE_0_2BPP), SPRITES_2BPP_PALETTE, SPRITE_WIDTH, SPRITE_HEIGHT, 4 };
static const Sprite2bpp SPRITE_1_2BPP_STRUCT = { reinterpret_cast<const uint8_t*>(SPRITE_1_2BPP), SPRITES_2BPP_PALETTE, SPRITE_WIDTH, SPRITE_HEIGHT, 4 };
static const Sprite2bpp SPRITE_2_2BPP_STRUCT = { reinterpret_cast<const uint8_t*>(SPRITE_2_2BPP), SPRITES_2BPP_PALETTE, SPRITE_WIDTH, SPRITE_HEIGHT, 4 };
static const Sprite2bpp SPRITE_3_2BPP_STRUCT = { reinterpret_cast<const uint8_t*>(SPRITE_3_2BPP), SPRITES_2BPP_PALETTE, SPRITE_WIDTH, SPRITE_HEIGHT, 4 };
static const Sprite2bpp SPRITE_4_2BPP_STRUCT = { reinterpret_cast<const uint8_t*>(SPRITE_4_2BPP), SPRITES_2BPP_PALETTE, SPRITE_WIDTH, SPRITE_HEIGHT, 4 };
static const Sprite2bpp SPRITE_5_2BPP_STRUCT = { reinterpret_cast<const uint8_t*>(SPRITE_5_2BPP), SPRITES_2BPP_PALETTE, SPRITE_WIDTH, SPRITE_HEIGHT, 4 };
static const Sprite2bpp SPRITE_6_2BPP_STRUCT = { reinterpret_cast<const uint8_t*>(SPRITE_6_2BPP), SPRITES_2BPP_PALETTE, SPRITE_WIDTH, SPRITE_HEIGHT, 4 };
static const Sprite2bpp SPRITE_7_2BPP_STRUCT = { reinterpret_cast<const uint8_t*>(SPRITE_7_2BPP), SPRITES_2BPP_PALETTE, SPRITE_WIDTH, SPRITE_HEIGHT, 4 };
static const Sprite2bpp SPRITE_8_2BPP_STRUCT = { reinterpret_cast<const uint8_t*>(SPRITE_8_2BPP), SPRITES_2BPP_PALETTE, SPRITE_WIDTH, SPRITE_HEIGHT, 4 };

static const Sprite2bpp* SPRITES_2BPP[] = {
    &SPRITE_0_2BPP_STRUCT,
    &SPRITE_1_2BPP_STRUCT,
    &SPRITE_2_2BPP_STRUCT,
    &SPRITE_3_2BPP_STRUCT,
    &SPRITE_4_2BPP_STRUCT,
    &SPRITE_5_2BPP_STRUCT,
    &SPRITE_6_2BPP_STRUCT,
    &SPRITE_7_2BPP_STRUCT,
    &SPRITE_8_2BPP_STRUCT
};

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

static constexpr uint8_t POPUP_WIDTH = 16;
static constexpr uint8_t POPUP_HEIGHT = 16;

static const Color POPUP_PALETTE[] = {
    Color::Transparent,
    Color::Black,
    Color::DarkGray,
    Color::DarkRed,
    Color::Purple,
    Color::Brown,
    Color::LightBlue,
    Color::Red,
    Color::Gold,
    Color::LightRed,
    Color::LightGray,
    Color::Yellow,
    Color::White,
    Color::White,
    Color::LightRed,
    Color::Pink
};

static const uint16_t* SPRITES_4BPP_DATA[] = {
    SPRITE_0_4BPP, SPRITE_1_4BPP, SPRITE_2_4BPP, SPRITE_3_4BPP,
    SPRITE_4_4BPP, SPRITE_5_4BPP, SPRITE_6_4BPP, SPRITE_7_4BPP,
    SPRITE_8_4BPP, SPRITE_9_4BPP, SPRITE_10_4BPP, SPRITE_11_4BPP,
    SPRITE_12_4BPP, SPRITE_13_4BPP, SPRITE_14_4BPP
};

#endif


class SpritesDemoBackground : public pr32::core::Entity {
public:
    SpritesDemoBackground()
        : pr32::core::Entity(0.0f, 0.0f, DISPLAY_WIDTH, DISPLAY_HEIGHT, pr32::core::EntityType::GENERIC) {
        setRenderLayer(0);
    }

    void update(unsigned long) override {
    }

    void draw(pr32::graphics::Renderer& renderer) override {
        renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, Color::Black);
    }
};

class SpritesDemoActor : public pr32::core::Entity {
public:
    SpritesDemoActor(float px, float py)
        : pr32::core::Entity(px, py, SPRITE_WIDTH, SPRITE_HEIGHT, pr32::core::EntityType::GENERIC),
          timeAccumulator(0),
          currentFrame(0) {
        setRenderLayer(1);
    }

    void update(unsigned long deltaTime) override {
        const unsigned long frameTimeMs = 150;
        timeAccumulator += deltaTime;
        while (timeAccumulator >= frameTimeMs) {
            timeAccumulator -= frameTimeMs;
            ++currentFrame;
            if (currentFrame >= 3) {
                currentFrame = 0;
            }
        }
    }

    void draw(pr32::graphics::Renderer& renderer) override {
        int drawX = static_cast<int>(position.x);
        int drawY = static_cast<int>(position.y);

        int spriteIndex = currentFrame;

        if (spriteIndex >= 0 && spriteIndex < static_cast<int>(sizeof(SPRITES_2BPP) / sizeof(SPRITES_2BPP[0]))) {
            const Sprite2bpp* sprite = SPRITES_2BPP[spriteIndex];
            renderer.drawSprite(*sprite, drawX, drawY, false);
        }
    }

private:
    unsigned long timeAccumulator;
    uint8_t currentFrame;
};

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

class SpritesPopupActor : public pr32::core::Entity {
public:
    SpritesPopupActor(float px, float py, const uint16_t* data)
        : pr32::core::Entity(px, py, POPUP_WIDTH, POPUP_HEIGHT, pr32::core::EntityType::GENERIC) {
        setRenderLayer(1);
        
        sprite.data = reinterpret_cast<const uint8_t*>(data);
        sprite.palette = POPUP_PALETTE;
        sprite.width = POPUP_WIDTH;
        sprite.height = POPUP_HEIGHT;
        sprite.paletteSize = static_cast<uint8_t>(sizeof(POPUP_PALETTE) / sizeof(Color));
    }

    void update(unsigned long) override {
    }

    void draw(pr32::graphics::Renderer& renderer) override {
        int drawX = static_cast<int>(position.x);
        int drawY = static_cast<int>(position.y);
        renderer.drawSprite(sprite, drawX, drawY, false);
    }

private:
    Sprite4bpp sprite;
};

#endif

}

void SpritesDemoScene::init() {
    pr32::graphics::setPalette(pr32::graphics::PaletteType::PR32);
    // 2BPP Sprite centered in the left half of the screen
    float px = (DISPLAY_WIDTH * 0.25f) - (SPRITE_WIDTH * 0.5f);
    float py = (DISPLAY_HEIGHT - SPRITE_HEIGHT) * 0.5f; 

    auto bg = std::make_unique<SpritesDemoBackground>();
    addEntity(bg.get());
    ownedEntities.push_back(std::move(bg));

    auto actor = std::make_unique<SpritesDemoActor>(px, py);
    addEntity(actor.get());
    ownedEntities.push_back(std::move(actor));

#ifdef PIXELROOT32_ENABLE_4BPP_SPRITES
    int numSprites = sizeof(SPRITES_4BPP_DATA) / sizeof(SPRITES_4BPP_DATA[0]);
    const int cols = 3;
    const int gap = 4;
    
    // Calculate grid dimensions
    int rows = (numSprites + cols - 1) / cols;
    float gridWidth = cols * POPUP_WIDTH + (cols - 1) * gap;
    float gridHeight = rows * POPUP_HEIGHT + (rows - 1) * gap;
    float startX = (DISPLAY_WIDTH * 0.75f) - (gridWidth * 0.5f);
    float startY = (DISPLAY_HEIGHT - gridHeight) * 0.5f;

    for (int i = 0; i < numSprites; ++i) {
        int col = i % cols;
        int row = i / cols;
        
        float popupX = startX + col * (POPUP_WIDTH + gap);
        float popupY = startY + row * (POPUP_HEIGHT + gap);
        
        auto popup = std::make_unique<SpritesPopupActor>(popupX, popupY, SPRITES_4BPP_DATA[i]);
        addEntity(popup.get());
        ownedEntities.push_back(std::move(popup));
    }
#endif
}

void SpritesDemoScene::update(unsigned long deltaTime) {
    pixelroot32::core::Scene::update(deltaTime);
}

void SpritesDemoScene::draw(pr32::graphics::Renderer& renderer) {
    pixelroot32::core::Scene::draw(renderer);
}

}

#endif // PIXELROOT32_ENABLE_2BPP_SPRITES || PIXELROOT32_ENABLE_4BPP_SPRITES
