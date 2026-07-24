#include <unity.h>
#include "audio/AudioMusicTypes.h"

using namespace pixelroot32::audio;

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// Note Enum Value Tests - verify all Note values exist and are valid
// ============================================================================

void test_note_enum_values(void) {
    // Test all Note enum values are accessible
    TEST_ASSERT_EQUAL(0, static_cast<uint8_t>(Note::C));
    TEST_ASSERT_EQUAL(1, static_cast<uint8_t>(Note::Cs));
    TEST_ASSERT_EQUAL(2, static_cast<uint8_t>(Note::D));
    TEST_ASSERT_EQUAL(3, static_cast<uint8_t>(Note::Ds));
    TEST_ASSERT_EQUAL(4, static_cast<uint8_t>(Note::E));
    TEST_ASSERT_EQUAL(5, static_cast<uint8_t>(Note::F));
    TEST_ASSERT_EQUAL(6, static_cast<uint8_t>(Note::Fs));
    TEST_ASSERT_EQUAL(7, static_cast<uint8_t>(Note::G));
    TEST_ASSERT_EQUAL(8, static_cast<uint8_t>(Note::Gs));
    TEST_ASSERT_EQUAL(9, static_cast<uint8_t>(Note::A));
    TEST_ASSERT_EQUAL(10, static_cast<uint8_t>(Note::As));
    TEST_ASSERT_EQUAL(11, static_cast<uint8_t>(Note::B));
    TEST_ASSERT_EQUAL(12, static_cast<uint8_t>(Note::Rest));
    TEST_ASSERT_EQUAL(13, static_cast<uint8_t>(Note::COUNT));
}

// ============================================================================
// noteToFrequency - Value Tests (covering all branches)
// ============================================================================

// Test: higher octave (shift > 0)
void test_note_to_frequency_octave_5(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 523.25f, noteToFrequency(Note::C, 5));  // C5
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 587.33f, noteToFrequency(Note::D, 5));  // D5
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 659.25f, noteToFrequency(Note::E, 5));  // E5
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 698.46f, noteToFrequency(Note::F, 5));  // F5
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 783.99f, noteToFrequency(Note::G, 5));  // G5
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 880.00f, noteToFrequency(Note::A, 5));  // A5
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 987.77f, noteToFrequency(Note::B, 5));  // B5
}

void test_note_to_frequency_octave_6(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1046.50f, noteToFrequency(Note::C, 6)); // C6
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1174.66f, noteToFrequency(Note::D, 6)); // D6
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1318.51f, noteToFrequency(Note::E, 6)); // E6
}

// Test: lower octave (shift < 0)
void test_note_to_frequency_octave_3(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 130.81f, noteToFrequency(Note::C, 3));  // C3
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 146.83f, noteToFrequency(Note::D, 3));  // D3
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 164.81f, noteToFrequency(Note::E, 3));  // E3
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 174.61f, noteToFrequency(Note::F, 3));  // F3
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 196.00f, noteToFrequency(Note::G, 3));  // G3
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 220.00f, noteToFrequency(Note::A, 3));  // A3
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 246.94f, noteToFrequency(Note::B, 3));  // B3
}

void test_note_to_frequency_octave_2(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 65.41f, noteToFrequency(Note::C, 2));   // C2
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 73.42f, noteToFrequency(Note::D, 2));   // D2
}

void test_note_to_frequency_octave_0(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 16.35f, noteToFrequency(Note::C, 0));   // C0
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 18.35f, noteToFrequency(Note::D, 0));   // D0
}

// Test: invalid note (Rest returns 0.0f)
void test_note_to_frequency_rest_returns_zero(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, noteToFrequency(Note::Rest, 4));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, noteToFrequency(Note::Rest, 0));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, noteToFrequency(Note::Rest, 8));
}

// Test: sharps/flats are accessible
void test_note_to_frequency_sharps(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 277.18f, noteToFrequency(Note::Cs, 4));  // C#4
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 311.13f, noteToFrequency(Note::Ds, 4));  // D#4
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 369.99f, noteToFrequency(Note::Fs, 4));  // F#4
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 415.30f, noteToFrequency(Note::Gs, 4));  // G#4
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 466.16f, noteToFrequency(Note::As, 4));  // A#4
}

// ============================================================================
// makeNote and makeRest - Value Tests
// ============================================================================

void test_make_note_with_default_octave(void) {
    MusicNote note = makeNote(INSTR_PULSE_LEAD, Note::A, 0.5f);
    
    TEST_ASSERT_EQUAL(Note::A, note.note);
    TEST_ASSERT_EQUAL(INSTR_PULSE_LEAD.defaultOctave, note.octave);  // 4
    TEST_ASSERT_EQUAL_FLOAT(0.5f, note.duration);
    TEST_ASSERT_EQUAL_FLOAT(INSTR_PULSE_LEAD.baseVolume, note.volume); // 0.35f
    TEST_ASSERT_EQUAL(&INSTR_PULSE_LEAD, note.preset);
}

void test_make_note_with_explicit_octave(void) {
    MusicNote note = makeNote(INSTR_TRIANGLE_BASS, Note::C, 3, 1.0f);
    
    TEST_ASSERT_EQUAL(Note::C, note.note);
    TEST_ASSERT_EQUAL(3, note.octave);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, note.duration);
    TEST_ASSERT_EQUAL_FLOAT(INSTR_TRIANGLE_BASS.baseVolume, note.volume); // 0.30f
    TEST_ASSERT_EQUAL(&INSTR_TRIANGLE_BASS, note.preset);
}

void test_make_note_different_instruments(void) {
    // Test different instrument presets produce correct volume
    MusicNote lead = makeNote(INSTR_PULSE_LEAD, Note::C, 0.25f);
    MusicNote kick = makeNote(INSTR_KICK, Note::Rest, 0.1f);
    MusicNote hihat = makeNote(INSTR_HIHAT, Note::Rest, 0.05f);
    MusicNote pad = makeNote(INSTR_TRIANGLE_PAD, Note::G, 0.5f);
    
    TEST_ASSERT_EQUAL_FLOAT(0.35f, lead.volume);   // INSTR_PULSE_LEAD.baseVolume
    TEST_ASSERT_EQUAL_FLOAT(0.45f, kick.volume);   // INSTR_KICK.baseVolume
    TEST_ASSERT_EQUAL_FLOAT(0.25f, hihat.volume);  // INSTR_HIHAT.baseVolume
    TEST_ASSERT_EQUAL_FLOAT(0.28f, pad.volume);    // INSTR_TRIANGLE_PAD.baseVolume
    
    // Verify octave is from preset
    TEST_ASSERT_EQUAL(4, lead.octave);   // INSTR_PULSE_LEAD.defaultOctave
    TEST_ASSERT_EQUAL(1, kick.octave);    // INSTR_KICK.defaultOctave (kick selector)
    TEST_ASSERT_EQUAL(3, hihat.octave);   // INSTR_HIHAT.defaultOctave (hihat selector)
    TEST_ASSERT_EQUAL(4, pad.octave);    // INSTR_TRIANGLE_PAD.defaultOctave
}

void test_make_rest(void) {
    MusicNote rest = makeRest(0.25f);
    
    TEST_ASSERT_EQUAL(Note::Rest, rest.note);
    TEST_ASSERT_EQUAL(0, rest.octave);
    TEST_ASSERT_EQUAL_FLOAT(0.25f, rest.duration);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rest.volume);
    TEST_ASSERT_EQUAL(nullptr, rest.preset);
}

// ============================================================================
// instrumentToFrequency - Value Tests (kick, snare, hi-hat branches)
// ============================================================================

void test_instrument_to_frequency_kick(void) {
    // Percussion: duty == 0, defaultOctave <= 1 -> Kick (80Hz)
    TEST_ASSERT_EQUAL_FLOAT(80.0f, instrumentToFrequency(INSTR_KICK, Note::C, 4));
    TEST_ASSERT_EQUAL_FLOAT(80.0f, instrumentToFrequency(INSTR_KICK, Note::Rest, 0));
}

void test_instrument_to_frequency_snare(void) {
    // Percussion: duty == 0, defaultOctave == 2 -> Snare (150Hz)
    TEST_ASSERT_EQUAL_FLOAT(150.0f, instrumentToFrequency(INSTR_SNARE, Note::C, 4));
}

void test_instrument_to_frequency_hihat(void) {
    // Percussion: duty == 0, defaultOctave >= 3 -> Hi-HAT (3000Hz)
    TEST_ASSERT_EQUAL_FLOAT(3000.0f, instrumentToFrequency(INSTR_HIHAT, Note::C, 4));
    // Test with octave > 3
    TEST_ASSERT_EQUAL_FLOAT(3000.0f, instrumentToFrequency(INSTR_HIHAT, Note::Rest, 5));
}

void test_instrument_to_frequency_melodic_returns_440(void) {
    // Melodic: duty > 0 -> fallback 440Hz
    TEST_ASSERT_EQUAL_FLOAT(440.0f, instrumentToFrequency(INSTR_PULSE_LEAD, Note::A, 4));
    TEST_ASSERT_EQUAL_FLOAT(440.0f, instrumentToFrequency(INSTR_TRIANGLE_BASS, Note::C, 3));
}

// ============================================================================
// InstrumentPreset Constant Value Tests - verify all values
// ============================================================================

void test_instr_pulse_lead_values(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.35f, INSTR_PULSE_LEAD.baseVolume);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, INSTR_PULSE_LEAD.duty);
    TEST_ASSERT_EQUAL(4, INSTR_PULSE_LEAD.defaultOctave);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, INSTR_PULSE_LEAD.defaultDuration);
    TEST_ASSERT_EQUAL(0, INSTR_PULSE_LEAD.noisePeriod);
    TEST_ASSERT_EQUAL_FLOAT(0.005f, INSTR_PULSE_LEAD.attackTime);
    TEST_ASSERT_EQUAL_FLOAT(0.20f, INSTR_PULSE_LEAD.decayTime);
    TEST_ASSERT_EQUAL_FLOAT(0.70f, INSTR_PULSE_LEAD.sustainLevel);
    TEST_ASSERT_EQUAL_FLOAT(0.15f, INSTR_PULSE_LEAD.releaseTime);
    TEST_ASSERT_EQUAL(LfoTarget::PITCH, INSTR_PULSE_LEAD.lfoTarget);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, INSTR_PULSE_LEAD.lfoFrequency);
    TEST_ASSERT_EQUAL_FLOAT(0.025f, INSTR_PULSE_LEAD.lfoDepth);
    TEST_ASSERT_EQUAL_FLOAT(0.15f, INSTR_PULSE_LEAD.lfoDelay);
    TEST_ASSERT_FALSE(INSTR_PULSE_LEAD.noiseShortMode);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, INSTR_PULSE_LEAD.dutySweep);
}

void test_instr_triangle_lead_values(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.32f, INSTR_TRIANGLE_LEAD.baseVolume);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, INSTR_TRIANGLE_LEAD.duty);
    TEST_ASSERT_EQUAL(5, INSTR_TRIANGLE_LEAD.defaultOctave);
    TEST_ASSERT_EQUAL_FLOAT(0.003f, INSTR_TRIANGLE_LEAD.attackTime);
    TEST_ASSERT_EQUAL_FLOAT(0.15f, INSTR_TRIANGLE_LEAD.decayTime);
    TEST_ASSERT_EQUAL_FLOAT(0.75f, INSTR_TRIANGLE_LEAD.sustainLevel);
    TEST_ASSERT_EQUAL(LfoTarget::PITCH, INSTR_TRIANGLE_LEAD.lfoTarget);
}

void test_instr_triangle_pad_values(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.28f, INSTR_TRIANGLE_PAD.baseVolume);
    TEST_ASSERT_EQUAL_FLOAT(4, INSTR_TRIANGLE_PAD.defaultOctave);
    TEST_ASSERT_EQUAL_FLOAT(0.015f, INSTR_TRIANGLE_PAD.attackTime);
    TEST_ASSERT_EQUAL_FLOAT(0.40f, INSTR_TRIANGLE_PAD.decayTime);
    TEST_ASSERT_EQUAL_FLOAT(0.65f, INSTR_TRIANGLE_PAD.sustainLevel);
    TEST_ASSERT_EQUAL_FLOAT(0.50f, INSTR_TRIANGLE_PAD.releaseTime);
    TEST_ASSERT_EQUAL(LfoTarget::VOLUME, INSTR_TRIANGLE_PAD.lfoTarget);
    TEST_ASSERT_EQUAL_FLOAT(2.5f, INSTR_TRIANGLE_PAD.lfoFrequency);
    TEST_ASSERT_EQUAL_FLOAT(0.15f, INSTR_TRIANGLE_PAD.lfoDepth);
    TEST_ASSERT_EQUAL_FLOAT(0.50f, INSTR_TRIANGLE_PAD.lfoDelay);
}

void test_instr_pulse_pad_values(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.26f, INSTR_PULSE_PAD.baseVolume);
    TEST_ASSERT_EQUAL_FLOAT(0.25f, INSTR_PULSE_PAD.duty);     // 1/4 duty
    TEST_ASSERT_EQUAL(4, INSTR_PULSE_PAD.defaultOctave);
    TEST_ASSERT_EQUAL_FLOAT(0.020f, INSTR_PULSE_PAD.attackTime);
    TEST_ASSERT_EQUAL_FLOAT(0.60f, INSTR_PULSE_PAD.decayTime);
    TEST_ASSERT_EQUAL_FLOAT(0.55f, INSTR_PULSE_PAD.sustainLevel);
    TEST_ASSERT_EQUAL(LfoTarget::PITCH, INSTR_PULSE_PAD.lfoTarget);
    TEST_ASSERT_EQUAL_FLOAT(0.08f, INSTR_PULSE_PAD.dutySweep);
}

void test_instr_pulse_harmony_values(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.22f, INSTR_PULSE_HARMONY.baseVolume);
    TEST_ASSERT_EQUAL_FLOAT(0.125f, INSTR_PULSE_HARMONY.duty); // 1/8 duty
    TEST_ASSERT_EQUAL(5, INSTR_PULSE_HARMONY.defaultOctave);
    TEST_ASSERT_EQUAL_FLOAT(0.50f, INSTR_PULSE_HARMONY.decayTime);
    TEST_ASSERT_EQUAL_FLOAT(0.60f, INSTR_PULSE_HARMONY.sustainLevel);
    TEST_ASSERT_EQUAL(LfoTarget::VOLUME, INSTR_PULSE_HARMONY.lfoTarget);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, INSTR_PULSE_HARMONY.lfoFrequency);
    TEST_ASSERT_EQUAL_FLOAT(0.30f, INSTR_PULSE_HARMONY.lfoDepth);
    TEST_ASSERT_EQUAL_FLOAT(0.15f, INSTR_PULSE_HARMONY.dutySweep);
}

void test_instr_triangle_bass_values(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.30f, INSTR_TRIANGLE_BASS.baseVolume);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, INSTR_TRIANGLE_BASS.duty);
    TEST_ASSERT_EQUAL(3, INSTR_TRIANGLE_BASS.defaultOctave);
    TEST_ASSERT_EQUAL_FLOAT(0.005f, INSTR_TRIANGLE_BASS.attackTime);
    TEST_ASSERT_EQUAL_FLOAT(0.10f, INSTR_TRIANGLE_BASS.decayTime);
    TEST_ASSERT_EQUAL_FLOAT(0.20f, INSTR_TRIANGLE_BASS.sustainLevel);
    TEST_ASSERT_EQUAL(LfoTarget::NONE, INSTR_TRIANGLE_BASS.lfoTarget);
}

void test_instr_pulse_bass_values(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.30f, INSTR_PULSE_BASS.baseVolume);
    TEST_ASSERT_EQUAL_FLOAT(0.25f, INSTR_PULSE_BASS.duty);   // 1/4
    TEST_ASSERT_EQUAL(2, INSTR_PULSE_BASS.defaultOctave);
    TEST_ASSERT_EQUAL_FLOAT(0.001f, INSTR_PULSE_BASS.attackTime);
    TEST_ASSERT_EQUAL_FLOAT(0.08f, INSTR_PULSE_BASS.decayTime);
    TEST_ASSERT_EQUAL_FLOAT(0.35f, INSTR_PULSE_BASS.sustainLevel);
    TEST_ASSERT_EQUAL(LfoTarget::NONE, INSTR_PULSE_BASS.lfoTarget);
}

void test_instr_kick_values(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.45f, INSTR_KICK.baseVolume);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, INSTR_KICK.duty);          // duty=0 -> NOISE channel
    TEST_ASSERT_EQUAL(1, INSTR_KICK.defaultOctave);         // kick selector
    TEST_ASSERT_EQUAL_FLOAT(0.12f, INSTR_KICK.defaultDuration);
    TEST_ASSERT_EQUAL(15, INSTR_KICK.noisePeriod);
    TEST_ASSERT_EQUAL_FLOAT(0.001f, INSTR_KICK.attackTime);
    TEST_ASSERT_EQUAL_FLOAT(0.10f, INSTR_KICK.decayTime);
    TEST_ASSERT_EQUAL_FLOAT(0.00f, INSTR_KICK.sustainLevel);
    TEST_ASSERT_EQUAL(LfoTarget::NONE, INSTR_KICK.lfoTarget);
    TEST_ASSERT_FALSE(INSTR_KICK.noiseShortMode);
}

void test_instr_snare_values(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.35f, INSTR_SNARE.baseVolume);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, INSTR_SNARE.duty);
    TEST_ASSERT_EQUAL(2, INSTR_SNARE.defaultOctave);        // snare selector
    TEST_ASSERT_EQUAL_FLOAT(0.15f, INSTR_SNARE.defaultDuration);
    TEST_ASSERT_EQUAL(60, INSTR_SNARE.noisePeriod);
    TEST_ASSERT_EQUAL_FLOAT(0.08f, INSTR_SNARE.decayTime);
    TEST_ASSERT_TRUE(INSTR_SNARE.noiseShortMode);            // metallic timbre
}

void test_instr_hihat_values(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.25f, INSTR_HIHAT.baseVolume);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, INSTR_HIHAT.duty);
    TEST_ASSERT_EQUAL(3, INSTR_HIHAT.defaultOctave);        // hihat selector
    TEST_ASSERT_EQUAL_FLOAT(0.05f, INSTR_HIHAT.defaultDuration);
    TEST_ASSERT_EQUAL(12, INSTR_HIHAT.noisePeriod);
    TEST_ASSERT_EQUAL_FLOAT(0.0005f, INSTR_HIHAT.attackTime);
    TEST_ASSERT_EQUAL_FLOAT(0.015f, INSTR_HIHAT.decayTime);
    TEST_ASSERT_TRUE(INSTR_HIHAT.noiseShortMode);            // metallic timbre
}

// ============================================================================
// Edge Cases - boundary conditions
// ============================================================================

void test_max_music_tracks_constant(void) {
    TEST_ASSERT_EQUAL(4, MAX_MUSIC_TRACKS);
}

void test_note_frequencies_octave_4_values(void) {
    // Verify NOTE_FREQUENCIES_OCTAVE_4 array contents
    TEST_ASSERT_EQUAL_FLOAT(261.63f, NOTE_FREQUENCIES_OCTAVE_4[0]);  // C
    TEST_ASSERT_EQUAL_FLOAT(277.18f, NOTE_FREQUENCIES_OCTAVE_4[1]);  // C#
    TEST_ASSERT_EQUAL_FLOAT(293.66f, NOTE_FREQUENCIES_OCTAVE_4[2]);  // D
    TEST_ASSERT_EQUAL_FLOAT(311.13f, NOTE_FREQUENCIES_OCTAVE_4[3]);  // D#
    TEST_ASSERT_EQUAL_FLOAT(329.63f, NOTE_FREQUENCIES_OCTAVE_4[4]);  // E
    TEST_ASSERT_EQUAL_FLOAT(349.23f, NOTE_FREQUENCIES_OCTAVE_4[5]);  // F
    TEST_ASSERT_EQUAL_FLOAT(369.99f, NOTE_FREQUENCIES_OCTAVE_4[6]);  // F#
    TEST_ASSERT_EQUAL_FLOAT(392.00f, NOTE_FREQUENCIES_OCTAVE_4[7]);  // G
    TEST_ASSERT_EQUAL_FLOAT(415.30f, NOTE_FREQUENCIES_OCTAVE_4[8]);  // G#
    TEST_ASSERT_EQUAL_FLOAT(440.00f, NOTE_FREQUENCIES_OCTAVE_4[9]);  // A
    TEST_ASSERT_EQUAL_FLOAT(466.16f, NOTE_FREQUENCIES_OCTAVE_4[10]); // A#
    TEST_ASSERT_EQUAL_FLOAT(493.88f, NOTE_FREQUENCIES_OCTAVE_4[11]); // B
}

// ============================================================================
// MusicNote struct field validation
// ============================================================================

void test_music_note_struct_fields(void) {
    MusicNote note{};
    note.note = Note::G;
    note.octave = 5;
    note.duration = 0.75f;
    note.volume = 0.8f;
    note.preset = &INSTR_PULSE_LEAD;
    
    TEST_ASSERT_EQUAL(Note::G, note.note);
    TEST_ASSERT_EQUAL(5, note.octave);
    TEST_ASSERT_EQUAL_FLOAT(0.75f, note.duration);
    TEST_ASSERT_EQUAL_FLOAT(0.8f, note.volume);
    TEST_ASSERT_EQUAL(&INSTR_PULSE_LEAD, note.preset);
}

void test_music_track_struct_fields(void) {
    static const MusicNote notes[] = {{Note::C, 4, 0.5f, 0.5f, nullptr}};
    static const MusicTrack track = {
        notes,
        1,
        true,
        WaveType::PULSE,
        0.5f,
        nullptr,  // secondVoice
        nullptr,  // thirdVoice
        nullptr   // percussion
    };
    
    TEST_ASSERT_EQUAL(notes, track.notes);
    TEST_ASSERT_EQUAL(1, track.count);
    TEST_ASSERT_TRUE(track.loop);
    TEST_ASSERT_EQUAL(WaveType::PULSE, track.channelType);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, track.duty);
    TEST_ASSERT_EQUAL(nullptr, track.secondVoice);
    TEST_ASSERT_EQUAL(nullptr, track.thirdVoice);
    TEST_ASSERT_EQUAL(nullptr, track.percussion);
}

// ============================================================================
// MusicTrack with secondVoice/thirdVoice/percussion
// ============================================================================

void test_music_track_with_voices(void) {
    static const MusicNote mainNotes[] = {{Note::C, 4, 1.0f, 0.5f, nullptr}};
    static const MusicNote subNotes[] = {{Note::G, 4, 1.0f, 0.3f, nullptr}};
    static const MusicNote percNotes[] = {{Note::Rest, 0, 0.1f, 0.2f, nullptr}};
    
    // secondVoice must be a pointer to a MusicTrack, not MusicNote
    static const MusicTrack subTrack = {subNotes, 1, false, WaveType::PULSE, 0.5f, nullptr, nullptr, nullptr};
    static const MusicTrack percTrack = {percNotes, 1, true, WaveType::NOISE, 0.0f, nullptr, nullptr, nullptr};
    
    static const MusicTrack mainTrack = {
        mainNotes, 1, false, WaveType::TRIANGLE, 0.5f,
        &subTrack,  // secondVoice (pointer to sub MusicTrack)
        nullptr,    // thirdVoice
        nullptr     // percussion
    };
    
    static const MusicTrack percussionOnlyTrack = {
        percNotes, 1, true, WaveType::NOISE, 0.0f,
        nullptr, nullptr, &percTrack  // percussion pointer (MusicTrack*)
    };
    
    // Verify secondVoice pointer is accessible
    TEST_ASSERT_EQUAL(&subTrack, mainTrack.secondVoice);
    TEST_ASSERT_EQUAL(nullptr, mainTrack.thirdVoice);
    
    // Verify percussion pointer
    TEST_ASSERT_EQUAL(&percTrack, percussionOnlyTrack.percussion);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // Note enum value tests
    RUN_TEST(test_note_enum_values);
    
    // noteToFrequency - value tests (all branches)
    RUN_TEST(test_note_to_frequency_octave_5);
    RUN_TEST(test_note_to_frequency_octave_6);
    RUN_TEST(test_note_to_frequency_octave_3);
    RUN_TEST(test_note_to_frequency_octave_2);
    RUN_TEST(test_note_to_frequency_octave_0);
    RUN_TEST(test_note_to_frequency_rest_returns_zero);
    RUN_TEST(test_note_to_frequency_sharps);
    
    // makeNote and makeRest
    RUN_TEST(test_make_note_with_default_octave);
    RUN_TEST(test_make_note_with_explicit_octave);
    RUN_TEST(test_make_note_different_instruments);
    RUN_TEST(test_make_rest);
    
    // instrumentToFrequency (kick, snare, hi-hat branches)
    RUN_TEST(test_instrument_to_frequency_kick);
    RUN_TEST(test_instrument_to_frequency_snare);
    RUN_TEST(test_instrument_to_frequency_hihat);
    RUN_TEST(test_instrument_to_frequency_melodic_returns_440);
    
    // InstrumentPreset constant value tests
    RUN_TEST(test_instr_pulse_lead_values);
    RUN_TEST(test_instr_triangle_lead_values);
    RUN_TEST(test_instr_triangle_pad_values);
    RUN_TEST(test_instr_pulse_pad_values);
    RUN_TEST(test_instr_pulse_harmony_values);
    RUN_TEST(test_instr_triangle_bass_values);
    RUN_TEST(test_instr_pulse_bass_values);
    RUN_TEST(test_instr_kick_values);
    RUN_TEST(test_instr_snare_values);
    RUN_TEST(test_instr_hihat_values);
    
    // Edge cases / boundary
    RUN_TEST(test_max_music_tracks_constant);
    RUN_TEST(test_note_frequencies_octave_4_values);
    
    // Struct field validation
    RUN_TEST(test_music_note_struct_fields);
    RUN_TEST(test_music_track_struct_fields);
    RUN_TEST(test_music_track_with_voices);
    
    return UNITY_END();
}