#include "BombermanBombs.h"

namespace bomberman {

bool bombAt(const Bomb (&bombs)[kMaxBombs], int cellX, int cellY) {
    for (int i = 0; i < kMaxBombs; ++i) {
        if (bombs[i].active && bombs[i].cellX == cellX && bombs[i].cellY == cellY) {
            return true;
        }
    }
    return false;
}

int activeBombCount(const Bomb (&bombs)[kMaxBombs]) {
    int count = 0;
    for (int i = 0; i < kMaxBombs; ++i) {
        if (bombs[i].active) {
            ++count;
        }
    }
    return count;
}

bool placeBomb(Bomb (&bombs)[kMaxBombs], int cellX, int cellY, int range) {
    if (bombAt(bombs, cellX, cellY)) {
        return false;
    }
    for (int i = 0; i < kMaxBombs; ++i) {
        if (!bombs[i].active) {
            bombs[i].cellX = static_cast<uint8_t>(cellX);
            bombs[i].cellY = static_cast<uint8_t>(cellY);
            bombs[i].fuseSteps = static_cast<uint8_t>(kBombFuseSteps);
            bombs[i].range = static_cast<uint8_t>(range);
            bombs[i].active = true;
            return true;
        }
    }
    return false;  // pool full
}

void tickFuses(Bomb (&bombs)[kMaxBombs], uint8_t (&detonationQueue)[kMaxBombs], int& queueTail) {
    for (int i = 0; i < kMaxBombs; ++i) {
        if (!bombs[i].active) {
            continue;
        }
        if (bombs[i].fuseSteps > 0) {
            --bombs[i].fuseSteps;
        }
        if (bombs[i].fuseSteps == 0) {
            bombs[i].active = false;
            if (queueTail < kMaxBombs) {  // bounded at the write site
                detonationQueue[queueTail++] = static_cast<uint8_t>(i);
            }
        }
    }
}

namespace {

/// Walks one direction from (cx, cy) out to `range` cells, painting
/// blastSteps and applying destruction/chain-triggering per the header's
/// propagation rules. `tail` is bounded against kMaxBombs at the write
/// site — never assumed safe purely because the caller promised it.
void paintArm(int cx, int cy, int dx, int dy, int range,
              TileType (&board)[kCells], uint8_t (&blastSteps)[kCells],
              uint8_t (&detonationQueue)[kMaxBombs], int& tail,
              Bomb (&bombs)[kMaxBombs], TileType hiddenPowerUp) {
    int x = cx;
    int y = cy;
    for (int step = 0; step < range; ++step) {
        x += dx;
        y += dy;
        if (!gameplay::containsCell(x, y, kBoardGrid)) {
            return;  // the border is always HardWall; unreachable in practice
        }
        const int idx = cellIndex(x, y);
        const TileType t = board[idx];

        if (t == TileType::HardWall) {
            return;  // stops the arm; the cell is untouched
        }
        if (isSoftWall(t)) {
            board[idx] = destroyedInto(t, hiddenPowerUp);
            blastSteps[idx] = static_cast<uint8_t>(kExplosionSteps);
            return;  // destroyed, then the arm stops
        }

        bool chained = false;
        for (int i = 0; i < kMaxBombs; ++i) {
            if (bombs[i].active && bombs[i].cellX == x && bombs[i].cellY == y) {
                bombs[i].active = false;
                if (tail < kMaxBombs) {  // bounded at the write site
                    detonationQueue[tail++] = static_cast<uint8_t>(i);
                }
                chained = true;
                break;
            }
        }
        blastSteps[idx] = static_cast<uint8_t>(kExplosionSteps);
        if (chained) {
            // The chained bomb produces its own independent cross on a
            // later iteration of the drain loop; this arm stops here so
            // the two crosses never repaint the same cells against each
            // other.
            return;
        }
        // Otherwise: Empty, or a cell whose bomb already detonated earlier
        // in this same drain — paint it and let the arm continue.
    }
}

}  // namespace

int resolveDetonations(uint8_t (&detonationQueue)[kMaxBombs], int queueTail,
                        Bomb (&bombs)[kMaxBombs],
                        TileType (&board)[kCells],
                        uint8_t (&blastSteps)[kCells],
                        TileType hiddenPowerUp) {
    int head = 0;
    int tail = (queueTail < kMaxBombs) ? queueTail : kMaxBombs;  // bounded at the write site

    static constexpr int kArmDX[4] = {0, 0, -1, 1};
    static constexpr int kArmDY[4] = {-1, 1, 0, 0};

    while (head < tail) {
        const Bomb b = bombs[detonationQueue[head++]];  // copy: paintArm mutates other slots
        blastSteps[cellIndex(b.cellX, b.cellY)] = static_cast<uint8_t>(kExplosionSteps);
        for (int d = 0; d < 4; ++d) {
            paintArm(b.cellX, b.cellY, kArmDX[d], kArmDY[d], b.range,
                     board, blastSteps, detonationQueue, tail, bombs, hiddenPowerUp);
        }
    }
    return tail;
}

void tickExplosions(uint8_t (&blastSteps)[kCells]) {
    for (int i = 0; i < kCells; ++i) {
        if (blastSteps[i] > 0) {
            --blastSteps[i];
        }
    }
}

}  // namespace bomberman
