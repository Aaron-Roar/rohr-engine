/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics.h"
#include "overlap_geometry.h"
#include "shape_decomposition.h"

#include <float.h>
#include <math.h>

static bool physics_sat_piece_axes_apply(
    Shape first,
    Shape second,
    const Shape *source,
    uint8_t piece_index,
    Vec2D center_delta,
    OverlapInfo *overlap
) {
    Vec2DList axes = math_vectors_normalize(math_normals_create(first));

    for(uint8_t edge = 0; edge < axes.amount_of_vectors; edge += 1) {
        Axis axis = axes.vectors[edge];
        if(math_dot_product(axis, center_delta) < 0.0f) {
            axis.x = -axis.x;
            axis.y = -axis.y;
        }
        Projection first_projection = math_project_shape_on_axis(first, axis);
        Projection second_projection = math_project_shape_on_axis(second, axis);
        float depth = math_projection_overlap(first_projection, second_projection);
        if(depth <= 0.0f) return false;
        /* Intersection length is not separation distance when either interval
         * contains the other (notably a thin beam inside a particle). */
        depth = first_projection.max - second_projection.min;
        if(physics_shape_collision_piece_edge_boundary_check(
                source, piece_index, edge) && depth < overlap->depth) {
            overlap->depth = depth;
            overlap->normal = axis;
        }
    }
    return true;
}

OverlapInfo physics_sat_collision_piece_overlap_get(
    const Shape *first_source,
    uint8_t first_index,
    const Shape *second_source,
    uint8_t second_index
) {
    Shape first = physics_shape_collision_piece_get(first_source, first_index);
    Shape second = physics_shape_collision_piece_get(second_source, second_index);
    OverlapInfo overlap = {.detected = true, .depth = FLT_MAX};
    Vec2D center_delta;

    center_delta = math_vector_subtract(
        math_polygon_centroid(*second_source),
        math_polygon_centroid(*first_source));
    if(!physics_sat_piece_axes_apply(first, second, first_source, first_index,
            center_delta, &overlap) ||
            !physics_sat_piece_axes_apply(second, first, second_source,
                second_index, (Vec2D){-center_delta.x, -center_delta.y},
                &overlap) || overlap.depth == FLT_MAX)
        return (OverlapInfo){0};
    if(math_dot_product(center_delta, overlap.normal) < 0.0f) {
        overlap.normal.x *= -1.0f;
        overlap.normal.y *= -1.0f;
    }
    return overlap;
}

OverlapInfo physics_sat_overlap_get(Shape shape_1, Shape shape_2) {
    OverlapInfo best = {0};
    uint8_t first_count;
    uint8_t second_count;

    if(!shape_1.collision_geometry_prepared &&
            !physics_shape_collision_prepare(shape_1, &shape_1)) return best;
    if(!shape_2.collision_geometry_prepared &&
            !physics_shape_collision_prepare(shape_2, &shape_2)) return best;
    first_count = physics_shape_collision_piece_count_get(&shape_1);
    second_count = physics_shape_collision_piece_count_get(&shape_2);
    for(uint8_t first = 0; first < first_count; first += 1) {
        for(uint8_t second = 0; second < second_count; second += 1) {
            OverlapInfo overlap = physics_sat_collision_piece_overlap_get(
                &shape_1, first, &shape_2, second);
            if(!overlap.detected) continue;
            if(!best.detected || overlap.depth > best.depth) best = overlap;
        }
    }
    return best;
}
Position physics_approximate_contact_point(Position p1, Position p2)
{
    return (Position){
        .x = (p1.x + p2.x) * 0.5f,
        .y = (p1.y + p2.y) * 0.5f
    };
}
