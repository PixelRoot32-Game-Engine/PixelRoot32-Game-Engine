/**
 * @file test_bomberbot_phase3_actors.cpp
 * @brief Unit tests for Bomberbot Phase 3 actor sprite wiring.
 */

#include <unity.h>
#include "graphics/Renderer.h"
#include "BomberbotConstants.h"
#include "PlayerActor.h"        // for playerWalkSpriteFor + PlayerWalkFrame
#include "EnemyActor.h"        // for enemyWalkSpriteFor
#include "assets/PlayerSprites.h"
#include "assets/EnemySprites.h"
#include "../../test_config.h"

using namespace bomberbot;
using pixelroot32::graphics::Sprite4bpp;

void setUp(void) { test_setup(); }
void tearDown(void) { test_teardown(); }

// --- Stage 1: Constant tests (T3-TEST-CONST-01) ---

void test_kplayer_anim_step_div_equals_4(void) {
    TEST_ASSERT_EQUAL_INT(4, kPlayerAnimStepDiv);
}

void test_kenemy_anim_step_div_equals_4(void) {
    TEST_ASSERT_EQUAL_INT(4, kEnemyAnimStepDiv);
}

// --- Stage 2: Helper tests (T3-TEST-HELPER-01) ---

void test_player_walk_sprite_for_down_at_rest(void) {
    PlayerWalkFrame f = playerWalkSpriteFor(0, 0);
    TEST_ASSERT_EQUAL_PTR(&kPlayerWalkDown[0], f.sprite);
    TEST_ASSERT_FALSE(f.flipX);
}

void test_player_walk_sprite_for_left_mirrors_right(void) {
    PlayerWalkFrame f = playerWalkSpriteFor(2, 4);
    // (4/4) % 3 = 1 → kPlayerWalkRight[1], flipX=true
    TEST_ASSERT_EQUAL_PTR(&kPlayerWalkRight[1], f.sprite);
    TEST_ASSERT_TRUE(f.flipX);
}

void test_player_walk_sprite_for_right(void) {
    PlayerWalkFrame f = playerWalkSpriteFor(3, 8);
    // (8/4) % 3 = 2 → kPlayerWalkRight[2], flipX=false
    TEST_ASSERT_EQUAL_PTR(&kPlayerWalkRight[2], f.sprite);
    TEST_ASSERT_FALSE(f.flipX);
}

void test_player_walk_sprite_for_up(void) {
    PlayerWalkFrame f = playerWalkSpriteFor(1, 11);
    // (11/4) % 3 = 2 → kPlayerWalkUp[2]
    TEST_ASSERT_EQUAL_PTR(&kPlayerWalkUp[2], f.sprite);
    TEST_ASSERT_FALSE(f.flipX);
}

void test_player_frame_index_caps_at_two(void) {
    PlayerWalkFrame f = playerWalkSpriteFor(3, 11);
    // (11 / 4) % 3 = 2 % 3 = 2
    TEST_ASSERT_EQUAL_PTR(&kPlayerWalkRight[2], f.sprite);
}

void test_enemy_walk_sprite_for_at_rest(void) {
    TEST_ASSERT_EQUAL_PTR(&kEnemySlimeWalk[0], enemyWalkSpriteFor(0));
}

void test_enemy_walk_sprite_for_mid_step(void) {
    // (4/4) % 7 = 1
    TEST_ASSERT_EQUAL_PTR(&kEnemySlimeWalk[1], enemyWalkSpriteFor(4));
}

void test_enemy_walk_sprite_for_wraps_around_7(void) {
    // 28/4 = 7, 7 % 7 = 0
    TEST_ASSERT_EQUAL_PTR(&kEnemySlimeWalk[0], enemyWalkSpriteFor(28));
}

// --- Stage 3: Sprite table size tests (T3-TEST-SHAPE-01) ---

void test_slime_walk_has_7_frames(void) {
    TEST_ASSERT_EQUAL_INT(7, (int)(sizeof(kEnemySlimeWalk) / sizeof(Sprite4bpp)));
}

void test_player_walk_down_table_has_3_frames(void) {
    TEST_ASSERT_EQUAL_INT(3, (int)(sizeof(kPlayerWalkDown) / sizeof(Sprite4bpp)));
    TEST_ASSERT_EQUAL_INT(3, (int)(sizeof(kPlayerWalkUp) / sizeof(Sprite4bpp)));
    TEST_ASSERT_EQUAL_INT(3, (int)(sizeof(kPlayerWalkRight) / sizeof(Sprite4bpp)));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_kplayer_anim_step_div_equals_4);
    RUN_TEST(test_kenemy_anim_step_div_equals_4);
    RUN_TEST(test_player_walk_sprite_for_down_at_rest);
    RUN_TEST(test_player_walk_sprite_for_left_mirrors_right);
    RUN_TEST(test_player_walk_sprite_for_right);
    RUN_TEST(test_player_walk_sprite_for_up);
    RUN_TEST(test_player_frame_index_caps_at_two);
    RUN_TEST(test_enemy_walk_sprite_for_at_rest);
    RUN_TEST(test_enemy_walk_sprite_for_mid_step);
    RUN_TEST(test_enemy_walk_sprite_for_wraps_around_7);
    RUN_TEST(test_slime_walk_has_7_frames);
    RUN_TEST(test_player_walk_down_table_has_3_frames);
    return UNITY_END();
}
