/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Compile-time contract for the optional persistence module. The module is
 * default-off, so this suite must pass under both [env:native_test] (flag off,
 * the contract every existing game relies on) and [env:native_test_persistence]
 * (flag on, where the same assertions prove the mirror actually tracks the
 * build flag instead of being hard-coded).
 */
#include <unity.h>
#include "platforms/EngineConfig.h"
#include "core/EngineModules.h"

namespace config = pixelroot32::platforms::config;
namespace modules = pixelroot32::modules;

void setUp(void) {}
void tearDown(void) {}

void test_persistence_flag_matches_build(void) {
#if PIXELROOT32_ENABLE_PERSISTENCE
    TEST_ASSERT_TRUE(config::EnablePersistence);
    TEST_ASSERT_TRUE(modules::Persistence);
#else
    TEST_ASSERT_FALSE(config::EnablePersistence);
    TEST_ASSERT_FALSE(modules::Persistence);
#endif
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_persistence_flag_matches_build);
    return UNITY_END();
}
