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

enum class MusicDemoState {
    MAIN,
    INSTRUMENT_PRESET,
    MELODIES,
    AUDIO_LAB
};

class MusicDemoScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    // Public methods for callbacks
    void showMenu(MusicDemoState state);
    void playInstrumentSound(const pixelroot32::audio::InstrumentPreset& preset);
    void playMelody(int melodyIndex);

    void playSweepDemo();
    void playSineChordDemo();
    void playSawChordDemo();
    void cycleMasterBitcrush();

private:
    int currentMelodyIndex = -1;
    int previousMelodyIndex = -1;
    uint8_t bitcrushCycleIndex_ = 0;
    
    MusicDemoState currentState = MusicDemoState::MAIN;
    
    // UI Elements
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> titleLabel;
    std::unique_ptr<pixelroot32::graphics::ui::UIVerticalLayout> buttonLayout;

    // Main menu buttons
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> instrumentPresetButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> melodiesButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> audioLabButton;

    // Instrument preset buttons (10 total)
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> instrLeadButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> instrHarmonyButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> instrBassButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> instrKickButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> instrSnareButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> instrHihatButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> instrTriangleLeadButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> instrTrianglePadButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> instrPulsePadButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> instrPulseBassButton;

    std::unique_ptr<pixelroot32::graphics::ui::UIButton> melody1Button;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> melody2Button;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> melody3Button;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> melody4Button;

    // Audio lab (Phase A/B): sweep, extra waves, bitcrush
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> audioLabSweepButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> audioLabSineButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> audioLabSawButton;
    std::unique_ptr<pixelroot32::graphics::ui::UIButton> audioLabBitcrushButton;

    // Navigation labels
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> lblNavigate;
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> lblSelect;
    std::unique_ptr<pixelroot32::graphics::ui::UILabel> lblBack;
    
    // Helper methods
    void setupMainMenu();
    void setupInstrumentPresetMenu();
    void setupMelodiesMenu();
    void setupAudioLabMenu();
    void goBack();
};

} // namespace musicdemo