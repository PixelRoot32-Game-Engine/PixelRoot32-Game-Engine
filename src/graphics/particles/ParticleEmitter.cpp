/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/particles/ParticleEmitter.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include <cmath>
#include "core/Engine.h"
#include <math/MathUtil.h>

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;
namespace pixelroot32::graphics::particles {

    using namespace pixelroot32::core;
    using namespace pixelroot32::graphics;
    using namespace pixelroot32::math;

    namespace {
        static uint32_t s_rngState = 123456789;

        // Xorshift32 - fast PRNG
        inline uint32_t fastRand() {
            uint32_t x = s_rngState;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            s_rngState = x;
            return x;
        }

        inline float fastRandFloat(float min, float max) {
            float r = (fastRand() & 0xFFFF) * (1.0f / 65535.0f);
            return min + r * (max - min);
        }

        template <typename T>
        inline T fastRandScalar(T min, T max) {
            if constexpr (std::is_same_v<T, float>) {
                return fastRandFloat(min, max);
            } else {
                // Fixed16 implementation using integer arithmetic to avoid float conversion
                 // 0xFFFF is 65535. We treat this as the fractional part.
                 int32_t randomFraction = fastRand() & 0xFFFF;
                 
                 Fixed16 range = max - min;
                 
                 // result = min + range * (randomFraction / 65536)
                 // Use int64_t to prevent overflow during multiplication
                 int64_t deltaRaw = (static_cast<int64_t>(range.raw) * randomFraction) >> 16;
                 
                 Fixed16 result;
                 result.raw = min.raw + static_cast<int32_t>(deltaRaw);
                 return result;
             }
        }

        inline int fastRandInt(int min, int max) {
            if (min >= max) return min;
            return min + (fastRand() % (max - min + 1));
        }
    }

    ParticleEmitter::ParticleEmitter(Scalar x, Scalar y, const ParticleConfig& cfg)
        : Entity(x, y, 0, 0, EntityType::GENERIC),
            config(cfg) {
             // Seed with something somewhat random if needed, or keep deterministic
             s_rngState = (uint32_t)((uintptr_t)this + 12345); 
    }

    void ParticleEmitter::update(unsigned long deltaTime) {
        (void)deltaTime;

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
                    Scalar t = toScalar(1) - (toScalar(p.life) / toScalar(p.maxLife));
                    p.color = lerpColor(resolveColor(p.startColor), resolveColor(p.endColor), t);
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
            
            renderer.drawFilledRectangleW(static_cast<int>(p.x), static_cast<int>(p.y), 2, 2, p.color);
        }
    }

    void ParticleEmitter::burst(Scalar x, Scalar y, int count) {
        int activated = 0;

        for (int i = 0; i < maxParticles && activated < count; i++) {
            Particle& p = particles[i];
            if (p.active) continue;

            p.active = true;
            p.x = x;
            p.y = y;

            Scalar angleDeg = fastRandScalar(config.minAngleDeg, config.maxAngleDeg);
            Scalar angle = angleDeg * kDegToRad;
            Scalar speed = fastRandScalar(config.minSpeed, config.maxSpeed);

            p.vx = pixelroot32::math::cos(angle) * speed;
            p.vy = pixelroot32::math::sin(angle) * speed;

            p.maxLife = fastRandInt(config.minLife, config.maxLife);
            p.life = p.maxLife;

            p.startColor = config.startColor;
            p.endColor   = config.endColor;
            p.color      = resolveColor(config.startColor);

            activated++;
        }
    }
}
