/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_NODE_LOOP_GEOMETRY_H
#define ROHR_NODE_LOOP_GEOMETRY_H

#include "math2d.h"
#include <stddef.h>

/* Internal geometry shared by future runtime and editor area authoring.
 * No entity handles, beam connectivity, allocation, or physics dependencies. */
#define NODE_LOOP_MAX_POINTS MAX_VERTICIES

typedef enum NodeLoopGeometryStatus {
    NODE_LOOP_GEOMETRY_OK,
    NODE_LOOP_GEOMETRY_INVALID_ARGUMENT,
    NODE_LOOP_GEOMETRY_INVALID_COUNT,
    NODE_LOOP_GEOMETRY_NONFINITE,
    NODE_LOOP_GEOMETRY_DUPLICATE_POINT,
    NODE_LOOP_GEOMETRY_DEGENERATE,
    NODE_LOOP_GEOMETRY_SELF_INTERSECTION,
    NODE_LOOP_GEOMETRY_TRIANGULATION_FAILED
} NodeLoopGeometryStatus;

typedef struct NodeLoopTriangulation {
    size_t point_count;
    size_t triangle_count;
    uint32_t triangles[NODE_LOOP_MAX_POINTS - 2][3];
} NodeLoopTriangulation;

/* Validate a simple, nonzero initial polygon and triangulate in its supplied
 * winding. The closing edge is implicit: do not repeat the first point.
 * Accepts concave loops and straight collinear boundary subdivisions; rejects
 * repeated positions, crossing/touching nonadjacent edges, and backtracking.
 * Geometry indistinguishable from collinear at double precision is treated as
 * collinear, using relative error bounds rather than a world-unit epsilon.
 * Output indices always address the original array; every point participates.
 * The caller owns output. No allocation. Failure leaves output unchanged.
 * Build only when authoring changes, then retain indices throughout motion. */
NodeLoopGeometryStatus node_loop_triangulation_get(const Vec2D *points,
    size_t point_count, NodeLoopTriangulation *output);

/* Resolve one stored triangle against current positions in the SAME node order.
 * Does not revalidate or retriangulate the boundary. Folds, inverted triangles,
 * and collapsed triangles are valid poses. The resolved positions must be finite.
 * No allocation; failure leaves output unchanged. */
NodeLoopGeometryStatus node_loop_triangle_get(const NodeLoopTriangulation *mesh,
    const Vec2D *points, size_t point_count, size_t triangle_index,
    Vec2D output[3]);

#endif
