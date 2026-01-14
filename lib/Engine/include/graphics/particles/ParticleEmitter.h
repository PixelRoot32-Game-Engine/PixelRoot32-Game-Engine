#pragma once
#include "core/Entity.h"
#include "Particle.h"
#include "particles/ParticleConfig.h"

#define MAX_PARTICLES_PER_EMITTER 50

/**
 * @class ParticleEmitter
 * @brief Manages a pool of particles to create visual effects.
 *
 * Inherits from Entity to participate in the scene's update/draw loop.
 * Uses a fixed-size array for particles to avoid dynamic allocation during runtime.
 */
class ParticleEmitter: public Entity {
public:

    /**
     * @brief Constructs a new ParticleEmitter.
     * @param x Initial X position.
     * @param y Initial Y position.
     * @param cfg Configuration for the emitted particles.
     */
    ParticleEmitter(float x, float y, const ParticleConfig& cfg);
    
    /**
     * @brief Updates all active particles.
     * Applies physics (gravity, friction) and updates lifetime.
     * @param deltaTime Time elapsed since last frame.
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Renders all active particles.
     * @param renderer The renderer instance.
     */
    void draw(Renderer& renderer) override;

    /**
     * @brief Emits a burst of particles from a specific location.
     * @param x Emission origin X.
     * @param y Emission origin Y.
     * @param count Number of particles to spawn.
     */
    void burst(float x, float y, int count);

private:
    ParticleConfig config;  
    Particle particles[MAX_PARTICLES_PER_EMITTER];
    int maxParticles = MAX_PARTICLES_PER_EMITTER;


    /**
     * @brief Linear interpolation between two 16-bit RGB565 colors.
     * @param c1 Start color.
     * @param c2 End color.
     * @param t Interpolation factor (0.0 - 1.0).
     * @return Interpolated RGB565 color.
     */
    inline uint16_t lerpColor(uint16_t c1, uint16_t c2, float t) {
        uint8_t r1 = (c1 >> 11) & 0x1F;
        uint8_t g1 = (c1 >> 5)  & 0x3F;
        uint8_t b1 = c1 & 0x1F;

        uint8_t r2 = (c2 >> 11) & 0x1F;
        uint8_t g2 = (c2 >> 5)  & 0x3F;
        uint8_t b2 = c2 & 0x1F;

        uint8_t r = r1 + (r2 - r1) * t;
        uint8_t g = g1 + (g2 - g1) * t;
        uint8_t b = b1 + (b2 - b1) * t;

        return (r << 11) | (g << 5) | b;
    }
};