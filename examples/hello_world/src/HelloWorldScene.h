#pragma once

#include <core/Scene.h>
#include <graphics/Renderer.h>
#include <graphics/Color.h>
#include <graphics/ui/UILabel.h>
#include <math/Vector2.h>
#include <memory>
#include <graphics/PartialUpdateController.h>

namespace helloworld {

class HelloWorldScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> textLabel;
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> buttonPressLabel;
    pixelroot32::graphics::Color backgroundColor = pixelroot32::graphics::Color::Black;
    pixelroot32::graphics::Color textColor = pixelroot32::graphics::Color::White;
    int frameCounter = 0;
    static constexpr int COLOR_CHANGE_INTERVAL = 60;
    char buttonPressText[32];
    
    // Benchmarking variables
    unsigned long benchmarkFrameCounter = 0;
    static constexpr int BENCHMARK_INTERVAL = 60; // Report every 60 frames (~2 seconds)
    int lastRegionCount = 0;
    int lastTotalSentPixels = 0;
    int totalRegionCount = 0;
    int totalSentPixels = 0;
    int framesWithOptimization = 0;
    int framesWithoutOptimization = 0;

    void changeBackground();
    void checkButtonPress();
    void reportBenchmarkStats(pixelroot32::graphics::Renderer& renderer);
};

} // namespace helloworld
