#pragma once

#include "platforms/PlatformDefaults.h"
#include "core/Scene.h"
#include "core/Log.h"
#include "graphics/ui/UILabel.h"
#include "graphics/ui/UIButton.h"
#include "graphics/ui/UIVerticalLayout.h"
#include <audio/AudioMusicTypes.h>
#include <audio/AudioTypes.h>

#include <memory>

namespace musicdemo {

class MusicDemoSceneC3 : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    void playMelody(int melodyIndex);

private:
    uint8_t bitcrushCycleIndex_ = 0;
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> label;
};

} // namespace musicdemo