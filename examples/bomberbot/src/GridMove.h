#pragma once

namespace bomberbot {

/**
 * @file GridMove.h
 * @brief Plain data describing one actor's position on the grid while it
 *        interpolates between cells.
 *
 * Deliberately data-only: no methods, no policy. Each actor that embeds a
 * GridMove writes its own advance loop, because the player and the enemy
 * disagree on what happens when the target cell is blocked, on where the
 * next direction comes from, and on whether a pass-through exemption
 * applies. Sharing this struct is the entire overlap between the two; a
 * shared controller would have to be handed the two actors' diverging
 * policies as callbacks, which is worse than the ~10-line loop it would
 * replace.
 */
struct GridMove {
    int cellX = 0;     ///< LOGICAL cell X — the one every rule reads.
    int cellY = 0;     ///< LOGICAL cell Y — the one every rule reads.
    int toX = 0;        ///< Target cell X; equals cellX when at rest.
    int toY = 0;        ///< Target cell Y; equals cellY when at rest.
    int progress = 0;   ///< 0..stepsPerCell-1; 0 means "at rest in (cellX, cellY)".
};

}  // namespace bomberbot
