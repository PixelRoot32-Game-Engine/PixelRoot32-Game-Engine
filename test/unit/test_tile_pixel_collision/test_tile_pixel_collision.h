/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for the per-pixel tile collision helper
 * (physics/TilePixelCollision.h). Declarations only; definitions live in
 * test_tile_pixel_collision.cpp.
 */
#pragma once

// isTilePixelSolid tests
void test_opaque_pixel_returns_true();
void test_transparent_pixel_returns_false();
void test_mixed_tile_corner_vs_center();
void test_out_of_bounds_pixel_returns_false();
void test_null_tile_returns_false();
void test_non_16x16_tile_formula();

// Erosion tests
void test_erode_isolated_pixel_returns_false();
void test_erode_solid_block_core_stays_solid();
void test_erode_tile_edge_pixel_false();

// isWorldPixelSolid tests
void test_world_pixel_flag_not_set_short_circuits();
void test_world_pixel_solid_flag_transparent_pixel();
void test_world_pixel_solid_flag_opaque_pixel();
void test_world_pixel_erode_isolated_pixel();
void test_world_pixel_out_of_bounds_tile_coords();
