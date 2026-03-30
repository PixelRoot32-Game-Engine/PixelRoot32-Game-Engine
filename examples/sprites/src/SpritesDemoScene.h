#pragma once
#include "platforms/PlatformDefaults.h"

#if defined(PIXELROOT32_ENABLE_2BPP_SPRITES) || defined(PIXELROOT32_ENABLE_4BPP_SPRITES)

#include "core/Scene.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "platforms/EngineConfig.h"
#include "assets/Sprites.h"
#include "assets/SpritesPopup.h"

#include <vector>
#include <memory>

namespace spritesdemo {

/**
 * @file SpritesDemoScene.h
 * @class SpritesDemoScene
 * @brief Demo of 2BPP (4 colors) and 4BPP (16 colors) sprites.
 */
class SpritesDemoScene : public pixelroot32::core::Scene {
public:
    void init() override;

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    std::vector<std::unique_ptr<pixelroot32::core::Entity>> ownedEntities;  ///< Owned entities
};

}

#endif // PIXELROOT32_ENABLE_2BPP_SPRITES || PIXELROOT32_ENABLE_4BPP_SPRITES
