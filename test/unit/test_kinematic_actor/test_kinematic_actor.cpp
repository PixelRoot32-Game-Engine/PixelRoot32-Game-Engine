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
    if (colSystem) delete colSystem;
    player = nullptr;
    wall = nullptr;
    sensor = nullptr;
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

    // 2. Move up (away from floor)
    player->moveAndSlide(Vector2(toScalar(0), toScalar(-5)));
    
    // Flags should be reset
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_floor(), "Floor flag should be reset");
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
    return UNITY_END();
}
