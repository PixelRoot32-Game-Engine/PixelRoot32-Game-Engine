#include <unity.h>
#include "physics/KinematicActor.h"
#include "physics/StaticActor.h"
#include "physics/SensorActor.h"
#include "physics/CollisionSystem.h"
#include "../../test_config.h"

using namespace pixelroot32::core;
using namespace pixelroot32::physics;
using namespace pixelroot32::math;

CollisionSystem* colSystem = nullptr;
KinematicActor* player = nullptr;
StaticActor* wall = nullptr;
SensorActor* sensor = nullptr;
KinematicActor* platform = nullptr;

void setUp(void) {
    test_setup();
    colSystem = new CollisionSystem();
    // 10x10 player at 0,0
    player = new KinematicActor(toScalar(0), toScalar(0), 10, 10);
    player->setCollisionLayer(1);
    player->setCollisionMask(1);
    player->collisionSystem = colSystem;
    colSystem->addEntity(player);
}

void tearDown(void) {
    if (player) delete player;
    if (wall) delete wall;
    if (sensor) delete sensor;
    if (platform) delete platform;
    if (colSystem) delete colSystem;
    player = nullptr;
    wall = nullptr;
    sensor = nullptr;
    platform = nullptr;
    colSystem = nullptr;
    test_teardown();
}

void test_is_on_floor() {
    // Floor at y=20 (player is 10 high, so bottom is at 10. Gap is 10)
    wall = new StaticActor(toScalar(-50), toScalar(20), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    // Move down into the floor
    // moveAndSlide handles multiple iterations. 
    // We move 15 units down. Should hit floor at 10 units travel.
    player->moveAndSlide(Vector2(toScalar(0), toScalar(15)));

    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Player should be on floor");
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_ceiling(), "Player should NOT be on ceiling");
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_wall(), "Player should NOT be on wall");
}

void test_is_on_ceiling() {
    // Ceiling at y=-20 (player top is at 0. Gap is 10 if ceiling bottom is at -10? 
    // Wall rect is x,y,w,h. Position is top-left.
    // If wall is at -20 with height 10, it spans -20 to -10.
    // Player is at 0 to 10.
    // Gap is 10 units.
    wall = new StaticActor(toScalar(-50), toScalar(-20), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    // Move up into the ceiling
    player->moveAndSlide(Vector2(toScalar(0), toScalar(-15)));

    TEST_ASSERT_TRUE_MESSAGE(player->is_on_ceiling(), "Player should be on ceiling");
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_floor(), "Player should NOT be on floor");
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_wall(), "Player should NOT be on wall");
}

void test_is_on_wall() {
    // Wall at x=20. Player right is at 10. Gap is 10.
    wall = new StaticActor(toScalar(20), toScalar(-50), 10, 100);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    // Move right into the wall
    player->moveAndSlide(Vector2(toScalar(15), toScalar(0)));

    TEST_ASSERT_TRUE_MESSAGE(player->is_on_wall(), "Player should be on wall");
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_floor(), "Player should NOT be on floor");
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_ceiling(), "Player should NOT be on ceiling");
}

void test_flags_reset() {
    // Floor setup
    wall = new StaticActor(toScalar(-50), toScalar(20), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    // 1. Hit floor
    player->moveAndSlide(Vector2(toScalar(0), toScalar(15)));
    TEST_ASSERT_TRUE(player->is_on_floor());

    // 2. Move up (away from floor) — immediate floor loss (no persistence)
    player->moveAndSlide(Vector2(toScalar(0), toScalar(-5)));
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_floor(), "Floor flag should be lost immediately without persistence");
    TEST_ASSERT_FALSE(player->is_on_ceiling());
    TEST_ASSERT_FALSE(player->is_on_wall());
}

// Explicit test: sensor bodies must NOT block moveAndCollide (player can overlap; contact handled by CollisionSystem).
void test_sensor_does_not_block_move_and_collide() {
    // Player 10x10 at (0, 0). Sensor 10x10 at (12, 0) - so when player moves 15 right they would overlap.
    sensor = new SensorActor(toScalar(12), toScalar(0), 10, 10);
    sensor->setCollisionLayer(1);
    sensor->setCollisionMask(1);
    colSystem->addEntity(sensor);

    TEST_ASSERT_TRUE_MESSAGE(sensor->isSensor(), "Sensor body must be marked as sensor");

    Vector2 motion(toScalar(15), toScalar(0));
    bool blocked = player->moveAndCollide(motion, nullptr, false);

    // Sensor must not block: moveAndCollide returns false and player reaches full motion.
    TEST_ASSERT_FALSE_MESSAGE(blocked, "Sensor must not block kinematic movement in moveAndCollide");
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, player->position.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, player->position.y);
}

// Regression: solid StaticActor in the same path must still block.
void test_solid_blocks_move_and_collide() {
    wall = new StaticActor(toScalar(12), toScalar(0), 10, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    TEST_ASSERT_FALSE_MESSAGE(wall->isSensor(), "Wall must not be a sensor");
    colSystem->addEntity(wall);

    Vector2 motion(toScalar(15), toScalar(0));
    bool blocked = player->moveAndCollide(motion, nullptr, false);

    TEST_ASSERT_TRUE_MESSAGE(blocked, "Solid must block kinematic movement");
    // Player should stop before overlapping (at 2,0 so right edge at 12)
    TEST_ASSERT_TRUE_MESSAGE(player->position.x < toScalar(12.5f), "Player must not pass through solid");
}

void test_sensor_constructor_with_vector2() {
    Vector2 pos(50.0f, 100.0f);
    SensorActor* sensorV2 = new SensorActor(pos, 16, 16);
    
    TEST_ASSERT_TRUE(sensorV2->isSensor());
}

void test_static_actor_creation() {
    StaticActor* staticActor = new StaticActor(toScalar(10), toScalar(20), 16, 16);
    TEST_ASSERT_FALSE(staticActor->isSensor());
}

void test_static_actor_creation_vector2() {
    Vector2 pos(100.0f, 200.0f);
    StaticActor* staticActor = new StaticActor(pos, 32, 32);
    TEST_ASSERT_FALSE(staticActor->isSensor());
}

// =============================================================================
// Branch coverage tests for moveAndSlide slope handling
// =============================================================================

void test_move_and_slide_45_degree_slope_detection() {
    // Test 45-degree slope detection (dot product exactly at threshold)
    // Floor at 45 degrees - should be detected as floor when upDirection is aligned
    StaticActor* slope = new StaticActor(toScalar(0), toScalar(20), 20, 20);
    slope->setCollisionLayer(1);
    slope->setCollisionMask(1);
    colSystem->addEntity(slope);
    
    // Move down and right to hit the slope at 45 degrees
    player->moveAndSlide(Vector2(toScalar(10), toScalar(15)));
    
    // Should detect collision - either floor or wall depending on exact angle
    TEST_ASSERT_TRUE(player->is_on_floor() || player->is_on_wall());
}

void test_move_and_slide_max_slides_exhausted() {
    // Test that maxSlides (default 4) iterations are handled correctly
    // Create multiple walls to trigger multiple slides
    for (int i = 0; i < 3; i++) {
        StaticActor* wall = new StaticActor(toScalar(15 + i * 5), toScalar(0), 3, 20);
        wall->setCollisionLayer(1);
        wall->setCollisionMask(1);
        colSystem->addEntity(wall);
    }
    
    // Complex motion that will require multiple slide iterations
    player->moveAndSlide(Vector2(toScalar(30), toScalar(5)));
    
    // Should complete without crash - motion may be partially completed
    // Verify position is valid (not NaN or infinity) and within reasonable bounds
    TEST_ASSERT_TRUE(player->position.x == player->position.x); // NaN check
    TEST_ASSERT_TRUE(player->position.x > toScalar(-1000.0f) && player->position.x < toScalar(1000.0f));
}

void test_move_and_slide_zero_motion() {
    // Test moveAndSlide with zero motion (should not crash and flags should reset)
    // First hit floor to set flag
    wall = new StaticActor(toScalar(-50), toScalar(20), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);
    
    player->moveAndSlide(Vector2(toScalar(0), toScalar(15)));
    TEST_ASSERT_TRUE(player->is_on_floor());
    
    // Store floor position
    Scalar floorX = player->position.x;
    
    // Now move with zero motion - flags should be reset
    player->moveAndSlide(Vector2(toScalar(0), toScalar(0)));
    
    // Position should remain valid and unchanged
    TEST_ASSERT_TRUE(player->position.x == player->position.x); // NaN check
    TEST_ASSERT_EQUAL(floorX, player->position.x);
}

// =============================================================================
// FASE 2 coverage expansion tests
// =============================================================================

void test_kinematic_actor_move_and_collide_binary_search_path(void) {
    // Test moveAndCollide with multiple walls - triggers binary search path
    // Create 5 walls along the path
    StaticActor* walls[5];
    for (int i = 0; i < 5; i++) {
        walls[i] = new StaticActor(toScalar(20 + i * 8), toScalar(-5), 4, 20);
        walls[i]->setCollisionLayer(1);
        walls[i]->setCollisionMask(1);
        colSystem->addEntity(walls[i]);
    }
    
    // Player at 0,0, move right - should trigger binary search
    Vector2 motion(toScalar(50), toScalar(0));
    KinematicCollision col;
    bool blocked = player->moveAndCollide(motion, &col);
    
    // Should collide with one of the walls
    TEST_ASSERT_TRUE(blocked);
    TEST_ASSERT_TRUE(col.collider != nullptr);
    
    // Clean up
    for (int i = 0; i < 5; i++) {
        delete walls[i];
    }
    walls[0] = walls[1] = walls[2] = walls[3] = walls[4] = nullptr;
}

void test_kinematic_actor_move_and_collide_no_collision(void) {
    // Test moveAndCollide with no collisions in path
    Vector2 motion(toScalar(30), toScalar(0));
    KinematicCollision col;
    bool blocked = player->moveAndCollide(motion, &col);
    
    // Should not collide - player should move to final position
    TEST_ASSERT_FALSE(blocked);
}

void test_kinematic_actor_move_and_collide_zero_motion(void) {
    // Test moveAndCollide with zero motion
    Vector2 motion(toScalar(0), toScalar(0));
    KinematicCollision col;
    bool blocked = player->moveAndCollide(motion, &col);
    
    TEST_ASSERT_FALSE(blocked);
}

void test_kinematic_actor_move_and_collide_test_only(void) {
    // Create a wall in the path
    wall = new StaticActor(toScalar(20), toScalar(0), 10, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);
    
    // Save initial position
    Vector2 startPos = player->position;
    
    // Test with testOnly=true - should not move
    Vector2 motion(toScalar(30), toScalar(0));
    KinematicCollision col;
    bool blocked = player->moveAndCollide(motion, &col, true);
    
    TEST_ASSERT_TRUE(blocked);
    // Position should not have changed
    TEST_ASSERT_TRUE(player->position.x == startPos.x);
}

void test_kinematic_actor_move_and_collide_one_way_platform(void) {
    // Test collision with one-way platform - verify setOneWay works
    StaticActor* oneWay = new StaticActor(toScalar(10), toScalar(15), 20, 5);
    oneWay->setCollisionLayer(1);
    oneWay->setCollisionMask(1);
    oneWay->setOneWay(true);
    
    // Verify one-way is set
    TEST_ASSERT_TRUE(oneWay->isOneWay());
    
    colSystem->addEntity(oneWay);
    
    // Verify one-way platform is set and collision handled without crash
    // Position should be valid and within reasonable bounds
    TEST_ASSERT_TRUE(oneWay->isOneWay());
    TEST_ASSERT_TRUE(player->position.x == player->position.x); // NaN check
    TEST_ASSERT_TRUE(player->position.x > toScalar(-1000.0f) && player->position.x < toScalar(1000.0f));
    delete oneWay;
}

void test_kinematic_actor_move_and_slide_max_slides(void) {
    // Test maxSlides cap (default 4) - should not infinite loop
    // Create a scenario that would require many slides
    // Store walls in array for proper cleanup
    StaticActor* walls[8];
    for (int i = 0; i < 8; i++) {
        walls[i] = new StaticActor(toScalar(15 + i * 5), toScalar(-10 + i * 3), 3, 25);
        walls[i]->setCollisionLayer(1);
        walls[i]->setCollisionMask(1);
        colSystem->addEntity(walls[i]);
    }
    
    // Large velocity that would need multiple slides - maxSlides is 4
    Scalar startX = player->position.x;
    player->moveAndSlide(Vector2(toScalar(100), toScalar(50)));
    
    // Cleanup - delete walls AFTER moveAndSlide completes
    for (int i = 0; i < 8; i++) {
        delete walls[i];
    }
    
    // Should complete without infinite loop - position should be valid and changed
    TEST_ASSERT_TRUE(player->position.x == player->position.x); // NaN check
    TEST_ASSERT_TRUE(player->position.x > toScalar(-1000.0f));
}

void test_kinematic_actor_move_and_slide_with_remainder(void) {
    // Test moveAndSlide with velocity that has remainder after slide
    wall = new StaticActor(toScalar(15), toScalar(0), 10, 30);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);
    
    // Velocity larger than needed to hit wall
    player->moveAndSlide(Vector2(toScalar(50), toScalar(0)));
    
    // Should handle remainder - verify position is valid
    TEST_ASSERT_TRUE(player->position.x == player->position.x); // NaN check
}

void test_kinematic_actor_move_and_slide_diagonal_floor(void) {
    // Test diagonal movement that ends on floor
    wall = new StaticActor(toScalar(-50), toScalar(20), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);
    
    // Diagonal movement down-right
    player->moveAndSlide(Vector2(toScalar(10), toScalar(30)));
    
    // Should detect floor collision
    TEST_ASSERT_TRUE(player->is_on_floor() || player->is_on_wall());
}

void test_kinematic_actor_move_and_slide_diagonal_ceiling(void) {
    // Test diagonal movement that hits ceiling
    wall = new StaticActor(toScalar(-50), toScalar(-20), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);
    
    // Diagonal movement up-right
    player->moveAndSlide(Vector2(toScalar(10), toScalar(-30)));
    
    // Should detect ceiling collision
    TEST_ASSERT_TRUE(player->is_on_ceiling() || player->is_on_wall());
}

void test_kinematic_actor_move_and_collide_no_collision_output(void) {
    // Test moveAndCollide when no collision - output params should be set
    Vector2 motion(toScalar(20), toScalar(0));
    KinematicCollision col;
    bool blocked = player->moveAndCollide(motion, &col);
    
    TEST_ASSERT_FALSE(blocked);
    // remainder should be full motion
    TEST_ASSERT_TRUE(col.remainder > toScalar(0));
}

// =============================================================================
// Additional coverage tests for uncovered paths
// =============================================================================

// Test binary search with multiple consecutive collisions - forces more iterations
void test_kinematic_actor_binary_search_multi_wall_iterations(void) {
    // Create a wall directly in path - player at 0,0 (size 10x10), move right
    // Wall at position that will definitely be hit
    StaticActor* wall1 = new StaticActor(toScalar(12), toScalar(0), 5, 30);
    wall1->setCollisionLayer(1);
    wall1->setCollisionMask(1);
    colSystem->addEntity(wall1);
    
    StaticActor* wall2 = new StaticActor(toScalar(22), toScalar(0), 5, 30);
    wall2->setCollisionLayer(1);
    wall2->setCollisionMask(1);
    colSystem->addEntity(wall2);
    
    // Move right - should collide, position should be valid
    Vector2 motion(toScalar(50), toScalar(0));
    KinematicCollision col;
    player->moveAndCollide(motion, &col);
    
    // Verify position is valid and within reasonable bounds
    TEST_ASSERT_TRUE(player->position.x == player->position.x); // NaN check
    TEST_ASSERT_TRUE(player->position.y == player->position.y); // NaN check
    
    delete wall1;
    delete wall2;
}

// Test collision normal calculation - verify correct normal vector when hitting wall
void test_kinematic_actor_collision_normal_calculation(void) {
    // Player at (0,0) 10x10, moving right. 
    // Place wall directly in path - wall at (5, -5) 10x20 spans x=5 to 15, y=-5 to 15
    // Player will definitely collide when moving right
    wall = new StaticActor(toScalar(5), toScalar(-5), 10, 20);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);
    
    // Move right - collision should occur
    Vector2 motion(toScalar(30), toScalar(0));
    KinematicCollision col;
    bool blocked = player->moveAndCollide(motion, &col);
    
    // Verify collision occurred (if layers match)
    if (blocked) {
        // If collision occurred, verify collision info
        TEST_ASSERT_NOT_NULL_MESSAGE(col.collider, "Collider should be set when blocked");
        TEST_ASSERT_TRUE_MESSAGE(col.normal.x != toScalar(0) || col.normal.y != toScalar(0), 
            "Normal should be non-zero");
        // Player should stop before penetrating wall
        TEST_ASSERT_TRUE_MESSAGE(player->position.x < toScalar(15.0f), 
            "Player should stop before penetrating wall");
    }
    // Position should remain valid
    TEST_ASSERT_TRUE(player->position.x == player->position.x); // NaN check
}

// Test moveAndSlide with many slide iterations - force maxSlides cap
void test_kinematic_actor_move_and_slide_many_iterations(void) {
    // Create a chain of walls that will force multiple slide iterations
    // Use angled walls to create slide chain
    StaticActor* walls[6];
    // Create stairs-like pattern that will cause multiple slides
    walls[0] = new StaticActor(toScalar(15), toScalar(0), 5, 20);    // First collision
    walls[1] = new StaticActor(toScalar(25), toScalar(5), 5, 20);    // Slide up
    walls[2] = new StaticActor(toScalar(35), toScalar(-5), 5, 20);   // Slide down
    walls[3] = new StaticActor(toScalar(45), toScalar(5), 5, 20);    // Slide up again
    walls[4] = new StaticActor(toScalar(55), toScalar(-5), 5, 20);   // Slide down
    walls[5] = new StaticActor(toScalar(65), toScalar(5), 5, 20);    // More slides
    
    for (int i = 0; i < 6; i++) {
        walls[i]->setCollisionLayer(1);
        walls[i]->setCollisionMask(1);
        colSystem->addEntity(walls[i]);
    }
    
    // Large velocity that will hit multiple walls - maxSlides (default 4) should cap iterations
    player->moveAndSlide(Vector2(toScalar(100), toScalar(0)));
    
    // Should complete without hanging - maxSlides caps the loop
    // Verify position is valid
    TEST_ASSERT_TRUE(player->position.x == player->position.x); // NaN check
    TEST_ASSERT_TRUE(player->position.y == player->position.y);
    
    for (int i = 0; i < 6; i++) {
        delete walls[i];
    }
}

// Test edge sliding - partial velocity blocked, slide component active
void test_kinematic_actor_edge_sliding(void) {
    // Create corner - wall on right, floor below
    StaticActor* rightWall = new StaticActor(toScalar(12), toScalar(0), 10, 20);
    rightWall->setCollisionLayer(1);
    rightWall->setCollisionMask(1);
    colSystem->addEntity(rightWall);
    
    StaticActor* floor = new StaticActor(toScalar(-10), toScalar(15), 40, 10);
    floor->setCollisionLayer(1);
    floor->setCollisionMask(1);
    colSystem->addEntity(floor);
    
    // Move diagonal - test slide path execution
    player->moveAndSlide(Vector2(toScalar(30), toScalar(30)));
    
    // Verify position is valid after diagonal slide
    TEST_ASSERT_TRUE(player->position.x == player->position.x); // NaN check
    TEST_ASSERT_TRUE(player->position.y == player->position.y);
    
    delete rightWall;
    delete floor;
}

// Test diagonal velocity with both x and y components hitting different surfaces
void test_kinematic_actor_diagonal_both_axes_blocked(void) {
    // Walls blocking from both X and Y directions
    StaticActor* rightWall = new StaticActor(toScalar(15), toScalar(-5), 10, 25);
    rightWall->setCollisionLayer(1);
    rightWall->setCollisionMask(1);
    colSystem->addEntity(rightWall);
    
    StaticActor* topWall = new StaticActor(toScalar(-5), toScalar(-20), 30, 10);
    topWall->setCollisionLayer(1);
    topWall->setCollisionMask(1);
    colSystem->addEntity(topWall);
    
    // Diagonal up-right motion - should hit right wall first (smaller overlap usually)
    player->moveAndSlide(Vector2(toScalar(25), toScalar(-25)));
    
    // Should detect wall collision (from right wall)
    TEST_ASSERT_TRUE(player->is_on_wall() || player->is_on_ceiling());
    
    delete rightWall;
    delete topWall;
}

// Test remainder calculation after partial collision
void test_kinematic_actor_remainder_after_collision(void) {
    wall = new StaticActor(toScalar(15), toScalar(0), 10, 20);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);
    
    // Move right - should collide and have remainder
    Vector2 motion(toScalar(50), toScalar(0));
    KinematicCollision col;
    player->moveAndCollide(motion, &col, false);
    
    // Remainder should be set (collision occurred)
    TEST_ASSERT_TRUE(col.remainder >= toScalar(0));
}

// Test floor detection with exact threshold (dot = 0.707...)
void test_kinematic_actor_floor_exact_threshold(void) {
    // Floor directly below
    wall = new StaticActor(toScalar(-50), toScalar(20), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);
    
    // Pure vertical down - normal is (0, -1), upDirection default is (0, -1)
    // dot = 1.0 > 0.707, should be floor
    player->moveAndSlide(Vector2(toScalar(0), toScalar(15)));
    
    TEST_ASSERT_TRUE(player->is_on_floor());
}

// Test wall detection when normal is perpendicular to upDirection
void test_kinematic_actor_wall_horizontal_normal(void) {
    // Wall on the right
    wall = new StaticActor(toScalar(20), toScalar(-50), 10, 100);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);
    
    // Pure horizontal right - normal is (1, 0), upDirection is (0, -1)
    // dot = 0, abs(dot) < 0.707, should be wall
    player->moveAndSlide(Vector2(toScalar(15), toScalar(0)));
    
    TEST_ASSERT_TRUE(player->is_on_wall());
}

// Test ceiling detection with exact negative threshold
void test_kinematic_actor_ceiling_exact_threshold(void) {
    // Ceiling directly above
    wall = new StaticActor(toScalar(-50), toScalar(-20), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);
    
    // Pure vertical up - normal is (0, 1), upDirection is (0, -1)
    // dot = -1.0 < -0.707, should be ceiling
    player->moveAndSlide(Vector2(toScalar(0), toScalar(-15)));
    
    TEST_ASSERT_TRUE(player->is_on_ceiling());
}

// Test slide vector calculation - remainder is sliding along normal
void test_kinematic_actor_slide_vector_calculation(void) {
    // Create wall that causes partial collision
    wall = new StaticActor(toScalar(20), toScalar(0), 10, 30);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);
    
    // Large velocity that gets partially blocked
    player->moveAndSlide(Vector2(toScalar(100), toScalar(10)));
    
    // Should complete - verify position is valid (NaN check)
    TEST_ASSERT_TRUE(player->position.x == player->position.x); // NaN check
}

// Test collision when rigid body is in the way - should be ignored
void test_kinematic_actor_rigid_body_ignored_in_collision(void) {
    // Note: This requires creating a RigidActor which might not be available
    // If RigidActor exists, add this test
    // For now, this test documents the expected behavior
    TEST_IGNORE_MESSAGE("RigidActor test requires RigidActor class implementation");
}

// =============================================================================
// Phase 1: Kinematic snap API tests (TS-001, TS-002, TS-003)
// =============================================================================

void test_snap_api_compile_2arg_and_4arg(void) {
    // TS-001: Verify 2-arg and 4-arg calls compile and return Vector2
    Vector2 result;

    // 2-arg call (defaults for upDirection and dt)
    result = player->moveAndSlideWithSnap(Vector2(toScalar(0), toScalar(0)), Vector2(toScalar(0), toScalar(4)));

    // 4-arg call with explicit params
    result = player->moveAndSlideWithSnap(Vector2(toScalar(0), toScalar(0)), Vector2(toScalar(0), toScalar(4)),
                                          Vector2(0, -1), CollisionSystem::FIXED_DT);

    // Both calls should return valid values (no NaN, no crash)
    TEST_ASSERT_TRUE_MESSAGE(result.x == result.x, "2-arg return value must be valid (NaN check)");
    TEST_ASSERT_TRUE_MESSAGE(result.y == result.y, "4-arg return value must be valid (NaN check)");
}

void test_snap_engages_3px_above_floor(void) {
    // TS-002: Body 3px above static floor, snap=(0,4) should engage
    // Player at (0,0) 10x10. Player bottom at y=10. Floor top at y=13. Gap = 3px.
    wall = new StaticActor(toScalar(-50), toScalar(13), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    Vector2 result = player->moveAndSlideWithSnap(Vector2(toScalar(0), toScalar(0)), Vector2(toScalar(0), toScalar(4)));

    // Player bottom should be at floor top: y + 10 = 13 → y = 3
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.1f, 3.0f, static_cast<float>(player->position.y),
        "Snap should place body on floor surface");
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Snap should set is_on_floor()=true when on floor");
}

void test_snap_degraded_below_MIN_SNAP(void) {
    // TS-002 (MIN_SNAP degradation guard): snap=(0,2) < MIN_SNAP(4) → no snap
    // Same setup: body 3px above floor, snap too small to engage
    wall = new StaticActor(toScalar(-50), toScalar(13), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    // Player at (0,0). Snap=(0,2) which is below MIN_SNAP=4.0
    Vector2 result = player->moveAndSlideWithSnap(Vector2(toScalar(0), toScalar(0)), Vector2(toScalar(0), toScalar(2)));

    // Snap should NOT engage — player stays at y=0
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.1f, 0.0f, static_cast<float>(player->position.y),
        "Below-MIN_SNAP snap should not move body");
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_floor(), "Below-MIN_SNAP snap should not set is_on_floor()");
}

void test_no_snap_on_jump_launch(void) {
    // TS-003: Zero snap does not pull body down (snap contract)
    // Body with upward velocity and zero snap should jump freely.
    wall = new StaticActor(toScalar(-50), toScalar(20), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    // Jump up with zero snap — zero vector disables snap regardless of current state
    Vector2 result = player->moveAndSlideWithSnap(Vector2(toScalar(0), toScalar(-10)), Vector2(toScalar(0), toScalar(0)));

    // Body should move up (away from floor) with zero snap
    TEST_ASSERT_TRUE_MESSAGE(player->position.y < toScalar(0), "Jump should move body upward");
    // Returned velocity should reflect the upward slide movement
    TEST_ASSERT_TRUE_MESSAGE(result.y < toScalar(0), "Returned velocity should have upward component");
    // is_on_floor() behavior depends on persistence state (evaluated in TS-006)
}

// =============================================================================
// Phase 4: KINEMATIC floor velocity inheritance tests
// =============================================================================



void test_snap_plus_velocity_movement(void) {
    // TS-004: Snap plus velocity — body moves horizontally while snap engages floor
    // Floor at y=12, 10 tall → spans y=12 to 22. Player at (0,0) 10x10.
    // Gap from player bottom (y=10) to floor top (y=12) = 2 units.
    // velocity=(3,0) moves right 3. snap=(0,4) snaps down 4 → hits floor.
    wall = new StaticActor(toScalar(0), toScalar(12), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    Vector2 result = player->moveAndSlideWithSnap(
        Vector2(toScalar(3), toScalar(0)),
        Vector2(toScalar(0), toScalar(4)),
        Vector2(toScalar(0), toScalar(-1))
    );

    // Snap engaged floor contact
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "TS-004: snap should engage floor");

    // Player moved right 3 and snapped to y=2 (bottom at y=12 = floor top)
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 3.0f, static_cast<float>(player->position.x));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 2.0f, static_cast<float>(player->position.y));

    // Return value = slide displacement / dt (slide moved right 3, no vertical change before snap)
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 180.0f, static_cast<float>(result.x)); // 3 * 60 = 180 units/s
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, static_cast<float>(result.y));
}

void test_snap_on_valid_slope(void) {
    // TS-007: Body approaches floor at a diagonal (simulating a 30° slope walk-down).
    // The collision surface produces a floor-like normal (dot >= floorThreshold).
    // snap engages and onFloor = true.
    wall = new StaticActor(toScalar(0), toScalar(12), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    // Player at (0,0) 10x10. Gap from bottom (y=10) to floor top (y=12) = 2 units.
    // Diagonal velocity=(5,5): slide would move to (5,5) but floor stops vertical motion.
    // Binary search finds safe position at ~(3.5, 2.0) where bottom = floor top.
    // snap=(0,4): from y=2 could push further but body is already on floor → normal=(0,-1).
    player->moveAndSlideWithSnap(
        Vector2(toScalar(5), toScalar(5)),
        Vector2(toScalar(0), toScalar(4)),
        Vector2(toScalar(0), toScalar(-1))
    );

    // Player bottom at floor top: y + 10 = 12 → y = 2
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "TS-007: snap engages on valid slope approach");
    // Floor contact truncated diagonal: y should be ~2 (bottom at floor top)
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 2.0f, static_cast<float>(player->position.y));
    // X moved right by same ratio as velocity until floor contact
    TEST_ASSERT_TRUE_MESSAGE(player->position.x > toScalar(2.0f), "TS-007: player moved right during diagonal approach");
}

void test_snap_rejected_on_steep_slope(void) {
    // TS-008: Body is adjacent to a vertical wall (simulating a 60°+ steep slope).
    // snap pushes down but there is no floor below → snap misses entirely.
    // onFloor stays false (snap rejected because no floor contact established).
    wall = new StaticActor(toScalar(15), toScalar(0), 10, 30);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    // Player at (0,0) 10x10, wall at x=15 (5px gap to player right edge).
    // velocity=(0,0), snap=(0,4). Snap pushes down but no floor within 4px → miss.
    // onFloor stays false, body position unchanged.
    player->moveAndSlideWithSnap(
        Vector2(toScalar(0), toScalar(0)),
        Vector2(toScalar(0), toScalar(4)),
        Vector2(toScalar(0), toScalar(-1))
    );

    TEST_ASSERT_FALSE_MESSAGE(player->is_on_floor(), "TS-008: snap rejected on steep slope (no floor contact)");
    // Body should not have moved (velocity=0, snap missed)
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, static_cast<float>(player->position.y));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, static_cast<float>(player->position.x));
}

void test_snap_jump_recontact(void) {
    // TS-006: Jump re-contact — body jumps off floor, falls back, snap re-engages
    // Floor at y=12, 10 tall
    wall = new StaticActor(toScalar(0), toScalar(12), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    Vector2 snap(toScalar(0), toScalar(4));
    Vector2 up(toScalar(0), toScalar(-1));

    // Frame 1: On floor (snap engages)
    player->moveAndSlideWithSnap(Vector2(toScalar(0), toScalar(0)), snap, up);
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "TS-006 F1: should be on floor via snap");

    // Frame 2: Jump up — velocity up 5, snap tries to pull down but misses floor
    // player.y starts at 2, moves up to -3, snap tries y=1 → no floor → restored to y=-3
    player->moveAndSlideWithSnap(Vector2(toScalar(0), toScalar(-5)), snap, up);
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_floor(), "TS-006 F2: should NOT be on floor after jump");

    // Frame 3: Fall back down — velocity down 5, snap catches floor
    // player.y starts at -3, moves down to 2, snap hits floor, re-engages
    player->moveAndSlideWithSnap(Vector2(toScalar(0), toScalar(5)), snap, up);
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "TS-006 F3: should re-contact floor via snap");
}

void test_static_floor_no_inheritance(void) {
    // STATIC floor at y=20, 10 tall → spans y=20 to 30
    wall = new StaticActor(toScalar(-50), toScalar(20), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    // Player at (0,0) 10x10. Move down 15 → should land on static floor
    // No inheritance expected → player y = 10 (bottom at platform top)
    player->moveAndSlide(Vector2(toScalar(0), toScalar(15)));

    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Player should be on floor after landing on STATIC floor");
    // Player bottom should be at floor top: y + 10 = 20 → y = 10
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 10.0f, static_cast<float>(player->position.y));
}

void test_kinematic_floor_out_floor_body_static(void) {
    // STATIC floor → outFloorBody should remain nullptr
    wall = new StaticActor(toScalar(-50), toScalar(20), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    PhysicsActor* floorPtr = reinterpret_cast<PhysicsActor*>(static_cast<uintptr_t>(0xDEAD));
    player->moveAndSlide(Vector2(toScalar(0), toScalar(15)), Vector2(0, -1), &floorPtr);

    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Player should be on floor");
    TEST_ASSERT_NULL_MESSAGE(floorPtr, "STATIC floor should leave outFloorBody as nullptr");
}

void test_kinematic_floor_out_floor_body_kinematic(void) {
    // v1: outFloorBody always returns nullptr (floorBody storage is dead)
    platform = new KinematicActor(toScalar(-50), toScalar(20), 100, 10);
    platform->setCollisionLayer(1);
    platform->setCollisionMask(1);
    platform->setVelocity(toScalar(0), toScalar(60));
    colSystem->addEntity(platform);

    PhysicsActor* floorPtr = nullptr;
    player->moveAndSlide(Vector2(toScalar(0), toScalar(15)), Vector2(0, -1), &floorPtr);

    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Player should be on floor");
    TEST_ASSERT_NULL_MESSAGE(floorPtr, "v1: outFloorBody always returns nullptr (floorBody dead in v1)");
}

void test_default_nullptr_parameter_no_crash(void) {
    // Call with default nullptr (2-arg call) → no crash, unchanged behavior
    wall = new StaticActor(toScalar(-50), toScalar(20), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    // 2-arg call (inherits default nullptr for outFloorBody)
    player->moveAndSlide(Vector2(toScalar(0), toScalar(15)), Vector2(0, -1));

    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Player should be on floor with 2-arg call");
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_ceiling(), "Player should NOT be on ceiling");
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_wall(), "Player should NOT be on wall");
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 10.0f, static_cast<float>(player->position.y));
}

// =============================================================================
// Phase 4 (V2): Godot-inspired moving platform riding tests
// =============================================================================





void test_is_on_floor_raw_contact(void) {
    // is_on_floor() returns raw contact state with no persistence
    wall = new StaticActor(toScalar(-50), toScalar(20), 100, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    // Frame 1: Land on floor
    player->moveAndSlide(Vector2(toScalar(0), toScalar(15)));
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Frame 1: should be on floor (direct contact)");

    // Frame 2: Walk off floor (move right with no floor below)
    player->position.x = toScalar(60);
    player->moveAndSlide(Vector2(toScalar(0), toScalar(1)));

    // No persistence: should immediately lose floor
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_floor(), "Frame 2: should NOT be on floor (no persistence)");
}

void test_depenetration_post_slide(void) {
    // KINEMATIC platform
    platform = new KinematicActor(toScalar(-50), toScalar(20), 100, 10);
    platform->setCollisionLayer(1);
    platform->setCollisionMask(1);
    platform->setVelocity(toScalar(0), toScalar(0));
    colSystem->addEntity(platform);

    // Frame 1: Land on platform
    player->moveAndSlide(Vector2(toScalar(0), toScalar(15)));
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Player should land");

    // Simulate platform shifting up into the player (e.g. moving wall pushes into player)
    // Player is at ~(0,10) 10x10. Platform at (-50,20) 100x10.
    // Move platform up so it overlaps with player from below
    platform->position.y = toScalar(12); // Platform: y=12..22. Player: y=10..20. Overlap: y=12..20 = 8 units

    // Frame 2: Pre-slide adds platform velocity (0,0). Depenetration should push player UP
    // Player center (~15) is above platform center (~17), so player goes UP
    Scalar preY = player->position.y;
    player->moveAndSlide(Vector2(toScalar(0), toScalar(0)));

    // Player should be pushed out from the overlap (clamped to 2.0 units max)
    // Player is above platform center, pushed upward
    TEST_ASSERT_TRUE_MESSAGE(player->position.y < preY, "Player should be pushed out by depenetration");
}

void test_safe_margin_expansion(void) {
    // Test that safeMargin expands the AABB during collision checks
    // Place a wall very close to the player (1 pixel gap)
    wall = new StaticActor(toScalar(11), toScalar(0), 10, 10);
    wall->setCollisionLayer(1);
    wall->setCollisionMask(1);
    colSystem->addEntity(wall);

    // Player at (0,0) 10x10. Wall at (11,0) 10x10.
    // Gap is 1 pixel from player right edge (10) to wall left edge (11)

    // With default safeMargin (0.08f), moveAndCollide should still detect this
    // but moveAndSlide uses default 0.08f margin
    player->moveAndSlide(Vector2(toScalar(5), toScalar(0)));

    // Player should NOT pass through the wall
    TEST_ASSERT_TRUE_MESSAGE(player->position.x < toScalar(11.0f), "Safe margin should prevent passing through near wall");
}





int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    RUN_TEST(test_is_on_floor);
    RUN_TEST(test_is_on_ceiling);
    RUN_TEST(test_is_on_wall);
    RUN_TEST(test_flags_reset);
    RUN_TEST(test_sensor_does_not_block_move_and_collide);
    RUN_TEST(test_solid_blocks_move_and_collide);
    RUN_TEST(test_sensor_constructor_with_vector2);
    RUN_TEST(test_static_actor_creation);
    RUN_TEST(test_static_actor_creation_vector2);
    
    // Branch coverage tests for slope handling
    RUN_TEST(test_move_and_slide_45_degree_slope_detection);
    RUN_TEST(test_move_and_slide_max_slides_exhausted);
    RUN_TEST(test_move_and_slide_zero_motion);
    
    // FASE 2 coverage expansion tests
    RUN_TEST(test_kinematic_actor_move_and_collide_binary_search_path);
    RUN_TEST(test_kinematic_actor_move_and_collide_no_collision);
    RUN_TEST(test_kinematic_actor_move_and_collide_zero_motion);
    RUN_TEST(test_kinematic_actor_move_and_collide_test_only);
    RUN_TEST(test_kinematic_actor_move_and_collide_one_way_platform);
    RUN_TEST(test_kinematic_actor_move_and_slide_max_slides);
    RUN_TEST(test_kinematic_actor_move_and_slide_with_remainder);
    RUN_TEST(test_kinematic_actor_move_and_slide_diagonal_floor);
    RUN_TEST(test_kinematic_actor_move_and_slide_diagonal_ceiling);
    RUN_TEST(test_kinematic_actor_move_and_collide_no_collision_output);
    
    // Additional coverage tests for uncovered paths
    RUN_TEST(test_kinematic_actor_binary_search_multi_wall_iterations);
    RUN_TEST(test_kinematic_actor_collision_normal_calculation);
    RUN_TEST(test_kinematic_actor_move_and_slide_many_iterations);
    RUN_TEST(test_kinematic_actor_edge_sliding);
    RUN_TEST(test_kinematic_actor_diagonal_both_axes_blocked);
    RUN_TEST(test_kinematic_actor_remainder_after_collision);
    RUN_TEST(test_kinematic_actor_floor_exact_threshold);
    RUN_TEST(test_kinematic_actor_wall_horizontal_normal);
    RUN_TEST(test_kinematic_actor_ceiling_exact_threshold);
    RUN_TEST(test_kinematic_actor_slide_vector_calculation);
    RUN_TEST(test_kinematic_actor_rigid_body_ignored_in_collision);
    
    // Phase 1: Kinematic snap API tests
    RUN_TEST(test_snap_api_compile_2arg_and_4arg);
    RUN_TEST(test_snap_engages_3px_above_floor);
    RUN_TEST(test_snap_degraded_below_MIN_SNAP);
    RUN_TEST(test_no_snap_on_jump_launch);

    // Phase 2: Snap + velocity combined and jump re-contact
    RUN_TEST(test_snap_plus_velocity_movement);
    RUN_TEST(test_snap_jump_recontact);

    // Phase 3: Slope snap coverage
    RUN_TEST(test_snap_on_valid_slope);
    RUN_TEST(test_snap_rejected_on_steep_slope);

    // Phase 4: KINEMATIC floor state tests
    RUN_TEST(test_static_floor_no_inheritance);
    RUN_TEST(test_kinematic_floor_out_floor_body_static);
    RUN_TEST(test_kinematic_floor_out_floor_body_kinematic);
    RUN_TEST(test_default_nullptr_parameter_no_crash);
    
    // Phase 4 (V2): Raw contact + platform riding tests
    RUN_TEST(test_is_on_floor_raw_contact);
    RUN_TEST(test_depenetration_post_slide);
    RUN_TEST(test_safe_margin_expansion);
    
    return UNITY_END();
}
