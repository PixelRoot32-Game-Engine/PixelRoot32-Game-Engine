/**
 * @file DualPaletteTestScene.h
 */
#pragma once
#include <core/Scene.h>
#include <graphics/Renderer.h>
#include <platforms/EngineConfig.h>

#include <memory>

namespace dualpalettetest {

/**
 * @class DualPaletteTestScene
 * @brief Demo of dual palette mode (background vs sprite palettes).
 *
 * Background layer uses NES palette; sprite layer uses GB palette.
 * Same color index can map to different RGB per layer.
 */
class DualPaletteTestScene : public pixelroot32::core::Scene {
public:
    DualPaletteTestScene();
    ~DualPaletteTestScene();
    
    void init() override;

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    class TestBackground;
    class TestSprite;
    
    std::unique_ptr<TestBackground> background;
    std::unique_ptr<TestSprite> testSprite;
};

}
