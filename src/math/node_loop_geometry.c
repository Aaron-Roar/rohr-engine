/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "node_loop_geometry.h"

#include <float.h>
#include <math.h>
#include <string.h>

static int node_loop_orientation_get(Vec2D a, Vec2D b, Vec2D c) {
    double left = ((double)b.x - a.x) * ((double)c.y - a.y);
    double right = ((double)b.y - a.y) * ((double)c.x - a.x);
    double cross = left - right;
    double tolerance = 8.0 * DBL_EPSILON * (fabs(left) + fabs(right));
    return cross > tolerance ? 1 : cross < -tolerance ? -1 : 0;
}

static bool node_loop_on_segment_check(Vec2D point, Vec2D a, Vec2D b) {
    return point.x >= fminf(a.x, b.x) && point.x <= fmaxf(a.x, b.x) &&
        point.y >= fminf(a.y, b.y) && point.y <= fmaxf(a.y, b.y);
}

static bool node_loop_segments_overlap_check(Vec2D a, Vec2D b,
        Vec2D c, Vec2D d) {
    int ab_c = node_loop_orientation_get(a, b, c);
    int ab_d = node_loop_orientation_get(a, b, d);
    int cd_a = node_loop_orientation_get(c, d, a);
    int cd_b = node_loop_orientation_get(c, d, b);
    return (ab_c * ab_d < 0 && cd_a * cd_b < 0) ||
        (ab_c == 0 && node_loop_on_segment_check(c, a, b)) ||
        (ab_d == 0 && node_loop_on_segment_check(d, a, b)) ||
        (cd_a == 0 && node_loop_on_segment_check(a, c, d)) ||
        (cd_b == 0 && node_loop_on_segment_check(b, c, d));
}

static NodeLoopGeometryStatus node_loop_winding_get(const Vec2D *points,
        size_t count, int *winding) {
    double area = 0, magnitude = 0;
    for(size_t i = 0; i < count; i += 1) {
        if(!isfinite(points[i].x) || !isfinite(points[i].y))
            return NODE_LOOP_GEOMETRY_NONFINITE;
        for(size_t j = 0; j < i; j += 1)
            if(points[i].x == points[j].x && points[i].y == points[j].y)
                return NODE_LOOP_GEOMETRY_DUPLICATE_POINT;
    }
    for(size_t i = 0; i < count; i += 1) {
        size_t next = (i + 1) % count;
        Vec2D a = points[i], b = points[next], c = points[(i + 2) % count];
        if(node_loop_orientation_get(a, b, c) == 0 &&
                (((double)b.x - a.x) * ((double)c.x - b.x) +
                 ((double)b.y - a.y) * ((double)c.y - b.y)) <= 0)
            return NODE_LOOP_GEOMETRY_DEGENERATE;
        for(size_t j = i + 1; j < count; j += 1) {
            size_t j_next = (j + 1) % count;
            if(next == j || j_next == i) continue;
            if(node_loop_segments_overlap_check(a, b, points[j], points[j_next]))
                return NODE_LOOP_GEOMETRY_SELF_INTERSECTION;
        }
        /* Translate before multiplying to avoid cancellation far from zero. */
        double left = ((double)a.x - points[0].x) * ((double)b.y - points[0].y);
        double right = ((double)a.y - points[0].y) * ((double)b.x - points[0].x);
        area += left - right;
        magnitude += fabs(left) + fabs(right);
    }
    if(fabs(area) <= 8.0 * (double)count * DBL_EPSILON * magnitude)
        return NODE_LOOP_GEOMETRY_DEGENERATE;
    *winding = area > 0 ? 1 : -1;
    return NODE_LOOP_GEOMETRY_OK;
}

static bool node_loop_triangle_contains_check(Vec2D point,
        Vec2D a, Vec2D b, Vec2D c, int winding) {
    /* Include the boundary: clipping across another authored point would lose
     * that point or leave a degenerate triangle at the end. */
    return node_loop_orientation_get(a, b, point) * winding >= 0 &&
        node_loop_orientation_get(b, c, point) * winding >= 0 &&
        node_loop_orientation_get(c, a, point) * winding >= 0;
}

NodeLoopGeometryStatus node_loop_triangulation_get(const Vec2D *points,
        size_t point_count, NodeLoopTriangulation *output) {
    NodeLoopTriangulation mesh = {0};
    uint32_t remaining[NODE_LOOP_MAX_POINTS];
    size_t count = point_count;
    int winding;
    if(points == NULL || output == NULL) return NODE_LOOP_GEOMETRY_INVALID_ARGUMENT;
    if(point_count < 3 || point_count > NODE_LOOP_MAX_POINTS)
        return NODE_LOOP_GEOMETRY_INVALID_COUNT;
    NodeLoopGeometryStatus status = node_loop_winding_get(points, point_count, &winding);
    if(status != NODE_LOOP_GEOMETRY_OK) return status;
    mesh.point_count = point_count;
    for(size_t i = 0; i < count; i += 1) remaining[i] = (uint32_t)i;
    while(count > 3) {
        bool clipped = false;
        for(size_t i = 0; i < count; i += 1) {
            uint32_t a = remaining[(i + count - 1) % count];
            uint32_t b = remaining[i], c = remaining[(i + 1) % count];
            bool contains = false;
            if(node_loop_orientation_get(points[a], points[b], points[c]) * winding <= 0)
                continue;
            for(size_t j = 0; j < count; j += 1) {
                uint32_t candidate = remaining[j];
                if(candidate == a || candidate == b || candidate == c) continue;
                if(node_loop_triangle_contains_check(points[candidate],
                        points[a], points[b], points[c], winding)) {
                    contains = true;
                    break;
                }
            }
            if(contains) continue;
            uint32_t *triangle = mesh.triangles[mesh.triangle_count++];
            triangle[0] = a; triangle[1] = b; triangle[2] = c;
            memmove(&remaining[i], &remaining[i + 1], (count - i - 1) * sizeof(*remaining));
            count -= 1;
            clipped = true;
            break;
        }
        if(!clipped) return NODE_LOOP_GEOMETRY_TRIANGULATION_FAILED;
    }
    if(node_loop_orientation_get(points[remaining[0]], points[remaining[1]],
            points[remaining[2]]) * winding <= 0)
        return NODE_LOOP_GEOMETRY_TRIANGULATION_FAILED;
    memcpy(mesh.triangles[mesh.triangle_count++], remaining, 3 * sizeof(*remaining));
    *output = mesh;
    return NODE_LOOP_GEOMETRY_OK;
}

NodeLoopGeometryStatus node_loop_triangle_get(const NodeLoopTriangulation *mesh,
        const Vec2D *points, size_t point_count, size_t triangle_index,
        Vec2D output[3]) {
    Vec2D triangle[3];
    if(mesh == NULL || points == NULL || output == NULL)
        return NODE_LOOP_GEOMETRY_INVALID_ARGUMENT;
    if(point_count < 3 || point_count > NODE_LOOP_MAX_POINTS ||
            mesh->point_count != point_count || mesh->triangle_count != point_count - 2 ||
            triangle_index >= mesh->triangle_count)
        return NODE_LOOP_GEOMETRY_INVALID_COUNT;
    for(size_t i = 0; i < 3; i += 1) {
        uint32_t index = mesh->triangles[triangle_index][i];
        if(index >= point_count) return NODE_LOOP_GEOMETRY_INVALID_ARGUMENT;
        triangle[i] = points[index];
        if(!isfinite(triangle[i].x) || !isfinite(triangle[i].y))
            return NODE_LOOP_GEOMETRY_NONFINITE;
    }
    memcpy(output, triangle, sizeof(triangle));
    return NODE_LOOP_GEOMETRY_OK;
}
