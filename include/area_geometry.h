/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef AREA_GEOMETRY_H
#define AREA_GEOMETRY_H
#include <stddef.h>
#include "math2d.h"

/* Persistent material corner. A node corner uses nodes[0]. A crossing uses
 * the mean of the two stored beam interpolants, even after deformation.
 * edge is the beam carrying the outgoing boundary segment; zero marks a
 * doubled internal seam joining boundary rings around holes. */
typedef struct AreaBoundaryPoint {
    uint32_t nodes[4];
    uint32_t beams[2];
    float fractions[2];
    uint32_t edge;
} AreaBoundaryPoint;
typedef struct AreaSegment {
    uint32_t id, node_a, node_b;
    Vec2D a, b;
} AreaSegment;
typedef struct AreaFace {
    AreaBoundaryPoint *points;
    size_t count;
} AreaFace;
typedef struct AreaFaces {
    AreaFace *items;
    size_t count;
} AreaFaces;
typedef struct AreaMesh {
    Vec2D (*triangles)[3];
    size_t count;
} AreaMesh;
/* These create functions allocate caller-owned output; destroy releases it.
 * Failure returns false with an empty output. Inputs are borrowed. */
bool area_faces_create(const AreaSegment *segments, size_t count, AreaFaces *out);
void area_faces_destroy(AreaFaces *faces);
bool area_mesh_create(const Vec2D *boundary, size_t count, AreaMesh *out);
void area_mesh_destroy(AreaMesh *mesh);
bool area_polygon_contains_check(const Vec2D *boundary, size_t count, Vec2D p);
double area_mesh_overlap_get(const AreaMesh *a, const AreaMesh *b);
#endif
