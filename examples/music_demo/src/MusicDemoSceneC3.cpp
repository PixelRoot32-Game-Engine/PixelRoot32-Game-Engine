#include "MusicDemoConstants.h"
#include "MusicDemoSceneC3.h"
#include "assets/classic_arcade_melody.h"
#include "assets/adventure_melody.h"
#include "assets/action_melody.h"
#include "assets/arpeggio_melody.h"

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

static MusicDemoSceneC3* sSceneInstance = nullptr;

void MusicDemoSceneC3::init() {
    clearEntities();
    pr32::graphics::setPalette(pr32::graphics::PaletteType::PR32);
    int sw = engine.getRenderer().getLogicalWidth();
    int sh = engine.getRenderer().getLogicalHeight();
    sSceneInstance = this;

    label = std::make_unique<pr32::graphics::ui::UILabel>("Music Demo", Vector2(0, static_cast<int>(sh/2)), Color::White, TITLE_FONT_SIZE);
    label->centerX(sw); 
    label->setRenderLayer(2); 
    addEntity(label.get());

    // Random melody index
    // 1: Classic Arcade
    // 2: Adventure
    // 3: Action
    // 4: Arpeggio
    int melodyIndex = 4;

    playMelody(melodyIndex);
}

void MusicDemoSceneC3::update(unsigned long dt) { Scene::update(dt); }

void MusicDemoSceneC3::draw(pr32::graphics::Renderer& r) { Scene::draw(r); }

void MusicDemoSceneC3::playMelody(int idx) {
    auto& player = engine.getMusicPlayer();
    player.setMasterVolume(1.0f);

    if (player.isPlaying()) {
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
} // namespace musicdemo