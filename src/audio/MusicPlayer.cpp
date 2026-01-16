#include "audio/MusicPlayer.h"
#include <cmath>

namespace pixelroot32::audio {

MusicPlayer::MusicPlayer(AudioEngine& engine) 
    : engine(engine), currentTrack(nullptr), currentNoteIndex(0), 
      noteTimer(0.0f), playing(false), paused(false) {}

void MusicPlayer::play(const MusicTrack& track) {
    currentTrack = &track;
    currentNoteIndex = 0;
    noteTimer = 0.0f;
    playing = true;
    paused = false;
    
    // Play the first note immediately
    playCurrentNote();
}

void MusicPlayer::stop() {
    playing = false;
    paused = false;
    currentTrack = nullptr;
    currentNoteIndex = 0;
    // Note: This stops the sequencer, but the last played note 
    // will continue until its duration expires (AudioEvent behavior).
}

void MusicPlayer::pause() {
    if (playing) paused = true;
}

void MusicPlayer::resume() {
    if (playing && paused) paused = false;
}

void MusicPlayer::update(unsigned long deltaTime) {
    if (!playing || paused || !currentTrack) return;

    // Convert ms to seconds
    float dt = static_cast<float>(deltaTime) / 1000.0f;
    noteTimer += dt;

    bool noteChanged = false;
    
    // Handle note transitions
    // Use a while loop to handle cases where deltaTime > note duration (lag)
    while (playing && currentTrack && noteTimer >= currentTrack->notes[currentNoteIndex].duration) {
        noteTimer -= currentTrack->notes[currentNoteIndex].duration;
        currentNoteIndex++;
        noteChanged = true;

        if (currentNoteIndex >= currentTrack->count) {
            if (currentTrack->loop) {
                currentNoteIndex = 0;
            } else {
                playing = false;
                currentTrack = nullptr;
                return; // Playback finished
            }
        }
    }
    
    if (noteChanged && playing) {
        playCurrentNote();
    }
}

void MusicPlayer::playCurrentNote() {
    if (!currentTrack) return;
    
    const MusicNote& note = currentTrack->notes[currentNoteIndex];
    
    // Check for Rest (Silence)
    if (note.note == Note::Rest) {
        // Do nothing, just wait for duration to pass in update()
        return;
    }

    AudioEvent event;
    event.type = currentTrack->channelType;
    event.frequency = noteToFrequency(note.note, note.octave);
    event.duration = note.duration;
    event.volume = note.volume;
    
    if (event.type == WaveType::PULSE) {
        event.duty = currentTrack->duty;
    } else {
        event.duty = 0.5f;
    }

    engine.playEvent(event);
}

bool MusicPlayer::isPlaying() const {
    return playing;
}

} // namespace pixelroot32::audio
