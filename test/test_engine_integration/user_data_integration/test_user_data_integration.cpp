/**
 * @file test_user_data_integration.cpp
 * @brief Integration tests for PhysicsActor userData functionality.
 * 
 * Tests the complete flow from tile creation through collision detection
 * to verify that userData enables proper tile identification in real scenarios.
 */

#include <unity.h>
#include "core/PhysicsActor.h"
#include "physics/StaticActor.h"
#include "physics/KinematicActor.h"
#include "graphics/Renderer.h"
#include "graphics/DisplayConfig.h"
#include "math/Vector2.h"
#include "../../test_config.h"
#include "../../mocks/MockDrawSurface.h"
#include <memory>
#include <cstdint>

using namespace pixelroot32::core;
using namespace pixelroot32::physics;
using namespace pixelroot32::graphics;
using namespace pixelroot32::math;

int MockDrawSurface::instances = 0;

// Helper functions for coordinate encoding (matching TilemapCollisionBuilder)
inline uintptr_t packTileCoord(uint16_t x, uint16_t y) {
    return (static_cast<uintptr_t>(y) << 16) | x;
}

inline void unpackTileCoord(uintptr_t packed, uint16_t& x, uint16_t& y) {
    x = static_cast<uint16_t>(packed & 0xFFFF);
    y = static_cast<uint16_t>(packed >> 16);
}

// Mock InteractiveTile structure (matching Pixel Leap)
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

// Mock InteractiveTileManager (simplified for testing)
class MockInteractiveTileManager {
private:
    std::vector<MockInteractiveTile> interactiveTiles_;
    
public:
    explicit MockInteractiveTileManager(const std::vector<MockInteractiveTile>& tiles)
        : interactiveTiles_(tiles) {}
    
    MockInteractiveTile* getInteractiveTile(int x, int y) {
        for (auto& tile : interactiveTiles_) {
            if (tile.tileX == x && tile.tileY == y) {
                return &tile;
            }
        }
        return nullptr;
    }
    
    MockInteractiveTile* getInteractiveTileByBody(StaticActor* body) {
        if (!body) return nullptr;
        
        // Extract coordinates from userData
        uintptr_t packed = reinterpret_cast<uintptr_t>(body->getUserData());
        if (packed == 0) return nullptr; // userData not set
        
        uint16_t x, y;
        unpackTileCoord(packed, x, y);
        
        // Use coordinate lookup
        return getInteractiveTile(static_cast<int>(x), static_cast<int>(y));
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
};

// Mock PlayerActor (simplified collision testing)
class MockPlayerActor : public KinematicActor {
public:
    MockInteractiveTileManager* tileManager = nullptr;
    MockInteractiveTile* lastCollidedTile = nullptr;
    
    MockPlayerActor(Vector2 position) : KinematicActor(position, 16, 16) {
        setCollisionLayer(1);
        setCollisionMask(2);
    }
    
    void onCollision(Actor* other) override {
        if (tileManager) {
            MockInteractiveTile* iTile = tileManager->getInteractiveTileByBody(
                static_cast<StaticActor*>(other));
            
            if (iTile && iTile->isActive) {
                lastCollidedTile = iTile;
                // Simulate collection
                tileManager->disableInteractiveTile(iTile->tileX, iTile->tileY);
            }
        }
    }
};

void setUp(void) {
    MockDrawSurface::instances = 0;
}

void tearDown(void) {
    // Clean up
}

// =============================================================================
// Integration Test Cases
// =============================================================================

/**
 * @brief Test complete tile creation and collision identification flow.
 */
void test_user_data_tile_creation_and_collision(void) {
    // Create display config for mock renderer
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    MockDrawSurface mockSurface;
    Renderer renderer(std::move(config));
    renderer.init();
    
    // Create interactive tile at coordinates (10, 15)
    uint16_t tileX = 10, tileY = 15;
    auto body = std::make_unique<StaticActor>(
        Vector2(pixelroot32::math::toScalar(tileX * 16), pixelroot32::math::toScalar(tileY * 16)), 
        16, 16);
    
    // Store coordinates in userData (simulating TilemapCollisionBuilder)
    uintptr_t packed = packTileCoord(tileX, tileY);
    body->setUserData(reinterpret_cast<void*>(packed));
    
    // Create interactive tile
    MockInteractiveTile tile(tileX, tileY, 1, body.get());
    
    // Create manager with the tile
    std::vector<MockInteractiveTile> tiles = {tile};
    MockInteractiveTileManager manager(tiles);
    
    // Test lookup by body using userData
    MockInteractiveTile* foundTile = manager.getInteractiveTileByBody(body.get());
    
    TEST_ASSERT_NOT_NULL(foundTile);
    TEST_ASSERT_EQUAL(tileX, foundTile->tileX);
    TEST_ASSERT_EQUAL(tileY, foundTile->tileY);
    TEST_ASSERT_TRUE(foundTile->isActive);
}

/**
 * @brief test user data coordinate encoding with real-world values.
 */
void test_user_data_real_world_coordinates(void) {
    // Test typical tilemap coordinates
    std::vector<std::pair<uint16_t, uint16_t>> testCoords = {
        {0, 0},       // Origin
        {10, 15},     // Typical tile
        {255, 255},   // Large tilemap
        {65535, 65535} // Maximum limits
    };
    
    for (auto [x, y] : testCoords) {
        // Pack coordinates
        uintptr_t packed = packTileCoord(x, y);
        
        // Create mock actor with userData
        StaticActor actor(Vector2(0, 0), 16, 16);
        actor.setUserData(reinterpret_cast<void*>(packed));
        
        // Extract coordinates
        uintptr_t retrieved = reinterpret_cast<uintptr_t>(actor.getUserData());
        uint16_t unpackedX, unpackedY;
        unpackTileCoord(retrieved, unpackedX, unpackedY);
        
        TEST_ASSERT_EQUAL(x, unpackedX);
        TEST_ASSERT_EQUAL(y, unpackedY);
        
        char msg[64];
        snprintf(msg, sizeof(msg), "Coordinates (%d,%d) encode/decode", x, y);
        TEST_PASS_MESSAGE(msg);
    }
}

/**
 * @brief Test collision detection with multiple tiles using userData.
 */
void test_user_data_multiple_tile_collision(void) {
    // Create display config
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    MockDrawSurface mockSurface;
    Renderer renderer(std::move(config));
    renderer.init();
    
    // Create multiple interactive tiles
    std::vector<MockInteractiveTile> tiles;
    std::vector<std::unique_ptr<StaticActor>> bodies;
    
    std::vector<std::pair<uint16_t, uint16_t>> tilePositions = {
        {5, 5}, {10, 10}, {15, 15}, {20, 20}
    };
    
    for (auto [x, y] : tilePositions) {
        auto body = std::make_unique<StaticActor>(
            Vector2(pixelroot32::math::toScalar(x * 16), pixelroot32::math::toScalar(y * 16)), 
            16, 16);
        
        // Store coordinates in userData
        uintptr_t packed = packTileCoord(x, y);
        body->setUserData(reinterpret_cast<void*>(packed));
        
        tiles.emplace_back(x, y, 1, body.get());
        bodies.push_back(std::move(body));
    }
    
    // Create manager
    MockInteractiveTileManager manager(tiles);
    
    // Test lookup for each tile
    for (size_t i = 0; i < tiles.size(); ++i) {
        MockInteractiveTile* foundTile = manager.getInteractiveTileByBody(tiles[i].body);
        
        TEST_ASSERT_NOT_NULL(foundTile);
        TEST_ASSERT_EQUAL(tiles[i].tileX, foundTile->tileX);
        TEST_ASSERT_EQUAL(tiles[i].tileY, foundTile->tileY);
        
        char msg[64];
        snprintf(msg, sizeof(msg), "Tile %zu lookup correct", i);
        TEST_PASS_MESSAGE(msg);
    }
}

/**
 * @brief Test player collision with userData-enabled tiles.
 */
void test_user_data_player_collision_integration(void) {
    // Create display config
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    MockDrawSurface mockSurface;
    Renderer renderer(std::move(config));
    renderer.init();
    
    // Create coin tile at player position
    uint16_t tileX = 8, tileY = 8;
    auto coinBody = std::make_unique<StaticActor>(
        Vector2(pixelroot32::math::toScalar(tileX * 16), pixelroot32::math::toScalar(tileY * 16)), 
        16, 16);
    coinBody->setCollisionLayer(2);
    coinBody->setCollisionMask(1);
    
    // Store coordinates in userData
    uintptr_t packed = packTileCoord(tileX, tileY);
    coinBody->setUserData(reinterpret_cast<void*>(packed));
    
    // Create interactive tile
    MockInteractiveTile coinTile(tileX, tileY, 1, coinBody.get());
    
    // Create manager
    std::vector<MockInteractiveTile> tiles = {coinTile};
    MockInteractiveTileManager manager(tiles);
    
    // Create player at same position
    MockPlayerActor player(Vector2(pixelroot32::math::toScalar(tileX * 16), pixelroot32::math::toScalar(tileY * 16)));
    player.tileManager = &manager;
    
    // Simulate collision
    player.onCollision(coinBody.get());
    
    // Verify collision was detected and tile was identified
    TEST_ASSERT_NOT_NULL(player.lastCollidedTile);
    TEST_ASSERT_EQUAL(tileX, player.lastCollidedTile->tileX);
    TEST_ASSERT_EQUAL(tileY, player.lastCollidedTile->tileY);
    TEST_ASSERT_FALSE(player.lastCollidedTile->isActive); // Should be disabled after collection
}

/**
 * @brief Test userData behavior with null and invalid values.
 */
void test_user_data_edge_cases(void) {
    StaticActor actor(Vector2(0, 0), 16, 16);
    
    // Test default userData is null
    TEST_ASSERT_NULL(actor.getUserData());
    
    // Test setting and getting null
    actor.setUserData(nullptr);
    TEST_ASSERT_NULL(actor.getUserData());
    
    // Test setting valid pointer
    int testValue = 42;
    actor.setUserData(&testValue);
    TEST_ASSERT_EQUAL(&testValue, actor.getUserData());
    
    // Test coordinate unpacking with zero (invalid userData)
    uintptr_t zeroPacked = 0;
    uint16_t x, y;
    unpackTileCoord(zeroPacked, x, y);
    TEST_ASSERT_EQUAL(0, x);
    TEST_ASSERT_EQUAL(0, y);
    
    // Test manager lookup with null userData
    std::vector<MockInteractiveTile> emptyTiles;
    MockInteractiveTileManager emptyManager(emptyTiles);
    StaticActor nullActor(Vector2(0, 0), 16, 16);
    nullActor.setUserData(nullptr);
    
    MockInteractiveTile* result = emptyManager.getInteractiveTileByBody(&nullActor);
    TEST_ASSERT_NULL(result);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    
    // userData integration tests
    RUN_TEST(test_user_data_tile_creation_and_collision);
    RUN_TEST(test_user_data_real_world_coordinates);
    RUN_TEST(test_user_data_multiple_tile_collision);
    RUN_TEST(test_user_data_player_collision_integration);
    RUN_TEST(test_user_data_edge_cases);
    
    return UNITY_END();
}
