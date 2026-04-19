#include <audio/AudioTypes.h>
#include <audio/AudioMusicTypes.h>

// Base four-note bass pattern: "tu tu tu tu"
static const pixelroot32::audio::InstrumentPreset BASS_INSTRUMENT = pixelroot32::audio::INSTR_TRIANGLE_BASS;

static const pixelroot32::audio::MusicNote BGM_SLOW_NOTES[] = {
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.21f),
    pixelroot32::audio::makeRest(0.207f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.21f),
    pixelroot32::audio::makeRest(0.207f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.21f),
    pixelroot32::audio::makeRest(0.207f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.21f),
    pixelroot32::audio::makeRest(0.207f),
};

static const pixelroot32::audio::MusicNote BGM_MEDIUM_NOTES[] = {
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.12f),
    pixelroot32::audio::makeRest(0.06f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.12f),
    pixelroot32::audio::makeRest(0.06f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.12f),
    pixelroot32::audio::makeRest(0.06f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.12f),
    pixelroot32::audio::makeRest(0.06f),
};

static const pixelroot32::audio::MusicNote BGM_FAST_NOTES[] = {
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.08f),
    pixelroot32::audio::makeRest(0.04f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.08f),
    pixelroot32::audio::makeRest(0.04f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.08f),
    pixelroot32::audio::makeRest(0.04f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.08f),
    pixelroot32::audio::makeRest(0.04f),
};

static const pixelroot32::audio::MusicTrack BGM_SLOW_TRACK = {
    BGM_SLOW_NOTES,
    sizeof(BGM_SLOW_NOTES) / sizeof(pixelroot32::audio::MusicNote),
    true,
    pixelroot32::audio::WaveType::PULSE,
    BASS_INSTRUMENT.duty
};

static const pixelroot32::audio::MusicTrack BGM_MEDIUM_TRACK = {
    BGM_MEDIUM_NOTES,
    sizeof(BGM_MEDIUM_NOTES) / sizeof(pixelroot32::audio::MusicNote),
    true,
    pixelroot32::audio::WaveType::PULSE,
    BASS_INSTRUMENT.duty
};

static const pixelroot32::audio::MusicTrack BGM_FAST_TRACK = {
    BGM_FAST_NOTES,
    sizeof(BGM_FAST_NOTES) / sizeof(pixelroot32::audio::MusicNote),
    true,
    pixelroot32::audio::WaveType::PULSE,
    BASS_INSTRUMENT.duty
};

// --- WIN / GAME OVER MUSIC ---

static const pixelroot32::audio::MusicNote WIN_NOTES[] = {
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.15f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::E, 0.15f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::G, 0.15f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.4f), // C High ideally, but using C for safety if C_High undefined
    pixelroot32::audio::makeRest(0.1f)
};

static const pixelroot32::audio::MusicTrack WIN_TRACK = {
    WIN_NOTES,
    sizeof(WIN_NOTES) / sizeof(pixelroot32::audio::MusicNote),
    false, // No loop
    pixelroot32::audio::WaveType::PULSE,
    BASS_INSTRUMENT.duty
};

static const pixelroot32::audio::MusicNote GAME_OVER_NOTES[] = {
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::G, 0.2f),
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::E, 0.2f), // Using E instead of Eb if Eb undefined, checking later
    pixelroot32::audio::makeNote(BASS_INSTRUMENT, pixelroot32::audio::Note::C, 0.4f),
    pixelroot32::audio::makeRest(0.1f)
};

static const pixelroot32::audio::MusicTrack GAME_OVER_TRACK = {
    GAME_OVER_NOTES,
    sizeof(GAME_OVER_NOTES) / sizeof(pixelroot32::audio::MusicNote),
    false, // No loop
    pixelroot32::audio::WaveType::PULSE,
    BASS_INSTRUMENT.duty
};
