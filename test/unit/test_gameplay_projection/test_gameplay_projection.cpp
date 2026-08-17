/**
 * @file test_gameplay_projection.cpp
 * @brief Unit tests for the gameplay/Projection module.
 *
 * Covers the spec requirements for gameplay-projection:
 * - projection-spec-is-a-constexpr-friendly-value-describing-an-origin-and-a-2x2-integer-basis
 * - projection-det-is-computed-from-the-basis-and-is-never-stored
 * - cell-to-screen-maps-a-cell-index-to-a-screen-position-without-division
 * - screen-to-cell-inverts-the-projection-with-exactly-one-integer-division-per-axis
 * - the-inverse-floors-toward-negative-infinity-never-truncating-toward-zero
 * - cell-to-screen-and-screen-to-cell-round-trip-exactly-for-every-supported-layout
 * - projection-spec-is-valid-rejects-a-degenerate-basis
 * - projection-spec-is-valid-range-checks-all-four-projected-corners
 * - double-overloads-are-deleted
 * - feature-gated-and-zero-cost-when-disabled
 *
 * Every forward/inverse assertion runs over the SAME four specs — orthogonal,
 * isometric 2:1, isometric 1:1 and oblique — because the capability's whole
 * claim is that one type and one code path serve all of them (design D1).
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_GAMEPLAY_PROJECTION
 * is enabled, since Projection.h is entirely guarded behind that flag. This
 * file therefore compiles cleanly in BOTH the default (flag off) and opt-in
 * (flag on) configurations, matching the flags-off / flags-on CI matrix.
 *
 * Not testable here, by nature:
 * - The deleted double overloads (a compile error cannot be a runtime case).
 *   Their absence is enforced by review and by the header itself.
 * - "Exactly one integer div per axis" is a structural property of the source,
 *   not an observable one. Asserted by review; the header is ~40 lines.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_PROJECTION

#include "gameplay/Projection.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE
#include "gameplay/GridSpace.h"
#endif

#include <cstddef>
#include <cstdint>

using namespace pixelroot32::gameplay;

namespace {

// ---------------------------------------------------------------------------
// The four layouts under test. One type, one code path, four visual styles.
// ---------------------------------------------------------------------------

/// Orthogonal 16x16 — the identity case. Must reproduce GridSpace's mapping.
constexpr ProjectionSpec kOrtho16{0, 0, 16, 0, 0, 16};
static_assert(projectionSpecIsValid(kOrtho16, 20, 15), "kOrtho16 must be valid.");
static_assert(projectionDet(kOrtho16) == 256, "kOrtho16 determinant.");

/// Isometric 2:1, 32x16 tiles. The classic diamond.
constexpr ProjectionSpec kIso2to1{0, 0, 16, 8, -16, 8};
static_assert(projectionSpecIsValid(kIso2to1, 32, 32), "kIso2to1 must be valid.");
static_assert(projectionDet(kIso2to1) == 256, "kIso2to1 determinant.");

/// Isometric 1:1, 32x32 tiles.
constexpr ProjectionSpec kIso1to1{0, 0, 16, 16, -16, 16};
static_assert(projectionSpecIsValid(kIso1to1, 32, 32), "kIso1to1 must be valid.");
static_assert(projectionDet(kIso1to1) == 512, "kIso1to1 determinant.");

/// Oblique — a sheared axis-aligned grid. Not isometric, still expressible.
constexpr ProjectionSpec kOblique{0, 0, 16, 0, 8, 16};
static_assert(projectionSpecIsValid(kOblique, 20, 20), "kOblique must be valid.");
static_assert(projectionDet(kOblique) == 256, "kOblique determinant.");

/// Isometric 2:1 with a non-zero origin, to catch an origin folded on the
/// wrong side of the floor division.
constexpr ProjectionSpec kIsoShifted{7, 5, 16, 8, -16, 8};
static_assert(projectionSpecIsValid(kIsoShifted, 32, 32), "kIsoShifted must be valid.");

// ---------------------------------------------------------------------------
// Specs that must be REJECTED.
// ---------------------------------------------------------------------------

/// Collinear basis: det == 0, no inverse. RISC-V div-by-zero returns -1
/// instead of trapping, so this must be caught by the predicate.
constexpr ProjectionSpec kCollinear{0, 0, 16, 0, 32, 0};
static_assert(projectionDet(kCollinear) == 0, "kCollinear determinant is zero.");
static_assert(!projectionSpecIsValid(kCollinear, 10, 10), "kCollinear must be rejected (det == 0).");

/// Left-handed basis: det < 0. Invertible, but it would call the floor
/// division with a negative divisor and silently invert the floor direction.
/// Expressed instead by swapping the two axis columns.
constexpr ProjectionSpec kLeftHanded{0, 0, -16, 8, 16, 8};
static_assert(projectionDet(kLeftHanded) == -256, "kLeftHanded determinant is negative.");
static_assert(!projectionSpecIsValid(kLeftHanded, 10, 10), "kLeftHanded must be rejected (det < 0).");

// ---------------------------------------------------------------------------
// A table of cells exercised in both directions, including negatives.
// ---------------------------------------------------------------------------

struct CellCase {
    int cellX;
    int cellY;
};

constexpr CellCase kRoundTripCells[] = {
    {0, 0}, {3, 2}, {-1, 0}, {0, -1}, {-4, -7},
};

constexpr ProjectionSpec kAllSpecs[] = {
    kOrtho16, kIso2to1, kIso1to1, kOblique, kIsoShifted,
};

/// Helper: assert one cell survives a projection round-trip under one spec.
void assertRoundTrip(const ProjectionSpec& spec, int cellX, int cellY, const char* what) {
    const int sx = cellToScreenX(cellX, cellY, spec);
    const int sy = cellToScreenY(cellX, cellY, spec);

    TEST_ASSERT_EQUAL_INT_MESSAGE(cellX, screenToCellX(sx, sy, spec), what);
    TEST_ASSERT_EQUAL_INT_MESSAGE(cellY, screenToCellY(sx, sy, spec), what);
}

}  // namespace

// ===========================================================================
// ProjectionSpec is a constexpr-friendly six-int aggregate
// ===========================================================================

void test_projection_spec_is_a_plain_six_int_aggregate(void) {
    // Six ints, no derived field, no padding: the determinant is a function
    // (design D4), so a seventh field cannot desynchronise from the basis.
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(6 * sizeof(int)),
                             static_cast<uint32_t>(sizeof(ProjectionSpec)));
}

void test_projection_spec_defaults_to_the_identity_basis(void) {
    // A default-constructed spec maps every cell to itself, so a caller that
    // forgets to initialise it gets an obviously-wrong-but-harmless mapping
    // rather than a degenerate one.
    constexpr ProjectionSpec kIdentity{};
    static_assert(projectionDet(kIdentity) == 1, "identity determinant is 1");

    TEST_ASSERT_EQUAL_INT(5, cellToScreenX(5, 3, kIdentity));
    TEST_ASSERT_EQUAL_INT(3, cellToScreenY(5, 3, kIdentity));
    TEST_ASSERT_EQUAL_INT(5, screenToCellX(5, 3, kIdentity));
    TEST_ASSERT_EQUAL_INT(3, screenToCellY(5, 3, kIdentity));
}

void test_one_type_serves_orthogonal_isometric_and_oblique_layouts(void) {
    // The spec's "no layout-specific overload, flag or template argument"
    // requirement: the same two calls accept all four bases.
    for (const ProjectionSpec& spec : kAllSpecs) {
        const int sx = cellToScreenX(1, 1, spec);
        const int sy = cellToScreenY(1, 1, spec);
        TEST_ASSERT_EQUAL_INT(1, screenToCellX(sx, sy, spec));
        TEST_ASSERT_EQUAL_INT(1, screenToCellY(sx, sy, spec));
    }
}

// ===========================================================================
// projectionDet
// ===========================================================================

void test_projection_det_of_the_documented_layouts(void) {
    TEST_ASSERT_EQUAL_INT(256, projectionDet(kOrtho16));
    TEST_ASSERT_EQUAL_INT(256, projectionDet(kIso2to1));
    TEST_ASSERT_EQUAL_INT(512, projectionDet(kIso1to1));
    TEST_ASSERT_EQUAL_INT(256, projectionDet(kOblique));
}

// ===========================================================================
// cellToScreen — forward mapping
// ===========================================================================

void test_cell_to_screen_orthogonal_reproduces_the_grid_mapping(void) {
    TEST_ASSERT_EQUAL_INT(48, cellToScreenX(3, 2, kOrtho16));
    TEST_ASSERT_EQUAL_INT(32, cellToScreenY(3, 2, kOrtho16));

#if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE
    // Same numbers as the equivalent GridSpec, which is the concrete statement
    // of "orthogonal games pay nothing for this capability existing".
    constexpr GridSpec kEquivalent{0, 0, 16, 16, 20, 15};
    TEST_ASSERT_EQUAL_INT(cellToWorldX(3, kEquivalent), cellToScreenX(3, 2, kOrtho16));
    TEST_ASSERT_EQUAL_INT(cellToWorldY(2, kEquivalent), cellToScreenY(3, 2, kOrtho16));
#endif
}

void test_cell_to_screen_iso_places_the_unit_cells_on_the_diamond_axes(void) {
    // +1 cellX goes right-and-down; +1 cellY goes left-and-down.
    TEST_ASSERT_EQUAL_INT(16, cellToScreenX(1, 0, kIso2to1));
    TEST_ASSERT_EQUAL_INT(8,  cellToScreenY(1, 0, kIso2to1));

    TEST_ASSERT_EQUAL_INT(-16, cellToScreenX(0, 1, kIso2to1));
    TEST_ASSERT_EQUAL_INT(8,   cellToScreenY(0, 1, kIso2to1));
}

void test_cell_to_screen_iso_distinct_cells_share_a_row_at_distinct_columns(void) {
    // (1,1) and (2,0) project to the same screen Y and different screen X —
    // the property that makes world-Y depth sorting wrong (scene-depth-sort).
    TEST_ASSERT_EQUAL_INT(0,  cellToScreenX(1, 1, kIso2to1));
    TEST_ASSERT_EQUAL_INT(16, cellToScreenY(1, 1, kIso2to1));

    TEST_ASSERT_EQUAL_INT(32, cellToScreenX(2, 0, kIso2to1));
    TEST_ASSERT_EQUAL_INT(16, cellToScreenY(2, 0, kIso2to1));
}

void test_cell_to_screen_depends_on_both_cell_coordinates(void) {
    // A one-coordinate signature would be silently wrong for exactly the case
    // this capability exists to serve (design D0). Both directions need a spec
    // whose corresponding cross-axis component is non-zero: kIso2to1 has
    // axisYx = -16 (so screen X depends on cellY) and axisXy = 8 (so screen Y
    // depends on cellX). kOblique deliberately has axisXy = 0 and is NOT a
    // valid probe for the Y direction.
    TEST_ASSERT_NOT_EQUAL(cellToScreenX(3, 0, kIso2to1), cellToScreenX(3, 1, kIso2to1));
    TEST_ASSERT_NOT_EQUAL(cellToScreenY(0, 3, kIso2to1), cellToScreenY(1, 3, kIso2to1));

    // And the oblique shear is visible on the axis it actually shears.
    TEST_ASSERT_NOT_EQUAL(cellToScreenX(3, 0, kOblique), cellToScreenX(3, 1, kOblique));
}

void test_cell_to_screen_honours_a_non_zero_origin(void) {
    TEST_ASSERT_EQUAL_INT(7 + 48 - 32, cellToScreenX(3, 2, kIsoShifted));
    TEST_ASSERT_EQUAL_INT(5 + 24 + 16, cellToScreenY(3, 2, kIsoShifted));
}

// ===========================================================================
// screenToCell — inverse mapping
// ===========================================================================

void test_screen_to_cell_recovers_a_projected_anchor(void) {
    TEST_ASSERT_EQUAL_INT(1, screenToCellX(0, 16, kIso2to1));
    TEST_ASSERT_EQUAL_INT(1, screenToCellY(0, 16, kIso2to1));
}

void test_screen_to_cell_recovers_a_cell_on_the_x_axis(void) {
    TEST_ASSERT_EQUAL_INT(1, screenToCellX(16, 8, kIso2to1));
    TEST_ASSERT_EQUAL_INT(0, screenToCellY(16, 8, kIso2to1));
}

void test_screen_to_cell_floors_one_pixel_left_of_the_origin_anchor(void) {
    // THE hazard. numX = (-1)*8 - 0*(-16) = -8. Floor gives -1; truncation
    // toward zero gives 0, placing a point outside the map into cell (0,0).
    TEST_ASSERT_EQUAL_INT(-1, screenToCellX(-1, 0, kIso2to1));
    TEST_ASSERT_EQUAL_INT(0,  screenToCellY(-1, 0, kIso2to1));
}

void test_screen_to_cell_orthogonal_floor_table_matches_world_to_cell(void) {
    // The Phase 3 GridSpace floor table, reproduced through the projection
    // path. Every one of these returns the wrong answer under truncation.
    TEST_ASSERT_EQUAL_INT(1,  screenToCellX(20, 0, kOrtho16));
    TEST_ASSERT_EQUAL_INT(0,  screenToCellX(15, 0, kOrtho16));
    TEST_ASSERT_EQUAL_INT(1,  screenToCellX(16, 0, kOrtho16));
    TEST_ASSERT_EQUAL_INT(-1, screenToCellX(-1, 0, kOrtho16));
    TEST_ASSERT_EQUAL_INT(-1, screenToCellX(-16, 0, kOrtho16));
    TEST_ASSERT_EQUAL_INT(-2, screenToCellX(-17, 0, kOrtho16));
    TEST_ASSERT_EQUAL_INT(-2, screenToCellX(-32, 0, kOrtho16));

#if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE
    constexpr GridSpec kEquivalent{0, 0, 16, 16, 20, 15};
    TEST_ASSERT_EQUAL_INT(worldToCellX(-17, kEquivalent), screenToCellX(-17, 0, kOrtho16));
    TEST_ASSERT_EQUAL_INT(worldToCellX(-32, kEquivalent), screenToCellX(-32, 0, kOrtho16));
#endif
}

void test_screen_to_cell_floors_on_the_y_axis_too(void) {
    TEST_ASSERT_EQUAL_INT(-1, screenToCellY(0, -1, kOrtho16));
    TEST_ASSERT_EQUAL_INT(-2, screenToCellY(0, -17, kOrtho16));
}

void test_screen_to_cell_honours_a_non_zero_origin(void) {
    // cell (3,2) under kIsoShifted projects to (23, 45).
    TEST_ASSERT_EQUAL_INT(3, screenToCellX(23, 45, kIsoShifted));
    TEST_ASSERT_EQUAL_INT(2, screenToCellY(23, 45, kIsoShifted));
}

// ===========================================================================
// Round-trip across every layout and both signs
// ===========================================================================

void test_round_trip_holds_across_layouts_and_signs(void) {
    for (const ProjectionSpec& spec : kAllSpecs) {
        for (const CellCase& c : kRoundTripCells) {
            assertRoundTrip(spec, c.cellX, c.cellY, "round-trip must be exact");
        }
    }
}

// ===========================================================================
// projectionSpecIsValid
// ===========================================================================

void test_projection_spec_is_valid_rejects_a_collinear_basis(void) {
    TEST_ASSERT_FALSE(projectionSpecIsValid(kCollinear, 10, 10));
}

void test_projection_spec_is_valid_rejects_a_left_handed_basis(void) {
    TEST_ASSERT_FALSE(projectionSpecIsValid(kLeftHanded, 10, 10));
}

void test_projection_spec_is_valid_rejects_a_negative_extent(void) {
    TEST_ASSERT_FALSE(projectionSpecIsValid(kIso2to1, -1, 10));
    TEST_ASSERT_FALSE(projectionSpecIsValid(kIso2to1, 10, -1));
}

void test_projection_spec_is_valid_rejects_a_minimum_corner_overflow(void) {
    // The case a copied gridSpecIsValid ACCEPTS. With axisYx = -16 the minimum
    // projected X is at corner (0, rows) = -65536, while the naive per-axis
    // form originX + cols * axisXx evaluates to 16 and passes.
    TEST_ASSERT_EQUAL_INT(16, kIso2to1.originX + 1 * kIso2to1.axisXx);
    TEST_ASSERT_FALSE(projectionSpecIsValid(kIso2to1, 1, 4096));
}

void test_projection_spec_is_valid_rejects_a_maximum_corner_overflow(void) {
    // Corner (cols, 0) = (65536, 32768), both past the Scalar range.
    TEST_ASSERT_FALSE(projectionSpecIsValid(kIso2to1, 4096, 1));
}

void test_projection_spec_is_valid_accepts_an_in_range_extent(void) {
    TEST_ASSERT_TRUE(projectionSpecIsValid(kIso2to1, 32, 32));
    TEST_ASSERT_TRUE(projectionSpecIsValid(kOrtho16, 20, 15));
    TEST_ASSERT_TRUE(projectionSpecIsValid(kIso1to1, 32, 32));
    TEST_ASSERT_TRUE(projectionSpecIsValid(kOblique, 20, 20));
}

// ===========================================================================
// The duplicated floor-division primitive (design D3)
// ===========================================================================

#if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE
void test_projection_floor_div_agrees_with_the_grid_primitive(void) {
    // Projection.h cannot reach detail::gridFloorDiv — that primitive is
    // inside the GRID_SPACE guard and this capability must not depend on it.
    // The body is therefore duplicated, and this test is what stands between
    // the duplication and a silent divergence.
    static const int kValues[] = {0, 1, 15, 16, 17, 20, 255, 256, 257,
                                  -1, -8, -15, -16, -17, -32, -255, -256, -257};
    static const int kDivisors[] = {1, 2, 10, 16, 50, 256, 512};

    for (int v : kValues) {
        for (int d : kDivisors) {
            TEST_ASSERT_EQUAL_INT_MESSAGE(detail::gridFloorDiv(v, d),
                                          detail::projectionFloorDiv(v, d),
                                          "projectionFloorDiv must match gridFloorDiv");
        }
    }
}
#endif

// ===========================================================================
// Zero cost
// ===========================================================================

void test_gameplay_projection_zero_cost_when_disabled(void) {
    // Flag ON: the capability is a constexpr aggregate plus free functions, so
    // a constexpr consumer pays zero runtime footprint. The mirror of this
    // test in the #else branch asserts the flag-off property.
    constexpr ProjectionSpec kProbe{0, 0, 8, 4, -8, 4};
    static_assert(projectionSpecIsValid(kProbe, 4, 4), "probe spec must be constexpr-valid");
    static_assert(projectionDet(kProbe) == 64, "projectionDet must be constexpr-evaluable");
    static_assert(cellToScreenX(1, 0, kProbe) == 8, "cellToScreenX must be constexpr-evaluable");
    static_assert(cellToScreenY(1, 0, kProbe) == 4, "cellToScreenY must be constexpr-evaluable");
    static_assert(screenToCellX(8, 4, kProbe) == 1, "screenToCellX must be constexpr-evaluable");
    static_assert(screenToCellY(8, 4, kProbe) == 0, "screenToCellY must be constexpr-evaluable");
    static_assert(screenToCellX(-1, 0, kProbe) == -1, "the inverse must floor at compile time too");

    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_GAMEPLAY_PROJECTION=1: ProjectionSpec stays a plain "
        "six-int aggregate and every free function is constexpr-evaluable, "
        "so a constexpr consumer pays zero runtime footprint.");
}

#else  // !PIXELROOT32_ENABLE_GAMEPLAY_PROJECTION

void test_gameplay_projection_zero_cost_when_disabled(void) {
    // With the flag off, ProjectionSpec/projectionDet/projectionSpecIsValid/
    // cellToScreen*/screenToCell* are not compiled at all — their entire
    // declaration lives inside the #if PIXELROOT32_ENABLE_GAMEPLAY_PROJECTION
    // guard in include/gameplay/Projection.h. This translation unit compiling
    // and passing without referencing any of them IS the "zero bytes reserved"
    // property required by the spec's "Feature-Gated And Zero-Cost When
    // Disabled" requirement.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_GAMEPLAY_PROJECTION=0: ProjectionSpec/cellToScreen/"
        "screenToCell/projectionSpecIsValid are not compiled, zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_GAMEPLAY_PROJECTION

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

#if PIXELROOT32_ENABLE_GAMEPLAY_PROJECTION
    RUN_TEST(test_projection_spec_is_a_plain_six_int_aggregate);
    RUN_TEST(test_projection_spec_defaults_to_the_identity_basis);
    RUN_TEST(test_one_type_serves_orthogonal_isometric_and_oblique_layouts);
    RUN_TEST(test_projection_det_of_the_documented_layouts);
    RUN_TEST(test_cell_to_screen_orthogonal_reproduces_the_grid_mapping);
    RUN_TEST(test_cell_to_screen_iso_places_the_unit_cells_on_the_diamond_axes);
    RUN_TEST(test_cell_to_screen_iso_distinct_cells_share_a_row_at_distinct_columns);
    RUN_TEST(test_cell_to_screen_depends_on_both_cell_coordinates);
    RUN_TEST(test_cell_to_screen_honours_a_non_zero_origin);
    RUN_TEST(test_screen_to_cell_recovers_a_projected_anchor);
    RUN_TEST(test_screen_to_cell_recovers_a_cell_on_the_x_axis);
    RUN_TEST(test_screen_to_cell_floors_one_pixel_left_of_the_origin_anchor);
    RUN_TEST(test_screen_to_cell_orthogonal_floor_table_matches_world_to_cell);
    RUN_TEST(test_screen_to_cell_floors_on_the_y_axis_too);
    RUN_TEST(test_screen_to_cell_honours_a_non_zero_origin);
    RUN_TEST(test_round_trip_holds_across_layouts_and_signs);
    RUN_TEST(test_projection_spec_is_valid_rejects_a_collinear_basis);
    RUN_TEST(test_projection_spec_is_valid_rejects_a_left_handed_basis);
    RUN_TEST(test_projection_spec_is_valid_rejects_a_negative_extent);
    RUN_TEST(test_projection_spec_is_valid_rejects_a_minimum_corner_overflow);
    RUN_TEST(test_projection_spec_is_valid_rejects_a_maximum_corner_overflow);
    RUN_TEST(test_projection_spec_is_valid_accepts_an_in_range_extent);
#if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE
    RUN_TEST(test_projection_floor_div_agrees_with_the_grid_primitive);
#endif
#endif
    RUN_TEST(test_gameplay_projection_zero_cost_when_disabled);

    return UNITY_END();
}
