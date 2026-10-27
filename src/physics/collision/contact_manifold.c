/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "contact_manifold.h"
#include "overlap_geometry.h"
#include "physics.h"
#include "shape_decomposition.h"

#include <float.h>
#include <math.h>

typedef struct ContactFace {
    Vec2D first;
    Vec2D second;
    Axis normal;
    float alignment;
} ContactFace;

static bool contact_face_extension_check(
    Vec2D first,
    Vec2D second,
    Vec2D added,
    Vec2D origin,
    double direction_x,
    double direction_y
) {
    double edge_x = (double)second.x - first.x;
    double edge_y = (double)second.y - first.y;
    double length = hypot(edge_x, edge_y);
    double distance = direction_x * ((double)added.y - origin.y) -
        direction_y * ((double)added.x - origin.x);

    if(length <= 0.00001) return false;
    edge_x /= length;
    edge_y /= length;
    return direction_x * edge_x + direction_y * edge_y > 0.0 &&
        fabs(direction_x * edge_y - direction_y * edge_x) <= 8.0 * FLT_EPSILON &&
        fabs(distance) <= 0.001f;
}

/* Clipping needs the entire supporting face, even when authored vertices split
 * it into several edges. Compare every extension with the original edge so
 * small turns cannot accumulate into a curved face. Geometry stays untouched. */
static ContactFace contact_face_expanded_get(
    const Shape *shape,
    uint16_t edge,
    ContactFace face
) {
    uint16_t count = shape->amount_of_vertices;
    uint16_t first = edge;
    uint16_t second = (uint16_t)((edge + 1) % count);
    uint16_t included = 1;
    Vec2D origin = face.first;
    double direction_x = (double)face.second.x - origin.x;
    double direction_y = (double)face.second.y - origin.y;
    double length = hypot(direction_x, direction_y);

    if(length <= 0.00001) return face;
    direction_x /= length;
    direction_y /= length;
    while(included < count) {
        uint16_t previous = (uint16_t)((first + count - 1) % count);
        Vec2D point = shape->vertices[previous];
        if(!contact_face_extension_check(point, shape->vertices[first], point,
                origin, direction_x, direction_y)) break;
        first = previous;
        included += 1;
    }
    while(included < count) {
        uint16_t next = (uint16_t)((second + 1) % count);
        Vec2D point = shape->vertices[next];
        if(!contact_face_extension_check(shape->vertices[second], point, point,
                origin, direction_x, direction_y)) break;
        second = next;
        included += 1;
    }
    face.first = shape->vertices[first];
    face.second = shape->vertices[second];
    return face;
}

static ContactFace contact_face_best_get(Shape shape, Axis direction) {
    ContactFace result = {.alignment = -FLT_MAX};
    Vec2D center = math_polygon_centroid(shape);
    uint16_t selected = 0;

    for(uint16_t i = 0; i < shape.amount_of_vertices; i += 1) {
        Vec2D first = shape.vertices[i];
        Vec2D second = shape.vertices[(i + 1) % shape.amount_of_vertices];
        Vec2D edge = math_vector_subtract(second, first);
        float length = math_vector_magnitude(edge);
        Vec2D midpoint;
        Axis normal;
        float alignment;

        if(length <= 0.00001f) continue;
        normal = (Axis){edge.y / length, -edge.x / length};
        midpoint = (Vec2D){
            (first.x + second.x) * 0.5f,
            (first.y + second.y) * 0.5f
        };
        if(math_dot_product(normal, math_vector_subtract(midpoint, center)) < 0.0f) {
            normal.x = -normal.x;
            normal.y = -normal.y;
        }
        alignment = math_dot_product(normal, direction);
        if(alignment > result.alignment) {
            result = (ContactFace){first, second, normal, alignment};
            selected = i;
        }
    }
    return contact_face_expanded_get(&shape, selected, result);
}

static ContactFace contact_face_incident_get(Shape shape, Axis reference_normal) {
    ContactFace result = {.alignment = FLT_MAX};
    Vec2D center = math_polygon_centroid(shape);
    uint16_t selected = 0;

    for(uint16_t i = 0; i < shape.amount_of_vertices; i += 1) {
        Vec2D first = shape.vertices[i];
        Vec2D second = shape.vertices[(i + 1) % shape.amount_of_vertices];
        Vec2D edge = math_vector_subtract(second, first);
        float length = math_vector_magnitude(edge);
        Vec2D midpoint;
        Axis normal;
        float alignment;

        if(length <= 0.00001f) continue;
        normal = (Axis){edge.y / length, -edge.x / length};
        midpoint = (Vec2D){
            (first.x + second.x) * 0.5f,
            (first.y + second.y) * 0.5f
        };
        if(math_dot_product(normal, math_vector_subtract(midpoint, center)) < 0.0f) {
            normal.x = -normal.x;
            normal.y = -normal.y;
        }
        alignment = math_dot_product(normal, reference_normal);
        if(alignment < result.alignment) {
            result = (ContactFace){first, second, normal, alignment};
            selected = i;
        }
    }
    return contact_face_expanded_get(&shape, selected, result);
}

static uint8_t contact_segment_clip(
    Vec2D input[2],
    Vec2D output[2],
    Axis normal,
    float offset
) {
    float first_distance = math_dot_product(normal, input[0]) - offset;
    float second_distance = math_dot_product(normal, input[1]) - offset;
    uint8_t count = 0;

    if(first_distance <= 0.0f) output[count++] = input[0];
    if(second_distance <= 0.0f) output[count++] = input[1];
    if(first_distance * second_distance < 0.0f && count < 2) {
        float amount = first_distance / (first_distance - second_distance);
        output[count++] = (Vec2D){
            input[0].x + (input[1].x - input[0].x) * amount,
            input[0].y + (input[1].y - input[0].y) * amount
        };
    }
    return count;
}

static ContactManifold contact_manifold_convex_get(
    Shape first,
    Shape second,
    Axis normal
) {
    ContactManifold manifold = {.normal = normal};
    ContactFace first_face;
    ContactFace second_face;
    ContactFace reference;
    ContactFace incident;
    Vec2D tangent;
    float tangent_length;
    Vec2D clipped_first[2];
    Vec2D clipped_second[2];
    uint8_t count;

    if(first.amount_of_vertices < 2 || second.amount_of_vertices < 2) return manifold;
    first_face = contact_face_best_get(first, normal);
    second_face = contact_face_best_get(second, (Axis){-normal.x, -normal.y});
    if(first_face.alignment >= second_face.alignment) {
        reference = first_face;
        incident = contact_face_incident_get(second, reference.normal);
    } else {
        reference = second_face;
        incident = contact_face_incident_get(first, reference.normal);
    }
    tangent = math_vector_subtract(reference.second, reference.first);
    tangent_length = math_vector_magnitude(tangent);
    if(tangent_length <= 0.00001f) return manifold;
    tangent.x /= tangent_length;
    tangent.y /= tangent_length;
    clipped_first[0] = incident.first;
    clipped_first[1] = incident.second;
    count = contact_segment_clip(
        clipped_first,
        clipped_second,
        (Axis){-tangent.x, -tangent.y},
        -math_dot_product(tangent, reference.first));
    if(count < 2) return manifold;
    count = contact_segment_clip(
        clipped_second,
        clipped_first,
        tangent,
        math_dot_product(tangent, reference.second));
    for(uint8_t i = 0; i < count && manifold.count < CONTACT_MANIFOLD_POINT_MAX; i += 1) {
        float separation = math_dot_product(
            reference.normal,
            math_vector_subtract(clipped_first[i], reference.first));

        if(separation > 0.001f) continue;
        manifold.point_depths[manifold.count] = -separation;
        manifold.points[manifold.count++] = (Vec2D){
            clipped_first[i].x - reference.normal.x * separation * 0.5f,
            clipped_first[i].y - reference.normal.y * separation * 0.5f
        };
    }
    return manifold;
}

static void contact_manifold_point_add(
    ContactManifold *manifold,
    Vec2D point,
    float depth,
    uint32_t feature
) {
    Axis tangent;
    float projections[CONTACT_MANIFOLD_POINT_MAX];
    float projection;

    if(manifold == NULL) return;
    tangent = (Axis){-manifold->normal.y, manifold->normal.x};
    projection = math_dot_product(point, tangent);
    for(uint8_t i = 0; i < manifold->count; i += 1) {
        Vec2D delta = math_vector_subtract(point, manifold->points[i]);
        if(math_dot_product(delta, delta) <= 0.000001f) {
            manifold->point_depths[i] = fmaxf(manifold->point_depths[i], depth);
            return;
        }
        projections[i] = math_dot_product(manifold->points[i], tangent);
    }
    if(manifold->count < CONTACT_MANIFOLD_POINT_MAX) {
        manifold->points[manifold->count] = point;
        manifold->point_depths[manifold->count] = depth;
        manifold->feature_ids[manifold->count] = feature;
        manifold->count += 1;
        return;
    }
    if(projection < projections[0] && projections[0] <= projections[1]) {
        manifold->points[0] = point;
        manifold->point_depths[0] = depth;
        manifold->feature_ids[0] = feature;
    } else if(projection < projections[1] && projections[1] < projections[0]) {
        manifold->points[1] = point;
        manifold->point_depths[1] = depth;
        manifold->feature_ids[1] = feature;
    } else if(projection > projections[0] && projections[0] >= projections[1]) {
        manifold->points[0] = point;
        manifold->point_depths[0] = depth;
        manifold->feature_ids[0] = feature;
    } else if(projection > projections[1] && projections[1] > projections[0]) {
        manifold->points[1] = point;
        manifold->point_depths[1] = depth;
        manifold->feature_ids[1] = feature;
    }
}

ContactManifoldSet contact_manifold_set_polygon_get(Shape first, Shape second) {
    ContactManifoldSet set = {0};
    uint8_t first_count;
    uint8_t second_count;

    if(!first.collision_geometry_prepared &&
            !physics_shape_collision_prepare(first, &first)) return set;
    if(!second.collision_geometry_prepared &&
            !physics_shape_collision_prepare(second, &second)) return set;
    first_count = physics_shape_collision_piece_count_get(&first);
    second_count = physics_shape_collision_piece_count_get(&second);
    for(uint8_t first_index = 0; first_index < first_count; first_index += 1) {
        Shape first_piece = physics_shape_collision_piece_get(&first, first_index);
        for(uint8_t second_index = 0; second_index < second_count; second_index += 1) {
            Shape second_piece = physics_shape_collision_piece_get(
                &second, second_index);
            OverlapInfo overlap = physics_sat_collision_piece_overlap_get(
                &first, first_index, &second, second_index);
            ContactManifold candidate;
            ContactManifold *target = NULL;

            if(!overlap.detected) continue;
            candidate = contact_manifold_convex_get(
                first_piece, second_piece, overlap.normal);
            if(candidate.count == 0) continue;
            for(uint8_t i = 0; i < set.count; i += 1) {
                if(math_dot_product(set.values[i].normal, overlap.normal) >= 0.98f) {
                    target = &set.values[i];
                    break;
                }
            }
            if(target == NULL) {
                if(set.count >= CONTACT_MANIFOLD_MAX) continue;
                target = &set.values[set.count++];
                target->normal = overlap.normal;
            }
            if(overlap.depth > target->depth) target->depth = overlap.depth;
            for(uint8_t point = 0; point < candidate.count; point += 1) {
                uint32_t feature = ((uint32_t)first_index << 16) |
                    ((uint32_t)second_index << 8) | (uint32_t)(point + 1);
                contact_manifold_point_add(
                    target, candidate.points[point], candidate.point_depths[point], feature);
            }
        }
    }
    return set;
}

ContactManifold contact_manifold_polygon_get(
    Shape first,
    Shape second,
    Axis normal
) {
    ContactManifold merged = {.normal = normal};
    Position minimum_point = {0};
    Position maximum_point = {0};
    uint32_t minimum_feature = 0;
    uint32_t maximum_feature = 0;
    float minimum_depth = 0, maximum_depth = 0;
    Axis tangent = {-normal.y, normal.x};
    float minimum_projection = FLT_MAX;
    float maximum_projection = -FLT_MAX;
    uint8_t first_count;
    uint8_t second_count;

    if(!first.collision_geometry_prepared &&
            !physics_shape_collision_prepare(first, &first)) return merged;
    if(!second.collision_geometry_prepared &&
            !physics_shape_collision_prepare(second, &second)) return merged;
    first_count = physics_shape_collision_piece_count_get(&first);
    second_count = physics_shape_collision_piece_count_get(&second);
    for(uint8_t first_index = 0; first_index < first_count; first_index += 1) {
        Shape first_piece = physics_shape_collision_piece_get(&first, first_index);
        for(uint8_t second_index = 0; second_index < second_count; second_index += 1) {
            Shape second_piece = physics_shape_collision_piece_get(
                &second, second_index);
            OverlapInfo overlap = physics_sat_collision_piece_overlap_get(
                &first, first_index, &second, second_index);
            ContactManifold candidate;
            if(!overlap.detected ||
                    math_dot_product(overlap.normal, normal) < 0.98f) continue;
            candidate = contact_manifold_convex_get(
                first_piece, second_piece, normal);
            for(uint8_t point = 0; point < candidate.count; point += 1) {
                float projection = math_dot_product(candidate.points[point], tangent);
                uint32_t feature = ((uint32_t)first_index << 16) |
                    ((uint32_t)second_index << 8) | (uint32_t)(point + 1);
                if(projection < minimum_projection) {
                    minimum_projection = projection;
                    minimum_point = candidate.points[point];
                    minimum_feature = feature;
                    minimum_depth = candidate.point_depths[point];
                }
                if(projection > maximum_projection) {
                    maximum_projection = projection;
                    maximum_point = candidate.points[point];
                    maximum_feature = feature;
                    maximum_depth = candidate.point_depths[point];
                }
            }
        }
    }
    if(minimum_projection == FLT_MAX) return merged;
    merged.points[merged.count++] = minimum_point;
    merged.feature_ids[0] = minimum_feature;
    merged.point_depths[0] = minimum_depth;
    if(maximum_projection - minimum_projection > 0.0001f) {
        merged.points[merged.count++] = maximum_point;
        merged.feature_ids[1] = maximum_feature;
        merged.point_depths[1] = maximum_depth;
    }
    return merged;
}
