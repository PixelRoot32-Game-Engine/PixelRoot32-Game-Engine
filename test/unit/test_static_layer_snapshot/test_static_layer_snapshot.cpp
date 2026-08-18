/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * StaticLayerSnapshot: the projection-agnostic counterpart to
 * StaticTilemapLayerCache. It caches what the game DREW rather than a
 * TileMap4bpp it redraws itself, which is what lets an isometric room -- whose
 * floor cannot be expressed as an axis-aligned tilemap -- stop redrawing its
 * static layer every frame.
 *
 * The cases below concentrate on the two things that make it either correct or
 * silently wrong: capture/restore round-tripping the exact bytes, and the
 * interaction with whatever `Renderer::beginFrame` did to the framebuffer
 * first. A selective per-cell restore is only sound when the clear left the
 * pixels outside those cells alone; when beginFrame wiped everything, the
 * snapshot must fall back to a full copy or the static layer comes back black.
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/StaticLayerSnapshot.h"
#include "graphics/Renderer.h"
#include "graphics/DisplayConfig.h"
#include "../../mocks/MockDrawSurface.h"

#include <cstdint>
#include <cstring>
#include <memory>

using namespace pixelroot32::graphics;

namespace {

constexpr int kFbWidth = 32;
constexpr int kFbHeight = 32;
constexpr std::size_t kFbBytes = static_cast<std::size_t>(kFbWidth) * kFbHeight;

uint8_t gFrameBuffer[kFbBytes];

/// Owns a Renderer whose draw surface exposes gFrameBuffer as its 8bpp buffer.
struct Harness {
    std::unique_ptr<Renderer> renderer;
    MockDrawSurface* mock = nullptr;

    explicit Harness(bool exposeSpriteBuffer = true) {
        auto owned = std::make_unique<MockDrawSurface>();
        mock = owned.get();
        mock->setSpriteBuffer(gFrameBuffer, sizeof(gFrameBuffer));
        mock->enableSpriteBuffer(exposeSpriteBuffer);

        DisplayConfig config =
            PIXELROOT32_CUSTOM_DISPLAY(owned.release(), kFbWidth, kFbHeight);
        renderer = std::make_unique<Renderer>(std::move(config));
        renderer->init();
    }
};

/// Distinct value per pixel, so an offset error cannot pass as a match.
void fillPositional(uint8_t* buf, uint8_t bias) {
    for (std::size_t i = 0; i < kFbBytes; ++i) {
        buf[i] = static_cast<uint8_t>((i + bias) & 0xFF);
    }
}

}  // namespace

void setUp(void) {
    std::memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
}

void tearDown(void) {}

#if PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT

/// Without a buffer nothing is claimed to work: restore() reports cold so the
/// caller draws its static layers normally.
void test_unallocated_snapshot_reports_cold(void) {
    Harness h;
    StaticLayerSnapshot snapshot;

    TEST_ASSERT_FALSE(snapshot.isValid());
    TEST_ASSERT_FALSE(snapshot.restore(*h.renderer));
    TEST_ASSERT_FALSE(snapshot.capture(*h.renderer));
}

void test_allocate_for_renderer_reports_success(void) {
    Harness h;
    StaticLayerSnapshot snapshot;

    TEST_ASSERT_TRUE(snapshot.allocateForRenderer(*h.renderer));
    // Allocated but never captured: still nothing to hand back.
    TEST_ASSERT_FALSE(snapshot.isValid());
    TEST_ASSERT_FALSE(snapshot.restore(*h.renderer));
}

void test_allocate_rejects_non_positive_dimensions(void) {
    StaticLayerSnapshot snapshot;
    TEST_ASSERT_FALSE(snapshot.allocateForLogicalSize(0, 16));
    TEST_ASSERT_FALSE(snapshot.allocateForLogicalSize(16, -1));
}

/// A driver with no 8bpp framebuffer (SDL2/native) must degrade, not crash.
void test_capture_without_sprite_buffer_reports_unavailable(void) {
    Harness h(/*exposeSpriteBuffer=*/false);
    StaticLayerSnapshot snapshot;

    TEST_ASSERT_TRUE(snapshot.allocateForLogicalSize(kFbWidth, kFbHeight));
    TEST_ASSERT_FALSE(snapshot.capture(*h.renderer));
    TEST_ASSERT_FALSE(snapshot.isValid());
}

/// The core contract: what was on the framebuffer at capture() comes back.
void test_capture_then_full_restore_round_trips(void) {
    Harness h;
    StaticLayerSnapshot snapshot;
    TEST_ASSERT_TRUE(snapshot.allocateForRenderer(*h.renderer));

    fillPositional(gFrameBuffer, 0);
    TEST_ASSERT_TRUE(snapshot.capture(*h.renderer));
    TEST_ASSERT_TRUE(snapshot.isValid());

    // Something dynamic scribbles over the whole buffer.
    std::memset(gFrameBuffer, 0xEE, sizeof(gFrameBuffer));

    TEST_ASSERT_TRUE(snapshot.restore(*h.renderer));
    for (std::size_t i = 0; i < kFbBytes; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(static_cast<uint8_t>(i & 0xFF), gFrameBuffer[i],
            "restore() must reproduce the captured framebuffer byte for byte");
    }
}

/// invalidate() is how a game says "my static layers changed"; the next
/// restore() has to report cold so the caller redraws and re-captures.
void test_invalidate_forces_a_cold_restore(void) {
    Harness h;
    StaticLayerSnapshot snapshot;
    TEST_ASSERT_TRUE(snapshot.allocateForRenderer(*h.renderer));

    fillPositional(gFrameBuffer, 0);
    TEST_ASSERT_TRUE(snapshot.capture(*h.renderer));

    snapshot.invalidate();

    TEST_ASSERT_FALSE(snapshot.isValid());
    TEST_ASSERT_FALSE(snapshot.restore(*h.renderer));
}

/// clear() releases the buffer, so the snapshot degrades to "unavailable"
/// rather than merely "cold".
void test_clear_releases_the_buffer(void) {
    Harness h;
    StaticLayerSnapshot snapshot;
    TEST_ASSERT_TRUE(snapshot.allocateForRenderer(*h.renderer));
    fillPositional(gFrameBuffer, 0);
    TEST_ASSERT_TRUE(snapshot.capture(*h.renderer));

    snapshot.clear();

    TEST_ASSERT_FALSE(snapshot.isValid());
    TEST_ASSERT_FALSE(snapshot.restore(*h.renderer));
    TEST_ASSERT_FALSE(snapshot.capture(*h.renderer));
}

/// Re-allocating the same size keeps the buffer but drops the contents, so a
/// stale static layer can never survive a re-init.
void test_reallocating_same_size_invalidates_contents(void) {
    Harness h;
    StaticLayerSnapshot snapshot;
    TEST_ASSERT_TRUE(snapshot.allocateForRenderer(*h.renderer));
    fillPositional(gFrameBuffer, 0);
    TEST_ASSERT_TRUE(snapshot.capture(*h.renderer));
    TEST_ASSERT_TRUE(snapshot.isValid());

    TEST_ASSERT_TRUE(snapshot.allocateForRenderer(*h.renderer));
    TEST_ASSERT_FALSE(snapshot.isValid());
}

/**
 * @brief After a full-buffer clear, restore() must repaint EVERYTHING.
 *
 * This is the case that made a per-cell restore unsafe on its own.
 * `beginFrame()` wipes the whole framebuffer whenever the grid is fullDirty or
 * nothing moved last frame. A selective restore would then repaint only the
 * previously-dirty cells and leave the rest of the static layer black — a
 * mostly-blank room, which is exactly the kind of bug that looks like an art
 * problem. Renderer reports whether its clear was selective, and the snapshot
 * falls back to a full copy when it was not.
 */
void test_restore_after_full_clear_repaints_whole_framebuffer(void) {
    Harness h;
    StaticLayerSnapshot snapshot;
    TEST_ASSERT_TRUE(snapshot.allocateForRenderer(*h.renderer));

    fillPositional(gFrameBuffer, 0);
    TEST_ASSERT_TRUE(snapshot.capture(*h.renderer));

    // beginFrame() with nothing marked takes the full-clear branch.
    h.renderer->beginFrame();
    std::memset(gFrameBuffer, 0x00, sizeof(gFrameBuffer));

    TEST_ASSERT_TRUE(snapshot.restore(*h.renderer));
    for (std::size_t i = 0; i < kFbBytes; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(static_cast<uint8_t>(i & 0xFF), gFrameBuffer[i],
            "A full clear must be followed by a full restore, not a per-cell one");
    }
}

/// The advice is an optimisation hint, never a correctness lever: a scene that
/// forgets it still renders correctly, so this only pins that it is callable
/// in both states without disturbing the snapshot.
void test_advise_is_safe_in_both_states(void) {
    Harness h;
    StaticLayerSnapshot snapshot;
    TEST_ASSERT_TRUE(snapshot.allocateForRenderer(*h.renderer));

    snapshot.adviseFramebufferBeforeBeginFrame(*h.renderer);  // cold
    TEST_ASSERT_FALSE(snapshot.isValid());

    fillPositional(gFrameBuffer, 0);
    TEST_ASSERT_TRUE(snapshot.capture(*h.renderer));
    snapshot.adviseFramebufferBeforeBeginFrame(*h.renderer);  // warm
    TEST_ASSERT_TRUE(snapshot.isValid());
}

#else

void test_snapshot_disabled_degrades_to_no_cache(void) {
    Harness h;
    StaticLayerSnapshot snapshot;

    // Every entry point reports "unavailable" so callers draw normally.
    TEST_ASSERT_FALSE(snapshot.allocateForRenderer(*h.renderer));
    TEST_ASSERT_FALSE(snapshot.isValid());
    TEST_ASSERT_FALSE(snapshot.capture(*h.renderer));
    TEST_ASSERT_FALSE(snapshot.restore(*h.renderer));
    snapshot.adviseFramebufferBeforeBeginFrame(*h.renderer);
    snapshot.invalidate();
    snapshot.clear();
}

#endif  // PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT
    RUN_TEST(test_unallocated_snapshot_reports_cold);
    RUN_TEST(test_allocate_for_renderer_reports_success);
    RUN_TEST(test_allocate_rejects_non_positive_dimensions);
    RUN_TEST(test_capture_without_sprite_buffer_reports_unavailable);
    RUN_TEST(test_capture_then_full_restore_round_trips);
    RUN_TEST(test_invalidate_forces_a_cold_restore);
    RUN_TEST(test_clear_releases_the_buffer);
    RUN_TEST(test_reallocating_same_size_invalidates_contents);
    RUN_TEST(test_restore_after_full_clear_repaints_whole_framebuffer);
    RUN_TEST(test_advise_is_safe_in_both_states);
#else
    RUN_TEST(test_snapshot_disabled_degrades_to_no_cache);
#endif

    return UNITY_END();
}
