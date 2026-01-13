#include "graphics/particles/ParticleEmitter.h"
#include "graphics/Renderer.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <EDGE.h>
#include <math/MathUtil.h>

extern EDGE engine;

ParticleEmitter::ParticleEmitter(float x, float y, const ParticleConfig& cfg)
    : Entity(x, y, 0, 0, EntityType::GENERIC),
        config(cfg) {}

void ParticleEmitter::update(unsigned long deltaTime) {
    int screenW = engine.getRenderer().getWidth();
    int screenH = engine.getRenderer().getHeight();

    for (int i = 0; i < maxParticles; i++) {
        Particle& p = particles[i];
        if (!p.active) continue;

        p.x += p.vx;
        p.y += p.vy;

        p.vy += config.gravity;
        p.vx *= config.friction;
        p.vy *= config.friction;

        if (p.x < 0 || p.x > screenW || p.y < 0 || p.y > screenH) {
            p.active = false;
            continue;
        }

        if (p.life > 0) {
            p.life--;

            if (config.fadeColor) {
                float t = 1.0f - (float)p.life / p.maxLife;
                p.color = lerpColor(p.startColor, p.endColor, t);
            }

        } else {
            p.active = false;
        }
    }
}

void ParticleEmitter::draw(Renderer& renderer) {
    for (int i = 0; i < maxParticles; i++) {
        Particle& p = particles[i]; 
        if (!p.active) continue;
        
        renderer.drawFilledRectangle(p.x, p.y, 2, 2, p.color);
    }
}

void ParticleEmitter::burst(float x, float y, int count) {
    int activated = 0;

    for (int i = 0; i < maxParticles && activated < count; i++) {
        Particle& p = particles[i];
        if (p.active) continue;

        p.active = true;
        p.x = x;
        p.y = y;

        float angle = (rand() % 360) * Math::kDegToRad;;
        float speed = config.minSpeed + 
                      (rand() / (float)RAND_MAX) * (config.maxSpeed - config.minSpeed);

        p.vx = cos(angle) * speed;
        p.vy = sin(angle) * speed;

        p.maxLife = config.minLife + (rand() % (config.maxLife - config.minLife + 1));
        p.life = p.maxLife;

        p.startColor = config.startColor;
        p.endColor   = config.endColor;
        p.color      = config.startColor;

        activated++;
    }
}
