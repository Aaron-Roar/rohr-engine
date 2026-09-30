/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef AREA_GEOMETRY_H
#define AREA_GEOMETRY_H
#include <stddef.h>
#include "math2d.h"

/* Persistent boundary corner. A node corner uses nodes[0]. A crossing uses
 * the current intersection of its two beams. Fractions describe the authored
 * boundary directions; they do not fix the crossing's current position.
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
    Vec2D (*edges)[2]; /* Exterior segments, when resolving a beam boundary. */
    size_t edge_count;
} AreaMesh;
/* These create functions allocate caller-owned output; destroy releases it.
 * Failure returns false with an empty output. Inputs are borrowed. */
bool area_faces_create(const AreaSegment *segments, size_t count, AreaFaces *out);
void area_faces_destroy(AreaFaces *faces);
bool area_mesh_create(const Vec2D *boundary, size_t count, AreaMesh *out);
bool area_boundary_position_get(const AreaSegment *segments, size_t count,
    AreaBoundaryPoint point, Vec2D *out);
bool area_boundary_mesh_create(const AreaSegment *segments, size_t count,
    const AreaBoundaryPoint *boundary, size_t boundary_count, AreaMesh *out);
/* Resolves definitions together, including newly enclosed regions contributed
 * by several boundaries. Caller provides one output per definition and owns
 * each output's allocations. Failure leaves all outputs empty. */
bool area_boundary_meshes_create(const AreaSegment *segments, size_t count,
    const AreaFace *definitions, size_t definition_count, AreaMesh *out);
bool area_mesh_contains_check(const AreaMesh *mesh, Vec2D point);
void area_mesh_destroy(AreaMesh *mesh);
bool area_polygon_contains_check(const Vec2D *boundary, size_t count, Vec2D p);
double area_mesh_overlap_get(const AreaMesh *a, const AreaMesh *b);
#endif
