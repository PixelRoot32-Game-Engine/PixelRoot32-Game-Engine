#pragma once

#include "AudioConfig.h"
#include "AudioTypes.h"
#include <cstdint>

namespace pixelroot32::audio {

    /**
     * @class AudioEngine
     * @brief Core class for the NES-like audio subsystem.
     * 
     * This class manages the audio channels (Pulse, Triangle, Noise), mixes their output,
     * and provides the audio stream to the backend.
     */
    class AudioEngine {
    public:
        /**
         * @brief Constructs the AudioEngine with the given configuration.
         * @param config Configuration struct containing the backend and parameters.
         */
        AudioEngine(const AudioConfig& config);
        
        /**
         * @brief Initializes the audio subsystem and the backend.
         */
        void init();

        /**
         * @brief Updates the audio state based on game time.
         * This should be called from the main game loop (Engine::update).
         * @param deltaTime Time elapsed since last frame in milliseconds.
         */
        void update(unsigned long deltaTime);

        /**
         * @brief Fills the provided buffer with mixed audio samples.
         * This method is typically called by the AudioBackend from an audio callback or task.
         * @param stream Pointer to the buffer to fill.
         * @param length Number of samples to generate.
         */
        void generateSamples(int16_t* stream, int length);

        /**
         * @brief Plays a sound event on an available channel.
         * @param event The audio event parameters.
         */
        void playEvent(const AudioEvent& event);

    private:
        AudioConfig config;

        // Fixed channels: 2 Pulse, 1 Triangle, 1 Noise
        static constexpr int NUM_CHANNELS = 4;
        AudioChannel channels[NUM_CHANNELS];

        // Helpers
        AudioChannel* findFreeChannel(WaveType type);
        int16_t generateSampleForChannel(AudioChannel& ch);
    };

}
