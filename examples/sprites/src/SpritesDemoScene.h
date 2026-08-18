#pragma once
#include "platforms/PlatformDefaults.h"

#if defined(PIXELROOT32_ENABLE_2BPP_SPRITES) || defined(PIXELROOT32_ENABLE_4BPP_SPRITES)

#include "core/Scene.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "platforms/EngineConfig.h"
#include "assets/Sprites.h"
#include "assets/SpritesPopup.h"

#include <cstdint>
#include <vector>
#include <memory>

namespace spritesdemo {

/**
 * @file SpritesDemoScene.h
 * @class SpritesDemoScene
 * @brief Demo of 2BPP (4 colors) and 4BPP (16 colors) sprites, in single- and dual-palette mode.
 *
 * Press A to switch palette mode. The scene draws background swatches and
 * sprites from the same colour indices, so the two modes differ on screen:
 * single mode resolves both through one table, dual mode resolves background
 * pixels through the background table and sprite pixels through the sprite one.
 */
class SpritesDemoScene : public pixelroot32::core::Scene {
public:
    void init() override;

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /** @brief Palette tables the demo cycles through with the A button. */
    enum class PaletteMode : uint8_t {
        Single = 0,        ///< One table (PR32) for background and sprites alike.
        Dual = 1,          ///< NES for background draws, GB for sprite draws.
        DualInverted = 2,  ///< The same two tables, swapped: GB background, NES sprites.
        Count = 3          ///< Cycle length. Not a mode.
    };

    /** @return Label for the current mode, drawn over the scene. */
    const char* paletteModeLabel() const;

private:
    /** @brief Pushes paletteMode to the renderer's colour tables. */
    void applyPaletteMode();

    std::vector<std::unique_ptr<pixelroot32::core::Entity>> ownedEntities;  ///< Owned entities
    PaletteMode paletteMode = PaletteMode::Single;
};

}

#endif // PIXELROOT32_ENABLE_2BPP_SPRITES || PIXELROOT32_ENABLE_4BPP_SPRITES
