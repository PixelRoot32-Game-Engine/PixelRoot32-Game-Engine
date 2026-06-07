/**
 * @file test_player_jump_integration.cpp
 * @brief Integration tests for player jump, free-fall, and landing sequences
 *        using real game-loop dt values (NOT synthetic FIXED_DT).
 *
 * These tests guard against the half-jump and gliding regressions by
 * verifying the full velocity -> moveAndSlideWithSnap pipeline at 30/60/120 FPS.
 *
 * KS-INTEGRATION-TEST-SUITE
 */

#include <unity.h>
#include "physics/KinematicActor.h"
#include "physics/StaticActor.h"
#include "physics/CollisionSystem.h"
#include "../test_config.h"

using namespace pixelroot32::core;
using namespace pixelroot32::physics;
using namespace pixelroot32::math;

// =============================================================================
// Test constants (matching game examples: PLAYER_GRAVITY=400, JUMP_VELOCITYs)
// =============================================================================
static constexpr float GRAVITY = 400.0f;                  // units/s^2 (both examples)
static constexpr float JUMP_VEL_CUBE = 220.0f;            // PlayerCube: units/s upward
static constexpr float JUMP_VEL_ACTOR = 160.0f;           // PlayerActor (metroidvania): units/s upward
static constexpr float MIN_SNAP = 4.0f;                   // matches KinematicActor::MIN_SNAP
static constexpr float TOLERANCE_5PC = 0.05f;             // 5% tolerance
static constexpr float POS_TOL = 0.5f;                    // position tolerance (units)

// Frame rates under test
static constexpr float DT_120FPS = 1.0f / 120.0f;
static constexpr float DT_60FPS = 1.0f / 60.0f;
static constexpr float DT_30FPS = 1.0f / 30.0f;

// =============================================================================
// Test fixture
// =============================================================================
static CollisionSystem* colSystem = nullptr;
static KinematicActor* player = nullptr;
static StaticActor* floorBody = nullptr;

void setUp(void) {
    test_setup();
    colSystem = new CollisionSystem();
    player = new KinematicActor(toScalar(0), toScalar(0), 16, 16);
    player->setCollisionLayer(1);
    player->setCollisionMask(1);
    player->collisionSystem = colSystem;
    colSystem->addEntity(player);
    floorBody = nullptr;
}

void tearDown(void) {
    if (floorBody) delete floorBody;
    if (player) delete player;
    if (colSystem) delete colSystem;
    floorBody = nullptr;
    player = nullptr;
    colSystem = nullptr;
    test_teardown();
}

// =============================================================================
// Helper: create a floor StaticActor with its top surface at floorTopY
// =============================================================================
static void createFloor(Scalar floorTopY) {
    floorBody = new StaticActor(toScalar(-200), floorTopY, 400, 20);
    floorBody->setCollisionLayer(1);
    floorBody->setCollisionMask(1);
    colSystem->addEntity(floorBody);
}

// =============================================================================
// Helper: land the player on the floor and establish wasSnapFloor=true.
//         Player starts just above floorTopY (1 unit gap).
//         After this, player is on floor and wasSnapFloor=true for next frame.
// =============================================================================
static void setupOnFloor(Scalar floorTopY, Scalar dt) {
    createFloor(floorTopY);
    // Place player 1 unit above floor top (player bottom at floorTopY - 1)
    // Player is 16 tall, so position.y = floorTopY - 16 - 1 = floorTopY - 17
    player->position.y = floorTopY - toScalar(17);

    // Step 1: fall into floor via moveAndSlideWithSnap with sufficient velocity.
    //         displacement = velocity * dt must exceed the 1-unit gap.
    //         Use 600/s at 120FPS: 600/120 = 5 units > 1. Well above threshold.
    //         At 30FPS: 600/30 = 20 units > 1. Also fine.
    //         The slide step detects floor -> onFloor=true.
    //         wasSnapFloor=false at this point, so snap is skipped.
    //         But at the end: wasSnapFloor = onFloor = true.
    Vector2 up(0, -1);
    Vector2 snap(toScalar(0), toScalar(MIN_SNAP));
    player->moveAndSlideWithSnap(
        Vector2(toScalar(0), toScalar(600)),
        snap, dt, up
    );

    // Step 2: establish wasSnapFloor via persistent floor contact.
    //         Small downward velocity + snap.
    //         wasSnapFloor=true from step 1 -> snap fires -> onFloor=true.
    player->moveAndSlideWithSnap(
        Vector2(toScalar(0), toScalar(60)),
        snap, dt, up
    );
}

// =============================================================================
// Helper: reference Forward Euler for expected jump apex (y-displacement)
//         Returns negative value (upward displacement).
// =============================================================================
static Scalar referenceJumpApex(Scalar v0, Scalar dt) {
    // Frame 0: jump overrides gravity, so velocity = -v0 directly
    //          (matching PlayerCube::update order: gravity first, then jump)
    Scalar v = -v0;
    Scalar y = v * dt;  // Frame 0 displacement
    Scalar gDt = toScalar(GRAVITY) * dt;
    // Frames 1+: gravity accumulates on the velocity returned from the
    //            previous moveAndSlideWithSnap frame (which equals the
    //            input velocity in free space, i.e., no collisions).
    while (v < toScalar(0)) {
        v += gDt;  // gravity applied to prev frame's return velocity
        if (v > toScalar(0)) v = toScalar(0);
        y += v * dt;
    }
    return y;  // negative (upward shift)
}

// =============================================================================
// Helper: reference Forward Euler for free-fall y after N frames
// =============================================================================
static Scalar referenceFallPosition(int numFrames, Scalar dt) {
    Scalar v = toScalar(0);
    Scalar y = toScalar(0);
    Scalar gDt = toScalar(GRAVITY) * dt;
    for (int i = 0; i < numFrames; i++) {
        v += gDt;
        y += v * dt;
    }
    return y;
}

// =============================================================================
// Helper: simulate one frame of PlayerCube-style physics
//         Returns true if player is on floor after the frame.
// =============================================================================
static bool simulatePlayerCubeFrame(Scalar& vy, bool wantsJump, Scalar dt) {
    Vector2 up(0, -1);
    // Gravity first
    vy += toScalar(GRAVITY) * dt;
    // Jump overrides if requested
    bool jumpThisFrame = (wantsJump && player->is_on_floor());
    if (jumpThisFrame) {
        vy = toScalar(-JUMP_VEL_CUBE);
    }
    // Snap: disabled on jump frame (matching PlayerCube logic)
    Vector2 snap = jumpThisFrame
        ? Vector2{}
        : Vector2(toScalar(0), toScalar(MIN_SNAP));
    Vector2 result = player->moveAndSlideWithSnap(
        Vector2(toScalar(0), vy), snap, dt, up
    );
    vy = static_cast<float>(result.y);
    return player->is_on_floor();
}

// =============================================================================
// Helper: simulate one frame of PlayerActor-style physics with state machine
// =============================================================================
enum class TestState { IDLE, JUMP };

static bool simulatePlayerActorFrame(Scalar& vy, bool wantsJump, TestState& state, Scalar dt) {
    Vector2 up(0, -1);
    // Gravity
    vy += toScalar(GRAVITY) * dt;
    bool jumpThisFrame = false;
    if (wantsJump && player->is_on_floor()) {
        vy = toScalar(-JUMP_VEL_ACTOR);
        state = TestState::JUMP;
        jumpThisFrame = true;
    }
    // Snap: disabled during jump ascent frame
    Vector2 snap = (state == TestState::JUMP && jumpThisFrame)
        ? Vector2{}
        : Vector2(toScalar(0), toScalar(MIN_SNAP));
    Vector2 result = player->moveAndSlideWithSnap(
        Vector2(toScalar(0), vy), snap, dt, up
    );
    vy = static_cast<float>(result.y);
    // State machine transition
    if (player->is_on_floor()) {
        state = TestState::IDLE;
    } else {
        state = TestState::JUMP;
    }
    return player->is_on_floor();
}

// =============================================================================
// TESTS
// =============================================================================

// ---------------------------------------------------------------------------
// Test 1: Jump height >= 0.95x theoretical at 120 FPS
// ---------------------------------------------------------------------------
void test_jump_height_120fps(void) {
    Scalar dt = toScalar(DT_120FPS);
    setupOnFloor(toScalar(50), dt);
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "120fps: player should be on floor after setup");

    // Record starting Y and jump
    Scalar startY = player->position.y;
    Scalar vy = toScalar(0);
    Scalar minY = player->position.y;
    int maxFrames = 400;

    for (int frame = 0; frame < maxFrames; frame++) {
        bool onFloor = simulatePlayerCubeFrame(vy, frame == 0, dt);
        if (player->position.y < minY) minY = player->position.y;
        if (onFloor && frame > 10) break;  // landed after meaningful ascent
    }

    Scalar jumpHeight = minY - startY;  // negative (upward)
    Scalar expectedApex = referenceJumpApex(toScalar(JUMP_VEL_CUBE), dt);
    float actualRatio = static_cast<float>(jumpHeight) / static_cast<float>(expectedApex);

    char msg[80];
    snprintf(msg, sizeof(msg), "120fps jump ratio=%.3f (need >=%.3f)",
             static_cast<double>(actualRatio), static_cast<double>(1.0f - TOLERANCE_5PC));
    TEST_ASSERT_TRUE_MESSAGE(actualRatio >= (1.0f - TOLERANCE_5PC), msg);

    snprintf(msg, sizeof(msg), "120fps jump ratio=%.3f (need <=%.3f)",
             static_cast<double>(actualRatio), static_cast<double>(1.0f + TOLERANCE_5PC));
    TEST_ASSERT_TRUE_MESSAGE(actualRatio <= (1.0f + TOLERANCE_5PC), msg);
}

// ---------------------------------------------------------------------------
// Test 2: Jump height at 60 FPS
// ---------------------------------------------------------------------------
void test_jump_height_60fps(void) {
    Scalar dt = toScalar(DT_60FPS);
    setupOnFloor(toScalar(50), dt);
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "60fps: player should be on floor after setup");

    Scalar startY = player->position.y;
    Scalar vy = toScalar(0);
    Scalar minY = player->position.y;
    int maxFrames = 200;

    for (int frame = 0; frame < maxFrames; frame++) {
        bool onFloor = simulatePlayerCubeFrame(vy, frame == 0, dt);
        if (player->position.y < minY) minY = player->position.y;
        if (onFloor && frame > 10) break;
    }

    Scalar jumpHeight = minY - startY;
    Scalar expectedApex = referenceJumpApex(toScalar(JUMP_VEL_CUBE), dt);
    float actualRatio = static_cast<float>(jumpHeight) / static_cast<float>(expectedApex);

    char msg[80];
    snprintf(msg, sizeof(msg), "60fps jump ratio=%.3f (need >=%.3f)",
             static_cast<double>(actualRatio), static_cast<double>(1.0f - TOLERANCE_5PC));
    TEST_ASSERT_TRUE_MESSAGE(actualRatio >= (1.0f - TOLERANCE_5PC), msg);

    snprintf(msg, sizeof(msg), "60fps jump ratio=%.3f (need <=%.3f)",
             static_cast<double>(actualRatio), static_cast<double>(1.0f + TOLERANCE_5PC));
    TEST_ASSERT_TRUE_MESSAGE(actualRatio <= (1.0f + TOLERANCE_5PC), msg);
}

// ---------------------------------------------------------------------------
// Test 3: Jump height at 30 FPS
// ---------------------------------------------------------------------------
void test_jump_height_30fps(void) {
    Scalar dt = toScalar(DT_30FPS);
    setupOnFloor(toScalar(50), dt);
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "30fps: player should be on floor after setup");

    Scalar startY = player->position.y;
    Scalar vy = toScalar(0);
    Scalar minY = player->position.y;
    int maxFrames = 100;

    for (int frame = 0; frame < maxFrames; frame++) {
        bool onFloor = simulatePlayerCubeFrame(vy, frame == 0, dt);
        if (player->position.y < minY) minY = player->position.y;
        if (onFloor && frame > 10) break;
    }

    Scalar jumpHeight = minY - startY;
    Scalar expectedApex = referenceJumpApex(toScalar(JUMP_VEL_CUBE), dt);
    float actualRatio = static_cast<float>(jumpHeight) / static_cast<float>(expectedApex);

    char msg[80];
    snprintf(msg, sizeof(msg), "30fps jump ratio=%.3f (need >=%.3f)",
             static_cast<double>(actualRatio), static_cast<double>(1.0f - TOLERANCE_5PC));
    TEST_ASSERT_TRUE_MESSAGE(actualRatio >= (1.0f - TOLERANCE_5PC), msg);

    snprintf(msg, sizeof(msg), "30fps jump ratio=%.3f (need <=%.3f)",
             static_cast<double>(actualRatio), static_cast<double>(1.0f + TOLERANCE_5PC));
    TEST_ASSERT_TRUE_MESSAGE(actualRatio <= (1.0f + TOLERANCE_5PC), msg);
}

// ---------------------------------------------------------------------------
// Test 4: Free-fall trajectory at 120 FPS
//         Drop player from rest at y=0 with no floor, verify fall matches
//         reference Forward Euler within 2% at 5 sample points.
// ---------------------------------------------------------------------------
void test_free_fall_trajectory_120fps(void) {
    Scalar dt = toScalar(DT_120FPS);
    player->position = Vector2(toScalar(0), toScalar(0));
    Scalar vy = toScalar(0);
    Vector2 up(0, -1);
    Vector2 snap(toScalar(0), toScalar(MIN_SNAP));

    struct Sample { int frames; float t; };
    Sample samples[] = {
        {static_cast<int>(0.10f * 120), 0.10f},
        {static_cast<int>(0.20f * 120), 0.20f},
        {static_cast<int>(0.30f * 120), 0.30f},
        {static_cast<int>(0.40f * 120), 0.40f},
        {static_cast<int>(0.50f * 120), 0.50f}
    };
    int sampleIdx = 0;
    int totalFrames = samples[4].frames;

    for (int frame = 1; frame <= totalFrames; frame++) {
        vy += toScalar(GRAVITY) * dt;
        Vector2 result = player->moveAndSlideWithSnap(
            Vector2(toScalar(0), vy), snap, dt, up
        );
        vy = result.y;

        if (sampleIdx < 5 && frame == samples[sampleIdx].frames) {
            Scalar expectedY = referenceFallPosition(frame, dt);
            float actualY = static_cast<float>(player->position.y);
            float expected = static_cast<float>(expectedY);
            float diffPct = (actualY - expected) / (expected > 0.001f ? expected : 1.0f);
            char msg[64];
            snprintf(msg, sizeof(msg), "120fps t=%.2fs pos=%.2f exp=%.2f diff=%.1f%%",
                     samples[sampleIdx].t, actualY, expected, diffPct * 100.0f);
            TEST_ASSERT_TRUE_MESSAGE(std::fabs(diffPct) <= 0.02f, msg);
            sampleIdx++;
        }
    }
}

// ---------------------------------------------------------------------------
// Test 5: Free-fall trajectory at 60 FPS
// ---------------------------------------------------------------------------
void test_free_fall_trajectory_60fps(void) {
    Scalar dt = toScalar(DT_60FPS);
    player->position = Vector2(toScalar(0), toScalar(0));
    Scalar vy = toScalar(0);
    Vector2 up(0, -1);
    Vector2 snap(toScalar(0), toScalar(MIN_SNAP));

    struct Sample { int frames; float t; };
    Sample samples[] = {
        {static_cast<int>(0.10f * 60), 0.10f},
        {static_cast<int>(0.20f * 60), 0.20f},
        {static_cast<int>(0.30f * 60), 0.30f},
        {static_cast<int>(0.40f * 60), 0.40f},
        {static_cast<int>(0.50f * 60), 0.50f}
    };
    int sampleIdx = 0;
    int totalFrames = samples[4].frames;

    for (int frame = 1; frame <= totalFrames; frame++) {
        vy += toScalar(GRAVITY) * dt;
        Vector2 result = player->moveAndSlideWithSnap(
            Vector2(toScalar(0), vy), snap, dt, up
        );
        vy = result.y;

        if (sampleIdx < 5 && frame == samples[sampleIdx].frames) {
            Scalar expectedY = referenceFallPosition(frame, dt);
            float actualY = static_cast<float>(player->position.y);
            float expected = static_cast<float>(expectedY);
            float diffPct = (actualY - expected) / (expected > 0.001f ? expected : 1.0f);
            char msg[64];
            snprintf(msg, sizeof(msg), "60fps t=%.2fs pos=%.2f exp=%.2f diff=%.1f%%",
                     samples[sampleIdx].t, actualY, expected, diffPct * 100.0f);
            TEST_ASSERT_TRUE_MESSAGE(std::fabs(diffPct) <= 0.02f, msg);
            sampleIdx++;
        }
    }
}

// ---------------------------------------------------------------------------
// Test 6: Landing detection + wasSnapFloor transitions
//         false -> true -> false -> true through jump-land sequence.
// ---------------------------------------------------------------------------
void test_landing_wasSnapFloor_transition(void) {
    Scalar dt = toScalar(DT_60FPS);
    setupOnFloor(toScalar(30), dt);

    // F1: Confirm on floor with snap (wasSnapFloor=true)
    Scalar vy = toScalar(60);
    Vector2 up(0, -1);
    Vector2 snap(toScalar(0), toScalar(MIN_SNAP));
    player->moveAndSlideWithSnap(Vector2(toScalar(0), vy), snap, dt, up);
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "F1: on floor via snap (wasSnapFloor=true)");
    Scalar floorY = player->position.y;

    // F2: Jump with zero snap -> leaves floor
    vy = toScalar(-JUMP_VEL_CUBE);
    player->moveAndSlideWithSnap(Vector2(toScalar(0), vy), Vector2{}, dt, up);
    TEST_ASSERT_FALSE_MESSAGE(player->is_on_floor(), "F2: not on floor after jump");
    TEST_ASSERT_TRUE_MESSAGE(player->position.y < floorY, "F2: above floor after jump");

    // F3-F30+: wasSnapFloor is now false. Snap should NOT re-engage during
    //           flight. The jump-land cycle takes ~70 frames at 60 FPS
    //           (33 up + 37 down for v0=220, g=400).
    int landFrame = -1;
    for (int frame = 3; frame <= 120; frame++) {
        vy += toScalar(GRAVITY) * dt;
        player->moveAndSlideWithSnap(Vector2(toScalar(0), vy), snap, dt, up);
        if (player->is_on_floor()) {
            // Landed! Verify landing is NOT too early (would indicate
            // wasSnapFloor guard failing and snap re-engaging mid-flight).
            // Earliest natural landing: 33 ascent + ~30 descent = ~63 frames
            TEST_ASSERT_TRUE_MESSAGE(frame >= 55,
                "Should NOT snap-land early (wasSnapFloor guard failed)");
            landFrame = frame;
            break;
        }
    }

    // Verify we did eventually land
    TEST_ASSERT_TRUE_MESSAGE(landFrame > 0, "Should land back on floor naturally");
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Should be on floor after landing");

    // F(n+1): After landing, snap can re-engage (wasSnapFloor=true again)
    vy = toScalar(60);
    player->moveAndSlideWithSnap(Vector2(toScalar(0), vy), snap, dt, up);
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "After landing: on floor via snap re-engage");
}

// ---------------------------------------------------------------------------
// Test 7: PlayerCube calling pattern
// ---------------------------------------------------------------------------
void test_player_cube_pattern_jump(void) {
    Scalar dt = toScalar(DT_60FPS);
    setupOnFloor(toScalar(50), dt);
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Cube: on floor after setup");

    // Establish wasSnapFloor
    Scalar vy = toScalar(0);
    simulatePlayerCubeFrame(vy, false, dt);
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Cube: on floor after snap setup");

    Scalar startY = player->position.y;
    bool jumped = false;

    for (int frame = 0; frame < 200; frame++) {
        bool onFloor = simulatePlayerCubeFrame(vy, frame == 0, dt);
        if (frame == 0 && !player->is_on_floor()) jumped = true;
        if (onFloor && frame > 10) break;
    }

    TEST_ASSERT_TRUE_MESSAGE(jumped, "Cube: player should have jumped");
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Cube: player should have landed");
    TEST_ASSERT_TRUE_MESSAGE(std::fabs(vy) < 1.0f,
        "Cube: final velocity should be near-zero after landing");
}

// ---------------------------------------------------------------------------
// Test 8: PlayerActor calling pattern with state machine
// ---------------------------------------------------------------------------
void test_player_actor_pattern_jump(void) {
    Scalar dt = toScalar(DT_60FPS);
    setupOnFloor(toScalar(50), dt);
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Actor: on floor after setup");

    Scalar vy = toScalar(0);
    TestState state = TestState::IDLE;
    simulatePlayerActorFrame(vy, false, state, dt);
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Actor: on floor after snap setup");

    bool jumped = false;
    for (int frame = 0; frame < 200; frame++) {
        bool onFloor = simulatePlayerActorFrame(vy, frame == 0, state, dt);
        if (frame == 0 && !player->is_on_floor()) jumped = true;
        if (onFloor && frame > 10) break;
    }

    TEST_ASSERT_TRUE_MESSAGE(jumped, "Actor: player should have jumped");
    TEST_ASSERT_TRUE_MESSAGE(player->is_on_floor(), "Actor: player should have landed");
    TEST_ASSERT_EQUAL_MESSAGE(static_cast<int>(TestState::IDLE), static_cast<int>(state),
        "Actor: final state should be IDLE after landing");
}

// =============================================================================
// Main
// =============================================================================
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    // Jump height at different frame rates
    RUN_TEST(test_jump_height_120fps);
    RUN_TEST(test_jump_height_60fps);
    RUN_TEST(test_jump_height_30fps);

    // Free-fall trajectory
    RUN_TEST(test_free_fall_trajectory_120fps);
    RUN_TEST(test_free_fall_trajectory_60fps);

    // Landing and wasSnapFloor transitions
    RUN_TEST(test_landing_wasSnapFloor_transition);

    // Caller patterns
    RUN_TEST(test_player_cube_pattern_jump);
    RUN_TEST(test_player_actor_pattern_jump);

    return UNITY_END();
}
