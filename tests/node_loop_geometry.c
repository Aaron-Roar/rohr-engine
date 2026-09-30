/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "math/node_loop_geometry.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { if(!(condition)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
    return false; \
} } while(0)

static double cross_get(Vec2D a, Vec2D b, Vec2D c) {
    return ((double)b.x - a.x) * ((double)c.y - a.y) -
        ((double)b.y - a.y) * ((double)c.x - a.x);
}

static double polygon_area_get(const Vec2D *points, size_t count) {
    double area = 0;
    for(size_t i = 1; i + 1 < count; i += 1)
        area += cross_get(points[0], points[i], points[i + 1]);
    return area * 0.5;
}

static bool polygon_contains_check(const Vec2D *points, size_t count,
        double x, double y) {
    bool inside = false;
    for(size_t i = 0, j = count - 1; i < count; j = i++) {
        if((points[i].y > y) != (points[j].y > y) &&
                x < ((double)points[j].x - points[i].x) * (y - points[i].y) /
                    ((double)points[j].y - points[i].y) + points[i].x)
            inside = !inside;
    }
    return inside;
}

static bool triangulation_check(const Vec2D *points, size_t count) {
    NodeLoopTriangulation mesh = {0}, repeated = {0};
    unsigned edges[NODE_LOOP_MAX_POINTS][NODE_LOOP_MAX_POINTS] = {{0}};
    bool used[NODE_LOOP_MAX_POINTS] = {false};
    double expected_area = polygon_area_get(points, count), actual_area = 0;
    REQUIRE(node_loop_triangulation_get(points, count, &mesh) == NODE_LOOP_GEOMETRY_OK);
    REQUIRE(mesh.point_count == count && mesh.triangle_count == count - 2);
    REQUIRE(node_loop_triangulation_get(points, count, &repeated) == NODE_LOOP_GEOMETRY_OK);
    REQUIRE(memcmp(mesh.triangles, repeated.triangles, sizeof(mesh.triangles)) == 0);
    for(size_t i = 0; i < mesh.triangle_count; i += 1) {
        Vec2D triangle[3];
        REQUIRE(node_loop_triangle_get(&mesh, points, count, i, triangle) == NODE_LOOP_GEOMETRY_OK);
        double area = cross_get(triangle[0], triangle[1], triangle[2]) * 0.5;
        REQUIRE(area * expected_area > 0);
        actual_area += area;
        REQUIRE(polygon_contains_check(points, count,
            ((double)triangle[0].x + triangle[1].x + triangle[2].x) / 3.0,
            ((double)triangle[0].y + triangle[1].y + triangle[2].y) / 3.0));
        for(size_t j = 0; j < 3; j += 1) {
            uint32_t a = mesh.triangles[i][j], b = mesh.triangles[i][(j + 1) % 3];
            REQUIRE(a < count && b < count && a != b);
            REQUIRE(triangle[j].x == points[a].x && triangle[j].y == points[a].y);
            used[a] = true;
            edges[a][b] += 1;
        }
    }
    REQUIRE(fabs(actual_area - expected_area) <= fabs(expected_area) * 1e-10);
    for(size_t a = 0; a < count; a += 1) {
        REQUIRE(used[a]);
        for(size_t b = a + 1; b < count; b += 1) {
            if(b == a + 1 || (a == 0 && b == count - 1))
                REQUIRE(edges[a][b] + edges[b][a] == 1);
            else REQUIRE(edges[a][b] == edges[b][a] && edges[a][b] <= 1);
        }
    }
    return true;
}

static bool permutations_check(const Vec2D *points, size_t count) {
    Vec2D changed[NODE_LOOP_MAX_POINTS];
    for(size_t start = 0; start < count; start += 1) {
        for(size_t winding = 0; winding < 2; winding += 1) {
            for(size_t i = 0; i < count; i += 1)
                changed[i] = points[(start + (winding ? count - i : i)) % count];
            REQUIRE(triangulation_check(changed, count));
        }
    }
    return true;
}

static bool valid_loops_check(void) {
    const Vec2D triangle[] = {{0, 0}, {5, 0}, {2, 3}};
    const Vec2D square[] = {{0, 0}, {4, 0}, {4, 4}, {0, 4}};
    const Vec2D concave[] = {{0, 0}, {6, 0}, {6, 6}, {4, 6},
        {4, 2}, {2, 2}, {2, 6}, {0, 6}};
    const Vec2D subdivided[] = {{0, 0}, {2, 0}, {4, 0}, {4, 2},
        {4, 4}, {2, 4}, {0, 4}, {0, 2}};
    const Vec2D concave_collinear[] = {{0, 0}, {4, 0}, {4, 1}, {2, 1},
        {1, 1}, {1, 4}, {0, 4}};
    REQUIRE(permutations_check(triangle, 3));
    REQUIRE(permutations_check(square, 4));
    REQUIRE(permutations_check(concave, 8));
    REQUIRE(permutations_check(subdivided, 8));
    REQUIRE(permutations_check(concave_collinear, 7));
    const float scales[] = {1e-18f, 1e-6f, 1, 1e18f, 1e30f};
    for(size_t scale = 0; scale < sizeof(scales) / sizeof(scales[0]); scale += 1) {
        Vec2D transformed[8];
        for(size_t i = 0; i < 8; i += 1)
            transformed[i] = (Vec2D){concave[i].x * scales[scale],
                concave[i].y * scales[scale]};
        REQUIRE(triangulation_check(transformed, 8));
    }
    Vec2D translated[8];
    for(size_t i = 0; i < 8; i += 1)
        translated[i] = (Vec2D){concave[i].x + 1000000, concave[i].y - 1000000};
    REQUIRE(triangulation_check(translated, 8));
    const Vec2D thin[] = {{0, 0}, {1, 0}, {1, 1e-12f}, {0, 1e-12f}};
    REQUIRE(permutations_check(thin, 4));
    const Vec2D extreme[] = {{-FLT_MAX, -FLT_MAX}, {FLT_MAX, -FLT_MAX},
        {FLT_MAX, FLT_MAX}, {-FLT_MAX, FLT_MAX}};
    REQUIRE(triangulation_check(extreme, 4));
    return true;
}

static bool rejected_loop_check(const Vec2D *points, size_t count,
        NodeLoopGeometryStatus status) {
    NodeLoopTriangulation mesh, saved;
    memset(&mesh, 0xa5, sizeof(mesh));
    memcpy(&saved, &mesh, sizeof(mesh));
    REQUIRE(node_loop_triangulation_get(points, count, &mesh) == status);
    REQUIRE(memcmp(&saved, &mesh, sizeof(mesh)) == 0);
    return true;
}

static bool invalid_loops_check(void) {
    const Vec2D square[] = {{0, 0}, {4, 0}, {4, 4}, {0, 4}};
    const Vec2D duplicate[] = {{0, 0}, {4, 0}, {4, 0}, {0, 4}};
    const Vec2D closed[] = {{0, 0}, {4, 0}, {4, 4}, {0, 0}};
    const Vec2D line[] = {{0, 0}, {1, 1}, {2, 2}};
    const Vec2D backtrack[] = {{0, 0}, {4, 0}, {2, 0}, {4, 4}, {0, 4}};
    const Vec2D crossing[] = {{0, 0}, {4, 4}, {0, 4}, {4, 0}};
    const Vec2D crossing_nonzero[] = {{0, 0}, {6, 4}, {0, 4}, {3, 0}};
    const Vec2D touching[] = {{0, 0}, {4, 0}, {4, 4}, {2, 0}, {0, 4}};
    const Vec2D overlapping[] = {{0, 0}, {4, 0}, {4, 4}, {1, 0}, {3, 0}, {0, 4}};
    REQUIRE(rejected_loop_check(NULL, 4, NODE_LOOP_GEOMETRY_INVALID_ARGUMENT));
    REQUIRE(node_loop_triangulation_get(square, 4, NULL) == NODE_LOOP_GEOMETRY_INVALID_ARGUMENT);
    REQUIRE(rejected_loop_check(square, 0, NODE_LOOP_GEOMETRY_INVALID_COUNT));
    REQUIRE(rejected_loop_check(square, 2, NODE_LOOP_GEOMETRY_INVALID_COUNT));
    REQUIRE(rejected_loop_check(square, NODE_LOOP_MAX_POINTS + 1, NODE_LOOP_GEOMETRY_INVALID_COUNT));
    REQUIRE(rejected_loop_check(duplicate, 4, NODE_LOOP_GEOMETRY_DUPLICATE_POINT));
    REQUIRE(rejected_loop_check(closed, 4, NODE_LOOP_GEOMETRY_DUPLICATE_POINT));
    REQUIRE(rejected_loop_check(line, 3, NODE_LOOP_GEOMETRY_DEGENERATE));
    REQUIRE(rejected_loop_check(backtrack, 5, NODE_LOOP_GEOMETRY_DEGENERATE));
    REQUIRE(rejected_loop_check(crossing, 4, NODE_LOOP_GEOMETRY_SELF_INTERSECTION));
    REQUIRE(rejected_loop_check(crossing_nonzero, 4, NODE_LOOP_GEOMETRY_SELF_INTERSECTION));
    REQUIRE(rejected_loop_check(touching, 5, NODE_LOOP_GEOMETRY_SELF_INTERSECTION));
    REQUIRE(rejected_loop_check(overlapping, 6, NODE_LOOP_GEOMETRY_SELF_INTERSECTION));
    for(size_t i = 0; i < 4; i += 1) {
        Vec2D changed[4];
        memcpy(changed, square, sizeof(changed));
        changed[i].x = NAN;
        REQUIRE(rejected_loop_check(changed, 4, NODE_LOOP_GEOMETRY_NONFINITE));
        changed[i].x = square[i].x; changed[i].y = INFINITY;
        REQUIRE(rejected_loop_check(changed, 4, NODE_LOOP_GEOMETRY_NONFINITE));
    }
    return true;
}

static bool deformation_check(void) {
    const Vec2D square[] = {{0, 0}, {4, 0}, {4, 4}, {0, 4}};
    NodeLoopTriangulation mesh, saved;
    REQUIRE(node_loop_triangulation_get(square, 4, &mesh) == NODE_LOOP_GEOMETRY_OK);
    memcpy(&saved, &mesh, sizeof(mesh));
    const Vec2D poses[][4] = {
        {{0, 0}, {4, 4}, {4, 0}, {0, 4}}, /* Self-intersection after authoring. */
        {{0, 0}, {-4, 0}, {-4, 4}, {0, 4}}, /* Winding inversion. */
        {{0, 0}, {1, 0}, {2, 0}, {3, 0}}, /* Fully collapsed. */
        {{9, 7}, {9, 7}, {9, 7}, {9, 7}}
    };
    for(size_t pose = 0; pose < sizeof(poses) / sizeof(poses[0]); pose += 1)
        for(size_t t = 0; t < mesh.triangle_count; t += 1) {
            Vec2D triangle[3];
            REQUIRE(node_loop_triangle_get(&mesh, poses[pose], 4, t, triangle) == NODE_LOOP_GEOMETRY_OK);
            for(size_t v = 0; v < 3; v += 1) {
                Vec2D expected = poses[pose][saved.triangles[t][v]];
                REQUIRE(triangle[v].x == expected.x && triangle[v].y == expected.y);
            }
            REQUIRE(memcmp(&saved, &mesh, sizeof(mesh)) == 0);
        }
    Vec2D output[3] = {{17, 18}, {19, 20}, {21, 22}}, previous[3];
    memcpy(previous, output, sizeof(output));
    REQUIRE(node_loop_triangle_get(NULL, square, 4, 0, output) == NODE_LOOP_GEOMETRY_INVALID_ARGUMENT);
    REQUIRE(node_loop_triangle_get(&mesh, NULL, 4, 0, output) == NODE_LOOP_GEOMETRY_INVALID_ARGUMENT);
    REQUIRE(node_loop_triangle_get(&mesh, square, 4, 0, NULL) == NODE_LOOP_GEOMETRY_INVALID_ARGUMENT);
    REQUIRE(node_loop_triangle_get(&mesh, square, 3, 0, output) == NODE_LOOP_GEOMETRY_INVALID_COUNT);
    REQUIRE(node_loop_triangle_get(&mesh, square, 4, 2, output) == NODE_LOOP_GEOMETRY_INVALID_COUNT);
    mesh.triangles[0][2] = 4;
    REQUIRE(node_loop_triangle_get(&mesh, square, 4, 0, output) == NODE_LOOP_GEOMETRY_INVALID_ARGUMENT);
    mesh = saved;
    Vec2D invalid[4];
    memcpy(invalid, square, sizeof(invalid));
    invalid[mesh.triangles[0][2]].y = NAN;
    REQUIRE(node_loop_triangle_get(&mesh, invalid, 4, 0, output) == NODE_LOOP_GEOMETRY_NONFINITE);
    mesh.triangle_count = NODE_LOOP_MAX_POINTS;
    REQUIRE(node_loop_triangle_get(&mesh, square, 4, 0, output) == NODE_LOOP_GEOMETRY_INVALID_COUNT);
    REQUIRE(memcmp(previous, output, sizeof(output)) == 0);
    return true;
}

static bool generated_loops_check(void) {
    uint32_t seed = UINT32_C(0x71ad902b);
    Vec2D points[NODE_LOOP_MAX_POINTS];
    for(size_t sample = 0; sample < 200; sample += 1) {
        size_t count = 3 + sample % (NODE_LOOP_MAX_POINTS - 2);
        for(size_t i = 0; i < count; i += 1) {
            seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
            double radius = 0.5 + (double)(seed % 10000) / 10000;
            double angle = 6.283185307179586 * (double)i / (double)count;
            points[i] = (Vec2D){(float)(cos(angle) * radius),
                (float)(sin(angle) * radius)};
        }
        REQUIRE(triangulation_check(points, count));
    }
    return true;
}

int main(void) {
    return valid_loops_check() && invalid_loops_check() && deformation_check() &&
        generated_loops_check() ? 0 : 1;
}
