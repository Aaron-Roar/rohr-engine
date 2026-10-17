/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_MARKER_GEOMETRY_H
#define ROHR_MARKER_GEOMETRY_H
#include "object.h"
/* World-space filled polygons. The sink borrows each polygon for this call. */
typedef bool (*MarkerPolygonDraw)(const Position *points, size_t count, Color color, void *context);
typedef struct MarkerGeometry {
    ObjectKind kind;
    JointType joint;
    Position origin, endpoint_a, endpoint_b;
    Orientation orientation;
    float magnitude;
    ObjectVisuals visuals;
} MarkerGeometry;
bool graphics_marker_geometry_draw(const MarkerGeometry *geometry, MarkerPolygonDraw draw, void *context);
bool graphics_marker_entity_draw(Entity entity, ObjectKind kind, ObjectVisuals visuals);
#endif
