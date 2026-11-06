/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Renderer::drawSpriteInternal(Sprite2bpp) must read exactly the bytes of the
 * sprite and decode every pixel the way the compiler's pack_2bpp lays it out.
 *
 * Layout: rows are (width * 2 + 7) / 8 bytes apart; pixel `col` lives in byte
 * `col >> 2` at shift `(col & 3) * 2` (LSB = leftmost pixel).
 *
 * The blit used to read each row as 16-bit words through a reinterpret_cast.
 * With width % 8 in 1..4 the last row's final word reached one byte past the
 * sprite data, and rows of an odd stride started at odd addresses (unaligned
 * 16-bit loads). The over-read never changed a pixel (the extra byte only
 * holds columns >= width), so it cannot be seen in the framebuffer. These
 * tests make it observable instead: every sprite's data ends exactly at the
 * start of a no-access guard page, so a read past the end faults and the
 * suite fails.
 */

// The platform headers come first: the engine's mock Arduino layer defines
// macros (INPUT, ...) that collide with <windows.h> declarations.
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include "graphics/DisplayConfig.h"
#include "../../mocks/MockDrawSurface.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>

using namespace pixelroot32::graphics;

namespace {

constexpr int kFbWidth = 48;
constexpr int kFbHeight = 16;
constexpr uint8_t kSentinel = 0xAAu;
constexpr int kOriginX = 5;
constexpr int kOriginY = 3;

uint8_t gFrameBuffer[kFbWidth * kFbHeight];

const uint16_t kPalette[16] = {
    0x0000u, 0xF800u, 0x07E0u, 0x001Fu,
    0xFFE0u, 0xF81Fu, 0x07FFu, 0xFFFFu,
    0x8410u, 0xC618u, 0x7BEFu, 0x39E7u,
    0xFC00u, 0x03E0u, 0x0018u, 0xE01Fu,
};

const Color kIdentityColors2bpp[4] = {
    static_cast<Color>(0), static_cast<Color>(1),
    static_cast<Color>(2), static_cast<Color>(3),
};

/// Two pages: the first holds sprite data, the second is no-access. Data is
/// placed so its last byte is the last byte of the first page.
class GuardedBuffer {
public:
    GuardedBuffer() {
#if defined(_WIN32)
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        pageSize_ = static_cast<size_t>(info.dwPageSize);
        base_ = static_cast<uint8_t*>(VirtualAlloc(nullptr, pageSize_ * 2,
                                                   MEM_RESERVE | MEM_COMMIT,
                                                   PAGE_READWRITE));
        DWORD old = 0;
        guarded_ = base_ != nullptr &&
                   VirtualProtect(base_ + pageSize_, pageSize_, PAGE_NOACCESS, &old) != 0;
#else
        pageSize_ = static_cast<size_t>(sysconf(_SC_PAGESIZE));
        void* p = mmap(nullptr, pageSize_ * 2, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        base_ = (p == MAP_FAILED) ? nullptr : static_cast<uint8_t*>(p);
        guarded_ = base_ != nullptr &&
                   mprotect(base_ + pageSize_, pageSize_, PROT_NONE) == 0;
#endif
    }

    ~GuardedBuffer() {
        if (base_ == nullptr) return;
#if defined(_WIN32)
        VirtualFree(base_, 0, MEM_RELEASE);
#else
        munmap(base_, pageSize_ * 2);
#endif
    }

    GuardedBuffer(const GuardedBuffer&) = delete;
    GuardedBuffer& operator=(const GuardedBuffer&) = delete;

    bool ok() const { return guarded_; }

    /// Returns a zeroed region of `size` bytes ending at the guard page.
    uint8_t* tail(size_t size) {
        uint8_t* p = base_ + pageSize_ - size;
        std::memset(p, 0, size);
        return p;
    }

private:
    uint8_t* base_ = nullptr;
    size_t pageSize_ = 0;
    bool guarded_ = false;
};

constexpr int rowBytesFor(int width) { return (width * 2 + 7) / 8; }

/// Deterministic pixel pattern covering all four values, with transparent
/// pixels sprinkled in, and differing per row and per width.
uint8_t patternPixel(int width, int row, int col) {
    const uint32_t h = static_cast<uint32_t>(width * 131 + row * 17 + col * 7 + 3);
    return static_cast<uint8_t>((h ^ (h >> 3)) & 0x03u);
}

/// Packs the pattern into `data` with the compiler's pack_2bpp layout.
void packPattern(uint8_t* data, int width, int height) {
    const int rowBytes = rowBytesFor(width);
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const uint8_t v = patternPixel(width, row, col);
            data[row * rowBytes + (col >> 2)] |= static_cast<uint8_t>(v << ((col & 3) * 2));
        }
    }
}

/// Reference decoder: one pixel, byte-wise, straight from the layout.
uint8_t referencePixel(const uint8_t* data, int width, int row, int col) {
    const uint8_t byte = data[row * rowBytesFor(width) + (col >> 2)];
    return static_cast<uint8_t>((byte >> ((col & 3) * 2)) & 0x03u);
}

/// Same narrowing as the renderer's framebuffer write (TFT_eSprite 8bpp).
uint8_t packExpected(uint16_t rgb565) {
    return static_cast<uint8_t>(((rgb565 & 0xE000) >> 8) |
                                ((rgb565 & 0x0700) >> 6) |
                                ((rgb565 & 0x0018) >> 3));
}

struct Harness {
    std::unique_ptr<Renderer> renderer;

    Harness() {
        auto mock = std::make_unique<MockDrawSurface>();
        MockDrawSurface* mockRaw = mock.get();
        mockRaw->setSpriteBuffer(gFrameBuffer, sizeof(gFrameBuffer));

        DisplayConfig config =
            PIXELROOT32_CUSTOM_DISPLAY(mock.release(), kFbWidth, kFbHeight);
        renderer = std::make_unique<Renderer>(std::move(config));
        renderer->init();
        renderer->beginFrame();
        std::memset(gFrameBuffer, kSentinel, sizeof(gFrameBuffer));
    }
};

/// Draws a width x height sprite whose data ends at the guard page and checks
/// every framebuffer byte against the reference decoder.
void checkWidth(GuardedBuffer& guard, int width, int height, bool flipX) {
    const size_t size = static_cast<size_t>(rowBytesFor(width) * height);
    uint8_t* data = guard.tail(size);
    packPattern(data, width, height);
    Sprite2bpp sprite{data, kIdentityColors2bpp,
                      static_cast<uint8_t>(width), static_cast<uint8_t>(height), 4};

    Harness h;
    h.renderer->drawSprite(sprite, kOriginX, kOriginY, flipX);

    uint8_t expectedColor[4];
    for (int v = 0; v < 4; ++v) {
        expectedColor[v] = packExpected(resolveColor(static_cast<Color>(v), PaletteContext::Sprite));
    }

    int opaque = 0;
    for (int y = 0; y < kFbHeight; ++y) {
        for (int x = 0; x < kFbWidth; ++x) {
            uint8_t expected = kSentinel;
            const int row = y - kOriginY;
            const int dx = x - kOriginX;
            if (row >= 0 && row < height && dx >= 0 && dx < width) {
                const int col = flipX ? (width - 1 - dx) : dx;
                const uint8_t v = referencePixel(data, width, row, col);
                if (v != 0) {
                    expected = expectedColor[v];
                    ++opaque;
                }
            }
            char msg[64];
            std::snprintf(msg, sizeof(msg), "width=%d flipX=%d x=%d y=%d",
                          width, flipX ? 1 : 0, x, y);
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected, gFrameBuffer[y * kFbWidth + x], msg);
        }
    }
    // Guard against a vacuous comparison of an untouched framebuffer.
    TEST_ASSERT_TRUE_MESSAGE(opaque > 0, "pattern drew no opaque pixel");
}

}  // namespace

void setUp(void) {
    setDualCustomPalette(kPalette, kPalette);
}

void tearDown(void) {}

/// A 12-wide sprite (width % 8 == 4) whose last row ends at a no-access page:
/// a word-wide read of the last row's second half would fault here.
void test_2bpp_blit_reads_no_byte_past_sprite_data(void) {
    GuardedBuffer guard;
    TEST_ASSERT_TRUE_MESSAGE(guard.ok(), "guard page unavailable");
    checkWidth(guard, 12, 3, false);
    checkWidth(guard, 12, 3, true);
}

/// Widths 1..17 (every width % 8 and the 16-pixel two-word boundary), flipX
/// off: the blit writes exactly what the reference decoder reads.
void test_2bpp_blit_matches_reference_decoder(void) {
    GuardedBuffer guard;
    TEST_ASSERT_TRUE_MESSAGE(guard.ok(), "guard page unavailable");
    for (int width = 1; width <= 17; ++width) {
        checkWidth(guard, width, 3, false);
    }
}

/// Same, mirrored.
void test_2bpp_blit_matches_reference_decoder_flipped(void) {
    GuardedBuffer guard;
    TEST_ASSERT_TRUE_MESSAGE(guard.ok(), "guard page unavailable");
    for (int width = 1; width <= 17; ++width) {
        checkWidth(guard, width, 3, true);
    }
}

int runUnityTests() {
    UNITY_BEGIN();
#if defined(PIXELROOT32_ENABLE_2BPP_SPRITES)
    RUN_TEST(test_2bpp_blit_reads_no_byte_past_sprite_data);
    RUN_TEST(test_2bpp_blit_matches_reference_decoder);
    RUN_TEST(test_2bpp_blit_matches_reference_decoder_flipped);
#endif
    return UNITY_END();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return runUnityTests();
}
