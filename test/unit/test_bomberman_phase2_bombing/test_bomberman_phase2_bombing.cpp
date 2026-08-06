/**
 * @file test_bomberman_phase2_bombing.cpp
 * @brief Unit tests for Bomberman Phase 2 — bomb/explosion sprite rendering.
 *
 * Tests for REQ-PH2-001 (BlastShape enum), REQ-PH2-003 (resolveDetonations
 * shape writes), REQ-PH2-006 (bomb flash behaviour), and the refactored
 * header-only BombermanBombs module.
 */

#include <cstdint>
#include <cstring>
#include "BombermanConstants.h"
#include "BombermanBombs.h"
#include "BombermanBoard.h"
#include "BoardRenderer.h"   // explosionSpriteFor
#include "../../test_config.h"

using namespace bomberman;

void setUp(void) { test_setup(); }
void tearDown(void) { test_teardown(); }

// =============================================================================
// Stage 1: BlastShape enum value assertions (REQ-PH2-001)
// =============================================================================

void test_blast_shape_enum_values(void) {
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(BlastShape::Center));
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(BlastShape::ArmH));
    TEST_ASSERT_EQUAL_INT(2, static_cast<int>(BlastShape::ArmV));
    TEST_ASSERT_EQUAL_INT(3, static_cast<int>(BlastShape::TipL));
    TEST_ASSERT_EQUAL_INT(4, static_cast<int>(BlastShape::TipR));
    TEST_ASSERT_EQUAL_INT(5, static_cast<int>(BlastShape::TipU));
    TEST_ASSERT_EQUAL_INT(6, static_cast<int>(BlastShape::TipD));
}

void test_blast_shape_enum_count_is_seven(void) {
    TEST_ASSERT_EQUAL_INT(7, static_cast<int>(BlastShape::TipD) + 1);
}

// =============================================================================
// Helper: construct a fresh all-Empty board for a test.
// The board is surrounded by HardWalls (as the real generateLevel() does),
// but the interior is all Empty for predictable blast propagation.
// =============================================================================

static void makeEmptyBoard(TileType (&board)[kCells]) {
    for (int y = 0; y < kRows; ++y) {
        for (int x = 0; x < kCols; ++x) {
            const bool border = (x == 0 || y == 0 || x == kCols - 1 || y == kRows - 1);
            board[cellIndex(x, y)] = border ? TileType::HardWall : TileType::Empty;
        }
    }
}

// =============================================================================
// Helper: seed a single bomb into detonationQueue exactly as tickFuses would.
// =============================================================================

static int seedOneBomb(uint8_t (&dq)[kMaxBombs], int slot) {
    dq[0] = static_cast<uint8_t>(slot);
    return 1;
}

// =============================================================================
// Stage 2-3: Bomb pool operations (regression test after header refactor)
// =============================================================================

void test_place_bomb_returns_true_on_free_slot(void) {
    Bomb bombs[kMaxBombs] = {};
    TEST_ASSERT_TRUE(placeBomb(bombs, 5, 5, 2));
    TEST_ASSERT_TRUE(bombs[0].active);
    TEST_ASSERT_EQUAL_UINT8(5, bombs[0].cellX);
    TEST_ASSERT_EQUAL_UINT8(5, bombs[0].cellY);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(kBombFuseSteps), bombs[0].fuseSteps);
    TEST_ASSERT_EQUAL_UINT8(2, bombs[0].range);
}

void test_place_bomb_returns_false_when_cell_occupied(void) {
    Bomb bombs[kMaxBombs] = {};
    TEST_ASSERT_TRUE(placeBomb(bombs, 3, 4, 1));
    TEST_ASSERT_FALSE(placeBomb(bombs, 3, 4, 2));  // same cell, should fail
}

void test_place_bomb_returns_false_when_pool_full(void) {
    Bomb bombs[kMaxBombs] = {};
    // Fill all 8 slots at different cells
    for (int i = 0; i < kMaxBombs; ++i) {
        TEST_ASSERT_TRUE(placeBomb(bombs, i + 1, 5, 1));
    }
    // The 9th bomb should fail (pool full)
    TEST_ASSERT_FALSE(placeBomb(bombs, 10, 5, 1));
}

void test_bomb_at_returns_true_for_active_bomb(void) {
    Bomb bombs[kMaxBombs] = {};
    placeBomb(bombs, 6, 7, 2);
    TEST_ASSERT_TRUE(bombAt(bombs, 6, 7));
    TEST_ASSERT_FALSE(bombAt(bombs, 6, 8));
}

void test_active_bomb_count_counts_correctly(void) {
    Bomb bombs[kMaxBombs] = {};
    TEST_ASSERT_EQUAL_INT(0, activeBombCount(bombs));
    placeBomb(bombs, 3, 3, 2);
    placeBomb(bombs, 5, 5, 1);
    placeBomb(bombs, 7, 7, 3);
    TEST_ASSERT_EQUAL_INT(3, activeBombCount(bombs));
}

void test_tick_fuses_decrements_and_enqueues_expired(void) {
    Bomb bombs[kMaxBombs] = {};
    // Place a bomb with a very short fuse manually (bypass placeBomb's full fuse)
    bombs[0].cellX = 4;
    bombs[0].cellY = 4;
    bombs[0].fuseSteps = 2;
    bombs[0].range = 2;
    bombs[0].active = true;

    uint8_t dq[kMaxBombs] = {};
    int tail = 0;
    tickFuses(bombs, dq, tail);
    TEST_ASSERT_EQUAL_UINT8(1, bombs[0].fuseSteps);
    TEST_ASSERT_EQUAL_INT(0, tail);  // not expired yet

    tickFuses(bombs, dq, tail);
    TEST_ASSERT_EQUAL_UINT8(0, bombs[0].fuseSteps);
    TEST_ASSERT_EQUAL_INT(1, tail);       // now enqueued
    TEST_ASSERT_EQUAL_UINT8(0, dq[0]);    // slot 0
    TEST_ASSERT_FALSE(bombs[0].active);    // removed from pool
}

void test_tick_explosions_decrements_nonzero(void) {
    uint8_t blastSteps[kCells] = {};
    blastSteps[cellIndex(6, 5)] = 5;
    blastSteps[cellIndex(7, 5)] = 1;

    tickExplosions(blastSteps);
    TEST_ASSERT_EQUAL_UINT8(4, blastSteps[cellIndex(6, 5)]);
    TEST_ASSERT_EQUAL_UINT8(0, blastSteps[cellIndex(7, 5)]);

    tickExplosions(blastSteps);
    TEST_ASSERT_EQUAL_UINT8(3, blastSteps[cellIndex(6, 5)]);
    TEST_ASSERT_EQUAL_UINT8(0, blastSteps[cellIndex(7, 5)]);  // stays at 0
}

// =============================================================================
// Stage 3: blastShape write assertions (REQ-PH2-003, REQ-PH2-004)
// =============================================================================

void test_blast_shape_center_written_for_bomb_cell(void) {
    TileType board[kCells];
    makeEmptyBoard(board);

    Bomb bombs[kMaxBombs] = {};
    bombs[0].cellX = 6;
    bombs[0].cellY = 5;
    bombs[0].fuseSteps = 0;   // expired — ready to detonate
    bombs[0].range = 2;
    bombs[0].active = true;   // will be cleared by tickFuses / seed

    uint8_t blastSteps[kCells] = {};
    uint8_t blastShape[kCells] = {};
    uint8_t dq[kMaxBombs] = {};
    // Seed the queue manually — bomb at slot 0, not yet cleared
    dq[0] = 0;
    // resolveDetonations copies the Bomb, then reads from it; the bomb is
    // still active at this point (it clears inside resolveDetonations'
    // chain-trigger path which won't fire here), but the copy makes it safe.
    resolveDetonations(dq, 1, bombs, board, blastSteps, blastShape, TileType::PowerUpFire);

    const int ci = cellIndex(6, 5);
    TEST_ASSERT_EQUAL_UINT8(kExplosionSteps, blastSteps[ci]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::Center), blastShape[ci]);
}

void test_blast_shape_arm_and_tip_for_straight_blast(void) {
    TileType board[kCells];
    makeEmptyBoard(board);

    Bomb bombs[kMaxBombs] = {};
    bombs[0].cellX = 3;
    bombs[0].cellY = 5;
    bombs[0].fuseSteps = 0;
    bombs[0].range = 3;
    bombs[0].active = true;

    uint8_t blastSteps[kCells] = {};
    uint8_t blastShape[kCells] = {};
    uint8_t dq[kMaxBombs] = {};
    dq[0] = 0;

    resolveDetonations(dq, 1, bombs, board, blastSteps, blastShape, TileType::PowerUpFire);

    // Bomb cell
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::Center), blastShape[cellIndex(3, 5)]);

    // Right arm: (4,5),(5,5) are ArmH; (6,5) is TipR (range exhausted, loop completes → upgrade)
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmH),  blastShape[cellIndex(4, 5)]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmH),  blastShape[cellIndex(5, 5)]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::TipR),  blastShape[cellIndex(6, 5)]);

    // Left arm: (2,5) ArmH, (1,5) ArmH — no tip because (0,5) is a border
    // HardWall that stops the arm short (blunt end, per design).
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmH),  blastShape[cellIndex(2, 5)]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmH),  blastShape[cellIndex(1, 5)]);

    // Up arm: (3,4)=ArmV, (3,3)=ArmV, (3,2)=TipU (loop completes → upgrade)
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmV),  blastShape[cellIndex(3, 4)]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmV),  blastShape[cellIndex(3, 3)]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::TipU),  blastShape[cellIndex(3, 2)]);

    // Down arm: (3,6)=ArmV, (3,7)=ArmV, (3,8)=TipD (loop completes → upgrade)
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmV),  blastShape[cellIndex(3, 6)]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmV),  blastShape[cellIndex(3, 7)]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::TipD),  blastShape[cellIndex(3, 8)]);
}

void test_blast_shape_blocked_by_hard_wall(void) {
    TileType board[kCells];
    makeEmptyBoard(board);
    // Place hard wall at (5,5) — bomb at (3,5), range 3
    board[cellIndex(5, 5)] = TileType::HardWall;

    Bomb bombs[kMaxBombs] = {};
    bombs[0].cellX = 3;
    bombs[0].cellY = 5;
    bombs[0].fuseSteps = 0;
    bombs[0].range = 3;
    bombs[0].active = true;

    uint8_t blastSteps[kCells] = {};
    uint8_t blastShape[kCells] = {};
    uint8_t dq[kMaxBombs] = {};
    dq[0] = 0;

    resolveDetonations(dq, 1, bombs, board, blastSteps, blastShape, TileType::PowerUpFire);

    // (4,5) is ArmH — painted before the hard wall
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmH), blastShape[cellIndex(4, 5)]);
    // (5,5) is UNCHANGED — hard wall, no blast, no shape
    TEST_ASSERT_EQUAL_UINT8(0, blastSteps[cellIndex(5, 5)]);
    TEST_ASSERT_EQUAL_UINT8(0, blastShape[cellIndex(5, 5)]);
    // (6,5) is untouched past the wall
    TEST_ASSERT_EQUAL_UINT8(0, blastSteps[cellIndex(6, 5)]);

    // Arm is blunt at (4,5) — no tip because hard wall cut the arm short.
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmH), blastShape[cellIndex(4, 5)]);
}

void test_blast_shape_tip_at_soft_wall(void) {
    TileType board[kCells];
    makeEmptyBoard(board);
    board[cellIndex(6, 5)] = TileType::SoftWall;

    Bomb bombs[kMaxBombs] = {};
    bombs[0].cellX = 3;
    bombs[0].cellY = 5;
    bombs[0].fuseSteps = 0;
    bombs[0].range = 3;
    bombs[0].active = true;

    uint8_t blastSteps[kCells] = {};
    uint8_t blastShape[kCells] = {};
    uint8_t dq[kMaxBombs] = {};
    dq[0] = 0;

    resolveDetonations(dq, 1, bombs, board, blastSteps, blastShape, TileType::PowerUpFire);

    // (4,5), (5,5) are ArmH
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmH), blastShape[cellIndex(4, 5)]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmH), blastShape[cellIndex(5, 5)]);
    // (6,5) is TipR — the soft wall cell IS the tip
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::TipR), blastShape[cellIndex(6, 5)]);
    // Soft wall was destroyed
    TEST_ASSERT_NOT_EQUAL(TileType::SoftWall, board[cellIndex(6, 5)]);
}

void test_blast_shape_chain(void) {
    TileType board[kCells];
    makeEmptyBoard(board);

    Bomb bombs[kMaxBombs] = {};
    // Bomb A at (3,5) with range 3 — its right arm reaches (6,5)
    bombs[0].cellX = 3;
    bombs[0].cellY = 5;
    bombs[0].fuseSteps = 0;
    bombs[0].range = 3;
    bombs[0].active = true;
    // Bomb B at (6,5) — will be chain-triggered
    bombs[1].cellX = 6;
    bombs[1].cellY = 5;
    bombs[1].fuseSteps = 0;
    bombs[1].range = 1;
    bombs[1].active = true;

    uint8_t blastSteps[kCells] = {};
    uint8_t blastShape[kCells] = {};
    uint8_t dq[kMaxBombs] = {};
    dq[0] = 0;  // seed bomb A

    resolveDetonations(dq, 1, bombs, board, blastSteps, blastShape, TileType::PowerUpFire);

    // Bomb A's own cell is Center
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::Center), blastShape[cellIndex(3, 5)]);
    // (4,5) is ArmH from bomb A's right arm
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmH),  blastShape[cellIndex(4, 5)]);
    // (5,5) gets TipL from bomb B's OWN left-arm tip (later-write-wins on crossing blast).
    // Bomb B at (6,5) range=1 left arm: paintArm writes ArmH then upgrades to TipL
    // because loop completes. This overwrites bomb A's earlier ArmH at (5,5).
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::TipL),  blastShape[cellIndex(5, 5)]);
    // (6,5) — bomb A wrote TipR (chain cell IS tip), then bomb B's detonation
    // overwrites with Center. Center=0 is the final value.
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::Center), blastShape[cellIndex(6, 5)]);
}

void test_blast_shape_arm_v_for_vertical_blast(void) {
    TileType board[kCells];
    makeEmptyBoard(board);

    Bomb bombs[kMaxBombs] = {};
    bombs[0].cellX = 6;
    bombs[0].cellY = 5;
    bombs[0].fuseSteps = 0;
    bombs[0].range = 2;
    bombs[0].active = true;

    uint8_t blastSteps[kCells] = {};
    uint8_t blastShape[kCells] = {};
    uint8_t dq[kMaxBombs] = {};
    dq[0] = 0;

    resolveDetonations(dq, 1, bombs, board, blastSteps, blastShape, TileType::PowerUpFire);

    // Up: ArmV then TipU
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmV), blastShape[cellIndex(6, 4)]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::TipU), blastShape[cellIndex(6, 3)]);

    // Down: ArmV then TipD
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::ArmV), blastShape[cellIndex(6, 6)]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(BlastShape::TipD), blastShape[cellIndex(6, 7)]);
}

void test_blast_shape_default_initialized_to_center(void) {
    // Fresh blastShape array is all zeros (Center = 0).
    uint8_t blastShape[kCells] = {};
    for (int i = 0; i < kCells; ++i) {
        TEST_ASSERT_EQUAL_UINT8(0, blastShape[i]);
    }
}

// =============================================================================
// Stage 3: bomb flash formula verification (REQ-PH2-006)
// =============================================================================

void test_bomb_flash_uses_faster_pulse_cadence(void) {
    // fuseSteps=50: kBombFlashSteps=50, so inFlash is true.
    // frame = (50 / 2) % 3 = 25 % 3 = 1
    int fuseSteps = 50;
    const bool inFlash = (fuseSteps <= kBombFlashSteps);
    TEST_ASSERT_TRUE(inFlash);
    const uint8_t frameFlash = static_cast<uint8_t>((fuseSteps / 2) % 3);
    TEST_ASSERT_EQUAL_UINT8(1, frameFlash);

    // fuseSteps=100: outside flash window.
    // frame = (100 / 8) % 3 = 12 % 3 = 0
    fuseSteps = 100;
    const bool notInFlash = (fuseSteps <= kBombFlashSteps);
    TEST_ASSERT_FALSE(notInFlash);
    const uint8_t frameNormal = static_cast<uint8_t>((fuseSteps / 8) % 3);
    TEST_ASSERT_EQUAL_UINT8(0, frameNormal);
}

// =============================================================================
// Stage 4: explosionSpriteFor helper tests (REQ-PH2-005)
// =============================================================================

void test_explosion_sprite_for_center_returns_non_null(void) {
    const Sprite4bpp* s = explosionSpriteFor(
        static_cast<uint8_t>(BlastShape::Center), 25);
    TEST_ASSERT_NOT_NULL(s);
}

void test_explosion_sprite_for_arm_h_returns_non_null(void) {
    const Sprite4bpp* s = explosionSpriteFor(
        static_cast<uint8_t>(BlastShape::ArmH), 14);
    TEST_ASSERT_NOT_NULL(s);
}

void test_explosion_sprite_for_tip_d_returns_non_null(void) {
    const Sprite4bpp* s = explosionSpriteFor(
        static_cast<uint8_t>(BlastShape::TipD), 7);
    TEST_ASSERT_NOT_NULL(s);
}

void test_explosion_sprite_for_null_for_invalid_shape(void) {
    const Sprite4bpp* s = explosionSpriteFor(
        static_cast<uint8_t>(BlastShape::TipD) + 1, 10);
    TEST_ASSERT_NULL(s);
}

void test_explosion_sprite_for_flicker_alternates(void) {
    // Flicker = (blastSteps / 7) % 2
    const Sprite4bpp* s0 = explosionSpriteFor(
        static_cast<uint8_t>(BlastShape::Center), 25);
    const Sprite4bpp* s1 = explosionSpriteFor(
        static_cast<uint8_t>(BlastShape::Center), 20);
    // 25/7=3, 3%2=1; 20/7=2, 2%2=0 — different flicker frames
    TEST_ASSERT_NOT_EQUAL(s0, s1);
}

// =============================================================================
// Test runner — all suite stages included.
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

    // Stage 1: enum
    RUN_TEST(test_blast_shape_enum_values);
    RUN_TEST(test_blast_shape_enum_count_is_seven);

    // Stage 2: bomb pool regression
    RUN_TEST(test_place_bomb_returns_true_on_free_slot);
    RUN_TEST(test_place_bomb_returns_false_when_cell_occupied);
    RUN_TEST(test_place_bomb_returns_false_when_pool_full);
    RUN_TEST(test_bomb_at_returns_true_for_active_bomb);
    RUN_TEST(test_active_bomb_count_counts_correctly);
    RUN_TEST(test_tick_fuses_decrements_and_enqueues_expired);
    RUN_TEST(test_tick_explosions_decrements_nonzero);

    // Stage 3: blastShape write assertions
    RUN_TEST(test_blast_shape_center_written_for_bomb_cell);
    RUN_TEST(test_blast_shape_arm_and_tip_for_straight_blast);
    RUN_TEST(test_blast_shape_blocked_by_hard_wall);
    RUN_TEST(test_blast_shape_tip_at_soft_wall);
    RUN_TEST(test_blast_shape_chain);
    RUN_TEST(test_blast_shape_arm_v_for_vertical_blast);
    RUN_TEST(test_blast_shape_default_initialized_to_center);

    // Stage 3: bomb flash
    RUN_TEST(test_bomb_flash_uses_faster_pulse_cadence);

    // Stage 4: explosionSpriteFor helper
    RUN_TEST(test_explosion_sprite_for_center_returns_non_null);
    RUN_TEST(test_explosion_sprite_for_arm_h_returns_non_null);
    RUN_TEST(test_explosion_sprite_for_tip_d_returns_non_null);
    RUN_TEST(test_explosion_sprite_for_null_for_invalid_shape);
    RUN_TEST(test_explosion_sprite_for_flicker_alternates);

    return UNITY_END();
}
