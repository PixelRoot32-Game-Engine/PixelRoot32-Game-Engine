#include <unity.h>
#include "physics/KinematicActor.h"
#include "physics/StaticActor.h"
#include "physics/CollisionSystem.h"
#include "../../test_config.h"

using namespace pixelroot32::core;
using namespace pixelroot32::physics;
using namespace pixelroot32::math;

CollisionSystem* colSystem = nullptr;
KinematicActor* player = nullptr;
StaticActor* wall = nullptr;

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
    if (colSystem) delete colSystem;
    player = nullptr;
    wall = nullptr;
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

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    RUN_TEST(test_is_on_floor);
    RUN_TEST(test_is_on_ceiling);
    RUN_TEST(test_is_on_wall);
    RUN_TEST(test_flags_reset);
    return UNITY_END();
}
