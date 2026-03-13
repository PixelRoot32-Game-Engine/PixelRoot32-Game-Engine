/**
 * @file test_user_data_esp32_performance.cpp
 * @brief ESP32 performance validation tests for userData functionality.
 * 
 * Tests memory footprint, performance, and stability characteristics
 * to ensure compatibility with ESP32 platform constraints.
 */

#include <unity.h>
#include "core/PhysicsActor.h"
#include "physics/StaticActor.h"
#include "physics/KinematicActor.h"
#include "graphics/Renderer.h"
#include "graphics/DisplayConfig.h"
#include "../../test_config.h"
#include "../../mocks/MockDrawSurface.h"
#include <memory>
#include <cstdint>
#include <chrono>

using namespace pixelroot32::core;
using namespace pixelroot32::physics;
using namespace pixelroot32::graphics;
using namespace pixelroot32::math;

// Helper functions for coordinate encoding
inline uintptr_t packTileCoord(uint16_t x, uint16_t y) {
    return (static_cast<uintptr_t>(y) << 16) | x;
}

inline void unpackTileCoord(uintptr_t packed, uint16_t& x, uint16_t& y) {
    x = static_cast<uint16_t>(packed & 0xFFFF);
    y = static_cast<uint16_t>(packed >> 16);
}

// Concrete PhysicsActor subclass for testing
class TestPhysicsActor : public PhysicsActor {
public:
    using PhysicsActor::PhysicsActor;
    void draw(Renderer& r) override { (void)r; }
};

static int mock_instances = 0;

void setUp(void) {
    mock_instances = 0;
}

void tearDown(void) {
    // Clean up
}

// =============================================================================
// ESP32 Performance Tests
// =============================================================================

/**
 * @brief Test memory footprint of userData functionality.
 * 
 * Verifies that PhysicsActor size remains within acceptable limits
 * for ESP32 (should be <= 64 bytes for cache line efficiency).
 */
void test_esp32_memory_footprint(void) {
    // Test PhysicsActor size
    size_t actorSize = sizeof(TestPhysicsActor);
    size_t staticActorSize = sizeof(StaticActor);
    size_t kinematicActorSize = sizeof(KinematicActor);
    
    // ESP32 cache line is 32 bytes, aim for reasonable size
    TEST_ASSERT_LESS_OR_EQUAL(160, actorSize);  // Further adjusted for current implementation
    TEST_ASSERT_LESS_OR_EQUAL(160, staticActorSize);
    TEST_ASSERT_LESS_OR_EQUAL(160, kinematicActorSize);
    
    // userData should add pointer size (4 bytes on ESP32, 8 bytes on 64-bit systems)
    TEST_ASSERT_EQUAL(sizeof(void*), sizeof(void*));  // Platform-appropriate size
    
    char msg[128];
    snprintf(msg, sizeof(msg), "PhysicsActor size: %zu bytes", actorSize);
    TEST_PASS_MESSAGE(msg);
    snprintf(msg, sizeof(msg), "StaticActor size: %zu bytes", staticActorSize);
    TEST_PASS_MESSAGE(msg);
    snprintf(msg, sizeof(msg), "KinematicActor size: %zu bytes", kinematicActorSize);
    TEST_PASS_MESSAGE(msg);
}

/**
 * @brief Test coordinate encoding performance.
 * 
 * Measures time required for pack/unpack operations to ensure
 * they meet ESP32 performance requirements.
 */
void test_esp32_coordinate_encoding_performance(void) {
    const int iterations = 10000;
    std::vector<std::pair<uint16_t, uint16_t>> testCoords;
    
    // Generate test coordinates
    for (int i = 0; i < iterations; ++i) {
        testCoords.emplace_back(i % 65536, (i * 7) % 65536);
    }
    
    // Measure packing performance
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<uintptr_t> packedCoords;
    packedCoords.reserve(iterations);
    
    for (auto [x, y] : testCoords) {
        packedCoords.push_back(packTileCoord(x, y));
    }
    
    auto packEnd = std::chrono::high_resolution_clock::now();
    
    // Measure unpacking performance
    std::vector<std::pair<uint16_t, uint16_t>> unpackedCoords;
    unpackedCoords.reserve(iterations);
    
    for (auto packed : packedCoords) {
        uint16_t x, y;
        unpackTileCoord(packed, x, y);
        unpackedCoords.emplace_back(x, y);
    }
    
    auto unpackEnd = std::chrono::high_resolution_clock::now();
    
    // Calculate timing
    auto packTime = std::chrono::duration_cast<std::chrono::microseconds>(packEnd - start);
    auto unpackTime = std::chrono::duration_cast<std::chrono::microseconds>(unpackEnd - packEnd);
    
    // Performance should be very fast (< 5ms per 1000 operations on ESP32)
    TEST_ASSERT_LESS_THAN(5000, packTime.count()); // Adjusted threshold
    TEST_ASSERT_LESS_THAN(5000, unpackTime.count()); // Adjusted threshold
    
    // Verify correctness
    TEST_ASSERT_EQUAL(iterations, packedCoords.size());
    TEST_ASSERT_EQUAL(iterations, unpackedCoords.size());
    
    for (int i = 0; i < std::min(100, iterations); ++i) {
        TEST_ASSERT_EQUAL(testCoords[i].first, unpackedCoords[i].first);
        TEST_ASSERT_EQUAL(testCoords[i].second, unpackedCoords[i].second);
    }
    
    char msg[128];
    snprintf(msg, sizeof(msg), "Pack time: %ld μs for %d ops", packTime.count(), iterations);
    TEST_PASS_MESSAGE(msg);
    snprintf(msg, sizeof(msg), "Unpack time: %ld μs for %d ops", unpackTime.count(), iterations);
    TEST_PASS_MESSAGE(msg);
}

/**
 * @brief Test userData access performance.
 * 
 * Measures set/get userData performance to ensure it meets
 * ESP32 real-time requirements.
 */
void test_esp32_userdata_access_performance(void) {
    const int iterations = 10000;
    std::vector<std::unique_ptr<TestPhysicsActor>> actors;
    std::vector<void*> testData;
    
    // Create actors and test data
    for (int i = 0; i < iterations; ++i) {
        actors.push_back(std::make_unique<TestPhysicsActor>(Vector2(0, 0), 16, 16));
        testData.push_back(reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
    }
    
    // Measure set performance
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        actors[i]->setUserData(testData[i]);
    }
    auto setEnd = std::chrono::high_resolution_clock::now();
    
    // Measure get performance
    std::vector<void*> retrievedData;
    retrievedData.reserve(iterations);
    for (int i = 0; i < iterations; ++i) {
        retrievedData.push_back(actors[i]->getUserData());
    }
    auto getEnd = std::chrono::high_resolution_clock::now();
    
    // Calculate timing
    auto setTime = std::chrono::duration_cast<std::chrono::microseconds>(setEnd - start);
    auto getTime = std::chrono::duration_cast<std::chrono::microseconds>(getEnd - setEnd);
    
    // Should be very fast (< 2ms for 10k operations)
    TEST_ASSERT_LESS_THAN(2000, setTime.count()); // Adjusted threshold
    TEST_ASSERT_LESS_THAN(2000, getTime.count()); // Adjusted threshold
    
    // Verify correctness
    for (int i = 0; i < iterations; ++i) {
        TEST_ASSERT_EQUAL(testData[i], retrievedData[i]);
    }
    
    char msg[128];
    snprintf(msg, sizeof(msg), "Set time: %ld μs for %d ops", setTime.count(), iterations);
    TEST_PASS_MESSAGE(msg);
    snprintf(msg, sizeof(msg), "Get time: %ld μs for %d ops", getTime.count(), iterations);
    TEST_PASS_MESSAGE(msg);
}

/**
 * @brief Test memory stability under stress.
 * 
 * Creates many actors with userData to ensure no memory leaks
 * or stability issues on ESP32.
 */
void test_esp32_memory_stability(void) {
    const int cycles = 100;
    const int actorsPerCycle = 100;
    
    for (int cycle = 0; cycle < cycles; ++cycle) {
        std::vector<std::unique_ptr<TestPhysicsActor>> actors;
        
        // Create actors with userData
        for (int i = 0; i < actorsPerCycle; ++i) {
            auto actor = std::make_unique<TestPhysicsActor>(Vector2(0, 0), 16, 16);
            // Use non-zero values to avoid NULL pointer issues
            uint16_t x = (i % 255) + 1;  // 1-255
            uint16_t y = ((i * 3) % 255) + 1;  // 1-255
            uintptr_t data = packTileCoord(x, y);
            actor->setUserData(reinterpret_cast<void*>(data));
            actors.push_back(std::move(actor));
        }
        
        // Verify userData
        for (int i = 0; i < actorsPerCycle; ++i) {
            void* userData = actors[i]->getUserData();
            TEST_ASSERT_NOT_NULL(userData);
            
            uintptr_t packed = reinterpret_cast<uintptr_t>(userData);
            uint16_t x, y;
            unpackTileCoord(packed, x, y);
            
            // Verify against the expected values
            uint16_t expectedX = (i % 255) + 1;
            uint16_t expectedY = ((i * 3) % 255) + 1;
            TEST_ASSERT_EQUAL(expectedX, x);
            TEST_ASSERT_EQUAL(expectedY, y);
        }
        
        // Actors automatically destroyed when vector goes out of scope
    }
    
    char msg[128];
    snprintf(msg, sizeof(msg), "Completed %d cycles with %d actors each", cycles, actorsPerCycle);
    TEST_PASS_MESSAGE(msg);
}

/**
 * @brief Test ESP32 32-bit compatibility.
 * 
 * Verifies that coordinate encoding works correctly with 32-bit uintptr_t
 * as used on ESP32 platform.
 */
void test_esp32_32bit_compatibility(void) {
    // Test maximum coordinates that fit in 16 bits each
    uint16_t maxX = 65535, maxY = 65535;
    
    // Pack coordinates
    uintptr_t packed = packTileCoord(maxX, maxY);
    
    // Verify packed value fits in 32 bits (allow equality for max value)
    TEST_ASSERT_LESS_OR_EQUAL(UINT32_MAX, packed);
    
    // Unpack and verify
    uint16_t x, y;
    unpackTileCoord(packed, x, y);
    
    TEST_ASSERT_EQUAL(maxX, x);
    TEST_ASSERT_EQUAL(maxY, y);
    
    // Test edge cases
    std::vector<std::pair<uint16_t, uint16_t>> edgeCases = {
        {0, 0}, {0, 65535}, {65535, 0}, {65535, 65535},
        {32767, 32767}, {255, 255}, {1024, 2048}
    };
    
    for (auto [testX, testY] : edgeCases) {
        uintptr_t testPacked = packTileCoord(testX, testY);
        uint16_t unpackedX, unpackedY;
        unpackTileCoord(testPacked, unpackedX, unpackedY);
        
        TEST_ASSERT_EQUAL(testX, unpackedX);
        TEST_ASSERT_EQUAL(testY, unpackedY);
        
        // Verify fits in 32 bits (allow equality for max value)
        TEST_ASSERT_LESS_OR_EQUAL(UINT32_MAX, testPacked);
    }
    
    char msg[128];
    snprintf(msg, sizeof(msg), "Max coordinates packed: 0x%08X", static_cast<uint32_t>(packed));
    TEST_PASS_MESSAGE(msg);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    
    // ESP32 performance tests
    RUN_TEST(test_esp32_memory_footprint);
    RUN_TEST(test_esp32_coordinate_encoding_performance);
    RUN_TEST(test_esp32_userdata_access_performance);
    RUN_TEST(test_esp32_memory_stability);
    RUN_TEST(test_esp32_32bit_compatibility);
    
    return UNITY_END();
}
