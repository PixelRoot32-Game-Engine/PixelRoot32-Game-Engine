# MusicPlayer

<Badge type="info" text="Class" />

**Source:** `MusicPlayer.h`

## Description

Simple sequencer to play MusicTracks using the AudioEngine.

Provides a high-level interface for music playback with tempo control,
pause/resume, and master volume management. Wraps audio commands to the
engine's scheduler.

## Methods

### `void play(const MusicTrack& track)`

**Description:**

Starts playing a track.

**Parameters:**

- `track`: The MusicTrack to play. Must remain in scope for duration.

### `void stop()`

**Description:**

Stops playback and silences all voices.

### `void pause()`

**Description:**

Pauses playback at the current position.

### `void resume()`

**Description:**

Resumes playback from the paused position.

### `bool isPlaying() const`

**Description:**

Checks if a track is currently playing.

**Returns:** true if playing, false otherwise.

### `void setTempoFactor(float factor)`

**Description:**

Sets the global tempo scaling factor.

**Parameters:**

- `factor`: 1.0f is normal speed, 2.0f is double speed, 0.5f is half speed.

### `float getTempoFactor() const`

**Description:**

Gets the current tempo scaling factor.

**Returns:** Current factor (default 1.0f).

### `void setBPM(float bpm)`

**Description:**

Sets the tempo in BPM (beats per minute).

**Parameters:**

- `bpm`: Beats per minute (affects note timing resolution).

### `float getBPM() const`

**Description:**

Gets the current BPM setting.

**Returns:** Current BPM (default 150).

### `size_t getActiveTrackCount() const`

**Description:**

Gets the number of currently active tracks.

**Returns:** Number of active tracks (1-4), 0 if not playing.

### `void setMasterVolume(float volume)`

**Description:**

Sets the master volume level for music playback.

**Parameters:**

- `volume`: Volume level [0.0f = silent, 1.0f = full volume].

### `float getMasterVolume() const`

**Description:**

Gets the current master volume level.

**Returns:** Current volume [0.0f - 1.0f].
