/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics/physics_step_internal.h"
#include "physics/physics_operation.h"
#include <float.h>
#include <math.h>

typedef struct SweepQuery {
    Entity source;
    EntityIndex index;
    double duration;
    double earliest;
} SweepQuery;

static Vec2D sweep_displacement_get(EntityIndex index, double dt) {
    if(!physics_entity_movable_get(index)) return (Vec2D){0};
    return (Vec2D){
        (velocities[index].x + (accelerations[index].x + force_accelerations[index].x) * dt) * dt,
        (velocities[index].y + (accelerations[index].y + force_accelerations[index].y) * dt) * dt};
}

static AABB sweep_bounds_get(EntityIndex index, double dt) {
    AABB bounds = math_aabb_create(world_hit_boxes[index]);
    Vec2D move = sweep_displacement_get(index, dt);
    bounds.min_x += fminf(0, move.x);
    bounds.max_x += fmaxf(0, move.x);
    bounds.min_y += fminf(0, move.y);
    bounds.max_y += fmaxf(0, move.y);
    return bounds;
}

static void sweep_projection_get(const Shape *shape, double x, double y,
    double *low, double *high) {
    *low = DBL_MAX; *high = -DBL_MAX;
    for(uint32_t i = 0; i < shape->amount_of_vertices; ++i) {
        double p = shape->vertices[i].x * x + shape->vertices[i].y * y;
        *low = fmin(*low, p); *high = fmax(*high, p);
    }
}

static bool sweep_pair_visit(Entity target, void *context) {
    SweepQuery *query = context;
    EntityIndex other;
    if(target <= query->source || !entity_index_get(target, &other) ||
            !entity_index_components_check(other, ROHR_COLLISION) ||
            !physics_collision_response_between_check(query->source, target)) return true;
    EntityIndex first = query->index;
    if(!physics_entity_movable_get(first) && !physics_entity_movable_get(other)) return true;
    const Shape *shapes[] = {&world_hit_boxes[first], &world_hit_boxes[other]};
    /* A concave shape's hull is conservative for timing only; the ordinary
     * narrow phase still decides which pieces actually touch. */
    Vec2D a = sweep_displacement_get(first, query->duration);
    Vec2D b = sweep_displacement_get(other, query->duration);
    double enter = -DBL_MAX, leave = DBL_MAX, entry_speed = 0, margin = 0;
    double thickness = DBL_MAX;
    double minimum_overlap = DBL_MAX, closing_travel = 0;
    for(unsigned s = 0; s < 2; ++s) {
        const Shape *shape = shapes[s];
        for(uint32_t i = 0; i < shape->amount_of_vertices; ++i) {
            Position p = shape->vertices[i], q = shape->vertices[(i + 1) % shape->amount_of_vertices];
            double nx = -(double)q.y + p.y, ny = (double)q.x - p.x;
            double length = hypot(nx, ny);
            if(length <= 0) continue;
            nx /= length; ny /= length;
            double al, ah, bl, bh;
            sweep_projection_get(shapes[0], nx, ny, &al, &ah);
            sweep_projection_get(shapes[1], nx, ny, &bl, &bh);
            thickness = fmin(thickness, fmin(ah - al, bh - bl));
            double move = ((double)b.x - a.x) * nx + ((double)b.y - a.y) * ny;
            double overlap = fmin(ah - bl, bh - al);
            if(overlap < minimum_overlap) {
                minimum_overlap = overlap;
                /* Orient the minimum-translation axis from first to second.
                 * Only approach along this axis threatens an existing face;
                 * tangent travel and separation do not crush the contact. */
                closing_travel = fmax(0, (bl + bh >= al + ah) ? -move : move);
            }
            if(fabs(move) < DBL_MIN) {
                if(ah < bl || bh < al) return true;
                continue;
            }
            double low = (al - bh) / move, high = (ah - bl) / move;
            if(low > high) { double swap = low; low = high; high = swap; }
            if(low > enter) {
                enter = low;
                entry_speed = fabs(move);
                margin = 4 * physics_rigid_contact_slop_get(first, other, (Vec2D){nx, ny});
            }
            leave = fmin(leave, high);
            if(enter > leave) return true;
        }
    }
    if(enter > 0 && enter <= 1 && leave >= enter && entry_speed > 0) {
        /* Advance just inside contact so the existing manifold builder can
         * solve it, with an overlap far smaller than the colliding feature. */
        double duration = query->duration * fmin(1, enter + margin / entry_speed);
        if(duration > 0) query->earliest = fmin(query->earliest, duration);
    } else if(enter <= 0 && leave >= 0 && thickness > 0) {
        /* Already touching: bound closing normal travel. Tangential sliding
         * must not require steps proportional to speed / wall thickness. */
        double travel = closing_travel;
        if(travel > 0.25 * thickness)
            query->earliest = fmin(query->earliest,
                query->duration * 0.25 * thickness / travel);
    }
    return true;
}

double physics_rigid_sweep_duration_get(double dt, Report *report) {
    if(dt <= 0) return dt;
    bool moving_collider = false;
    unsigned collider_count = 0;
    RohrCollisionCategoryMask categories = 0, responses = 0;
    RohrCollisionCategoryMask moving_categories = 0, moving_responses = 0;
    for(uint32_t n = 0; n < entity_alive_count_get(); ++n) {
        EntityIndex index;
        if(physics_step_alive_index_at(n, &index) &&
                entity_index_components_check(index, ROHR_HIT_BOX | ROHR_COLLISION)) {
            ++collider_count;
            CollisionFilterDescription filter = physics_collision_filter_description_default_get();
            if(entity_index_components_check(index, ROHR_COLLISION_FILTER)) filter = collision_filters[index];
            categories |= filter.category;
            responses |= filter.responds_to;
            if(physics_entity_movable_get(index)) {
                Vec2D displacement = sweep_displacement_get(index, dt);
                if(displacement.x != 0 || displacement.y != 0 || angular_velocities[index] != 0) {
                    moving_collider = true;
                    moving_categories |= filter.category;
                    moving_responses |= filter.responds_to;
                }
            }
        }
    }
    if(!moving_collider || collider_count < 2 ||
            (!(categories & moving_responses) && !(moving_categories & responses))) return dt;
    /* Delayed collision updates consume all time as bounded integration steps. */
    dt = fmin(dt, 1.0 / 60.0);
    aabb_tree_clear(&physics_broadphase_tree);
    for(uint32_t n = 0; n < entity_alive_count_get(); ++n) {
        EntityIndex index;
        Entity entity;
        if(!physics_step_alive_index_at(n, &index) ||
                !entity_index_components_check(index, ROHR_HIT_BOX | ROHR_COLLISION) ||
                !physics_step_entity_from_index_get(index, &entity) ||
                !physics_step_hitbox_update(index, report)) continue;
        AABB bounds = sweep_bounds_get(index, dt);
        if(!isfinite(bounds.min_x) || !isfinite(bounds.min_y) ||
                !isfinite(bounds.max_x) || !isfinite(bounds.max_y)) continue;
        EngineResult result = aabb_tree_insert(&physics_broadphase_tree, entity, bounds);
        if(result.kind == ERROR_RESULT_ERROR) {
            physics_report_issue_add(report, ROHR_PHYSICS_BROADPHASE, ROHR_REPORT_SEVERITY_FATAL,
                result.result.error, ROHR_REPORT_UPDATE_ABORTED, entity, 0,
                "could not build swept broadphase tree");
            return dt;
        }
    }
    double earliest = dt;
    for(uint32_t n = 0; n < entity_alive_count_get(); ++n) {
        EntityIndex index;
        Entity entity;
        if(!physics_step_alive_index_at(n, &index) ||
                !entity_index_components_check(index, ROHR_HIT_BOX | ROHR_COLLISION) ||
                !physics_step_entity_from_index_get(index, &entity) ||
                !world_hit_boxes[index].amount_of_vertices) continue;
        SweepQuery query = {.source = entity, .index = index, .duration = dt, .earliest = earliest};
        (void)aabb_tree_query(&physics_broadphase_tree, sweep_bounds_get(index, dt), sweep_pair_visit, &query);
        earliest = query.earliest;
    }
    return earliest;
}
