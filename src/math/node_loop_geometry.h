/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_NODE_LOOP_GEOMETRY_H
#define ROHR_NODE_LOOP_GEOMETRY_H
#include "math2d.h"
#include <stddef.h>
#include <stdint.h>

#define NODE_LOOP_MAX_POINTS 64
#define NODE_LOOP_MAX_LOOPS 17

typedef struct NodeLoopPoints {
    Vec2D points[NODE_LOOP_MAX_POINTS];
    size_t count;
} NodeLoopPoints;

typedef enum NodeLoopGeometryStatus {
    NODE_LOOP_GEOMETRY_OK,
    NODE_LOOP_GEOMETRY_INVALID_ARGUMENT,
    NODE_LOOP_GEOMETRY_INVALID_COUNT,
    NODE_LOOP_GEOMETRY_NONFINITE,
    NODE_LOOP_GEOMETRY_ALLOCATION_FAILED,
    NODE_LOOP_GEOMETRY_TOPOLOGY_FAILED
} NodeLoopGeometryStatus;

/* Owned triangle list (three world positions per triangle). */
typedef struct NodeLoopFill {
    Vec2D *vertices;
    size_t vertex_count, capacity;
    uint32_t enclosed_loops;
} NodeLoopFill;

typedef struct NodeLoopWorkspace NodeLoopWorkspace;
/* Explicit reusable allocation. The caller must destroy workspace and fills.
 * Build may grow workspace/output buffers; failure preserves the output.
 * No physics/entities. Loop zero is additive; remaining loops subtract their
 * bounded regions, independent of winding, repetition and hole order. */
NodeLoopWorkspace *node_loop_workspace_create(void);
void node_loop_workspace_destroy(NodeLoopWorkspace *workspace);
void node_loop_fill_destroy(NodeLoopFill *fill);
NodeLoopGeometryStatus node_loop_fill_build(const NodeLoopPoints *loops,
    size_t count, NodeLoopWorkspace *workspace, NodeLoopFill *output);
#endif
