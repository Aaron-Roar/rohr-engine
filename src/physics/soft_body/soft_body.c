/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics/soft_body/soft_body.h"

#include <math.h>

Shape soft_body_boundary_shape_create(
    Position start,
    Position end,
    float radius,
    float start_exclusion_radius,
    float end_exclusion_radius
) {
    Vec2D delta = math_vector_subtract(end, start);
    float length = math_vector_magnitude(delta);
    Shape shape = {0};
    Vec2D direction;
    Vec2D normal;
    float start_cut = 0.0f;
    float end_cut = 0.0f;
    const uint16_t cut_segments = 6;

#define ADD_LOCAL(x_value, y_value) do { \
    if(shape.amount_of_vertices >= MAX_VERTICIES) return (Shape){0}; \
    shape.vertices[shape.amount_of_vertices++] = (Vec2D){ \
        start.x + direction.x * (x_value) + normal.x * (y_value), \
        start.y + direction.y * (x_value) + normal.y * (y_value) \
    }; \
} while(0)

    if(length <= 0.0001f || radius <= 0.0f) return shape;
    direction = (Vec2D){delta.x / length, delta.y / length};
    normal = (Vec2D){-direction.y, direction.x};
    if(start_exclusion_radius > 0.0f) {
        if(start_exclusion_radius < radius) return shape;
        start_cut = sqrtf(fmaxf(0.0f,
            start_exclusion_radius * start_exclusion_radius - radius * radius));
    }
    if(end_exclusion_radius > 0.0f) {
        if(end_exclusion_radius < radius) return shape;
        end_cut = sqrtf(fmaxf(0.0f,
            end_exclusion_radius * end_exclusion_radius - radius * radius));
    }
    if(start_cut + end_cut >= length - 0.0001f) return shape;

    ADD_LOCAL(start_cut, radius);
    ADD_LOCAL(length - end_cut, radius);
    if(end_exclusion_radius > 0.0f) {
        float half_angle = asinf(fminf(1.0f, radius / end_exclusion_radius));
        for(uint16_t i = 1; i <= cut_segments; i += 1) {
            float angle = PI_F - half_angle + 2.0f * half_angle *
                (float)i / (float)cut_segments;
            ADD_LOCAL(length + cosf(angle) * end_exclusion_radius,
                sinf(angle) * end_exclusion_radius);
        }
    } else {
        ADD_LOCAL(length, -radius);
    }
    ADD_LOCAL(start_cut, -radius);
    if(start_exclusion_radius > 0.0f) {
        float half_angle = asinf(fminf(1.0f, radius / start_exclusion_radius));
        for(uint16_t i = 1; i < cut_segments; i += 1) {
            float angle = -half_angle + 2.0f * half_angle *
                (float)i / (float)cut_segments;
            ADD_LOCAL(cosf(angle) * start_exclusion_radius,
                sinf(angle) * start_exclusion_radius);
        }
    }
#undef ADD_LOCAL
    return shape;
}
