#ifndef BACKGROUND_ASSETS_H
#define BACKGROUND_ASSETS_H

#include <stdint.h>

/**
 * @file Background.h
 * @brief Starfield background assets for Space Invaders.
 *
 * Procedural star pattern generated in code. Coordinates are stored in
 * static arrays for efficient flash storage on embedded targets.
 */

namespace background_assets {

    /** Number of star positions in the pattern */
    extern const int STAR_COUNT;

    /** Star X coordinates (0..DISPLAY_WIDTH-1) */
    extern const uint8_t STAR_X[];

    /** Star Y coordinates (0..DISPLAY_HEIGHT-1) */
    extern const uint8_t STAR_Y[];

    /**
     * @brief Optional init. No-op; kept for compatibility.
     */
    void init();

} // namespace background_assets

#endif // BACKGROUND_ASSETS_H
