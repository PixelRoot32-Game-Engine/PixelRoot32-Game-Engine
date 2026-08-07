/**
 * @file test_bomberbot_phase1_wiring.cpp
 * @brief Unit tests for Bomberbot Phase 1 sprite wiring.
 *
 * Tests for REQ-PH1-002 (soft-wall secrecy invariant) and REQ-PH1-004
 * (palette registration invariants). The actual draw() call is not unit
 * tested (requires a real Renderer instance); visual verification is
 * manual per the audit doc §7 checklist.
 */

#include <unity.h>
#include "graphics/Renderer.h"
#include "BoardRenderer.h"     // softWallSpriteFor declaration
#include "assets/BomberbotPalette.h"
#include "assets/BoardTiles.h"  // kSoftWallSprite
#include "BomberbotBoard.h"     // TileType
#include "../../test_config.h"

using namespace bomberbot;
using pixelroot32::graphics::Sprite4bpp;

void setUp(void) { test_setup(); }
void tearDown(void) { test_teardown(); }

// Test 1
void test_soft_wall_sprite_for_returns_ksprite_for_all_3_variants(void) {
    const Sprite4bpp* a = softWallSpriteFor(TileType::SoftWall);
    const Sprite4bpp* b = softWallSpriteFor(TileType::SoftWallHidingExit);
    const Sprite4bpp* c = softWallSpriteFor(TileType::SoftWallHidingPowerUp);
    TEST_ASSERT_EQUAL_PTR(&kSoftWallSprite, a);
    TEST_ASSERT_EQUAL_PTR(&kSoftWallSprite, b);
    TEST_ASSERT_EQUAL_PTR(&kSoftWallSprite, c);
}

// Test 2
void test_soft_wall_sprite_for_returns_null_for_non_soft_walls(void) {
    TEST_ASSERT_NULL(softWallSpriteFor(TileType::HardWall));
    TEST_ASSERT_NULL(softWallSpriteFor(TileType::Empty));
    TEST_ASSERT_NULL(softWallSpriteFor(TileType::Exit));
    TEST_ASSERT_NULL(softWallSpriteFor(TileType::PowerUpFire));
    TEST_ASSERT_NULL(softWallSpriteFor(TileType::PowerUpBomb));
}

// Test 3 — the secrecy invariant load-bearing assertion
void test_soft_wall_pointer_equality_across_variants(void) {
    const Sprite4bpp* a = softWallSpriteFor(TileType::SoftWall);
    const Sprite4bpp* b = softWallSpriteFor(TileType::SoftWallHidingExit);
    const Sprite4bpp* c = softWallSpriteFor(TileType::SoftWallHidingPowerUp);
    TEST_ASSERT_EQUAL_PTR(a, b);
    TEST_ASSERT_EQUAL_PTR(b, c);
}

// Test 4
void test_bomberbot_palette_is_16_entries_with_transparent_index_zero(void) {
    TEST_ASSERT_EQUAL(16, (int)(sizeof(BOMBERBOT_SPRITE_PALETTE_RGB565) / sizeof(uint16_t)));
    TEST_ASSERT_EQUAL_UINT16(0x0000, BOMBERBOT_SPRITE_PALETTE_RGB565[0]);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_soft_wall_sprite_for_returns_ksprite_for_all_3_variants);
    RUN_TEST(test_soft_wall_sprite_for_returns_null_for_non_soft_walls);
    RUN_TEST(test_soft_wall_pointer_equality_across_variants);
    RUN_TEST(test_bomberbot_palette_is_16_entries_with_transparent_index_zero);
    return UNITY_END();
}
