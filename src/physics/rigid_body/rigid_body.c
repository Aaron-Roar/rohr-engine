/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics.h"
#include "physics/physics_internal.h"
#include <float.h>

#include <math.h>

Shape physics_shape_world_translate(
    Shape shape,
    Position position,
    Orientation angle
) {
    Shape world_shape = shape;
    float cosine = cosf(angle);
    float sine = sinf(angle);

    for(uint16_t i = 0; i < shape.amount_of_vertices; i += 1) {
        float x = shape.vertices[i].x;
        float y = shape.vertices[i].y;

        world_shape.vertices[i] = (Position){
            position.x + x * cosine - y * sine,
            position.y + x * sine + y * cosine
        };
    }
    return world_shape;
}

float physics_polygon_moment_of_inertia(Shape shape, Mass mass_value) {
    Position centroid;
    double unit_inertia;
    if(!isfinite(mass_value) || mass_value < 0 ||
            !physics_shape_mass_properties_get(&shape, &centroid, &unit_inertia)) return 0;
    double inertia = unit_inertia * mass_value;
    return isfinite(inertia) && inertia <= FLT_MAX ? (float)inertia : 0;
}
