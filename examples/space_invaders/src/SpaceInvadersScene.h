#pragma once

#include <core/Scene.h>
#include <graphics/Renderer.h>
#include <platforms/EngineConfig.h>

#include <vector>
#include <memory>

namespace spaceinvaders {

    class PlayerActor;
    class AlienActor;
    class ProjectileActor;
    class BunkerActor;
    class StarfieldBackground;

    /**
     * @struct EnemyExplosion
     * @brief Simple explosion effect for enemy hits.
     *
     * Uses a fixed array slot for visual feedback without dynamic allocation.
     */
    struct EnemyExplosion {
        bool active;                              ///< Whether this slot is in use
        pixelroot32::math::Vector2 position;       ///< World position of the explosion
        unsigned long remainingMs;                 ///< Time left before effect ends
    };

    /**
     * @class ExplosionAnimation
     * @brief Multi-frame explosion animation for the player death sequence.
     *
     * Plays a non-looping sprite animation at a fixed framerate.
     * Used during the pause state between player death and respawn.
     */
    class ExplosionAnimation {
    public:
        ExplosionAnimation();

        /**
         * @brief Starts the explosion animation at the given world position.
         * @param pos World position where the explosion is drawn.
         */
        void start(pixelroot32::math::Vector2 pos);

        /**
         * @brief Advances the animation based on elapsed time.
         * @param deltaTime Time elapsed since last frame in milliseconds.
         */
        void update(unsigned long deltaTime);

        /**
         * @brief Draws the current frame if the animation is active.
         * @param renderer Reference to the renderer.
         */
        void draw(pixelroot32::graphics::Renderer& renderer);

        /**
         * @brief Returns true while the animation is still playing.
         */
        bool isActive() const;

    private:
        bool active;                               ///< Whether the animation is playing
        pixelroot32::math::Vector2 position;       ///< Draw position
        unsigned long timeAccumulator;             ///< Accumulated time for frame stepping
        unsigned char stepsDone;                   ///< Frames already advanced
        pixelroot32::graphics::SpriteAnimation animation; ///< Sprite animation data
    };

    /**
     * @class SpaceInvadersScene
     * @brief Main scene for the Space Invaders clone.
     *
     * Manages game state, entity lifecycle, collision resolution, and procedural
     * audio. Uses swept-circle collision for projectiles and supports arena-based
     * memory when PIXELROOT32_ENABLE_SCENE_ARENA is defined.
     */
    class SpaceInvadersScene : public pixelroot32::core::Scene {
    public:
        SpaceInvadersScene();
        virtual ~SpaceInvadersScene();

        /**
         * @brief Initializes the scene and resets game state.
         */
        void init() override;

        /**
         * @brief Updates game logic, entities, and collisions.
         * @param deltaTime Time elapsed since last frame in milliseconds.
         */
        void update(unsigned long deltaTime) override;

        /**
         * @brief Draws all entities, HUD, and visual effects.
         * @param renderer Reference to the renderer.
         */
        void draw(pixelroot32::graphics::Renderer& renderer) override;

    private:
        void resetGame();
        void spawnAliens();
        void spawnBunkers();
        void cleanup();

#ifdef PIXELROOT32_ENABLE_SCENE_ARENA
        StarfieldBackground* background;
        PlayerActor* player;
        std::vector<AlienActor*> aliens;
        std::vector<ProjectileActor*> projectiles;
        std::vector<BunkerActor*> bunkers;
#else
        std::unique_ptr<StarfieldBackground> background;
        std::unique_ptr<PlayerActor> player;
        std::vector<std::unique_ptr<AlienActor>> aliens;
        std::vector<std::unique_ptr<ProjectileActor>> projectiles;
        std::vector<std::unique_ptr<BunkerActor>> bunkers;
#endif

        int score;                                 ///< Current score
        int lives;                                 ///< Remaining lives
        bool gameOver;                             ///< True when game has ended
        bool gameWon;                               ///< True if player cleared all aliens

        float stepTimer;                           ///< Accumulator for alien step timing
        unsigned long stepDelay;                   ///< Delay between alien steps (ms)
        int moveDirection;                         ///< 1: Right, -1: Left

        static constexpr int MaxEnemyExplosions = 8;
        EnemyExplosion enemyExplosions[MaxEnemyExplosions];
        ExplosionAnimation playerExplosion;
        bool isPaused;                              ///< True during player death/respawn

        static constexpr int MaxProjectiles = 12;

        bool fireInputReady;                        ///< Fire rate limiting state
        unsigned long lastFireTime;                 ///< Timestamp of last player shot
        int activePlayerBulletCount;               ///< Cached count of active player bullets (O(1) vs O(n) scan)
        float currentMusicTempoFactor;              ///< BGM tempo based on alien proximity

        void updateAliens(unsigned long deltaTime);
        void handleCollisions();
        void enemyShoot();
        int getActiveAlienCount() const;
        void updateMusicTempo();

        void updateEnemyExplosions(unsigned long deltaTime);
        void drawEnemyExplosions(pixelroot32::graphics::Renderer& renderer);
        void spawnEnemyExplosion(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y);

        void handlePlayerHit();
        void respawnPlayerUnderBunker();
    };

}
