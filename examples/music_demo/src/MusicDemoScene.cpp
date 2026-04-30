#include "MusicDemoConstants.h"
#include "MusicDemoScene.h"
#include "assets/melodies.h"
#include <core/Engine.h>
#include <cstdio>

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace musicdemo {

using Color = pr32::graphics::Color;
using Vector2 = pr32::math::Vector2;
using WaveType = pr32::audio::WaveType;
using Note = pr32::audio::Note;
using AudioEvent = pr32::audio::AudioEvent;
using MusicNote = pr32::audio::MusicNote;
using MusicTrack = pr32::audio::MusicTrack;

static MusicDemoScene* sSceneInstance = nullptr;

static void onInstrumentPreset() { if (sSceneInstance) sSceneInstance->showMenu(MusicDemoState::INSTRUMENT_PRESET); }
static void onMelodies() { if (sSceneInstance) sSceneInstance->showMenu(MusicDemoState::MELODIES); }
static void onLead() { if (sSceneInstance) sSceneInstance->playInstrumentSound(pr32::audio::INSTR_PULSE_LEAD); }
static void onHarmony() { if (sSceneInstance) sSceneInstance->playInstrumentSound(pr32::audio::INSTR_PULSE_HARMONY); }
static void onBass() { if (sSceneInstance) sSceneInstance->playInstrumentSound(pr32::audio::INSTR_TRIANGLE_BASS); }
static void onKick() { if (sSceneInstance) sSceneInstance->playInstrumentSound(pr32::audio::INSTR_KICK); }
static void onSnare() { if (sSceneInstance) sSceneInstance->playInstrumentSound(pr32::audio::INSTR_SNARE); }
static void onHihat() { if (sSceneInstance) sSceneInstance->playInstrumentSound(pr32::audio::INSTR_HIHAT); }
static void onTriangleLead() { if (sSceneInstance) sSceneInstance->playInstrumentSound(pr32::audio::INSTR_TRIANGLE_LEAD); }
static void onTrianglePad() { if (sSceneInstance) sSceneInstance->playInstrumentSound(pr32::audio::INSTR_TRIANGLE_PAD); }
static void onPulsePad() { if (sSceneInstance) sSceneInstance->playInstrumentSound(pr32::audio::INSTR_PULSE_PAD); }
static void onPulseBass() { if (sSceneInstance) sSceneInstance->playInstrumentSound(pr32::audio::INSTR_PULSE_BASS); }

static void onMelody1() { if (sSceneInstance) sSceneInstance->playMelody(1); }
static void onMelody2() { if (sSceneInstance) sSceneInstance->playMelody(2); }
static void onMelody3() { if (sSceneInstance) sSceneInstance->playMelody(3); }
static void onMelody4() { if (sSceneInstance) sSceneInstance->playMelody(4); }

static void onAudioLab() { if (sSceneInstance) sSceneInstance->showMenu(MusicDemoState::AUDIO_LAB); }
static void onSweepDemo() { if (sSceneInstance) sSceneInstance->playSweepDemo(); }
static void onSineChord() { if (sSceneInstance) sSceneInstance->playSineChordDemo(); }
static void onSawChord() { if (sSceneInstance) sSceneInstance->playSawChordDemo(); }
static void onBitcrushCycle() { if (sSceneInstance) sSceneInstance->cycleMasterBitcrush(); }


void MusicDemoScene::init() {
    clearEntities();
    pr32::graphics::setPalette(pr32::graphics::PaletteType::PR32);
    int sw = engine.getRenderer().getLogicalWidth();
    int sh = engine.getRenderer().getLogicalHeight();
    sSceneInstance = this;
    
    titleLabel = std::make_unique<pr32::graphics::ui::UILabel>("Music Demo", Vector2(0, static_cast<int>(TITLE_Y)), Color::White, TITLE_FONT_SIZE);
    titleLabel->centerX(sw); 
    titleLabel->setRenderLayer(2); 
    addEntity(titleLabel.get());

    buttonLayout = std::make_unique<pr32::graphics::ui::UIVerticalLayout>(static_cast<int>((sw - BTN_WIDTH)/2), static_cast<int>(BTN_START_Y), static_cast<int>(BTN_WIDTH), static_cast<int>(sh - BTN_START_Y - NAV_INSTR_Y_OFFSET - 10));
    buttonLayout->setPadding(0); 
    buttonLayout->setSpacing(static_cast<int>(BTN_GAP));
    buttonLayout->setScrollEnabled(true); 
    buttonLayout->setNavigationButtons(BTN_NAV_UP, BTN_NAV_DOWN);
    buttonLayout->setButtonStyle(Color::White, Color::Cyan, Color::White, Color::Black);
    buttonLayout->setRenderLayer(2); 
    addEntity(buttonLayout.get());

    setupMainMenu(); 
    setupInstrumentPresetMenu(); 
    setupMelodiesMenu();
    setupAudioLabMenu();

    lblNavigate = std::make_unique<pr32::graphics::ui::UILabel>("UP/DOWN: Navigate", Vector2(0, sh - static_cast<int>(NAV_INSTR_Y_OFFSET)), Color::Cyan, INSTRUCTION_FONT_SIZE);
    lblNavigate->centerX(sw); 
    lblNavigate->setRenderLayer(2);
    addEntity(lblNavigate.get());

    lblSelect = std::make_unique<pr32::graphics::ui::UILabel>("A: Select", Vector2(0, sh - static_cast<int>(SEL_INSTR_Y_OFFSET)), Color::Cyan, INSTRUCTION_FONT_SIZE);
    lblSelect->centerX(sw); 
    lblSelect->setRenderLayer(2); 
    addEntity(lblSelect.get());
    
    lblBack = std::make_unique<pr32::graphics::ui::UILabel>("B: Back", Vector2(0, sh - 15), Color::Cyan, INSTRUCTION_FONT_SIZE);
    lblBack->centerX(sw);
    lblBack->setRenderLayer(2); 
    addEntity(lblBack.get());

    currentState = MusicDemoState::MAIN; showMenu(MusicDemoState::MAIN);
}

void MusicDemoScene::update(unsigned long dt) {
    Scene::update(dt);

    auto& input = engine.getInputManager();
    static bool wasBack = false;
    bool isBackPressed = input.isButtonPressed(BTN_BACK); // B button
    if(isBackPressed && !wasBack) { 
        auto& player = engine.getMusicPlayer();
        if (player.isPlaying()) { player.stop(); }
        goBack(); 
    }
    wasBack = isBackPressed;

    static int lastIdx = -1;
    buttonLayout->handleInput(input);

    int newIdx = buttonLayout->getSelectedIndex();
    if (newIdx != lastIdx && newIdx >= 0) { lastIdx = newIdx; }
}

void MusicDemoScene::draw(pr32::graphics::Renderer& r) { Scene::draw(r); }

void MusicDemoScene::setupMainMenu() {
    instrumentPresetButton = std::make_unique<pr32::graphics::ui::UIButton>("INSTRUMENT PRESET", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onInstrumentPreset, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    melodiesButton = std::make_unique<pr32::graphics::ui::UIButton>("MELODIES", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onMelodies, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    audioLabButton = std::make_unique<pr32::graphics::ui::UIButton>("AUDIO LAB (SWEEP)", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onAudioLab, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
}

void MusicDemoScene::setupInstrumentPresetMenu() {
    instrLeadButton = std::make_unique<pr32::graphics::ui::UIButton>("PULSE LEAD", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onLead, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    instrHarmonyButton = std::make_unique<pr32::graphics::ui::UIButton>("PULSE HARMONY", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onHarmony, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    instrBassButton = std::make_unique<pr32::graphics::ui::UIButton>("TRIANGLE BASS", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onBass, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    instrKickButton = std::make_unique<pr32::graphics::ui::UIButton>("KICK", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onKick, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    instrSnareButton = std::make_unique<pr32::graphics::ui::UIButton>("SNARE", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onSnare, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    instrHihatButton = std::make_unique<pr32::graphics::ui::UIButton>("HI-HAT", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onHihat, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    instrTriangleLeadButton = std::make_unique<pr32::graphics::ui::UIButton>("TRIANGLE LEAD", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onTriangleLead, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    instrTrianglePadButton = std::make_unique<pr32::graphics::ui::UIButton>("TRIANGLE PAD", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onTrianglePad, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    instrPulsePadButton = std::make_unique<pr32::graphics::ui::UIButton>("PULSE PAD", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onPulsePad, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    instrPulseBassButton = std::make_unique<pr32::graphics::ui::UIButton>("PULSE BASS", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onPulseBass, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
}

void MusicDemoScene::setupMelodiesMenu() {
    melody1Button = std::make_unique<pr32::graphics::ui::UIButton>("Melody 1", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onMelody1, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    melody2Button = std::make_unique<pr32::graphics::ui::UIButton>("Melody 2", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onMelody2, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    melody3Button = std::make_unique<pr32::graphics::ui::UIButton>("Melody 3", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onMelody3, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    melody4Button = std::make_unique<pr32::graphics::ui::UIButton>("Melody 4 + ARP voice", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onMelody4, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
}

void MusicDemoScene::setupAudioLabMenu() {
    audioLabSweepButton = std::make_unique<pr32::graphics::ui::UIButton>("PULSE SWEEP (A)", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onSweepDemo, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    audioLabSineButton = std::make_unique<pr32::graphics::ui::UIButton>("SINE CHORD (B)", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onSineChord, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    audioLabSawButton = std::make_unique<pr32::graphics::ui::UIButton>("SAW CHORD (B)", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onSawChord, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
    audioLabBitcrushButton = std::make_unique<pr32::graphics::ui::UIButton>("MASTER BITCRUSH", BTN_SELECT, Vector2::ZERO(), Vector2{static_cast<int>(BTN_WIDTH), static_cast<int>(BTN_HEIGHT)}, onBitcrushCycle, pr32::graphics::ui::TextAlignment::CENTER, BTN_FONT_SIZE);
}

void MusicDemoScene::showMenu(MusicDemoState st) {
    currentState = st; buttonLayout->clearElements();
    int sw = engine.getRenderer().getLogicalWidth();
    switch (st) {
        case MusicDemoState::MAIN: 
            titleLabel->setText("Music Demo"); 
            buttonLayout->addElement(instrumentPresetButton.get()); 
            buttonLayout->addElement(melodiesButton.get()); 
            buttonLayout->addElement(audioLabButton.get());
            break;
        case MusicDemoState::INSTRUMENT_PRESET:
            titleLabel->setText("Instruments (10)");
            buttonLayout->addElement(instrLeadButton.get());
            buttonLayout->addElement(instrHarmonyButton.get());
            buttonLayout->addElement(instrBassButton.get());
            buttonLayout->addElement(instrPulseBassButton.get());
            buttonLayout->addElement(instrTriangleLeadButton.get());
            buttonLayout->addElement(instrTrianglePadButton.get());
            buttonLayout->addElement(instrPulsePadButton.get());
            buttonLayout->addElement(instrKickButton.get());
            buttonLayout->addElement(instrSnareButton.get());
            buttonLayout->addElement(instrHihatButton.get());
            break;
        case MusicDemoState::MELODIES: 
            titleLabel->setText("Melodies (4)");
            buttonLayout->addElement(melody1Button.get());
            buttonLayout->addElement(melody2Button.get());
            buttonLayout->addElement(melody3Button.get());
            buttonLayout->addElement(melody4Button.get());
            break;
        case MusicDemoState::AUDIO_LAB:
            bitcrushCycleIndex_ = 0;
            engine.getAudioEngine().setMasterBitcrush(0);
            titleLabel->setText("Audio Lab (A/B)");
            buttonLayout->addElement(audioLabSweepButton.get());
            buttonLayout->addElement(audioLabSineButton.get());
            buttonLayout->addElement(audioLabSawButton.get());
            buttonLayout->addElement(audioLabBitcrushButton.get());
            break;
    }
    titleLabel->centerX(sw);
}

void MusicDemoScene::goBack() {
    if (currentState == MusicDemoState::MAIN) {
        return;
    }
    if (currentState == MusicDemoState::AUDIO_LAB) {
        engine.getAudioEngine().setMasterBitcrush(0);
        bitcrushCycleIndex_ = 0;
    }
    showMenu(MusicDemoState::MAIN);
}

void MusicDemoScene::playInstrumentSound(const pr32::audio::InstrumentPreset& preset) {
    auto& audio = engine.getAudioEngine();
    
    // Percussion (duty == 0): use instrumentToFrequency and defaultDuration from preset
    if (preset.duty == 0.0f) {
        WaveType wt = WaveType::NOISE;
        float freq = pr32::audio::instrumentToFrequency(preset, Note::C, 4);
        float dur = (preset.defaultDuration > 0.0f) ? preset.defaultDuration : 0.15f;
        AudioEvent ev{wt, freq, dur, preset.baseVolume, 0.0f, preset.noisePeriod};
        audio.playEvent(ev);
    } else {
        // Melodic: play musical interval (chord)
        Note notes[] = {Note::C, Note::E, Note::G, Note::C};
        uint8_t octaves[] = {preset.defaultOctave, preset.defaultOctave, preset.defaultOctave, static_cast<uint8_t>(preset.defaultOctave + 1)};
        WaveType wt = (preset.duty >= 0.4f) ? WaveType::TRIANGLE : WaveType::PULSE;
        for (int i = 0; i < 4; i++) { 
            AudioEvent ev{wt, pr32::audio::noteToFrequency(notes[i], octaves[i]), 0.15f, preset.baseVolume, preset.duty, 0}; 
            audio.playEvent(ev); 
            delay(150); 
        }
    }
}

void MusicDemoScene::playMelody(int idx) {
    auto& player = engine.getMusicPlayer();

    previousMelodyIndex = currentMelodyIndex;
    currentMelodyIndex = idx;
    bool isSameMelody = (previousMelodyIndex == currentMelodyIndex);

    if (isSameMelody && player.isPlaying()) {
        player.stop();
        return;
    }

    switch (idx) {
        case 1: {
            player.setBPM(140.0f);
            player.play(sClassicArcadeTrack);
            break;
        }
        case 2: {
            player.setBPM(125.0f);
            player.play(sAdventureTrack);
            break;
        }
        case 3: {
            player.setBPM(160.0f);
            player.play(sActionTrack);
            break;
        }
        case 4: {
            player.setBPM(145.0f);
            player.play(sArpDemoTrack);
            break;
        }
    }
}

void MusicDemoScene::playSweepDemo() {
    auto& audio = engine.getAudioEngine();
    AudioEvent ev{};
    ev.type = WaveType::PULSE;
    ev.frequency = 1400.0f;
    ev.sweepEndHz = 220.0f;
    ev.sweepDurationSec = 0.22f;
    ev.duration = 0.38f;
    ev.volume = 0.65f;
    ev.duty = 0.5f;
    audio.playEvent(ev);
}

void MusicDemoScene::playSineChordDemo() {
    auto& audio = engine.getAudioEngine();
    const Note chord[] = {Note::C, Note::E, Note::G};
    for (unsigned i = 0; i < 3; i++) {
        AudioEvent ev{};
        ev.type = WaveType::SINE;
        ev.frequency = pr32::audio::noteToFrequency(chord[i], 4);
        ev.duration = 0.22f;
        ev.volume = 0.5f;
        ev.duty = 0.5f;
        audio.playEvent(ev);
    }
}

void MusicDemoScene::playSawChordDemo() {
    auto& audio = engine.getAudioEngine();
    const Note chord[] = {Note::A, Note::Cs, Note::E};
    for (unsigned i = 0; i < 3; i++) {
        AudioEvent ev{};
        ev.type = WaveType::SAW;
        ev.frequency = pr32::audio::noteToFrequency(chord[i], 4);
        ev.duration = 0.2f;
        ev.volume = 0.45f;
        ev.duty = 0.5f;
        audio.playEvent(ev);
    }
}


void MusicDemoScene::cycleMasterBitcrush() {
    static const uint8_t kLevels[] = {0, 6, 10, 14};
    bitcrushCycleIndex_ = static_cast<uint8_t>((bitcrushCycleIndex_ + 1) % 4);
    const uint8_t bits = kLevels[bitcrushCycleIndex_];
    engine.getAudioEngine().setMasterBitcrush(bits);
    static char titleBuf[48];
    std::snprintf(titleBuf, sizeof(titleBuf), "Audio Lab (crush %u)", static_cast<unsigned>(bits));
    titleLabel->setText(titleBuf);
}

} // namespace musicdemo