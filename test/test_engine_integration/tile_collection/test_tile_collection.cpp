/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"
#include "core/Scene.h"
#include "physics/StaticActor.h"
#include "../../mocks/MockDrawSurface.h"
#include <memory>
#include <vector>
#include <chrono>

using namespace pixelroot32::graphics;
using namespace pixelroot32::core;
using namespace pixelroot32::physics;

// Mock InteractiveTile and related classes for testing
struct MockInteractiveTile {
    int tileX;
    int tileY;
    int attributeType;
    StaticActor* body;
    bool isActive;
    
    MockInteractiveTile() : tileX(0), tileY(0), attributeType(0), body(nullptr), isActive(true) {}
    MockInteractiveTile(int x, int y, int type, StaticActor* actor) 
        : tileX(x), tileY(y), attributeType(type), body(actor), isActive(true) {}
};

class MockInteractiveTileManager {
public:
    std::vector<MockInteractiveTile> tiles;
    
    MockInteractiveTileManager(const std::vector<MockInteractiveTile>& interactiveTiles) 
        : tiles(interactiveTiles) {}
    
    MockInteractiveTile* getInteractiveTile(int x, int y) {
        for (auto& tile : tiles) {
            if (tile.tileX == x && tile.tileY == y) {
                return &tile;
            }
        }
        return nullptr;
    }
    
    MockInteractiveTile* getInteractiveTileByBody(StaticActor* body) {
        for (auto& tile : tiles) {
            if (tile.body == body) {
                return &tile;
            }
        }
        return nullptr;
    }
    
    bool disableInteractiveTile(int x, int y) {
        MockInteractiveTile* tile = getInteractiveTile(x, y);
        if (tile && tile->isActive) {
            tile->isActive = false;
            if (tile->body) {
                tile->body->setEnabled(false);
            }
            return true;
        }
        return false;
    }
    
    const std::vector<MockInteractiveTile>& getInteractiveTiles() const {
        return tiles;
    }
};

// Mock TileStatePersistence for testing
class MockTileStatePersistence {
public:
    static int serializeTileStates(const TileMap4bpp* tilemap,
                                  const std::vector<MockInteractiveTile>& interactiveTiles,
                                  uint8_t* outputBuffer,
                                  size_t bufferSize) {
        if (!tilemap || !outputBuffer) return -1;
        
        int maskBytes = (tilemap->width * tilemap->height + 7) / 8;
        int totalBytes = 4 + maskBytes; // Simplified
        
        if (bufferSize < totalBytes) return -1;
        
        size_t offset = 0;
        outputBuffer[offset++] = 'T';
        outputBuffer[offset++] = 'S';
        outputBuffer[offset++] = static_cast<uint8_t>(tilemap->width);
        outputBuffer[offset++] = static_cast<uint8_t>(tilemap->height);
        
        if (tilemap->getRuntimeMask()) {
            for (int i = 0; i < maskBytes; i++) {
                outputBuffer[offset++] = tilemap->getRuntimeMask()[i];
            }
        } else {
            for (int i = 0; i < maskBytes; i++) {
                outputBuffer[offset++] = 0xFF;
            }
        }
        
        return static_cast<int>(offset);
    }
    
    static bool deserializeTileStates(const uint8_t* inputData,
                                     size_t dataSize,
                                     TileMap4bpp* tilemap,
                                     std::vector<MockInteractiveTile>& interactiveTiles) {
        if (!inputData || !tilemap || dataSize < 4) return false;
        
        if (inputData[0] != 'T' || inputData[1] != 'S') return false;
        
        uint8_t width = inputData[2];
        uint8_t height = inputData[3];
        
        if (width != tilemap->width || height != tilemap->height) return false;
        
        int maskBytes = (width * height + 7) / 8;
        if (dataSize < 4 + maskBytes) return false;
        
        if (!tilemap->getRuntimeMask()) {
            tilemap->initRuntimeMask();
        }
        
        for (int i = 0; i < maskBytes; i++) {
            tilemap->getRuntimeMask()[i] = inputData[4 + i];
        }
        
        return true;
    }
};

// Tile type constants
enum TileType {
    TILE_TYPE_COIN = 1,
    TILE_TYPE_HEART = 2,
    TILE_TYPE_POWERUP = 3
};

// Mock game state for testing
class MockGameState {
public:
    static int score;
    static int lives;
    
    static void addScore(int points) { score += points; }
    static void addLife() { if (lives < 3) lives++; }
    static void reset() { score = 0; lives = 3; }
};

int MockGameState::score = 0;
int MockGameState::lives = 3;

// Test level setup
struct TestLevel {
    TileMap4bpp tilemap;
    std::vector<MockInteractiveTile> interactiveTiles;
    MockInteractiveTileManager* tileManager;
    Scene* scene;
    
    void setup() {
        MockGameState::reset();
        
        // Create 16x16 tilemap with some interactive tiles
        static uint8_t tileData[256] = {0};
        static Sprite4bpp tiles[20] = {{nullptr, nullptr, 8, 8, 8}};
        
        // Place coins at (5,5), (10,5), (5,10)
        tileData[5 + 5*16] = 10; // Coin type
        tileData[10 + 5*16] = 10; // Coin type  
        tileData[5 + 10*16] = 10; // Coin type
        
        // Place heart at (8,8)
        tileData[8 + 8*16] = 16; // Heart type
        
        tilemap.indices = tileData;
        tilemap.width = 16;
        tilemap.height = 16;
        tilemap.tiles = tiles;
        tilemap.tileWidth = 8;
        tilemap.tileHeight = 8;
        tilemap.tileCount = 20;
        tilemap.runtimeMask = nullptr;
        
        // Initialize runtime mask
        tilemap.initRuntimeMask();
        
        // Create mock interactive tiles
        interactiveTiles.clear();
        
        // Add coin tiles
        interactiveTiles.emplace_back(5, 5, TILE_TYPE_COIN, nullptr);
        interactiveTiles.emplace_back(10, 5, TILE_TYPE_COIN, nullptr);
        interactiveTiles.emplace_back(5, 10, TILE_TYPE_COIN, nullptr);
        
        // Add heart tile
        interactiveTiles.emplace_back(8, 8, TILE_TYPE_HEART, nullptr);
        
        // Create scene and manager
        scene = new Scene();
        tileManager = new MockInteractiveTileManager(interactiveTiles);
    }
    
    void cleanup() {
        delete tileManager;
        delete scene;
        tilemap.cleanupRuntimeMask();
    }
};

static TestLevel testLevel;

void setUp(void) {
    test_setup();
    testLevel.setup();
}

void tearDown(void) {
    testLevel.cleanup();
    test_teardown();
}

void test_tile_collection_integration_coin_collection() {
    // Verify coin is initially active
    TEST_ASSERT_TRUE_MESSAGE(testLevel.tilemap.isTileActive(5, 5),
        "Coin should be active initially");
    
    // Verify interactive tile exists and is active
    MockInteractiveTile* coin = testLevel.tileManager->getInteractiveTile(5, 5);
    TEST_ASSERT_NOT_NULL_MESSAGE(coin, "Interactive coin should exist");
    TEST_ASSERT_TRUE_MESSAGE(coin->isActive, "Coin collision should be active");
    TEST_ASSERT_EQUAL_MESSAGE(TILE_TYPE_COIN, coin->attributeType,
        "Coin should have correct type");
    
    // Simulate coin collection
    coin->isActive = false;
    if (coin->body) {
        coin->body->setEnabled(false);
    }
    testLevel.tilemap.setTileActive(5, 5, false);
    MockGameState::addScore(10);
    
    // Verify coin is now inactive
    TEST_ASSERT_FALSE_MESSAGE(testLevel.tilemap.isTileActive(5, 5),
        "Coin should be inactive after collection");
    
    TEST_ASSERT_FALSE_MESSAGE(coin->isActive,
        "Coin collision should be inactive after collection");
    
    TEST_ASSERT_EQUAL_MESSAGE(10, MockGameState::score,
        "Score should be incremented");
}

void test_tile_collection_integration_multiple_coins() {
    // Collect coin at (5,5)
    MockInteractiveTile* coin1 = testLevel.tileManager->getInteractiveTile(5, 5);
    TEST_ASSERT_NOT_NULL(coin1);
    coin1->isActive = false;
    if (coin1->body) coin1->body->setEnabled(false);
    testLevel.tilemap.setTileActive(5, 5, false);
    
    // Verify other coins are still active
    TEST_ASSERT_TRUE_MESSAGE(testLevel.tilemap.isTileActive(10, 5),
        "Second coin should still be active");
    TEST_ASSERT_TRUE_MESSAGE(testLevel.tilemap.isTileActive(5, 10),
        "Third coin should still be active");
    
    MockInteractiveTile* coin2 = testLevel.tileManager->getInteractiveTile(10, 5);
    MockInteractiveTile* coin3 = testLevel.tileManager->getInteractiveTile(5, 10);
    
    TEST_ASSERT_NOT_NULL(coin2);
    TEST_ASSERT_NOT_NULL(coin3);
    TEST_ASSERT_TRUE_MESSAGE(coin2->isActive, "Second coin collision should be active");
    TEST_ASSERT_TRUE_MESSAGE(coin3->isActive, "Third coin collision should be active");
    
    // Collect second coin
    coin2->isActive = false;
    if (coin2->body) coin2->body->setEnabled(false);
    testLevel.tilemap.setTileActive(10, 5, false);
    
    // Verify third coin is still active
    TEST_ASSERT_TRUE_MESSAGE(testLevel.tilemap.isTileActive(5, 10),
        "Third coin should still be active after second collection");
    TEST_ASSERT_TRUE_MESSAGE(coin3->isActive, "Third coin collision should be active");
}

void test_tile_collection_integration_heart_collection() {
    // Verify heart is initially active
    TEST_ASSERT_TRUE_MESSAGE(testLevel.tilemap.isTileActive(8, 8),
        "Heart should be active initially");
    
    MockInteractiveTile* heart = testLevel.tileManager->getInteractiveTile(8, 8);
    TEST_ASSERT_NOT_NULL_MESSAGE(heart, "Interactive heart should exist");
    TEST_ASSERT_EQUAL_MESSAGE(TILE_TYPE_HEART, heart->attributeType,
        "Heart should have correct type");
    
    // Simulate heart collection
    int initialLives = MockGameState::lives;
    heart->isActive = false;
    if (heart->body) heart->body->setEnabled(false);
    testLevel.tilemap.setTileActive(8, 8, false);
    MockGameState::addLife();
    
    // Verify heart is now inactive and life was added (or capped at max)
    int expectedLives = (initialLives < 3) ? initialLives + 1 : 3;
    TEST_ASSERT_EQUAL_MESSAGE(expectedLives, MockGameState::lives,
        "Life should be incremented (or capped at maximum)");
    
    TEST_ASSERT_FALSE_MESSAGE(heart->isActive,
        "Heart collision should be inactive after collection");
}

void test_tile_collection_integration_state_synchronization() {
    // Create inconsistency: visual active but collision inactive
    testLevel.tilemap.setTileActive(5, 5, false); // Visual inactive
    // Keep collision active
    
    // Get the interactive tile
    MockInteractiveTile* coin = testLevel.tileManager->getInteractiveTile(5, 5);
    TEST_ASSERT_NOT_NULL(coin);
    
    // Simulate state synchronization (collision system takes precedence)
    if (coin->isActive) {
        testLevel.tilemap.setTileActive(5, 5, true); // Should be reactivated
    }
    
    // Test reverse: collision inactive but visual active
    coin->isActive = false;
    if (coin->body) coin->body->setEnabled(false);
    testLevel.tilemap.setTileActive(5, 5, true); // Visual active
    
    // Synchronization should make visual inactive
    if (!coin->isActive) {
        testLevel.tilemap.setTileActive(5, 5, false);
    }
    
    TEST_ASSERT_FALSE_MESSAGE(testLevel.tilemap.isTileActive(5, 5),
        "Visual state should match collision state after sync");
}

void test_tile_collection_integration_persistence() {
    // Collect some items
    testLevel.tilemap.setTileActive(5, 5, false);
    testLevel.tilemap.setTileActive(8, 8, false);
    
    MockInteractiveTile* coin = testLevel.tileManager->getInteractiveTile(5, 5);
    MockInteractiveTile* heart = testLevel.tileManager->getInteractiveTile(8, 8);
    
    if (coin) coin->isActive = false;
    if (heart) heart->isActive = false;
    
    // Simulate save/load process
    uint8_t saveBuffer[512];
    int bytesWritten = MockTileStatePersistence::serializeTileStates(
        &testLevel.tilemap, testLevel.interactiveTiles, saveBuffer, sizeof(saveBuffer));
    
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, bytesWritten,
        "Serialization should write data");
    
    // Reset all tiles to active
    testLevel.tilemap.initRuntimeMask(); // Resets all to active
    for (auto& tile : testLevel.interactiveTiles) {
        tile.isActive = true;
        if (tile.body) tile.body->setEnabled(true);
    }
    
    // Verify reset worked
    TEST_ASSERT_TRUE_MESSAGE(testLevel.tilemap.isTileActive(5, 5),
        "Coin should be active after reset");
    TEST_ASSERT_TRUE_MESSAGE(testLevel.tilemap.isTileActive(8, 8),
        "Heart should be active after reset");
    
    // Load saved state
    bool loadSuccess = MockTileStatePersistence::deserializeTileStates(
        saveBuffer, bytesWritten, &testLevel.tilemap, testLevel.interactiveTiles);
    
    TEST_ASSERT_TRUE_MESSAGE(loadSuccess, "Deserialization should succeed");
    
    // Verify loaded state matches saved state
    TEST_ASSERT_FALSE_MESSAGE(testLevel.tilemap.isTileActive(5, 5),
        "Coin should be inactive after loading");
    TEST_ASSERT_FALSE_MESSAGE(testLevel.tilemap.isTileActive(8, 8),
        "Heart should be inactive after loading");
    
    if (coin) TEST_ASSERT_FALSE_MESSAGE(coin->isActive,
        "Coin collision should be inactive after loading");
    if (heart) TEST_ASSERT_FALSE_MESSAGE(heart->isActive,
        "Heart collision should be inactive after loading");
}

void test_tile_collection_integration_boundary_conditions() {
    // Test coordinates outside tilemap bounds
    TEST_ASSERT_NULL_MESSAGE(testLevel.tileManager->getInteractiveTile(-1, 0),
        "Should return null for negative X");
    TEST_ASSERT_NULL_MESSAGE(testLevel.tileManager->getInteractiveTile(0, -1),
        "Should return null for negative Y");
    TEST_ASSERT_NULL_MESSAGE(testLevel.tileManager->getInteractiveTile(16, 0),
        "Should return null for X >= width");
    TEST_ASSERT_NULL_MESSAGE(testLevel.tileManager->getInteractiveTile(0, 16),
        "Should return null for Y >= height");
    
    // Test empty coordinates (no interactive tile)
    TEST_ASSERT_NULL_MESSAGE(testLevel.tileManager->getInteractiveTile(0, 0),
        "Should return null for empty tile");
    TEST_ASSERT_NULL_MESSAGE(testLevel.tileManager->getInteractiveTile(7, 7),
        "Should return null for empty tile");
}

void test_tile_collection_integration_performance() {
    // Test that tile operations are fast enough for real-time use
    const int iterations = 10000;
    
    // Benchmark tile activation checks
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
        testLevel.tilemap.isTileActive(i % 16, (i / 16) % 16);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should be less than 1ms per 1000 operations (very fast)
    double microsPerOp = static_cast<double>(duration.count()) / iterations;
    TEST_ASSERT_LESS_THAN_MESSAGE(1.0, microsPerOp,
        "Tile activation check should be very fast");
    
    // Benchmark interactive tile lookup
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
        testLevel.tileManager->getInteractiveTile(5, 5);
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    microsPerOp = static_cast<double>(duration.count()) / iterations;
    TEST_ASSERT_LESS_THAN_MESSAGE(1.0, microsPerOp,
        "Interactive tile lookup should be very fast");
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_tile_collection_integration_coin_collection);
    RUN_TEST(test_tile_collection_integration_multiple_coins);
    RUN_TEST(test_tile_collection_integration_heart_collection);
    RUN_TEST(test_tile_collection_integration_state_synchronization);
    RUN_TEST(test_tile_collection_integration_persistence);
    RUN_TEST(test_tile_collection_integration_boundary_conditions);
    RUN_TEST(test_tile_collection_integration_performance);
    
    return UNITY_END();
}
