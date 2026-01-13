#pragma once
#include "core/Entity.h"
#include "Particle.h"
#include "particles/ParticleConfig.h"

#define MAX_PARTICLES_PER_EMITTER 50

class ParticleEmitter: public Entity {
public:

    ParticleEmitter(float x, float y, const ParticleConfig& cfg);
    
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

    void burst(float x, float y, int count);

private:
    ParticleConfig config;  
    Particle particles[MAX_PARTICLES_PER_EMITTER];
    int maxParticles = MAX_PARTICLES_PER_EMITTER;


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