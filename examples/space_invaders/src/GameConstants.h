#pragma once
#include <platforms/EngineConfig.h>
#include <cstdint>

namespace spaceinvaders {

/**
 * @file GameConstants.h
 * @brief Centralized game configuration for Space Invaders.
 *
 * Defines layout, dimensions, timing, and input mapping.
 * Collision layers: 1=Player, 2=Enemy, 3=PlayerProjectile, 4=EnemyProjectile, 5=Bunker.
 */

    /** Input button IDs (platform-specific mapping) */
    constexpr uint8_t BTN_LEFT = 2;
    constexpr uint8_t BTN_RIGHT = 3;
    constexpr uint8_t BTN_FIRE = 4;

    /** Scale factor for all sprites (ceil for collision dimensions) */
    constexpr float SPRITE_SCALE = 1.25f;

    /** Player ship sprite dimensions (raw pixels) */
    constexpr int PLAYER_SPRITE_W = 11;
    constexpr int PLAYER_SPRITE_H = 8;
    /** Player collision dimensions (scaled, ceil) */
    constexpr int PLAYER_WIDTH = (int)(PLAYER_SPRITE_W * SPRITE_SCALE + 0.99f);
    constexpr int PLAYER_HEIGHT = (int)(PLAYER_SPRITE_H * SPRITE_SCALE + 0.99f);

    constexpr float PLAYER_SPEED = 60.0f;
    constexpr float PLAYER_START_X = (DISPLAY_WIDTH - PLAYER_WIDTH) / 2.0f;
    constexpr float PLAYER_START_Y = DISPLAY_HEIGHT - PLAYER_HEIGHT - 4.0f;

    /** Projectile dimensions, speed, and limits (pixels per second) */
    constexpr int PROJECTILE_WIDTH = 1;
    constexpr int PROJECTILE_HEIGHT = 4;
    constexpr float PROJECTILE_SPEED = 80.0f;
    constexpr int MAX_PLAYER_BULLETS = 8;           ///< Max simultaneous player bullets
    constexpr unsigned long PLAYER_FIRE_COOLDOWN = 100;  ///< Milliseconds between shots

    /** Bunker layout and dimensions */
    constexpr int BUNKER_COUNT = 4;
    constexpr int BUNKER_WIDTH = 24;
    constexpr int BUNKER_HEIGHT = 16;
    constexpr float BUNKER_Y = DISPLAY_HEIGHT - PLAYER_HEIGHT - 40.0f;

    /** Alien formation grid */
    constexpr int ALIEN_ROWS = 4;
    constexpr int ALIEN_COLS = 8;

    /** Squid (top row) dimensions */
    constexpr int ALIEN_SQUID_SPRITE_W = 8;
    constexpr int ALIEN_SQUID_SPRITE_H = 8;
    constexpr int ALIEN_SQUID_W = (int)(ALIEN_SQUID_SPRITE_W * SPRITE_SCALE + 0.99f);
    constexpr int ALIEN_SQUID_H = (int)(ALIEN_SQUID_SPRITE_H * SPRITE_SCALE + 0.99f);

    /** Crab (middle rows) dimensions */
    constexpr int ALIEN_CRAB_SPRITE_W = 11;
    constexpr int ALIEN_CRAB_SPRITE_H = 8;
    constexpr int ALIEN_CRAB_W = (int)(ALIEN_CRAB_SPRITE_W * SPRITE_SCALE + 0.99f);
    constexpr int ALIEN_CRAB_H = (int)(ALIEN_CRAB_SPRITE_H * SPRITE_SCALE + 0.99f);

    /** Octopus (bottom rows) dimensions */
    constexpr int ALIEN_OCTOPUS_SPRITE_W = 12;
    constexpr int ALIEN_OCTOPUS_SPRITE_H = 8;
    constexpr int ALIEN_OCTOPUS_W = (int)(ALIEN_OCTOPUS_SPRITE_W * SPRITE_SCALE + 0.99f);
    constexpr int ALIEN_OCTOPUS_H = (int)(ALIEN_OCTOPUS_SPRITE_H * SPRITE_SCALE + 0.99f);

    /** Alien grid spacing and starting position */
    constexpr float ALIEN_SPACING_X = 17.0f;
    constexpr float ALIEN_SPACING_Y = 18.0f;
    constexpr float ALIEN_GRID_WIDTH = (ALIEN_COLS - 1) * ALIEN_SPACING_X + ALIEN_OCTOPUS_W;
    constexpr float ALIEN_START_X = (DISPLAY_WIDTH - ALIEN_GRID_WIDTH) * 0.5f;
    constexpr float ALIEN_START_Y = 40.0f;

    /** Alien formation step timing (ms) and movement */
    constexpr unsigned long BASE_STEP_DELAY = 417;  ///< 72 BPM (eighth note)
    constexpr unsigned long INITIAL_STEP_DELAY = 380;
    constexpr unsigned long MIN_STEP_DELAY = 150;
    constexpr float ALIEN_DROP_AMOUNT = 7.0f;
    constexpr float ALIEN_STEP_AMOUNT_X = 2.5f;

    /** Music tempo scaling based on alien proximity (threat factor) */
    constexpr float GAME_OVER_Y = 200.0f;
    constexpr float Y_RANGE = 160.0f;              ///< GAME_OVER_Y - ALIEN_START_Y
    constexpr float INV_Y_RANGE = 0.00625f;        ///< 1.0 / Y_RANGE

}
