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
    if(entity_index_components_check(index, ROHR_PARTICLE)) {
        Position center = physics_particle_world_origin_by_index_get(index);
        float radius = physics_particle_radius_by_index_get(index);
        bounds.min_x = fminf(bounds.min_x, center.x-radius);
        bounds.max_x = fmaxf(bounds.max_x, center.x+radius);
        bounds.min_y = fminf(bounds.min_y, center.y-radius);
        bounds.max_y = fmaxf(bounds.max_y, center.y+radius);
    }
    if(physics_entity_movable_get(index)) {
        /* Opposing velocity and acceleration can turn before the end of the
         * proposed step. Keep both excursions in the broadphase bound. */
        double vx = velocities[index].x*dt, vy = velocities[index].y*dt;
        double ax = (accelerations[index].x+force_accelerations[index].x)*dt*dt;
        double ay = (accelerations[index].y+force_accelerations[index].y)*dt*dt;
        bounds.min_x += fmin(0, vx)+fmin(0, ax);
        bounds.max_x += fmax(0, vx)+fmax(0, ax);
        bounds.min_y += fmin(0, vy)+fmin(0, ay);
        bounds.max_y += fmax(0, vy)+fmax(0, ay);
    }
    return bounds;
}

static double sweep_travel_time_get(double distance, double speed, double acceleration);

/* Particle pairs use circles in contact response, independently of the vertex
 * count used for mixed particle/polygon contacts. Their translating sweep can
 * use the same circles directly instead of projecting polygon edge axes. */
static bool sweep_particle_pair_check(SweepQuery *query, EntityIndex other) {
    EntityIndex first = query->index;
    if(!entity_index_components_check(first, ROHR_PARTICLE) ||
            !entity_index_components_check(other, ROHR_PARTICLE) ||
            !physics_entity_nonrotating_check(first) || !physics_entity_nonrotating_check(other)) return false;
    Position a = physics_particle_world_origin_by_index_get(first);
    Position b = physics_particle_world_origin_by_index_get(other);
    double r1 = physics_particle_radius_by_index_get(first), r2 = physics_particle_radius_by_index_get(other);
    if(!(r1 > 0) || !(r2 > 0)) return false;
    EntityIndex indices[2] = {first, other};
    double vx[2] = {0}, vy[2] = {0}, ax[2] = {0}, ay[2] = {0};
    for(unsigned i = 0; i < 2; ++i) {
        EntityIndex n = indices[i];
        if(entity_mask[n] & (ROHR_AXIS_LOCK | ROHR_TRANSFORM_LOCK)) return false;
        if(!physics_entity_movable_get(n)) continue;
        vx[i] = velocities[n].x; vy[i] = velocities[n].y;
        double factor = physics_soft_position_acceleration_factor_get(n);
        ax[i] = factor*(accelerations[n].x+force_accelerations[n].x);
        ay[i] = factor*(accelerations[n].y+force_accelerations[n].y);
    }
    if(ax[0] != ax[1] || ay[0] != ay[1]) {
        /* A relative accelerated path is curved. Its total travel bounds the
         * distance it can close even if its proposed endpoint returns home. */
        double x = (double)b.x-a.x, y = (double)b.y-a.y, distance = hypot(x, y);
        Vec2D normal = distance > 0 ? (Vec2D){x/distance, y/distance} : (Vec2D){1, 0};
        double gap = distance-r1-r2;
        double travel = gap > 0 ? gap+4*physics_rigid_contact_slop_get(first, other, normal) :
            .5*fmin(r1, r2);
        double duration = sweep_travel_time_get(travel, hypot(vx[0]-vx[1], vy[0]-vy[1]),
            hypot(ax[0]-ax[1], ay[0]-ay[1]));
        if(duration > 0) query->earliest = fmin(query->earliest, duration);
        return true;
    }
    Vec2D da = sweep_displacement_get(first, query->duration);
    Vec2D db = sweep_displacement_get(other, query->duration);
    double x = (double)b.x-a.x, y = (double)b.y-a.y;
    double dx = (double)db.x-da.x, dy = (double)db.y-da.y;
    double distance2 = x*x+y*y, dot = x*dx+y*dy, travel2 = dx*dx+dy*dy;
    double radius = r1+r2, gap = distance2-radius*radius;
    if(dot >= 0 || !(travel2 > 0)) return true;
    if(gap > 0) {
        double discriminant = dot*dot-travel2*gap;
        if(discriminant <= 0) return true;
        double root = sqrt(discriminant);
        double entry = gap/(-dot+root);
        if(entry > 1) return true;
        Vec2D normal = {(x+entry*dx)/radius, (y+entry*dy)/radius};
        double margin = 4*physics_rigid_contact_slop_get(first, other, normal);
        /* A grazing crossing can be shorter than the penetration margin.
         * Stop before closest approach instead of stepping past its exit. */
        double inside = fmin(margin*radius/root, .5*root/travel2);
        double duration = query->duration*fmin(1, entry+inside);
        if(duration > 0) query->earliest = fmin(query->earliest, duration);
    } else if(distance2 > 0) {
        double closing = -dot/sqrt(distance2), margin = .5*fmin(r1, r2);
        if(closing > margin)
            query->earliest = fmin(query->earliest, query->duration*margin/closing);
    }
    return true;
}

static void sweep_projection_get(const Shape *shape, double x, double y,
    double *low, double *high) {
    *low = DBL_MAX; *high = -DBL_MAX;
    for(uint32_t i = 0; i < shape->amount_of_vertices; ++i) {
        double p = shape->vertices[i].x * x + shape->vertices[i].y * y;
        *low = fmin(*low, p); *high = fmax(*high, p);
    }
}

static double sweep_travel_time_get(double distance, double speed, double acceleration) {
    speed = fmax(0, speed); acceleration = fmax(0, acceleration);
    if(acceleration > 0) return 2*distance/(speed+sqrt(speed*speed+4*acceleration*distance));
    return speed > 0 ? distance/speed : INFINITY;
}

/* Fixed-orientation shapes have fixed separating axes. Bound their prepared
 * quadratic motion directly: scaling a full-step displacement down linearly
 * needlessly repeats tiny steps at accelerating supported nodes. */
static bool sweep_translating_pair_check(SweepQuery *query, EntityIndex other) {
    EntityIndex indices[2] = {query->index, other};
    double vx[2] = {0}, vy[2] = {0}, ax[2] = {0}, ay[2] = {0};
    for(unsigned i = 0; i < 2; ++i) {
        EntityIndex n = indices[i];
        if(entity_mask[n] & (ROHR_AXIS_LOCK | ROHR_TRANSFORM_LOCK)) return false;
        if(!physics_entity_movable_get(n)) continue;
        if(!physics_entity_nonrotating_check(n)) return false;
        vx[i] = velocities[n].x; vy[i] = velocities[n].y;
        double factor = physics_soft_position_acceleration_factor_get(n);
        ax[i] = factor*(accelerations[n].x+force_accelerations[n].x);
        ay[i] = factor*(accelerations[n].y+force_accelerations[n].y);
    }
    const Shape *shapes[2] = {&world_hit_boxes[indices[0]], &world_hit_boxes[indices[1]]};
    double entry = 0, depth = INFINITY, thickness = INFINITY, closing = 0, acceleration = 0;
    for(unsigned s = 0; s < 2; ++s) {
        const Shape *shape = shapes[s];
        for(unsigned i = 0; i < shape->amount_of_vertices; ++i) {
            Position p = shape->vertices[i], q = shape->vertices[(i+1)%shape->amount_of_vertices];
            double nx = (double)p.y-q.y, ny = (double)q.x-p.x, length = hypot(nx, ny);
            if(length <= 0) continue;
            nx /= length; ny /= length;
            double al, ah, bl, bh;
            sweep_projection_get(shapes[0], nx, ny, &al, &ah);
            sweep_projection_get(shapes[1], nx, ny, &bl, &bh);
            thickness = fmin(thickness, fmin(ah-al, bh-bl));
            double sign = bl+bh >= al+ah ? 1 : -1;
            double v = sign*((vx[0]-vx[1])*nx+(vy[0]-vy[1])*ny);
            double a = sign*((ax[0]-ax[1])*nx+(ay[0]-ay[1])*ny);
            double overlap = fmin(ah-bl, bh-al);
            if(overlap < 0) {
                double margin = 4*physics_rigid_contact_slop_get(indices[0], indices[1], (Vec2D){nx, ny});
                double time = sweep_travel_time_get(-overlap+margin, v, a);
                if(time >= query->duration) return true;
                entry = fmax(entry, time);
            }
            if(overlap < depth) { depth = overlap; closing = v; acceleration = a; }
        }
    }
    double duration = entry > 0 ? entry : sweep_travel_time_get(.25*thickness, closing, acceleration);
    if(duration > 0) query->earliest = fmin(query->earliest, duration);
    return true;
}

static bool sweep_pair_visit(Entity target, void *context) {
    SweepQuery *query = context;
    EntityIndex other;
    if(target <= query->source || !entity_index_get(target, &other) ||
            !entity_index_components_check(other, ROHR_COLLISION) ||
            !physics_collision_response_between_by_index_check(query->index, other)) return true;
    EntityIndex first = query->index;
    if(!physics_entity_movable_get(first) && !physics_entity_movable_get(other)) return true;
    if(sweep_particle_pair_check(query, other)) return true;
    if(sweep_translating_pair_check(query, other)) return true;
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
                if(velocities[index].x != 0 || velocities[index].y != 0 ||
                        accelerations[index].x+force_accelerations[index].x != 0 ||
                        accelerations[index].y+force_accelerations[index].y != 0 ||
                        angular_velocities[index] != 0) {
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
