/**
 * @file test_tile_attribute_query_unit.cpp
 * @brief Unit tests for tile attribute query functions
 * 
 * Feature: refactor-tile-attribute-query-functions
 */

#include <unity.h>
#include "graphics/Renderer.h"
#include "../../test_config.h"
#include <cstring>

using namespace pixelroot32::graphics;

// =============================================================================
// Test Data (Hardcoded)
// =============================================================================

// Layer 0: "Background" with 2 tiles
static const TileAttribute layer0_tile0_attrs[] = {
    {"solid", "true"},
    {"type", "grass"}
};

static const TileAttribute layer0_tile1_attrs[] = {
    {"solid", "false"},
    {"type", "water"},
    {"damage", "10"}
};

static const TileAttributeEntry layer0_tiles[] = {
    {10, 5, 2, layer0_tile0_attrs},
    {12, 5, 3, layer0_tile1_attrs}
};

// Layer 1: "Foreground" with 1 tile
static const TileAttribute layer1_tile0_attrs[] = {
    {"interactable", "true"}
};

static const TileAttributeEntry layer1_tiles[] = {
    {10, 5, 1, layer1_tile0_attrs}
};

// Layer 2: "Empty"
static const LayerAttributes test_layer_attributes[] = {
    {"Background", 2, layer0_tiles},
    {"Foreground", 1, layer1_tiles},
    {"Empty", 0, nullptr}
};

static const uint8_t NUM_TEST_LAYERS = 3;

// =============================================================================
// Tests
// =============================================================================

void test_unit_query_existing_attribute(void) {
    const char* value = get_tile_attribute(test_layer_attributes, NUM_TEST_LAYERS, 0, 10, 5, "solid");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL_STRING("true", value);
    
    value = get_tile_attribute(test_layer_attributes, NUM_TEST_LAYERS, 0, 12, 5, "damage");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL_STRING("10", value);
    
    value = get_tile_attribute(test_layer_attributes, NUM_TEST_LAYERS, 1, 10, 5, "interactable");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL_STRING("true", value);
}

void test_unit_query_non_existent_attribute(void) {
    const char* value = get_tile_attribute(test_layer_attributes, NUM_TEST_LAYERS, 0, 10, 5, "non_existent");
    TEST_ASSERT_NULL(value);
}

void test_unit_query_non_existent_tile(void) {
    const char* value = get_tile_attribute(test_layer_attributes, NUM_TEST_LAYERS, 0, 0, 0, "solid");
    TEST_ASSERT_NULL(value);
}

void test_unit_query_out_of_bounds_layer(void) {
    const char* value = get_tile_attribute(test_layer_attributes, NUM_TEST_LAYERS, 5, 10, 5, "solid");
    TEST_ASSERT_NULL(value);
}

void test_unit_query_empty_layer(void) {
    const char* value = get_tile_attribute(test_layer_attributes, NUM_TEST_LAYERS, 2, 10, 5, "any");
    TEST_ASSERT_NULL(value);
}

void test_unit_tile_has_attributes(void) {
    TEST_ASSERT_TRUE(tile_has_attributes(test_layer_attributes, NUM_TEST_LAYERS, 0, 10, 5));
    TEST_ASSERT_TRUE(tile_has_attributes(test_layer_attributes, NUM_TEST_LAYERS, 0, 12, 5));
    TEST_ASSERT_TRUE(tile_has_attributes(test_layer_attributes, NUM_TEST_LAYERS, 1, 10, 5));
    
    TEST_ASSERT_FALSE(tile_has_attributes(test_layer_attributes, NUM_TEST_LAYERS, 0, 11, 5));
    TEST_ASSERT_FALSE(tile_has_attributes(test_layer_attributes, NUM_TEST_LAYERS, 2, 10, 5));
}

void test_unit_get_tile_entry(void) {
    const TileAttributeEntry* entry = get_tile_entry(test_layer_attributes, NUM_TEST_LAYERS, 0, 10, 5);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_INT(10, entry->x);
    TEST_ASSERT_EQUAL_INT(5, entry->y);
    TEST_ASSERT_EQUAL_INT(2, entry->num_attributes);
    
    entry = get_tile_entry(test_layer_attributes, NUM_TEST_LAYERS, 0, 0, 0);
    TEST_ASSERT_NULL(entry);
}

// =============================================================================
// Setup / Teardown / Main
// =============================================================================

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    
    RUN_TEST(test_unit_query_existing_attribute);
    RUN_TEST(test_unit_query_non_existent_attribute);
    RUN_TEST(test_unit_query_non_existent_tile);
    RUN_TEST(test_unit_query_out_of_bounds_layer);
    RUN_TEST(test_unit_query_empty_layer);
    RUN_TEST(test_unit_tile_has_attributes);
    RUN_TEST(test_unit_get_tile_entry);
    
    return UNITY_END();
}
