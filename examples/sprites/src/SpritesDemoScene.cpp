#include "SpritesDemoScene.h"

#if defined(PIXELROOT32_ENABLE_2BPP_SPRITES) || defined(PIXELROOT32_ENABLE_4BPP_SPRITES)

#include "graphics/Renderer.h"
#include "core/Engine.h"
#include <cstdint>

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace spritesdemo {

namespace gfx = pr32::graphics;

using gfx::Color;
using gfx::Sprite2bpp;
using gfx::Sprite4bpp;

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


/**
 * @brief The scene behind the sprites: sky, sun, hills, chequered ground and a
 *        full 16-entry palette ramp.
 *
 * Every draw here goes through PaletteContext::Background, so switching palette
 * mode repaints this whole scene from one table while the sprites in front of it
 * repaint from another. The ramp at the bottom is the literal contents of the
 * background table, index 0 to 15, which is what makes the swap readable rather
 * than merely colourful.
 */
class SpritesDemoBackground : public pr32::core::Entity {
public:
    SpritesDemoBackground()
        : pr32::core::Entity(0.0f, 0.0f, DISPLAY_WIDTH, DISPLAY_HEIGHT, pr32::core::EntityType::GENERIC) {
        setRenderLayer(0);
    }

    void update(unsigned long) override {
    }

    void draw(pr32::graphics::Renderer& renderer) override {
        // Primitives default to PaletteContext::Sprite — drawing behind
        // everything does not make a draw "background". Only drawTileMap sets
        // this on its own. Without it the whole scene below would resolve
        // through the sprite table and dual mode would look like single mode.
        gfx::PaletteContext bgContext = gfx::PaletteContext::Background;
        gfx::PaletteContext* savedContext = renderer.getRenderContext();
        renderer.setRenderContext(&bgContext);

        drawSky(renderer);
        drawSun(renderer);
        drawHills(renderer);
        drawGround(renderer);
        drawPaletteRamp(renderer);

        // Restore before bgContext leaves scope — the renderer stores the
        // pointer, not the value.
        renderer.setRenderContext(savedContext);
    }

private:
    static constexpr int kHorizonY  = 140;  ///< Where sky ends and hills begin.
    static constexpr int kGroundY   = 190;  ///< Top of the chequered ground.
    static constexpr int kRampY     = 214;  ///< Top of the palette ramp.
    static constexpr int kRampH     = 16;

    /** Banded sky. Four indices, darkest at the top. */
    static void drawSky(gfx::Renderer& renderer) {
        static const Color kBands[] = { Color::Navy, Color::Navy, Color::Blue, Color::Cyan };
        constexpr int kBandCount = static_cast<int>(sizeof(kBands) / sizeof(kBands[0]));
        const int bandH = kHorizonY / kBandCount;
        for (int i = 0; i < kBandCount; ++i) {
            const int h = (i == kBandCount - 1) ? (kHorizonY - i * bandH) : bandH;
            renderer.drawFilledRectangle(0, i * bandH, DISPLAY_WIDTH, h, kBands[i]);
        }
    }

    /** Sun with a ring, so two warm indices sit next to each other. */
    static void drawSun(gfx::Renderer& renderer) {
        constexpr int cx = 196;
        constexpr int cy = 34;
        renderer.drawFilledCircle(cx, cy, 18, Color::Yellow);
        renderer.drawCircle(cx, cy, 20, Color::Orange);
        renderer.drawCircle(cx, cy, 23, Color::Orange);
    }

    /** Three stepped hill bands between the horizon and the ground. */
    static void drawHills(gfx::Renderer& renderer) {
        static const Color kHills[] = { Color::DarkGreen, Color::Green, Color::LightGreen };
        constexpr int kHillCount = static_cast<int>(sizeof(kHills) / sizeof(kHills[0]));
        const int bandH = (kGroundY - kHorizonY) / kHillCount;
        for (int i = 0; i < kHillCount; ++i) {
            const int y = kHorizonY + i * bandH;
            const int h = (i == kHillCount - 1) ? (kGroundY - y) : bandH;
            renderer.drawFilledRectangle(0, y, DISPLAY_WIDTH, h, kHills[i]);
            // A row of humps along the top edge of each band breaks the
            // horizontal banding into something that reads as terrain.
            for (int x = (i * 12); x < DISPLAY_WIDTH; x += 48) {
                renderer.drawFilledCircle(x + 12, y, 10, kHills[i]);
            }
        }
    }

    /**
     * Chequerboard, so two indices alternate at high frequency.
     *
     * DarkRed and Purple, not DarkRed and Brown: `Brown` is an alias of
     * `DarkRed` (Color.h), so that pair would resolve to one index and paint a
     * flat block in every palette. Aliases are the easy way to write a demo
     * that silently shows nothing.
     */
    static void drawGround(gfx::Renderer& renderer) {
        constexpr int kCell = 12;
        for (int y = kGroundY; y < kRampY; y += kCell) {
            for (int x = 0; x < DISPLAY_WIDTH; x += kCell) {
                const bool even = (((x / kCell) + (y / kCell)) % 2) == 0;
                const int h = (y + kCell > kRampY) ? (kRampY - y) : kCell;
                renderer.drawFilledRectangle(x, y, kCell, h,
                                             even ? Color::DarkRed : Color::Purple);
            }
        }
    }

    /**
     * The background table itself, index 0 to 15 left to right.
     *
     * This is the readable part of the demo: the ramp is the table, so a mode
     * switch shows exactly which entries moved and which did not.
     */
    static void drawPaletteRamp(gfx::Renderer& renderer) {
        constexpr int kEntries = 16;
        const int cellW = DISPLAY_WIDTH / kEntries;
        for (int i = 0; i < kEntries; ++i) {
            renderer.drawFilledRectangle(i * cellW, kRampY, cellW, kRampH,
                                         static_cast<Color>(i));
        }
        // Index 0 is black in most tables and would vanish into the frame.
        renderer.drawRectangle(0, kRampY, DISPLAY_WIDTH, kRampH, Color::Gray);
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

    void draw(gfx::Renderer& renderer) override {
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

    void draw(gfx::Renderer& renderer) override {
        int drawX = static_cast<int>(position.x);
        int drawY = static_cast<int>(position.y);
        renderer.drawSprite(sprite, drawX, drawY, false);
    }

private:
    Sprite4bpp sprite;
};

#endif

}

void SpritesDemoScene::applyPaletteMode() {
    switch (paletteMode) {
        case PaletteMode::Single:
            // One table: a colour index means the same RGB wherever it is drawn.
            // setPalette also writes the legacy single-palette pointer, which is
            // what resolveColor(Color) reads, so this genuinely leaves dual mode
            // rather than just disabling the branch.
            gfx::enableDualPaletteMode(false);
            gfx::setPalette(gfx::PaletteType::PR32);
            break;

        case PaletteMode::Dual:
            // Two tables: the same index resolves to different RGB depending on
            // whether the pixel came from a background draw or from a sprite.
            gfx::enableDualPaletteMode(true);
            gfx::setDualPalette(gfx::PaletteType::NES, gfx::PaletteType::GB);
            break;

        case PaletteMode::DualInverted:
            // The same two tables with the roles swapped. Worth its own mode:
            // it shows the split is a property of *where a pixel came from*,
            // not of the palettes themselves. Nothing about NES makes it a
            // background table.
            gfx::enableDualPaletteMode(true);
            gfx::setDualPalette(gfx::PaletteType::GB, gfx::PaletteType::NES);
            break;

        case PaletteMode::Count:
            break;  // Not a mode; listed so the switch stays exhaustive.
    }
}

const char* SpritesDemoScene::paletteModeLabel() const {
    switch (paletteMode) {
        case PaletteMode::Dual:         return "A: dual  BG:NES SPR:GB";
        case PaletteMode::DualInverted: return "A: dual' BG:GB  SPR:NES";
        case PaletteMode::Single:
        case PaletteMode::Count:
        default:                        return "A: single  PR32";
    }
}

void SpritesDemoScene::init() {
    applyPaletteMode();
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
    pr32::core::Scene::update(deltaTime);

    // Button 4 (A / Space) cycles the palette tables. isButtonPressed is
    // edge-triggered, so holding the key does not spin through modes.
    if (engine.getInputManager().isButtonPressed(4)) {
        const uint8_t next = static_cast<uint8_t>(
            (static_cast<uint8_t>(paletteMode) + 1) % static_cast<uint8_t>(PaletteMode::Count));
        paletteMode = static_cast<PaletteMode>(next);
        applyPaletteMode();
    }
}

void SpritesDemoScene::draw(gfx::Renderer& renderer) {
    pr32::core::Scene::draw(renderer);

    // Drawn in the default sprite context on purpose: in the two dual modes the
    // label recolours with the sprites, which is a second, smaller read on which
    // table is currently feeding them.
    renderer.drawText(paletteModeLabel(), 4, DISPLAY_HEIGHT - 9, Color::White, 1);
}

}

#endif // PIXELROOT32_ENABLE_2BPP_SPRITES || PIXELROOT32_ENABLE_4BPP_SPRITES
