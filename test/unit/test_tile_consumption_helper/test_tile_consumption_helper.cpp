#include <unity.h>
#include "../../test_config.h"
#include "physics/TileConsumptionHelper.h"

using namespace pixelroot32::physics;

void test_tile_consumption_config_default() {
    TileConsumptionConfig config;
    
    TEST_ASSERT_TRUE(config.validateCoordinates);
    TEST_ASSERT_TRUE(config.updateTilemap);
    TEST_ASSERT_TRUE(config.logConsumption);
}

void test_tile_consumption_config_custom() {
    TileConsumptionConfig config;
    config.validateCoordinates = true;
    config.updateTilemap = true;
    config.logConsumption = true;
    
    TEST_ASSERT_TRUE(config.validateCoordinates);
    TEST_ASSERT_TRUE(config.updateTilemap);
    TEST_ASSERT_TRUE(config.logConsumption);
}

void test_tile_consumption_config_assignment() {
    TileConsumptionConfig config;
    config = TileConsumptionConfig();
    config.validateCoordinates = true;
    
    TEST_ASSERT_TRUE(config.validateCoordinates);
}

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_tile_consumption_config_default);
    RUN_TEST(test_tile_consumption_config_custom);
    RUN_TEST(test_tile_consumption_config_assignment);
    return UNITY_END();
}
