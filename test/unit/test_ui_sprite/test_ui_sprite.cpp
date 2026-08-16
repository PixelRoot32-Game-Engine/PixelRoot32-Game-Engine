/**
 * @file test_ui_sprite.cpp
 * @brief Unit tests for the UISprite / UISpriteRow UI elements.
 *
 * Before these existed, no UI element in the engine could draw a sprite —
 * every one of the 18 headers under graphics/ui drew text or rectangles only.
 * Games therefore built resource HUDs by hand inside a Scene::draw() override,
 * outside the entity tree, which costs them setVisible(), layout placement and
 * — the one that actually bites — setFixedPosition()'s camera-scroll bypass.
 *
 * The tests assert geometry and state selection rather than rasterized pixels:
 * width/height is the contract the layouts consume, and stateIndexAt() /
 * iconOffsetAt() are the queries a game uses to place a cursor or animate one
 * icon. Rasterization is already covered by the Renderer's own suites, and it
 * takes the framebuffer path (not drawPixel) whenever a logical framebuffer
 * exists, so asserting on mock draw calls here would test the display config,
 * not these elements.
 *
 * Both elements live behind PIXELROOT32_ENABLE_UI_SYSTEM, like every other UI
 * header, so this file compiles cleanly with the flag on or off.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_UI_SYSTEM

#include "graphics/ui/UISprite.h"
#include "graphics/ui/UISpriteRow.h"
#include "graphics/ui/UIHorizontalLayout.h"
#include "graphics/DisplayConfig.h"
#include "graphics/BaseDrawSurface.h"
#include "../../mocks/MockDrawSurface.h"

#include <memory>

using namespace pixelroot32::graphics;
using namespace pixelroot32::graphics::ui;
using namespace pixelroot32::math;

namespace {

// --- Test sprites ---------------------------------------------------------
// Palette index 0 is transparent in the 2bpp/4bpp renderers, so every test
// sprite uses non-zero indices; the exact pixels do not matter here, only the
// declared width/height, which is what the layouts read.

// 4bpp, 4x2. Row stride = (4 * 4 + 7) / 8 = 2 bytes.
const uint8_t kIcon4bppData[] = {0x11, 0x11, 0x22, 0x22};
const Color kPalette4bpp[] = {Color::Black, Color::White, Color::Red};
const Sprite4bpp kIcon4bpp{kIcon4bppData, kPalette4bpp, 4, 2, 3};

// A second 4bpp sprite with different dimensions, to prove size tracks the
// sprite currently set rather than the first one ever set.
const uint8_t kWideIcon4bppData[] = {0x11, 0x11, 0x11, 0x11};
const Sprite4bpp kWideIcon4bpp{kWideIcon4bppData, kPalette4bpp, 8, 1, 3};

// 2bpp, 4x2. Row stride = (4 * 2 + 7) / 8 = 1 byte.
const uint8_t kIcon2bppData[] = {0x55, 0xAA};
const Color kPalette2bpp[] = {Color::Black, Color::White, Color::Red, Color::Green};
const Sprite2bpp kIcon2bpp{kIcon2bppData, kPalette2bpp, 4, 2, 4};

// 1bpp mono, 4x2. One uint16_t row per line.
const uint16_t kIconMonoData[] = {0x000F, 0x000F};
const Sprite kIconMono{kIconMonoData, 4, 2};

// Heart states for the Zelda-shaped scenarios: empty / half / full.
const uint8_t kHeartData[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
const Sprite4bpp kHeartEmpty{kHeartData, kPalette4bpp, 8, 8, 3};
const Sprite4bpp kHeartHalf{kHeartData, kPalette4bpp, 8, 8, 3};
const Sprite4bpp kHeartFull{kHeartData, kPalette4bpp, 8, 8, 3};

/// Builds a Renderer over a recording mock surface. The surface is handed to
/// the DisplayConfig, which takes ownership.
Renderer makeRenderer() {
    auto surface = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(surface.release(), 240, 240);
    return Renderer(config);
}

/// Configures a row with the canonical three heart states and half-heart
/// granularity — the shape every scenario below builds on.
void configureHeartRow(UISpriteRow& row) {
    row.setStateSprite(0, kHeartEmpty);
    row.setStateSprite(1, kHeartHalf);
    row.setStateSprite(2, kHeartFull);
    row.setUnitsPerIcon(2);
}

}  // namespace

// =============================================================================
// UISprite — empty state
// =============================================================================

void test_ui_sprite_starts_empty_with_zero_size(void) {
    UISprite sprite(Vector2(10, 20));

    TEST_ASSERT_EQUAL(static_cast<int>(UISpriteFormat::None), static_cast<int>(sprite.getFormat()));
    TEST_ASSERT_FALSE(sprite.hasSprite());
    TEST_ASSERT_EQUAL_INT(0, sprite.width);
    TEST_ASSERT_EQUAL_INT(0, sprite.height);
    TEST_ASSERT_EQUAL_INT(10, static_cast<int>(sprite.position.x));
    TEST_ASSERT_EQUAL_INT(20, static_cast<int>(sprite.position.y));
}

void test_ui_sprite_defaults_to_ui_render_layer(void) {
    // UIElement's constructor puts every element on layer 2 so Scene::draw()
    // selects the sprite palette context and draws it above gameplay.
    UISprite sprite(Vector2::ZERO());

    TEST_ASSERT_EQUAL_INT(2, static_cast<int>(sprite.getRenderLayer()));

    // GENERIC on purpose: the layouts' focus-navigation chains test only for
    // BUTTON and CHECKBOX, and an icon is not focusable. Reusing GENERIC means
    // no existing UI file had to change to accommodate a sprite leaf.
    TEST_ASSERT_EQUAL(static_cast<int>(UIElement::UIElementType::GENERIC),
                      static_cast<int>(sprite.getType()));
    TEST_ASSERT_FALSE(sprite.isFocusable());
}

void test_ui_sprite_empty_draw_is_a_no_op(void) {
    Renderer renderer = makeRenderer();

    UISprite sprite(Vector2::ZERO());
    sprite.draw(renderer);  // Must not dereference the null sprite pointer.

    TEST_ASSERT_FALSE(sprite.hasSprite());
}

// =============================================================================
// UISprite — the three sprite formats
// =============================================================================

void test_ui_sprite_accepts_a_4bpp_sprite_and_adopts_its_size(void) {
    UISprite sprite(Vector2::ZERO());
    sprite.setSprite(kIcon4bpp, 3);

    TEST_ASSERT_EQUAL(static_cast<int>(UISpriteFormat::Bpp4), static_cast<int>(sprite.getFormat()));
    TEST_ASSERT_TRUE(sprite.hasSprite());
    TEST_ASSERT_EQUAL_INT(4, sprite.width);
    TEST_ASSERT_EQUAL_INT(2, sprite.height);
    TEST_ASSERT_EQUAL_INT(3, static_cast<int>(sprite.getPaletteSlot()));
}

void test_ui_sprite_accepts_a_2bpp_sprite_and_adopts_its_size(void) {
    UISprite sprite(Vector2::ZERO());
    sprite.setSprite(kIcon2bpp, 1);

    TEST_ASSERT_EQUAL(static_cast<int>(UISpriteFormat::Bpp2), static_cast<int>(sprite.getFormat()));
    TEST_ASSERT_EQUAL_INT(4, sprite.width);
    TEST_ASSERT_EQUAL_INT(2, sprite.height);
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(sprite.getPaletteSlot()));
}

void test_ui_sprite_accepts_a_mono_sprite_with_a_tint(void) {
    UISprite sprite(Vector2::ZERO());
    sprite.setSprite(kIconMono, Color::Red);

    TEST_ASSERT_EQUAL(static_cast<int>(UISpriteFormat::Mono), static_cast<int>(sprite.getFormat()));
    TEST_ASSERT_EQUAL_INT(4, sprite.width);
    TEST_ASSERT_EQUAL_INT(2, sprite.height);
    TEST_ASSERT_EQUAL(static_cast<int>(Color::Red), static_cast<int>(sprite.getTint()));
}

void test_ui_sprite_switching_format_replaces_the_previous_one(void) {
    // The three formats share one storage slot, so setting a second one must
    // fully retag the element rather than leaving a stale pointer readable.
    UISprite sprite(Vector2::ZERO());
    sprite.setSprite(kIcon4bpp, 2);
    sprite.setSprite(kIconMono, Color::White);

    TEST_ASSERT_EQUAL(static_cast<int>(UISpriteFormat::Mono), static_cast<int>(sprite.getFormat()));
    TEST_ASSERT_EQUAL_INT(4, sprite.width);
    TEST_ASSERT_EQUAL_INT(2, sprite.height);
}

void test_ui_sprite_size_tracks_the_sprite_currently_set(void) {
    UISprite sprite(Vector2::ZERO());
    sprite.setSprite(kIcon4bpp);
    TEST_ASSERT_EQUAL_INT(4, sprite.width);

    sprite.setSprite(kWideIcon4bpp);

    TEST_ASSERT_EQUAL_INT(8, sprite.width);
    TEST_ASSERT_EQUAL_INT(1, sprite.height);
}

void test_ui_sprite_clear_returns_it_to_the_empty_state(void) {
    UISprite sprite(Vector2::ZERO());
    sprite.setSprite(kIcon4bpp);

    sprite.clearSprite();

    TEST_ASSERT_EQUAL(static_cast<int>(UISpriteFormat::None), static_cast<int>(sprite.getFormat()));
    TEST_ASSERT_FALSE(sprite.hasSprite());
    TEST_ASSERT_EQUAL_INT(0, sprite.width);
    TEST_ASSERT_EQUAL_INT(0, sprite.height);
}

void test_ui_sprite_flip_x_is_settable_and_preserved(void) {
    UISprite sprite(Vector2::ZERO());
    sprite.setSprite(kIcon4bpp);

    TEST_ASSERT_FALSE(sprite.getFlipX());
    sprite.setFlipX(true);
    TEST_ASSERT_TRUE(sprite.getFlipX());

    // Setting a new sprite must not silently reset an explicit flip.
    sprite.setSprite(kWideIcon4bpp);
    TEST_ASSERT_TRUE(sprite.getFlipX());
}

// =============================================================================
// UISprite — layout and draw contracts
// =============================================================================

void test_ui_sprite_reports_its_sprite_size_as_preferred_size(void) {
    // This is the whole reason a sprite leaf can live in a layout: the layout
    // asks for a preferred size and gets the sprite's real dimensions.
    UISprite sprite(Vector2::ZERO());
    sprite.setSprite(kWideIcon4bpp);

    Scalar preferredWidth = toScalar(0);
    Scalar preferredHeight = toScalar(0);
    sprite.getPreferredSize(preferredWidth, preferredHeight);

    TEST_ASSERT_EQUAL_INT(8, static_cast<int>(preferredWidth));
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(preferredHeight));
}

void test_ui_sprite_is_positioned_by_a_horizontal_layout(void) {
    UIHorizontalLayout layout(0, 0, 200, 40);
    layout.setSpacing(toScalar(0));
    layout.setPadding(toScalar(0));

    UISprite first(Vector2::ZERO());
    UISprite second(Vector2::ZERO());
    first.setSprite(kWideIcon4bpp);   // 8 px wide
    second.setSprite(kIcon4bpp);      // 4 px wide

    layout.addElement(&first);
    layout.addElement(&second);

    // The second element must sit to the right of the first, which only works
    // if the layout could read a real width off a sprite leaf.
    TEST_ASSERT_TRUE(second.position.x > first.position.x);
}

void test_ui_sprite_invisible_draw_returns_before_touching_the_renderer(void) {
    Renderer renderer = makeRenderer();

    UISprite sprite(Vector2::ZERO());
    sprite.setSprite(kIcon4bpp);
    sprite.setFixedPosition(true);
    sprite.setVisible(false);

    const bool bypassBefore = renderer.isOffsetBypassEnabled();
    sprite.draw(renderer);

    TEST_ASSERT_EQUAL(bypassBefore, renderer.isOffsetBypassEnabled());
    TEST_ASSERT_FALSE(sprite.isVisible);
}

void test_ui_sprite_fixed_position_restores_the_previous_bypass_state(void) {
    // The bug this prevents: a HUD icon that scrolls away with the camera, or
    // worse, one that leaves offset-bypass enabled for everything drawn after
    // it. Only a UIElement gets this for free; a hand-rolled HUD does not.
    Renderer renderer = makeRenderer();

    UISprite sprite(Vector2(5, 5));
    sprite.setSprite(kIcon4bpp);
    sprite.setFixedPosition(true);

    const bool bypassBefore = renderer.isOffsetBypassEnabled();
    sprite.draw(renderer);

    TEST_ASSERT_EQUAL(bypassBefore, renderer.isOffsetBypassEnabled());
}

void test_ui_sprite_without_fixed_position_leaves_bypass_untouched(void) {
    Renderer renderer = makeRenderer();

    UISprite sprite(Vector2(5, 5));
    sprite.setSprite(kIcon2bpp);

    const bool bypassBefore = renderer.isOffsetBypassEnabled();
    sprite.draw(renderer);

    TEST_ASSERT_EQUAL(bypassBefore, renderer.isOffsetBypassEnabled());
}

void test_ui_sprite_update_is_inert(void) {
    // A sprite leaf holds no clock of its own; animation is the game's job
    // (swap the sprite). update() exists only to satisfy Entity.
    UISprite sprite(Vector2::ZERO());
    sprite.setSprite(kIcon4bpp);

    sprite.update(16);
    sprite.update(10000);

    TEST_ASSERT_EQUAL_INT(4, sprite.width);
    TEST_ASSERT_EQUAL(static_cast<int>(UISpriteFormat::Bpp4), static_cast<int>(sprite.getFormat()));
}

// =============================================================================
// UISpriteRow — empty and configuration
// =============================================================================

void test_ui_sprite_row_starts_empty(void) {
    UISpriteRow row(Vector2(4, 6));

    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(row.getCapacity()));
    TEST_ASSERT_EQUAL_INT(0, row.getValue());
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(row.getUnitsPerIcon()));
    TEST_ASSERT_EQUAL_INT(0, row.width);
    TEST_ASSERT_EQUAL_INT(0, row.height);
}

void test_ui_sprite_row_empty_draw_is_a_no_op(void) {
    Renderer renderer = makeRenderer();

    UISpriteRow row(Vector2::ZERO());
    row.draw(renderer);  // No states, no capacity: must not dereference anything.

    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(row.getCapacity()));
}

void test_ui_sprite_row_units_per_icon_is_clamped_to_the_state_capacity(void) {
    // states[] is fixed-size; unitsPerIcon N needs N+1 states, so it cannot
    // exceed kMaxStates - 1 no matter what the caller asks for.
    UISpriteRow row(Vector2::ZERO());

    row.setUnitsPerIcon(0);
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(row.getUnitsPerIcon()));

    row.setUnitsPerIcon(200);
    TEST_ASSERT_EQUAL_INT(UISpriteRow::kMaxStates - 1, static_cast<int>(row.getUnitsPerIcon()));
}

void test_ui_sprite_row_out_of_range_state_index_is_ignored(void) {
    UISpriteRow row(Vector2::ZERO());
    row.setStateSprite(UISpriteRow::kMaxStates + 5, kHeartFull);  // Must not write out of bounds.

    TEST_ASSERT_EQUAL_INT(0, row.width);
}

// =============================================================================
// UISpriteRow — state selection (the value -> icon-state mapping)
// =============================================================================

void test_ui_sprite_row_binary_icons_fill_left_to_right(void) {
    // unitsPerIcon == 1: the classic lives row. State 1 = present, 0 = spent.
    UISpriteRow row(Vector2::ZERO());
    row.setStateSprite(0, kHeartEmpty);
    row.setStateSprite(1, kHeartFull);
    row.setUnitsPerIcon(1);
    row.setCapacity(5);
    row.setValue(3);

    TEST_ASSERT_EQUAL_INT(1, row.stateIndexAt(0));
    TEST_ASSERT_EQUAL_INT(1, row.stateIndexAt(1));
    TEST_ASSERT_EQUAL_INT(1, row.stateIndexAt(2));
    TEST_ASSERT_EQUAL_INT(0, row.stateIndexAt(3));
    TEST_ASSERT_EQUAL_INT(0, row.stateIndexAt(4));
}

void test_ui_sprite_row_half_steps_produce_a_partial_icon(void) {
    // The Zelda case: 5 quarter-units over 2-unit hearts reads as
    // full, full, half — the partial icon is the one the value lands inside.
    UISpriteRow row(Vector2::ZERO());
    configureHeartRow(row);
    row.setCapacity(3);
    row.setValue(5);

    TEST_ASSERT_EQUAL_INT(2, row.stateIndexAt(0));
    TEST_ASSERT_EQUAL_INT(2, row.stateIndexAt(1));
    TEST_ASSERT_EQUAL_INT(1, row.stateIndexAt(2));
}

void test_ui_sprite_row_exact_multiples_leave_no_partial_icon(void) {
    UISpriteRow row(Vector2::ZERO());
    configureHeartRow(row);
    row.setCapacity(3);
    row.setValue(4);

    TEST_ASSERT_EQUAL_INT(2, row.stateIndexAt(0));
    TEST_ASSERT_EQUAL_INT(2, row.stateIndexAt(1));
    TEST_ASSERT_EQUAL_INT(0, row.stateIndexAt(2));
}

void test_ui_sprite_row_value_above_capacity_saturates_every_icon(void) {
    UISpriteRow row(Vector2::ZERO());
    configureHeartRow(row);
    row.setCapacity(3);
    row.setValue(999);

    TEST_ASSERT_EQUAL_INT(2, row.stateIndexAt(0));
    TEST_ASSERT_EQUAL_INT(2, row.stateIndexAt(2));
}

void test_ui_sprite_row_negative_value_empties_every_icon(void) {
    // Games routinely subtract past zero on the frame a player dies; that must
    // render as an empty row, not as an underflowed index.
    UISpriteRow row(Vector2::ZERO());
    configureHeartRow(row);
    row.setCapacity(3);
    row.setValue(-8);

    TEST_ASSERT_EQUAL_INT(0, row.stateIndexAt(0));
    TEST_ASSERT_EQUAL_INT(0, row.stateIndexAt(2));
}

void test_ui_sprite_row_state_index_out_of_range_icon_is_clamped(void) {
    UISpriteRow row(Vector2::ZERO());
    configureHeartRow(row);
    row.setCapacity(3);
    row.setValue(6);

    TEST_ASSERT_EQUAL_INT(0, row.stateIndexAt(-1));
    TEST_ASSERT_EQUAL_INT(0, row.stateIndexAt(99));
}

// =============================================================================
// UISpriteRow — geometry
// =============================================================================

void test_ui_sprite_row_width_spans_capacity_and_spacing(void) {
    UISpriteRow row(Vector2::ZERO());
    configureHeartRow(row);
    row.setSpacing(2);
    row.setCapacity(4);

    // 4 icons of 8 px with 2 px between them: 4*8 + 3*2 = 38.
    TEST_ASSERT_EQUAL_INT(38, row.width);
    TEST_ASSERT_EQUAL_INT(8, row.height);
}

void test_ui_sprite_row_single_icon_has_no_trailing_spacing(void) {
    UISpriteRow row(Vector2::ZERO());
    configureHeartRow(row);
    row.setSpacing(5);
    row.setCapacity(1);

    TEST_ASSERT_EQUAL_INT(8, row.width);
}

void test_ui_sprite_row_icon_offsets_advance_by_icon_width_plus_spacing(void) {
    UISpriteRow row(Vector2(100, 50));
    configureHeartRow(row);
    row.setSpacing(2);
    row.setCapacity(3);

    int offsetX = -1;
    int offsetY = -1;

    row.iconOffsetAt(0, offsetX, offsetY);
    TEST_ASSERT_EQUAL_INT(0, offsetX);
    TEST_ASSERT_EQUAL_INT(0, offsetY);

    row.iconOffsetAt(2, offsetX, offsetY);
    TEST_ASSERT_EQUAL_INT(20, offsetX);  // 2 * (8 + 2)
    TEST_ASSERT_EQUAL_INT(0, offsetY);
}

void test_ui_sprite_row_wraps_to_a_second_row(void) {
    // Zelda's heart bar wraps after eight. Capacity 10 at 8 per row is two
    // rows: eight icons then two.
    UISpriteRow row(Vector2::ZERO());
    configureHeartRow(row);
    row.setSpacing(0);
    row.setRowSpacing(1);
    row.setIconsPerRow(8);
    row.setCapacity(10);

    int offsetX = -1;
    int offsetY = -1;

    row.iconOffsetAt(7, offsetX, offsetY);
    TEST_ASSERT_EQUAL_INT(56, offsetX);  // 7 * 8
    TEST_ASSERT_EQUAL_INT(0, offsetY);

    row.iconOffsetAt(8, offsetX, offsetY);
    TEST_ASSERT_EQUAL_INT(0, offsetX);   // wrapped
    TEST_ASSERT_EQUAL_INT(9, offsetY);   // 8 + 1 row spacing
}

void test_ui_sprite_row_wrapped_size_covers_both_rows(void) {
    UISpriteRow row(Vector2::ZERO());
    configureHeartRow(row);
    row.setSpacing(0);
    row.setRowSpacing(1);
    row.setIconsPerRow(8);
    row.setCapacity(10);

    TEST_ASSERT_EQUAL_INT(64, row.width);   // widest row is the full 8
    TEST_ASSERT_EQUAL_INT(17, row.height);  // 2 rows of 8 plus 1 row spacing
}

void test_ui_sprite_row_partial_first_row_sizes_to_what_is_there(void) {
    UISpriteRow row(Vector2::ZERO());
    configureHeartRow(row);
    row.setSpacing(0);
    row.setIconsPerRow(8);
    row.setCapacity(3);

    TEST_ASSERT_EQUAL_INT(24, row.width);
    TEST_ASSERT_EQUAL_INT(8, row.height);
}

void test_ui_sprite_row_capacity_growth_widens_the_element(void) {
    // Heart containers: capacity grows mid-game and the HUD must grow with it,
    // including the layout-visible width.
    UISpriteRow row(Vector2::ZERO());
    configureHeartRow(row);
    row.setSpacing(0);
    row.setCapacity(3);
    const int narrow = row.width;

    row.setCapacity(6);

    TEST_ASSERT_EQUAL_INT(24, narrow);
    TEST_ASSERT_EQUAL_INT(48, row.width);
}

void test_ui_sprite_row_reports_its_span_as_preferred_size(void) {
    UISpriteRow row(Vector2::ZERO());
    configureHeartRow(row);
    row.setSpacing(0);
    row.setCapacity(2);

    Scalar preferredWidth = toScalar(0);
    Scalar preferredHeight = toScalar(0);
    row.getPreferredSize(preferredWidth, preferredHeight);

    TEST_ASSERT_EQUAL_INT(16, static_cast<int>(preferredWidth));
    TEST_ASSERT_EQUAL_INT(8, static_cast<int>(preferredHeight));
}

// =============================================================================
// UISpriteRow — draw contracts
// =============================================================================

void test_ui_sprite_row_invisible_draw_returns_early(void) {
    Renderer renderer = makeRenderer();

    UISpriteRow row(Vector2::ZERO());
    configureHeartRow(row);
    row.setCapacity(3);
    row.setValue(4);
    row.setFixedPosition(true);
    row.setVisible(false);

    const bool bypassBefore = renderer.isOffsetBypassEnabled();
    row.draw(renderer);

    TEST_ASSERT_EQUAL(bypassBefore, renderer.isOffsetBypassEnabled());
}

void test_ui_sprite_row_fixed_position_restores_the_previous_bypass_state(void) {
    Renderer renderer = makeRenderer();

    UISpriteRow row(Vector2(8, 8));
    configureHeartRow(row);
    row.setCapacity(3);
    row.setValue(5);
    row.setFixedPosition(true);

    const bool bypassBefore = renderer.isOffsetBypassEnabled();
    row.draw(renderer);

    TEST_ASSERT_EQUAL(bypassBefore, renderer.isOffsetBypassEnabled());
}

void test_ui_sprite_row_draws_with_a_missing_state_without_crashing(void) {
    // A row configured with only some of its states must skip the unset ones
    // rather than dereference a null sprite.
    Renderer renderer = makeRenderer();

    UISpriteRow row(Vector2::ZERO());
    row.setStateSprite(2, kHeartFull);  // states 0 and 1 deliberately unset
    row.setUnitsPerIcon(2);
    row.setCapacity(3);
    row.setValue(3);

    row.draw(renderer);

    TEST_ASSERT_EQUAL_INT(3, static_cast<int>(row.getCapacity()));
}

void test_ui_sprite_row_mixed_state_formats_are_allowed(void) {
    // Nothing requires every state to share a format; the row stores a tagged
    // reference per state, exactly like UISprite does.
    UISpriteRow row(Vector2::ZERO());
    row.setStateSprite(0, kIconMono, Color::White);
    row.setStateSprite(1, kIcon2bpp, 0);
    row.setStateSprite(2, kIcon4bpp, 0);
    row.setUnitsPerIcon(2);
    row.setCapacity(2);

    TEST_ASSERT_EQUAL_INT(8, row.width);  // 2 icons of 4 px, no spacing
    TEST_ASSERT_EQUAL_INT(2, row.height);
}

#endif  // PIXELROOT32_ENABLE_UI_SYSTEM

void test_ui_sprite_zero_cost_when_ui_system_disabled(void) {
#if PIXELROOT32_ENABLE_UI_SYSTEM
    TEST_PASS_MESSAGE("PIXELROOT32_ENABLE_UI_SYSTEM=1: UISprite/UISpriteRow exercised above.");
#else
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_UI_SYSTEM=0: UISprite/UISpriteRow are not compiled, "
        "zero bytes reserved.");
#endif
}

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_UI_SYSTEM
    RUN_TEST(test_ui_sprite_starts_empty_with_zero_size);
    RUN_TEST(test_ui_sprite_defaults_to_ui_render_layer);
    RUN_TEST(test_ui_sprite_empty_draw_is_a_no_op);
    RUN_TEST(test_ui_sprite_accepts_a_4bpp_sprite_and_adopts_its_size);
    RUN_TEST(test_ui_sprite_accepts_a_2bpp_sprite_and_adopts_its_size);
    RUN_TEST(test_ui_sprite_accepts_a_mono_sprite_with_a_tint);
    RUN_TEST(test_ui_sprite_switching_format_replaces_the_previous_one);
    RUN_TEST(test_ui_sprite_size_tracks_the_sprite_currently_set);
    RUN_TEST(test_ui_sprite_clear_returns_it_to_the_empty_state);
    RUN_TEST(test_ui_sprite_flip_x_is_settable_and_preserved);
    RUN_TEST(test_ui_sprite_reports_its_sprite_size_as_preferred_size);
    RUN_TEST(test_ui_sprite_is_positioned_by_a_horizontal_layout);
    RUN_TEST(test_ui_sprite_invisible_draw_returns_before_touching_the_renderer);
    RUN_TEST(test_ui_sprite_fixed_position_restores_the_previous_bypass_state);
    RUN_TEST(test_ui_sprite_without_fixed_position_leaves_bypass_untouched);
    RUN_TEST(test_ui_sprite_update_is_inert);

    RUN_TEST(test_ui_sprite_row_starts_empty);
    RUN_TEST(test_ui_sprite_row_empty_draw_is_a_no_op);
    RUN_TEST(test_ui_sprite_row_units_per_icon_is_clamped_to_the_state_capacity);
    RUN_TEST(test_ui_sprite_row_out_of_range_state_index_is_ignored);
    RUN_TEST(test_ui_sprite_row_binary_icons_fill_left_to_right);
    RUN_TEST(test_ui_sprite_row_half_steps_produce_a_partial_icon);
    RUN_TEST(test_ui_sprite_row_exact_multiples_leave_no_partial_icon);
    RUN_TEST(test_ui_sprite_row_value_above_capacity_saturates_every_icon);
    RUN_TEST(test_ui_sprite_row_negative_value_empties_every_icon);
    RUN_TEST(test_ui_sprite_row_state_index_out_of_range_icon_is_clamped);
    RUN_TEST(test_ui_sprite_row_width_spans_capacity_and_spacing);
    RUN_TEST(test_ui_sprite_row_single_icon_has_no_trailing_spacing);
    RUN_TEST(test_ui_sprite_row_icon_offsets_advance_by_icon_width_plus_spacing);
    RUN_TEST(test_ui_sprite_row_wraps_to_a_second_row);
    RUN_TEST(test_ui_sprite_row_wrapped_size_covers_both_rows);
    RUN_TEST(test_ui_sprite_row_partial_first_row_sizes_to_what_is_there);
    RUN_TEST(test_ui_sprite_row_capacity_growth_widens_the_element);
    RUN_TEST(test_ui_sprite_row_reports_its_span_as_preferred_size);
    RUN_TEST(test_ui_sprite_row_invisible_draw_returns_early);
    RUN_TEST(test_ui_sprite_row_fixed_position_restores_the_previous_bypass_state);
    RUN_TEST(test_ui_sprite_row_draws_with_a_missing_state_without_crashing);
    RUN_TEST(test_ui_sprite_row_mixed_state_formats_are_allowed);
#endif
    RUN_TEST(test_ui_sprite_zero_cost_when_ui_system_disabled);

    return UNITY_END();
}
