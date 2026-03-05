/**
 * @file test_tile_attribute_query_property.cpp
 * @brief Property-based tests for tile attribute query functions
 * @version 1.0
 * @date 2026-02-08
 * 
 * Property-based tests for:
 * - get_tile_attribute()
 * - tile_has_attributes()
 * - get_tile_entry()
 * 
 * Feature: refactor-tile-attribute-query-functions
 */

#include <unity.h>
#include "graphics/Renderer.h"
#include "../../test_config.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <memory>

using namespace pixelroot32::graphics;

// =============================================================================
// Test Data Structures and Generators
// =============================================================================

/**
 * @brief Helper structure to manage dynamically allocated test data
 * 
 * This structure owns all the memory for a test LayerAttributes array,
 * making it easy to generate random test data and clean it up.
 */
struct TestLayerData {
    std::vector<LayerAttributes> layers;
    std::vector<std::vector<TileAttributeEntry>> layer_tiles;
    std::vector<std::vector<std::vector<TileAttribute>>> tile_attributes;
    std::vector<std::string> all_keys;
    std::vector<std::string> all_values;
    std::vector<const char*> all_keys_ptrs;  // Stable pointers to keys
    std::vector<const char*> all_values_ptrs;  // Stable pointers to values
    std::vector<std::string> layer_names;
    std::vector<const char*> layer_names_ptrs;  // Stable pointers to layer names
    
    ~TestLayerData() {
        // Memory is automatically cleaned up by vectors
    }
    
    /**
     * @brief Generate random layer attributes structure
     * 
     * @param num_layers Number of layers to generate (0-10)
     * @param max_tiles_per_layer Maximum tiles per layer (0-50)
     * @param max_attrs_per_tile Maximum attributes per tile (0-10)
     * @param seed Random seed for reproducibility
     */
    void generate(uint8_t num_layers, uint16_t max_tiles_per_layer, 
                  uint8_t max_attrs_per_tile, unsigned int seed) {
        srand(seed);
        
        layers.resize(num_layers);
        layer_tiles.resize(num_layers);
        tile_attributes.resize(num_layers);
        layer_names.resize(num_layers);
        
        // PHASE 1: Generate all random structure and determine sizes
        // This ensures we know exactly how many strings we need before allocating
        std::vector<std::vector<uint8_t>> num_attrs_per_tile(num_layers);
        std::vector<std::vector<std::pair<uint16_t, uint16_t>>> tile_coords(num_layers);
        
        size_t total_attrs = 0;
        
        for (uint8_t layer_idx = 0; layer_idx < num_layers; layer_idx++) {
            uint16_t num_tiles = rand() % (max_tiles_per_layer + 1);
            layer_tiles[layer_idx].resize(num_tiles);
            tile_attributes[layer_idx].resize(num_tiles);
            num_attrs_per_tile[layer_idx].resize(num_tiles);
            tile_coords[layer_idx].resize(num_tiles);
            
            // Track used coordinates to avoid duplicates
            std::vector<std::pair<uint16_t, uint16_t>> used_coords;
            
            for (uint16_t tile_idx = 0; tile_idx < num_tiles; tile_idx++) {
                // Generate unique random coordinates
                uint16_t x, y;
                bool is_unique;
                int attempts = 0;
                do {
                    x = rand() % 256;
                    y = rand() % 256;
                    is_unique = true;
                    for (const auto& coord : used_coords) {
                        if (coord.first == x && coord.second == y) {
                            is_unique = false;
                            break;
                        }
                    }
                    attempts++;
                    // Prevent infinite loop in case we run out of unique coordinates
                    if (attempts > 1000) {
                        x = tile_idx % 256;
                        y = tile_idx / 256;
                        is_unique = true;
                    }
                } while (!is_unique);
                
                used_coords.push_back({x, y});
                tile_coords[layer_idx][tile_idx] = {x, y};
                
                // Generate random attribute count
                uint8_t num_attrs = rand() % (max_attrs_per_tile + 1);
                num_attrs_per_tile[layer_idx][tile_idx] = num_attrs;
                tile_attributes[layer_idx][tile_idx].resize(num_attrs);
                
                total_attrs += num_attrs;
            }
        }
        
        // PHASE 2: Pre-allocate ALL strings at once to prevent reallocation
        all_keys.reserve(total_attrs);
        all_values.reserve(total_attrs);
        
        for (size_t i = 0; i < total_attrs; i++) {
            all_keys.push_back("key_" + std::to_string(i));
            all_values.push_back("value_" + std::to_string(i));
        }
        
        // Create stable pointers AFTER all strings are created
        all_keys_ptrs.reserve(total_attrs);
        all_values_ptrs.reserve(total_attrs);
        for (size_t i = 0; i < total_attrs; i++) {
            all_keys_ptrs.push_back(all_keys[i].c_str());
            all_values_ptrs.push_back(all_values[i].c_str());
        }
        
        // Generate layer names
        for (uint8_t layer_idx = 0; layer_idx < num_layers; layer_idx++) {
            layer_names[layer_idx] = "Layer_" + std::to_string(layer_idx);
        }
        
        // Create stable pointers for layer names
        layer_names_ptrs.reserve(num_layers);
        for (uint8_t layer_idx = 0; layer_idx < num_layers; layer_idx++) {
            layer_names_ptrs.push_back(layer_names[layer_idx].c_str());
        }
        
        // PHASE 3: Now assign pointers (safe because vectors won't reallocate)
        size_t attr_counter = 0;
        
        for (uint8_t layer_idx = 0; layer_idx < num_layers; layer_idx++) {
            uint16_t num_tiles = static_cast<uint16_t>(layer_tiles[layer_idx].size());
            
            for (uint16_t tile_idx = 0; tile_idx < num_tiles; tile_idx++) {
                uint16_t x = tile_coords[layer_idx][tile_idx].first;
                uint16_t y = tile_coords[layer_idx][tile_idx].second;
                uint8_t num_attrs = num_attrs_per_tile[layer_idx][tile_idx];
                
                // Assign attribute pointers
                for (uint8_t attr_idx = 0; attr_idx < num_attrs; attr_idx++) {
                    tile_attributes[layer_idx][tile_idx][attr_idx].key = 
                        all_keys_ptrs[attr_counter];
                    tile_attributes[layer_idx][tile_idx][attr_idx].value = 
                        all_values_ptrs[attr_counter];
                    attr_counter++;
                }
                
                // Set up tile entry
                layer_tiles[layer_idx][tile_idx].x = x;
                layer_tiles[layer_idx][tile_idx].y = y;
                layer_tiles[layer_idx][tile_idx].num_attributes = num_attrs;
                layer_tiles[layer_idx][tile_idx].attributes = 
                    num_attrs > 0 ? tile_attributes[layer_idx][tile_idx].data() : nullptr;
            }
            
            // Set up layer
            layers[layer_idx].layer_name = layer_names_ptrs[layer_idx];
            layers[layer_idx].num_tiles_with_attributes = num_tiles;
            layers[layer_idx].tiles = num_tiles > 0 ? layer_tiles[layer_idx].data() : nullptr;
        }
    }
    
    /**
     * @brief Get pointer to layers array for testing
     */
    const LayerAttributes* get_layers() const {
        return layers.empty() ? nullptr : layers.data();
    }
    
    /**
     * @brief Get number of layers
     */
    uint8_t get_num_layers() const {
        return static_cast<uint8_t>(layers.size());
    }
};

// =============================================================================
// Setup / Teardown
// =============================================================================

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Property 1: Query Result Correctness
// **Validates: Requirements 2.2, 6.2, 6.3**
// =============================================================================

/**
 * @brief Property test: get_tile_attribute returns correct results
 * 
 * For any valid layer attributes array, layer index, tile coordinates, and 
 * attribute key, the get_tile_attribute function should return a non-null 
 * PROGMEM string pointer if and only if:
 * 1. The layer index is within bounds (< num_layers)
 * 2. A tile exists at the specified coordinates in that layer
 * 3. The tile has an attribute with the specified key
 * 
 * Otherwise, it should return nullptr.
 * 
 * This test generates random LayerAttributes structures and verifies the
 * property holds across minimum 100 iterations.
 */
void test_property_query_result_correctness(void) {
    const int NUM_ITERATIONS = 100;
    const uint8_t TEST_MAX_LAYERS = 10;
    const uint16_t TEST_MAX_TILES_PER_LAYER = 50;
    const uint8_t TEST_MAX_ATTRS_PER_TILE = 10;
    
    for (int iteration = 0; iteration < NUM_ITERATIONS; iteration++) {
        // Generate random test data
        TestLayerData test_data;
        uint8_t num_layers = (rand() % TEST_MAX_LAYERS) + 1; // At least 1 layer
        test_data.generate(num_layers, TEST_MAX_TILES_PER_LAYER, TEST_MAX_ATTRS_PER_TILE, 
                          iteration + 12345);
        
        const LayerAttributes* layers = test_data.get_layers();
        uint8_t layer_count = test_data.get_num_layers();
        
        // Test 1: Out of bounds layer index should return nullptr
        const char* result = get_tile_attribute(layers, layer_count, 
                                                layer_count + 1, 0, 0, "any_key");
        TEST_ASSERT_NULL_MESSAGE(result, 
            "Out of bounds layer index should return nullptr");
        
        // Test 2: For each layer, verify queries work correctly
        for (uint8_t layer_idx = 0; layer_idx < layer_count; layer_idx++) {
            const LayerAttributes& layer = test_data.layers[layer_idx];
            
            // Test 2a: Query non-existent tile should return nullptr
            result = get_tile_attribute(layers, layer_count, layer_idx, 
                                       9999, 9999, "any_key");
            TEST_ASSERT_NULL_MESSAGE(result, 
                "Non-existent tile should return nullptr");
            
            // Test 2b: For each tile in the layer
            for (uint16_t tile_idx = 0; tile_idx < layer.num_tiles_with_attributes; tile_idx++) {
                const TileAttributeEntry& tile = test_data.layer_tiles[layer_idx][tile_idx];
                
                // Test 2b-i: Query with non-existent key should return nullptr
                result = get_tile_attribute(layers, layer_count, layer_idx,
                                           tile.x, tile.y, "non_existent_key");
                TEST_ASSERT_NULL_MESSAGE(result,
                    "Non-existent attribute key should return nullptr");
                
                // Test 2b-ii: Query with existing keys should return correct values
                for (uint8_t attr_idx = 0; attr_idx < tile.num_attributes; attr_idx++) {
                    const TileAttribute& attr = 
                        test_data.tile_attributes[layer_idx][tile_idx][attr_idx];
                    
                    result = get_tile_attribute(layers, layer_count, layer_idx,
                                               tile.x, tile.y, attr.key);
                    
                    char debug_msg[256];
                    snprintf(debug_msg, sizeof(debug_msg),
                            "Failed to find key='%s' at tile (%d,%d) layer=%d",
                            attr.key, tile.x, tile.y, layer_idx);
                    
                    TEST_ASSERT_NOT_NULL_MESSAGE(result, debug_msg);
                    TEST_ASSERT_EQUAL_STRING_MESSAGE(attr.value, result,
                        "Returned value should match expected value");
                }
            }
        }
    }
}

/**
 * @brief Property test: Boundary conditions
 * 
 * Tests edge cases:
 * - Empty layers (num_tiles_with_attributes == 0)
 * - Tiles with no attributes (num_attributes == 0)
 * - Layer index at boundary (num_layers - 1)
 * - Coordinates at (0, 0)
 */
void test_property_boundary_conditions(void) {
    // Test 1: Empty layer
    TestLayerData empty_layer_data;
    empty_layer_data.generate(1, 0, 0, 54321); // 1 layer, 0 tiles
    
    const char* result = get_tile_attribute(empty_layer_data.get_layers(), 
                                           empty_layer_data.get_num_layers(),
                                           0, 0, 0, "any_key");
    TEST_ASSERT_NULL_MESSAGE(result, "Empty layer should return nullptr");
    
    // Test 2: Tile with no attributes
    TestLayerData no_attrs_data;
    no_attrs_data.layers.resize(1);
    no_attrs_data.layer_tiles.resize(1);
    no_attrs_data.layer_tiles[0].resize(1);
    no_attrs_data.layer_names.push_back("Layer_0");
    no_attrs_data.layer_names_ptrs.push_back(no_attrs_data.layer_names[0].c_str());
    
    no_attrs_data.layer_tiles[0][0].x = 5;
    no_attrs_data.layer_tiles[0][0].y = 5;
    no_attrs_data.layer_tiles[0][0].num_attributes = 0;
    no_attrs_data.layer_tiles[0][0].attributes = nullptr;
    
    no_attrs_data.layers[0].layer_name = no_attrs_data.layer_names_ptrs[0];
    no_attrs_data.layers[0].num_tiles_with_attributes = 1;
    no_attrs_data.layers[0].tiles = no_attrs_data.layer_tiles[0].data();
    
    result = get_tile_attribute(no_attrs_data.get_layers(), 
                               no_attrs_data.get_num_layers(),
                               0, 5, 5, "any_key");
    TEST_ASSERT_NULL_MESSAGE(result, "Tile with no attributes should return nullptr");
    
    // Test 3: Coordinates at (0, 0)
    TestLayerData zero_coords_data;
    zero_coords_data.layers.resize(1);
    zero_coords_data.layer_tiles.resize(1);
    zero_coords_data.layer_tiles[0].resize(1);
    zero_coords_data.tile_attributes.resize(1);
    zero_coords_data.tile_attributes[0].resize(1);
    zero_coords_data.tile_attributes[0][0].resize(1);
    zero_coords_data.layer_names.push_back("Layer_0");
    zero_coords_data.all_keys.push_back("test_key");
    zero_coords_data.all_values.push_back("test_value");
    
    // Create stable pointers
    zero_coords_data.layer_names_ptrs.push_back(zero_coords_data.layer_names[0].c_str());
    zero_coords_data.all_keys_ptrs.push_back(zero_coords_data.all_keys[0].c_str());
    zero_coords_data.all_values_ptrs.push_back(zero_coords_data.all_values[0].c_str());
    
    zero_coords_data.tile_attributes[0][0][0].key = zero_coords_data.all_keys_ptrs[0];
    zero_coords_data.tile_attributes[0][0][0].value = zero_coords_data.all_values_ptrs[0];
    
    zero_coords_data.layer_tiles[0][0].x = 0;
    zero_coords_data.layer_tiles[0][0].y = 0;
    zero_coords_data.layer_tiles[0][0].num_attributes = 1;
    zero_coords_data.layer_tiles[0][0].attributes = zero_coords_data.tile_attributes[0][0].data();
    
    zero_coords_data.layers[0].layer_name = zero_coords_data.layer_names_ptrs[0];
    zero_coords_data.layers[0].num_tiles_with_attributes = 1;
    zero_coords_data.layers[0].tiles = zero_coords_data.layer_tiles[0].data();
    
    result = get_tile_attribute(zero_coords_data.get_layers(), 
                               zero_coords_data.get_num_layers(),
                               0, 0, 0, "test_key");
    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Tile at (0,0) should be found");
    TEST_ASSERT_EQUAL_STRING("test_value", result);
}

/**
 * @brief Property test: Multiple attributes per tile
 * 
 * Verifies that tiles with multiple attributes return correct values
 * for each key.
 */
void test_property_multiple_attributes(void) {
    TestLayerData multi_attr_data;
    multi_attr_data.layers.resize(1);
    multi_attr_data.layer_tiles.resize(1);
    multi_attr_data.layer_tiles[0].resize(1);
    multi_attr_data.tile_attributes.resize(1);
    multi_attr_data.tile_attributes[0].resize(1);
    multi_attr_data.tile_attributes[0][0].resize(3);
    multi_attr_data.layer_names.push_back("Layer_0");
    
    // Add 3 attributes
    multi_attr_data.all_keys.push_back("solid");
    multi_attr_data.all_keys.push_back("type");
    multi_attr_data.all_keys.push_back("damage");
    multi_attr_data.all_values.push_back("true");
    multi_attr_data.all_values.push_back("door");
    multi_attr_data.all_values.push_back("10");
    
    // Create stable pointers
    multi_attr_data.layer_names_ptrs.push_back(multi_attr_data.layer_names[0].c_str());
    for (int i = 0; i < 3; i++) {
        multi_attr_data.all_keys_ptrs.push_back(multi_attr_data.all_keys[i].c_str());
        multi_attr_data.all_values_ptrs.push_back(multi_attr_data.all_values[i].c_str());
    }
    
    for (int i = 0; i < 3; i++) {
        multi_attr_data.tile_attributes[0][0][i].key = multi_attr_data.all_keys_ptrs[i];
        multi_attr_data.tile_attributes[0][0][i].value = multi_attr_data.all_values_ptrs[i];
    }
    
    multi_attr_data.layer_tiles[0][0].x = 10;
    multi_attr_data.layer_tiles[0][0].y = 5;
    multi_attr_data.layer_tiles[0][0].num_attributes = 3;
    multi_attr_data.layer_tiles[0][0].attributes = multi_attr_data.tile_attributes[0][0].data();
    
    multi_attr_data.layers[0].layer_name = multi_attr_data.layer_names_ptrs[0];
    multi_attr_data.layers[0].num_tiles_with_attributes = 1;
    multi_attr_data.layers[0].tiles = multi_attr_data.layer_tiles[0].data();
    
    // Verify each attribute can be queried
    const char* result;
    
    result = get_tile_attribute(multi_attr_data.get_layers(), 
                               multi_attr_data.get_num_layers(),
                               0, 10, 5, "solid");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("true", result);
    
    result = get_tile_attribute(multi_attr_data.get_layers(), 
                               multi_attr_data.get_num_layers(),
                               0, 10, 5, "type");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("door", result);
    
    result = get_tile_attribute(multi_attr_data.get_layers(), 
                               multi_attr_data.get_num_layers(),
                               0, 10, 5, "damage");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("10", result);
}

// =============================================================================
// Property 2: Existence Check Consistency
// **Validates: Requirements 2.4**
// =============================================================================

void test_property_existence_check_consistency(void) {
    const int NUM_ITERATIONS = 50;
    const uint8_t TEST_MAX_LAYERS = 5;
    const uint16_t TEST_MAX_TILES_PER_LAYER = 20;
    
    for (int iteration = 0; iteration < NUM_ITERATIONS; iteration++) {
        TestLayerData test_data;
        test_data.generate((rand() % TEST_MAX_LAYERS) + 1, TEST_MAX_TILES_PER_LAYER, 5, iteration + 67890);
        
        uint8_t layer_count = test_data.get_num_layers();
        const LayerAttributes* layers = test_data.get_layers();
        
        for (uint8_t layer_idx = 0; layer_idx < layer_count; layer_idx++) {
            // Test existing tiles
            for (uint16_t tile_idx = 0; tile_idx < test_data.layers[layer_idx].num_tiles_with_attributes; tile_idx++) {
                const TileAttributeEntry& tile = test_data.layer_tiles[layer_idx][tile_idx];
                TEST_ASSERT_TRUE_MESSAGE(tile_has_attributes(layers, layer_count, layer_idx, tile.x, tile.y),
                    "Existing tile should return true for tile_has_attributes");
            }
            
            // Test random coordinates (mostly non-existent)
            for (int i = 0; i < 20; i++) {
                uint16_t x = rand() % 256;
                uint16_t y = rand() % 256;
                
                bool found = false;
                for (uint16_t t = 0; t < test_data.layers[layer_idx].num_tiles_with_attributes; t++) {
                    if (test_data.layer_tiles[layer_idx][t].x == x && test_data.layer_tiles[layer_idx][t].y == y) {
                        found = true;
                        break;
                    }
                }
                
                TEST_ASSERT_EQUAL_INT(found, tile_has_attributes(layers, layer_count, layer_idx, x, y));
            }
        }
    }
}

// =============================================================================
// Property 3: Key String Format Independence
// **Validates: Requirements 2.3**
// =============================================================================

void test_property_key_string_format_independence(void) {
    TestLayerData test_data;
    test_data.generate(1, 5, 5, 11111);
    
    if (test_data.get_num_layers() == 0 || test_data.layers[0].num_tiles_with_attributes == 0) {
        return; // Skip if no data generated
    }

    const LayerAttributes* layers = test_data.get_layers();
    uint8_t layer_count = test_data.get_num_layers();
    
    // Find a tile with attributes
    for (uint16_t t = 0; t < test_data.layers[0].num_tiles_with_attributes; t++) {
        const TileAttributeEntry& tile = test_data.layer_tiles[0][t];
        if (tile.num_attributes > 0) {
            const TileAttribute& attr = test_data.tile_attributes[0][t][0];
            
            // Test with the pointer from our data structure
            const char* result1 = get_tile_attribute(layers, layer_count, 0, tile.x, tile.y, attr.key);
            
            // Test with a local copy of the string (different pointer, same content)
            std::string key_copy = attr.key;
            const char* result2 = get_tile_attribute(layers, layer_count, 0, tile.x, tile.y, key_copy.c_str());
            
            TEST_ASSERT_NOT_NULL(result1);
            TEST_ASSERT_NOT_NULL(result2);
            TEST_ASSERT_EQUAL_PTR(result1, result2);
            TEST_ASSERT_EQUAL_STRING(attr.value, result2);
            return;
        }
    }
}

// =============================================================================
// Property 5: Idempotence
// **Validates: Requirements 7.5**
// =============================================================================

void test_property_idempotence(void) {
    TestLayerData test_data;
    test_data.generate(1, 10, 5, 22222);
    
    if (test_data.get_num_layers() == 0) return;

    const LayerAttributes* layers = test_data.get_layers();
    uint8_t layer_count = test_data.get_num_layers();
    
    for (uint16_t i = 0; i < test_data.layers[0].num_tiles_with_attributes; i++) {
        const TileAttributeEntry& tile = test_data.layer_tiles[0][i];
        if (tile.num_attributes > 0) {
            const char* key = test_data.tile_attributes[0][i][0].key;
            
            const char* call1 = get_tile_attribute(layers, layer_count, 0, tile.x, tile.y, key);
            const char* call2 = get_tile_attribute(layers, layer_count, 0, tile.x, tile.y, key);
            
            TEST_ASSERT_EQUAL_PTR(call1, call2);
        }
    }
}

// =============================================================================
// Property 7: Optimized Entry Lookup Equivalence
// **Validates: Requirements 2.5**
// =============================================================================

void test_property_optimized_entry_lookup_equivalence(void) {
    const int NUM_ITERATIONS = 50;
    
    for (int iteration = 0; iteration < NUM_ITERATIONS; iteration++) {
        TestLayerData test_data;
        test_data.generate(2, 20, 5, iteration + 33333);
        
        uint8_t layer_count = test_data.get_num_layers();
        const LayerAttributes* layers = test_data.get_layers();
        
        for (uint8_t layer_idx = 0; layer_idx < layer_count; layer_idx++) {
            for (uint16_t tile_idx = 0; tile_idx < test_data.layers[layer_idx].num_tiles_with_attributes; tile_idx++) {
                const TileAttributeEntry& expected_tile = test_data.layer_tiles[layer_idx][tile_idx];
                
                const TileAttributeEntry* entry_ptr = get_tile_entry(layers, layer_count, layer_idx, expected_tile.x, expected_tile.y);
                
                TEST_ASSERT_NOT_NULL(entry_ptr);
                
                TileAttributeEntry entry;
                PIXELROOT32_MEMCPY_P(&entry, entry_ptr, sizeof(TileAttributeEntry));
                
                TEST_ASSERT_EQUAL_INT(expected_tile.x, entry.x);
                TEST_ASSERT_EQUAL_INT(expected_tile.y, entry.y);
                TEST_ASSERT_EQUAL_INT(expected_tile.num_attributes, entry.num_attributes);
                
                // Compare all attributes
                for (uint8_t a = 0; a < entry.num_attributes; a++) {
                    TileAttribute attr;
                    PIXELROOT32_MEMCPY_P(&attr, &entry.attributes[a], sizeof(TileAttribute));
                    
                    const char* value_via_query = get_tile_attribute(layers, layer_count, layer_idx, expected_tile.x, expected_tile.y, attr.key);
                    TEST_ASSERT_EQUAL_STRING(attr.value, value_via_query);
                }
            }
        }
    }
}

// =============================================================================
// Property 8: Safe Boundary Handling
// **Validates: Requirements 6.1, 6.4**
// =============================================================================

void test_property_safe_boundary_handling(void) {
    TestLayerData test_data;
    test_data.generate(2, 5, 2, 44444);
    
    const LayerAttributes* layers = test_data.get_layers();
    uint8_t layer_count = test_data.get_num_layers();
    
    // Test layer_idx >= num_layers
    TEST_ASSERT_NULL(get_tile_attribute(layers, layer_count, layer_count, 0, 0, "key"));
    TEST_ASSERT_NULL(get_tile_attribute(layers, layer_count, layer_count + 1, 0, 0, "key"));
    TEST_ASSERT_FALSE(tile_has_attributes(layers, layer_count, layer_count, 0, 0));
    TEST_ASSERT_NULL(get_tile_entry(layers, layer_count, layer_count, 0, 0));
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    
    RUN_TEST(test_property_query_result_correctness);
    RUN_TEST(test_property_boundary_conditions);
    RUN_TEST(test_property_multiple_attributes);
    RUN_TEST(test_property_existence_check_consistency);
    RUN_TEST(test_property_key_string_format_independence);
    RUN_TEST(test_property_idempotence);
    RUN_TEST(test_property_optimized_entry_lookup_equivalence);
    RUN_TEST(test_property_safe_boundary_handling);
    
    return UNITY_END();
}
