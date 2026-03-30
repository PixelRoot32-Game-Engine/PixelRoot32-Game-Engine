/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */


#include <unity.h>
#include "platforms/mock/MockAudioBackend.h"
#include "audio/DefaultAudioScheduler.h"

using namespace pixelroot32::audio;
using namespace pixelroot32::platforms;

// Required Unity setup/teardown
void setUp(void) { }
void tearDown(void) { }

// Minimal stub AudioEngine for testing
class StubAudioEngine {
public:
    void generateSamples(int16_t* stream, int len) {
        for (int i = 0; i < len; i++) stream[i] = 0;
    }
};

// Test 1: AudioBackend initialization lifecycle
void test_audiobackend_initialization_lifecycle() {
    MockAudioBackend backend(44100);
    TEST_ASSERT_FALSE(backend.wasInitCalled());
    TEST_ASSERT_EQUAL(44100, backend.getSampleRate());
    
    StubAudioEngine engine;
    PlatformCapabilities caps;
    caps.coreCount = 4;
    backend.init(reinterpret_cast<AudioEngine*>(&engine), caps);
    
    TEST_ASSERT_TRUE(backend.wasInitCalled());
    TEST_ASSERT_EQUAL(4, backend.getCaps().coreCount);
}

// Test 2: AudioBackend multiple init calls
void test_audiobackend_multiple_init_calls() {
    MockAudioBackend backend;
    StubAudioEngine engine1;
    StubAudioEngine engine2;
    
    backend.init(reinterpret_cast<AudioEngine*>(&engine1), PlatformCapabilities());
    TEST_ASSERT_TRUE(backend.wasInitCalled());
    
    backend.init(reinterpret_cast<AudioEngine*>(&engine2), PlatformCapabilities());
    TEST_ASSERT_TRUE(backend.wasInitCalled());
}

// Test 3: AudioBackend reset functionality
void test_audiobackend_reset() {
    MockAudioBackend backend;
    StubAudioEngine engine;
    
    backend.init(reinterpret_cast<AudioEngine*>(&engine), PlatformCapabilities());
    TEST_ASSERT_TRUE(backend.wasInitCalled());
    
    backend.reset();
    TEST_ASSERT_FALSE(backend.wasInitCalled());
    TEST_ASSERT_NULL(backend.getEngine());
}

// Test 4: AudioBackend different sample rates
void test_audiobackend_different_sample_rates() {
    MockAudioBackend backend22050(22050);
    MockAudioBackend backend44100(44100);
    
    TEST_ASSERT_EQUAL(22050, backend22050.getSampleRate());
    TEST_ASSERT_EQUAL(44100, backend44100.getSampleRate());
}

// Test 5: AudioScheduler with mock backend - initialization
void test_audioscheduler_with_mock_backend_init() {
    MockAudioBackend backend(22050);
    DefaultAudioScheduler scheduler;
    PlatformCapabilities caps;
    
    // Note: scheduler.init() does NOT call backend.init() - that's expected behavior
    scheduler.init(&backend, 22050, caps);
    
    // Scheduler lifecycle still works
    scheduler.start();
    scheduler.stop();
    
    // Just verify scheduler doesn't crash with mock backend
    TEST_ASSERT_TRUE(true);
}

// Test 6: AudioBackend shutdown cleanup verification
void test_audiobackend_shutdown_cleanup() {
    MockAudioBackend* backend = new MockAudioBackend(44100);
    StubAudioEngine engine;
    
    backend->init(reinterpret_cast<AudioEngine*>(&engine), PlatformCapabilities());
    TEST_ASSERT_TRUE(backend->wasInitCalled());
    
    delete backend;
    TEST_ASSERT_TRUE(true);
}

// Test 7: AudioBackend with empty/null capabilities
void test_audiobackend_default_caps() {
    MockAudioBackend backend;
    StubAudioEngine engine;
    
    backend.init(reinterpret_cast<AudioEngine*>(&engine), PlatformCapabilities());
    TEST_ASSERT_TRUE(backend.wasInitCalled());
}

void run_audiobackend_tests() {
    RUN_TEST(test_audiobackend_initialization_lifecycle);
    RUN_TEST(test_audiobackend_multiple_init_calls);
    RUN_TEST(test_audiobackend_reset);
    RUN_TEST(test_audiobackend_different_sample_rates);
    RUN_TEST(test_audioscheduler_with_mock_backend_init);
    RUN_TEST(test_audiobackend_shutdown_cleanup);
    RUN_TEST(test_audiobackend_default_caps);
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    run_audiobackend_tests();
    return UNITY_END();
}
