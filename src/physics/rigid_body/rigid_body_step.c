/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics/physics_step_internal.h"
#include "physics/physics_internal.h"
#include "systems.h"
#include "console.h"
#include "physics/collision/contact_manifold.h"
#include "math2d.h"
#include "physics/physics_operation.h"
#include "physics/particles/particle_solver.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <SDL3/SDL_bits.h>

#define RIGID_CONTACT_PENETRATION_SLOP 0.01f
#define RIGID_CONTACT_RELATIVE_SLOP 0.0001f
#define RIGID_CONTACT_POSITION_CORRECTION_MAX 5.0f

static double physics_rigid_elapsed_ms(uint64_t start) {
    return (double)(SDL_GetPerformanceCounter() - start) * 1000.0 /
        (double)SDL_GetPerformanceFrequency();
}

static bool hitbox_cache_active, hitbox_cached[MAX_ENTITIES];
static bool broadphase_reuse_allowed;
static Position hitbox_cached_positions[MAX_ENTITIES];
static Orientation hitbox_cached_orientations[MAX_ENTITIES];
/* Geometry only, sharing the hitbox cache's pose checks and invocation lifetime.
 * Velocity, impulses and response masks remain live between sequential pairs. */
static PhysicsParticleCircle hitbox_cached_circles[MAX_ENTITIES];
static Position hitbox_cached_centers[MAX_ENTITIES];
static bool hitbox_cached_center_valid[MAX_ENTITIES];
static bool hitbox_translation_safe[MAX_ENTITIES];
void physics_step_hitbox_cache_begin(void) {
    if(!hitbox_cache_active) {
        memset(hitbox_cached, 0, sizeof(hitbox_cached));
        memset(hitbox_cached_center_valid, 0, sizeof(hitbox_cached_center_valid));
    }
    hitbox_cache_active = true;
    /* A manual stage can follow arbitrary collider/geometry edits. Only the
     * complete substep proves the preceding gather is still structurally valid. */
    broadphase_reuse_allowed = physics_step_entities_active_check();
}
void physics_step_hitbox_cache_end(void) {
    if(!physics_step_entities_active_check()) hitbox_cache_active = false;
}

static bool system_hitbox_pose_cached_check(EntityIndex index) {
    return hitbox_cache_active && hitbox_cached[index] &&
        positions[index].x == hitbox_cached_positions[index].x &&
        positions[index].y == hitbox_cached_positions[index].y &&
        orientations[index] == hitbox_cached_orientations[index];
}

static PhysicsParticleCircle system_contact_circle_get(EntityIndex index) {
    if(system_hitbox_pose_cached_check(index)) return hitbox_cached_circles[index];
    return (PhysicsParticleCircle){physics_particle_world_origin_by_index_get(index),
        physics_particle_radius_by_index_get(index)};
}

static Position system_contact_center_get(EntityIndex index) {
    if(!system_hitbox_pose_cached_check(index)) return physics_com_world_by_index_get(index);
    if(!hitbox_cached_center_valid[index]) {
        hitbox_cached_centers[index] = physics_com_world_by_index_get(index);
        hitbox_cached_center_valid[index] = true;
    }
    return hitbox_cached_centers[index];
}

bool physics_step_hitbox_update(EntityIndex i, Report *report) {
    if(!physics_step_entity_alive_check(i) || !physics_step_entity_components_check(i, ROHR_HIT_BOX)) return false;
    /* Local geometry is immutable during a complete substep or manual solve. Pose correction
     * invalidates by exact comparison; no cache survives a public call. */
    if(system_hitbox_pose_cached_check(i))
        return world_hit_boxes[i].amount_of_vertices != 0;
    hitbox_cached_center_valid[i] = false;
    Shape next = {0};
    bool valid = physics_world_position_check(positions[i]) && isfinite(orientations[i]) &&
        hit_boxes[i].amount_of_vertices <= MAX_VERTICIES &&
        hit_boxes[i].concave_piece_count <= MAX_CONCAVE_PIECES;
    if(valid) {
        for(uint32_t v = 0; v < hit_boxes[i].amount_of_vertices; ++v)
            if(!physics_vector_finite_check(hit_boxes[i].vertices[v])) valid = false;
    }
    if(valid) {
        for(uint8_t p = 0; p < hit_boxes[i].concave_piece_count; ++p)
            for(unsigned v = 0; v < 3; ++v)
                if(hit_boxes[i].concave_pieces[p].vertex_indices[v] >= hit_boxes[i].amount_of_vertices)
                    valid = false;
    }
    if(valid) {
        next = physics_shape_world_translate(hit_boxes[i], positions[i], orientations[i]);
        for(uint32_t v = 0; v < next.amount_of_vertices; ++v)
            if(!physics_vector_finite_check(next.vertices[v])) valid = false;
    }
    if(physics_step_entity_components_check(i, ROHR_PARTICLE)) {
        Position center = physics_particle_world_origin_by_index_get(i);
        float radius = physics_particle_radius_by_index_get(i);
        valid = valid && physics_vector_finite_check(center) && isfinite(radius) && radius >= 0 &&
            isfinite(center.x - radius) && isfinite(center.x + radius) &&
            isfinite(center.y - radius) && isfinite(center.y + radius);
        if(valid) hitbox_cached_circles[i] = (PhysicsParticleCircle){center, radius};
    }
    if(!valid) {
        /* Invalidate derived geometry, never reuse an obsolete collider. */
        world_hit_boxes[i] = (Shape){0};
        physics_operation_reject(report, ROHR_PHYSICS_GEOMETRY, ERROR_ENGINE_STATE_INVALID,
            physics_operation_entity_get(i), 0);
        return false;
    }
    world_hit_boxes[i] = next;
    if(hitbox_cache_active) {
        hitbox_cached[i] = true;
        hitbox_cached_positions[i] = positions[i];
        hitbox_cached_orientations[i] = orientations[i];
        hitbox_translation_safe[i] = physics_simple_baseline_active_check();
        for(uint32_t v = 0; hitbox_translation_safe[i] && v < hit_boxes[i].amount_of_vertices; ++v)
            if(fabsf(hit_boxes[i].vertices[v].x) > FLT_MAX / 4.0f ||
                    fabsf(hit_boxes[i].vertices[v].y) > FLT_MAX / 4.0f)
                hitbox_translation_safe[i] = false;
    }
    return next.amount_of_vertices != 0;
}

/* Complete baseline substeps keep local geometry immutable. Circle response
 * needs the current circle, not a rebuilt polygon after every correction.
 * Leave the polygon pose stamp untouched: mixed contacts and the dirty flush
 * must still refresh it. Manual stages and unusual geometry use full validation. */
static bool system_particle_contact_circle_get(EntityIndex index,
        PhysicsParticleCircle *circle, Report *report) {
    if(physics_simple_baseline_active_check() && hitbox_cache_active &&
            hitbox_cached[index] && hitbox_translation_safe[index] &&
            physics_entity_nonrotating_check(index) &&
            orientations[index] == hitbox_cached_orientations[index] &&
            physics_world_position_check(positions[index])) {
        *circle = (PhysicsParticleCircle){physics_particle_world_origin_by_index_get(index),
            hitbox_cached_circles[index].radius};
        if(physics_vector_finite_check(circle->center) &&
                isfinite(circle->center.x - circle->radius) &&
                isfinite(circle->center.x + circle->radius) &&
                isfinite(circle->center.y - circle->radius) &&
                isfinite(circle->center.y + circle->radius))
            return world_hit_boxes[index].amount_of_vertices != 0;
    }
    if(!physics_step_hitbox_update(index, report)) return false;
    *circle = hitbox_cached_circles[index];
    return true;
}

static void system_positions_update(double dt, Report *report) {
    for(uint32_t n = 0; n < entity_alive_count_get(); ++n) {
        EntityIndex i;
        if(!physics_step_alive_index_at(n, &i) || !physics_entity_movable_get(i)) continue;
        if(physics_joint_motion_integrated_check(i) || physics_soft_motion_integrated_check(i)) continue;
        PhysicsChange change = physics_change_get(i, PHYSICS_CHANGE_POSITION);
        change.position = (Position){positions[i].x + velocities[i].x * dt,
            positions[i].y + velocities[i].y * dt};
        physics_report_counter_add(report, ROHR_REPORT_PROCESSED_COUNT, 1);
        (void)physics_changes_commit(&change, 1, report, ROHR_PHYSICS_POSITION,
            physics_operation_entity_get(i), 0);
    }
}

static void system_orientations_update(double dt, Report *report) {
    for(uint32_t n = 0; n < entity_alive_count_get(); ++n) {
        EntityIndex i;
        if(!physics_step_alive_index_at(n, &i) || !physics_entity_movable_get(i) ||
                physics_step_entity_components_check(i, ROHR_SOFT_BODY) ||
                physics_entity_nonrotating_check(i)) continue;
        if(physics_joint_motion_integrated_check(i) || physics_soft_motion_integrated_check(i)) continue;
        PhysicsChange change = physics_change_get(i, PHYSICS_CHANGE_ORIENTATION);
        physics_change_orientation_set(&change, orientations[i] + angular_velocities[i] * dt);
        (void)physics_changes_commit(&change, 1, report, ROHR_PHYSICS_ORIENTATION,
            physics_operation_entity_get(i), 0);
    }
}

static void system_angular_velocities_update(double dt, Report *report) {
    for(uint32_t n = 0; n < entity_alive_count_get(); ++n) {
        EntityIndex i;
        if(!physics_step_alive_index_at(n, &i) || !physics_entity_movable_get(i) ||
                physics_step_entity_components_check(i, ROHR_SOFT_BODY)) continue;
        if(physics_joint_motion_integrated_check(i) || physics_soft_motion_integrated_check(i)) continue;
        PhysicsChange change = physics_change_get(i, PHYSICS_CHANGE_ANGULAR_VELOCITY);
        change.angular_velocity = physics_entity_nonrotating_check(i) ? 0.0f :
            angular_velocities[i] + (angular_accelerations[i] + torque_angular_accelerations[i]) * dt;
        (void)physics_changes_commit(&change, 1, report, ROHR_PHYSICS_ANGULAR_VELOCITY,
            physics_operation_entity_get(i), 0);
    }
}

static void system_angular_velocity_maximums_apply(Report *report) {
    for(uint32_t alive_position = 0; alive_position < entity_alive_count_get();
            alive_position += 1) {
        EntityIndex i;
        AngularVelocity maximum;

        if(!physics_step_alive_index_at(alive_position, &i) ||
                i >= angular_velocity_maximums_pool.capacity ||
                !angular_velocity_maximums_pool.used[i]) continue;
        maximum = angular_velocity_maximums[i];
        if(!isfinite(maximum) || maximum < 0 || !isfinite(angular_velocities[i])) {
            physics_operation_reject(report, ROHR_PHYSICS_ANGULAR_VELOCITY,
                ERROR_ENGINE_STATE_INVALID, physics_operation_entity_get(i), 0);
            continue;
        }
        if(angular_velocities[i] > maximum) angular_velocities[i] = maximum;
        if(angular_velocities[i] < -maximum) angular_velocities[i] = -maximum;
    }
}

static void system_velocities_update(double dt, Report *report) {
    for(uint32_t alive_position = 0; alive_position < entity_alive_count_get(); alive_position += 1) {
        EntityIndex i;

        if(!physics_step_alive_index_at(alive_position, &i)) {
            continue;
        }
        if(physics_entity_movable_get(i)) {
            if(physics_joint_motion_integrated_check(i) || physics_soft_motion_integrated_check(i)) continue;
            PhysicsChange change = physics_change_get(i, PHYSICS_CHANGE_VELOCITY);
            change.velocity = (Velocity){
                .x = velocities[i].x + (accelerations[i].x + force_accelerations[i].x)*dt,
                .y = velocities[i].y + (accelerations[i].y + force_accelerations[i].y)*dt
            };
            (void)physics_changes_commit(&change, 1, report, ROHR_PHYSICS_VELOCITY,
                physics_operation_entity_get(i), 0);
        }
    }
}

static void system_loads_apply(Report *report, bool torque) {
    physics_sources_apply(report, torque);
}

void system_force_torque_accelerations_clear(void) {
    for(uint32_t alive_position = 0; alive_position < entity_alive_count_get(); alive_position += 1) {
        EntityIndex i;

        if(!physics_step_alive_index_at(alive_position, &i)) {
            continue;
        }
        force_accelerations[i].x = 0;
        force_accelerations[i].y = 0;
        torque_angular_accelerations[i] = 0;
    }
}

void physics_rigid_gravity_apply(Acceleration gravity, Report *report) {
    for(uint32_t alive_position = 0; alive_position < entity_alive_count_get();
            alive_position += 1) {
        EntityIndex index;

        if(!physics_step_alive_index_at(alive_position, &index) ||
                !physics_entity_simulated_get(index) ||
                !physics_step_entity_components_check(index, ROHR_GRAVITY)) continue;
        PhysicsChange change = physics_change_get(index, PHYSICS_CHANGE_FORCE_ACCELERATION);
        change.force_acceleration.x += gravity.x;
        change.force_acceleration.y += gravity.y;
        (void)physics_changes_commit(&change, 1, report, ROHR_PHYSICS_FORCE_CONTRIBUTION,
            physics_operation_entity_get(index), 0);
    }
}

OverlapInfo system_entity_overlap_get(Entity entity_1, Entity entity_2) {
    if(!physics_step_entity_components_check(entity_1, ROHR_HIT_BOX) ||
            !physics_step_entity_components_check(entity_2, ROHR_HIT_BOX)) return (OverlapInfo){0};
    if(physics_step_entity_components_check(entity_1, ROHR_PARTICLE) && physics_step_entity_components_check(entity_2, ROHR_PARTICLE)) {
        return physics_particle_entities_overlap_get(entity_1, entity_2);
    }
    return physics_sat_overlap_get(world_hit_boxes[entity_1], world_hit_boxes[entity_2]);
}

/* Masks affect only this pair's effective mass, never the stored motion mode. */
static bool system_contact_response_check(EntityIndex first, EntityIndex second) {
    return physics_collision_response_to_by_index_check(first, second);
}

/* Measure thickness in local space so a distant world origin does not consume
 * the precision needed by a small collider. The fixed tolerance is an upper
 * bound, not permission for a thin body to disappear into its support. */
static float system_contact_thickness_get(EntityIndex entity, Vec2D normal) {
    if(physics_step_entity_components_check(entity, ROHR_PARTICLE))
        return 2.0f * (system_hitbox_pose_cached_check(entity) ?
            hitbox_cached_circles[entity].radius : physics_particle_radius_by_index_get(entity));
    Vec2D local_normal = math_vector_rotate(normal, -orientations[entity]);
    const Shape *shape = &hit_boxes[entity];
    float low = FLT_MAX, high = -FLT_MAX;
    for(uint32_t i = 0; i < shape->amount_of_vertices; ++i) {
        float projection = math_dot_product(shape->vertices[i], local_normal);
        low = fminf(low, projection);
        high = fmaxf(high, projection);
    }
    return shape->amount_of_vertices ? high - low : 0.0f;
}

float physics_rigid_contact_slop_get(EntityIndex first, EntityIndex second, Vec2D normal) {
    return fminf(RIGID_CONTACT_PENETRATION_SLOP,
        RIGID_CONTACT_RELATIVE_SLOP * fminf(
            system_contact_thickness_get(first, normal),
            system_contact_thickness_get(second, normal)));
}

void system_separate_entities_tuned(
    Entity entity_1,
    Entity entity_2,
    OverlapInfo collision, Report *report
) {
    bool dynamic_1 = physics_entity_simulated_get(entity_1) &&
        system_contact_response_check(entity_1, entity_2);
    bool dynamic_2 = physics_entity_simulated_get(entity_2) &&
        system_contact_response_check(entity_2, entity_1);

    float inv_mass_1 =
        dynamic_1 && physics_step_entity_components_check(entity_1, ROHR_MASS) &&
            mass[entity_1] > 0.0f
        ? 1.0f / mass[entity_1]
        : 0.0f;

    float inv_mass_2 =
        dynamic_2 && physics_step_entity_components_check(entity_2, ROHR_MASS) &&
            mass[entity_2] > 0.0f
        ? 1.0f / mass[entity_2]
        : 0.0f;

    float inv_mass_sum = inv_mass_1 + inv_mass_2;

    if(inv_mass_sum <= 0.0f) {
        return;
    }

    float slop = physics_rigid_contact_slop_get(entity_1, entity_2, collision.normal);
    float correction_depth = fminf(
        fmaxf(collision.depth - slop, 0.0f),
        RIGID_CONTACT_POSITION_CORRECTION_MAX);
    Vec2D correction;

    if(correction_depth <= 0.0f) return;
    correction = (Vec2D){
        .x = collision.normal.x * correction_depth,
        .y = collision.normal.y * correction_depth
    };

    float share_1 = inv_mass_1 / inv_mass_sum;
    float share_2 = inv_mass_2 / inv_mass_sum;

    PhysicsChange changes[] = {
        physics_change_get(entity_1, PHYSICS_CHANGE_POSITION),
        physics_change_get(entity_2, PHYSICS_CHANGE_POSITION)
    };
    changes[0].position.x -= correction.x * share_1;
    changes[0].position.y -= correction.y * share_1;

    changes[1].position.x += correction.x * share_2;
    changes[1].position.y += correction.y * share_2;
    (void)physics_changes_commit(changes, 2, report, ROHR_PHYSICS_POSITION_CORRECTION,
            physics_operation_entity_get(entity_1), physics_operation_entity_get(entity_2));
}

Position system_support_point_average(Shape shape, Vec2D direction)
{
    float best_projection = -FLT_MAX;
    Position sum = {0};
    int count = 0;

    const float epsilon = 0.0001f;

    for(int i = 0; i < shape.amount_of_vertices; i += 1) {
        Position vertex = shape.vertices[i];
        float projection = math_dot_product(vertex, direction);

        if(projection > best_projection + epsilon) {
            best_projection = projection;
            sum = vertex;
            count = 1;
        }
        else if(fabsf(projection - best_projection) <= epsilon) {
            sum.x += vertex.x;
            sum.y += vertex.y;
            count += 1;
        }
    }

    if(count > 0) {
        sum.x /= count;
        sum.y /= count;
    }

    return sum;
}

Position system_particle_edge_get(Entity entity, Vec2D normal, Vec1D radius) {
    Position center = physics_particle_world_origin_by_index_get(entity);

    return (Position) {
        .x = center.x + normal.x * radius,
        .y = center.y + normal.y * radius,
    };
}

Position system_collision_contact_point(Entity entity_1, Entity entity_2, OverlapInfo collision)
{
    bool entity_1_dynamic = physics_entity_movable_get(entity_1);
    bool entity_2_dynamic = physics_entity_movable_get(entity_2);

    Vec2D normal = collision.normal;

    Vec2D opposite_normal = {
        .x = -normal.x,
        .y = -normal.y
    };

    Position point_1 = {0};
    Position point_2 = {0};
    bool particle_pair =
        physics_step_entity_components_check(entity_1, ROHR_PARTICLE) &&
        physics_step_entity_components_check(entity_2, ROHR_PARTICLE);
    bool entity_1_particle = particle_pair;
    bool entity_2_particle = particle_pair;

    if(entity_1_particle) {
        PhysicsParticleCircle circle = system_contact_circle_get(entity_1);
        point_1 = (Position){circle.center.x + normal.x * circle.radius,
            circle.center.y + normal.y * circle.radius};
    } else {
        point_1 = system_support_point_average(world_hit_boxes[entity_1], normal);
    }
    if(entity_2_particle) {
        PhysicsParticleCircle circle = system_contact_circle_get(entity_2);
        point_2 = (Position){circle.center.x + opposite_normal.x * circle.radius,
            circle.center.y + opposite_normal.y * circle.radius};
    } else {
        point_2 = system_support_point_average(world_hit_boxes[entity_2], opposite_normal);
    }

    /*
     * A frictionless normal impulse on a particle must pass through its
     * center. Polygon support averaging can otherwise create a false lever
     * arm and convert translational bounce energy into particle spin.
     */
    if(entity_1_particle && !entity_2_particle) {
        return point_1;
    }
    if(!entity_1_particle && entity_2_particle) {
        return point_2;
    }

    //Use dynamic entities contact point
    if(entity_1_dynamic && !entity_2_dynamic) {
        return point_1;
    }

    if(!entity_1_dynamic && entity_2_dynamic) {
        return point_2;
    }

    //Midpoint is good enough for now
    return (Position){
        .x = (point_1.x + point_2.x) * 0.5f,
        .y = (point_1.y + point_2.y) * 0.5f
    };
}

static bool system_contact_impulse_commit(Entity first, Entity second, Vec2D first_offset,
        Vec2D second_offset, Vec2D impulse, float inverse_mass_first, float inverse_mass_second,
        float inverse_inertia_first, float inverse_inertia_second, bool compact_velocity,
        Report *report, PhysicsReportStage stage) {
    if(compact_velocity) {
        PhysicsVelocityChange changes[] = {
            {first, velocities[first], angular_velocities[first]},
            {second, velocities[second], angular_velocities[second]}
        };
        changes[0].velocity.x -= impulse.x * inverse_mass_first;
        changes[0].velocity.y -= impulse.y * inverse_mass_first;
        changes[1].velocity.x += impulse.x * inverse_mass_second;
        changes[1].velocity.y += impulse.y * inverse_mass_second;
        changes[0].angular_velocity += math_radians_to_degrees(math_cross_2d(first_offset, impulse) *
            inverse_inertia_first);
        changes[1].angular_velocity -= math_radians_to_degrees(math_cross_2d(second_offset, impulse) *
            inverse_inertia_second);
        return physics_velocity_pair_commit(changes, report, stage);
    }
    PhysicsChange changes[] = {
        physics_change_get(first, PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY),
        physics_change_get(second, PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY)
    };
    changes[0].velocity.x -= impulse.x * inverse_mass_first;
    changes[0].velocity.y -= impulse.y * inverse_mass_first;
    changes[1].velocity.x += impulse.x * inverse_mass_second;
    changes[1].velocity.y += impulse.y * inverse_mass_second;
    changes[0].angular_velocity += math_radians_to_degrees(math_cross_2d(first_offset, impulse) *
        inverse_inertia_first);
    changes[1].angular_velocity -= math_radians_to_degrees(math_cross_2d(second_offset, impulse) *
        inverse_inertia_second);
    return physics_changes_commit(changes, 2, report, stage,
        physics_operation_entity_get(first), physics_operation_entity_get(second));
}

static Vec2D system_contact_friction_apply(
    Entity entity_1,
    Entity entity_2,
    OverlapInfo collision,
    Vec2D r1,
    Vec2D r2,
    float normal_impulse_magnitude,
    float inv_mass_1,
    float inv_mass_2,
    float inv_inertia_1,
    float inv_inertia_2,
    Vec2D previous_impulse, Report *report, bool compact_velocity
) {
    bool moving_1 = physics_entity_movable_get(entity_1);
    bool moving_2 = physics_entity_movable_get(entity_2);
    Vec2D angular_v1 = !moving_1 || physics_entity_nonrotating_check(entity_1)
        ? (Vec2D){0}
        : math_angular_velocity_cross_vec(angular_velocities[entity_1], r1);
    Vec2D angular_v2 = !moving_2 || physics_entity_nonrotating_check(entity_2)
        ? (Vec2D){0}
        : math_angular_velocity_cross_vec(angular_velocities[entity_2], r2);

    Vec2D contact_v1 = {
        .x = (moving_1 ? velocities[entity_1].x : 0.0f) + angular_v1.x,
        .y = (moving_1 ? velocities[entity_1].y : 0.0f) + angular_v1.y
    };

    Vec2D contact_v2 = {
        .x = (moving_2 ? velocities[entity_2].x : 0.0f) + angular_v2.x,
        .y = (moving_2 ? velocities[entity_2].y : 0.0f) + angular_v2.y
    };

    Vec2D rel_v = {
        .x = contact_v2.x - contact_v1.x,
        .y = contact_v2.y - contact_v1.y
    };

    float rel_v_along_normal = math_dot_product(rel_v, collision.normal);

    Vec2D tangent = {
        .x = rel_v.x - collision.normal.x * rel_v_along_normal,
        .y = rel_v.y - collision.normal.y * rel_v_along_normal
    };

    float tangent_mag = sqrtf(tangent.x * tangent.x + tangent.y * tangent.y);

    if(tangent_mag <= 0.000001f) {
        return previous_impulse;
    }

    tangent.x /= tangent_mag;
    tangent.y /= tangent_mag;

    float r1_cross_t = math_cross_2d(r1, tangent);
    float r2_cross_t = math_cross_2d(r2, tangent);

    float denominator =
        inv_mass_1 +
        inv_mass_2 +
        (r1_cross_t * r1_cross_t) * inv_inertia_1 +
        (r2_cross_t * r2_cross_t) * inv_inertia_2;

    if(denominator <= 0) {
        return (Vec2D){0};
    }

    float impulse_delta = -math_dot_product(rel_v, tangent) / denominator;
    float previous_magnitude = math_dot_product(previous_impulse, tangent);

    float mu = sqrtf(frictions[entity_1] * frictions[entity_2]);

    float max_friction = fabsf(normal_impulse_magnitude) * mu;

    if(!isfinite(tangent_mag) || !isfinite(denominator) || !isfinite(impulse_delta) ||
            !isfinite(previous_magnitude) || !isfinite(max_friction)) {
        physics_operation_reject(report, ROHR_PHYSICS_FRICTION_IMPULSE, ERROR_ENGINE_STATE_INVALID,
            physics_operation_entity_get(entity_1), physics_operation_entity_get(entity_2));
        return previous_impulse;
    }
    float impulse_magnitude = fmaxf(-max_friction,
        fminf(previous_magnitude + impulse_delta, max_friction));
    impulse_delta = impulse_magnitude - previous_magnitude;

    Vec2D friction_impulse = {
        .x = tangent.x * impulse_delta,
        .y = tangent.y * impulse_delta
    };

    if(!system_contact_impulse_commit(entity_1, entity_2, r1, r2, friction_impulse,
            inv_mass_1, inv_mass_2, inv_inertia_1, inv_inertia_2, compact_velocity,
            report, ROHR_PHYSICS_FRICTION_IMPULSE)) return previous_impulse;
    return (Vec2D){
        tangent.x * impulse_magnitude,
        tangent.y * impulse_magnitude
    };
}

Vec2D system_friction_impulse_apply(Entity entity_1, Entity entity_2, OverlapInfo collision,
        Vec2D r1, Vec2D r2, float normal_impulse_magnitude, float inv_mass_1, float inv_mass_2,
        float inv_inertia_1, float inv_inertia_2, Vec2D previous_impulse, Report *report) {
    return system_contact_friction_apply(entity_1, entity_2, collision, r1, r2,
        normal_impulse_magnitude, inv_mass_1, inv_mass_2, inv_inertia_1, inv_inertia_2,
        previous_impulse, report, false);
}

static float system_contact_rebound_get(EntityIndex first,EntityIndex second,
        Position point,Vec2D normal,bool enabled) {
    if(!enabled)return 0;
    EntityIndex bodies[]={first,second};Velocity v[2]={{0,0},{0,0}};
    for(unsigned i=0;i<2;++i)if(physics_entity_movable_get(bodies[i])) {
        EntityIndex body=bodies[i];
        Vec2D angular=math_angular_velocity_cross_vec(physics_entity_nonrotating_check(body)?0:angular_velocities[body],
            math_vector_subtract(point,system_contact_center_get(body)));
        v[i]=(Velocity){velocities[body].x+angular.x,velocities[body].y+angular.y};
    }
    float normal_velocity=math_dot_product(math_vector_subtract(v[1],v[0]),normal);
    return -fminf(restitutions[first],restitutions[second])*fminf(normal_velocity,0);
}

/* Nonrotating circles have no lever-arm terms. Preserve sequential normal and
 * friction commits and accumulated impulses without preparing angular state. */
static bool system_particle_point_solve(EntityIndex first, EntityIndex second,
        Vec2D normal, bool restitution_enabled, float inverse_mass_first,
        float inverse_mass_second, const ContactPointInfo *previous,
        const float *previous_rebound, float *rebound_speed,
        Velocity *relative_velocity, Vec2D *normal_impulse, Vec2D *friction_impulse,
        Report *report) {
    bool moving_first = physics_entity_movable_get(first);
    bool moving_second = physics_entity_movable_get(second);
    Velocity relative = {
        (moving_second ? velocities[second].x : 0.0f) -
            (moving_first ? velocities[first].x : 0.0f),
        (moving_second ? velocities[second].y : 0.0f) -
            (moving_first ? velocities[first].y : 0.0f)};
    if(relative_velocity != NULL && physics_vector_finite_check(relative))
        *relative_velocity = relative;
    float along = math_dot_product(relative, normal);
    float previous_normal = previous ? math_dot_product(previous->normal_impulse, normal) : 0;
    float restitution = restitution_enabled && previous_normal <= 0 && along < 0
        ? fminf(restitutions[first], restitutions[second]) : 0;
    float denominator = inverse_mass_first + inverse_mass_second;
    if(!isfinite(restitutions[first]) || !isfinite(restitutions[second])) {
        physics_operation_reject(report, ROHR_PHYSICS_NORMAL_IMPULSE, ERROR_ENGINE_STATE_INVALID,
            physics_operation_entity_get(first), physics_operation_entity_get(second));
        return false;
    }
    if(denominator <= 0) return false;
    float rebound = previous_rebound ? *previous_rebound : -restitution * fminf(along, 0);
    *rebound_speed = rebound;
    float delta = (rebound - along) / denominator;
    if(!isfinite(delta) || !isfinite(denominator) || !isfinite(previous_normal + delta)) {
        physics_operation_reject(report, ROHR_PHYSICS_NORMAL_IMPULSE, ERROR_ENGINE_STATE_INVALID,
            physics_operation_entity_get(first), physics_operation_entity_get(second));
        return false;
    }
    float accumulated = fmaxf(previous_normal + delta, 0);
    delta = accumulated - previous_normal;
    Vec2D impulse = {normal.x * delta, normal.y * delta};
    PhysicsVelocityChange changes[] = {
        {first, velocities[first], angular_velocities[first]},
        {second, velocities[second], angular_velocities[second]}};
    changes[0].velocity.x -= impulse.x * inverse_mass_first;
    changes[0].velocity.y -= impulse.y * inverse_mass_first;
    changes[1].velocity.x += impulse.x * inverse_mass_second;
    changes[1].velocity.y += impulse.y * inverse_mass_second;
    if(!physics_velocity_pair_commit(changes, report, ROHR_PHYSICS_NORMAL_IMPULSE)) return false;
    if(normal_impulse) *normal_impulse = (Vec2D){normal.x * accumulated, normal.y * accumulated};
    if(!friction_impulse) return true;
    relative = (Velocity){
        (moving_second ? velocities[second].x : 0.0f) -
            (moving_first ? velocities[first].x : 0.0f),
        (moving_second ? velocities[second].y : 0.0f) -
            (moving_first ? velocities[first].y : 0.0f)};
    along = math_dot_product(relative, normal);
    Vec2D tangent = {relative.x - normal.x * along, relative.y - normal.y * along};
    float magnitude = sqrtf(tangent.x * tangent.x + tangent.y * tangent.y);
    if(magnitude <= 0.000001f) return true;
    tangent.x /= magnitude; tangent.y /= magnitude;
    delta = -math_dot_product(relative, tangent) / denominator;
    float previous_friction = math_dot_product(*friction_impulse, tangent);
    float maximum = fabsf(accumulated) * sqrtf(frictions[first] * frictions[second]);
    if(!isfinite(magnitude) || !isfinite(delta) || !isfinite(previous_friction) || !isfinite(maximum)) {
        physics_operation_reject(report, ROHR_PHYSICS_FRICTION_IMPULSE, ERROR_ENGINE_STATE_INVALID,
            physics_operation_entity_get(first), physics_operation_entity_get(second));
        return true;
    }
    float friction = fmaxf(-maximum, fminf(previous_friction + delta, maximum));
    delta = friction - previous_friction;
    impulse = (Vec2D){tangent.x * delta, tangent.y * delta};
    changes[0].velocity.x -= impulse.x * inverse_mass_first;
    changes[0].velocity.y -= impulse.y * inverse_mass_first;
    changes[1].velocity.x += impulse.x * inverse_mass_second;
    changes[1].velocity.y += impulse.y * inverse_mass_second;
    if(physics_velocity_pair_commit(changes, report, ROHR_PHYSICS_FRICTION_IMPULSE))
        *friction_impulse = (Vec2D){tangent.x * friction, tangent.y * friction};
    return true;
}

static bool system_contact_point_solve(
    Entity first,
    Entity second,
    OverlapInfo overlap,
    Position point,
    bool restitution_enabled,
    float inverse_mass_first,
    float inverse_mass_second,
    float inverse_inertia_first,
    float inverse_inertia_second,
    const ContactPointInfo *previous,
    const float *previous_rebound,
    float *rebound_speed,
    Velocity *relative_velocity,
    Vec2D *normal_impulse,
    Vec2D *friction_impulse, Report *report, bool compact_velocity
) {
    if(normal_impulse != NULL) *normal_impulse = previous ? previous->normal_impulse : (Vec2D){0};
    if(friction_impulse != NULL) *friction_impulse = previous ? previous->friction_impulse : (Vec2D){0};
    if(physics_simple_baseline_active_check() && compact_velocity &&
            physics_step_entity_components_check(first, ROHR_PARTICLE) &&
            physics_step_entity_components_check(second, ROHR_PARTICLE) &&
            physics_entity_nonrotating_check(first) && physics_entity_nonrotating_check(second))
        return system_particle_point_solve(first, second, overlap.normal, restitution_enabled,
            inverse_mass_first, inverse_mass_second, previous, previous_rebound,
            rebound_speed, relative_velocity, normal_impulse, friction_impulse, report);
    bool moving_first = physics_entity_movable_get(first);
    bool moving_second = physics_entity_movable_get(second);
    Vec2D first_offset = math_vector_subtract(point, system_contact_center_get(first));
    Vec2D second_offset = math_vector_subtract(point, system_contact_center_get(second));
    Vec2D first_angular_velocity = !moving_first ||
            physics_entity_nonrotating_check(first)
        ? (Vec2D){0}
        : math_angular_velocity_cross_vec(angular_velocities[first], first_offset);
    Vec2D second_angular_velocity = !moving_second ||
            physics_entity_nonrotating_check(second)
        ? (Vec2D){0}
        : math_angular_velocity_cross_vec(angular_velocities[second], second_offset);
    Velocity current_relative_velocity = {
        (moving_second ? velocities[second].x : 0.0f) +
            second_angular_velocity.x -
            (moving_first ? velocities[first].x : 0.0f) -
            first_angular_velocity.x,
        (moving_second ? velocities[second].y : 0.0f) +
            second_angular_velocity.y -
            (moving_first ? velocities[first].y : 0.0f) -
            first_angular_velocity.y
    };
    float normal_velocity = math_dot_product(current_relative_velocity, overlap.normal);
    float restitution;
    float first_lever;
    float second_lever;
    float denominator;
    float impulse_magnitude;
    float previous_impulse_magnitude;
    float accumulated_impulse_magnitude;
    Vec2D impulse;

    if(relative_velocity != NULL && physics_vector_finite_check(current_relative_velocity))
        *relative_velocity = current_relative_velocity;
    previous_impulse_magnitude = previous == NULL ? 0.0f :
        math_dot_product(previous->normal_impulse, overlap.normal);
    restitution = restitution_enabled && previous_impulse_magnitude <= 0.0f &&
            normal_velocity < 0.0f
        ? fminf(restitutions[first], restitutions[second]) : 0.0f;
    if(!isfinite(restitutions[first]) || !isfinite(restitutions[second])) {
        physics_operation_reject(report, ROHR_PHYSICS_NORMAL_IMPULSE, ERROR_ENGINE_STATE_INVALID,
            physics_operation_entity_get(first), physics_operation_entity_get(second));
        return false;
    }
    first_lever = math_cross_2d(first_offset, overlap.normal);
    second_lever = math_cross_2d(second_offset, overlap.normal);
    denominator = inverse_mass_first + inverse_mass_second +
        first_lever * first_lever * inverse_inertia_first +
        second_lever * second_lever * inverse_inertia_second;
    if(denominator <= 0.0f) return false;
    float rebound = previous_rebound != NULL ? *previous_rebound :
        -restitution * fminf(normal_velocity, 0.0f);
    *rebound_speed = rebound;
    impulse_magnitude = (rebound - normal_velocity) / denominator;
    if(!isfinite(impulse_magnitude) || !isfinite(denominator) ||
            !isfinite(previous_impulse_magnitude + impulse_magnitude)) {
        physics_operation_reject(report, ROHR_PHYSICS_NORMAL_IMPULSE, ERROR_ENGINE_STATE_INVALID,
            physics_operation_entity_get(first), physics_operation_entity_get(second));
        return false;
    }
    accumulated_impulse_magnitude = fmaxf(
        previous_impulse_magnitude + impulse_magnitude, 0.0f);
    impulse_magnitude = accumulated_impulse_magnitude - previous_impulse_magnitude;
    impulse = (Vec2D){
        overlap.normal.x * impulse_magnitude,
        overlap.normal.y * impulse_magnitude
    };
    if(!system_contact_impulse_commit(first, second, first_offset, second_offset, impulse,
            inverse_mass_first, inverse_mass_second, inverse_inertia_first, inverse_inertia_second,
            compact_velocity, report, ROHR_PHYSICS_NORMAL_IMPULSE)) return false;
    if(normal_impulse != NULL) *normal_impulse = (Vec2D){
        overlap.normal.x * accumulated_impulse_magnitude,
        overlap.normal.y * accumulated_impulse_magnitude
    };
    if(friction_impulse != NULL) {
        *friction_impulse = system_contact_friction_apply(
            first, second, overlap, first_offset, second_offset,
            accumulated_impulse_magnitude,
            inverse_mass_first, inverse_mass_second,
            inverse_inertia_first, inverse_inertia_second,
            previous == NULL ? (Vec2D){0} : previous->friction_impulse, report, compact_velocity);
    }
    return true;
}

/* The baseline circle path has exactly one point and no angular terms. Keep
 * the same response/commit operations without preparing polygon manifolds. */
static ContactInfo system_particle_collision_resolve(EntityIndex first, EntityIndex second,
        OverlapInfo overlap, bool restitution_enabled, const ContactInfo *previous,
        const float *previous_rebounds, float *rebounds, Report *report, bool *applied) {
    ContactInfo result = {.detected = true, .normal = overlap.normal,
        .depth = overlap.depth, .point_count = 1};
    ContactPointInfo *point = &result.points[0];
    point->position = system_collision_contact_point(first, second, overlap);
    float inverse_first = physics_entity_simulated_get(first) &&
        system_contact_response_check(first, second) ? 1.0f / mass[first] : 0.0f;
    float inverse_second = physics_entity_simulated_get(second) &&
        system_contact_response_check(second, first) ? 1.0f / mass[second] : 0.0f;
    if(inverse_first + inverse_second <= 0) return result;
    const ContactPointInfo *old = previous && previous->point_count == 1
        ? &previous->points[0] : NULL;
    float rebound = old ? previous_rebounds[0] :
        system_contact_rebound_get(first, second, point->position, overlap.normal, restitution_enabled);
    point->normal_impulse = old ? old->normal_impulse : (Vec2D){0};
    point->friction_impulse = old ? old->friction_impulse : (Vec2D){0};
    if(system_particle_point_solve(first, second, overlap.normal, restitution_enabled,
            inverse_first, inverse_second, old, &rebound, &rebounds[0],
            &point->relative_velocity, &point->normal_impulse, &point->friction_impulse, report))
        *applied = true;
    return result;
}

ContactInfo system_resolve_collision(
    Entity first,
    Entity second,
    OverlapInfo overlap,
    bool restitution_enabled,
    const ContactInfo *previous_contact, const float *previous_rebounds,
    float *rebounds, Report *report, bool *applied
) {
    bool first_particle = physics_step_entity_components_check(first, ROHR_PARTICLE);
    bool second_particle = physics_step_entity_components_check(second, ROHR_PARTICLE);
    if(physics_simple_baseline_active_check() && first_particle && second_particle &&
            physics_entity_nonrotating_check(first) && physics_entity_nonrotating_check(second))
        return system_particle_collision_resolve(first, second, overlap, restitution_enabled,
            previous_contact, previous_rebounds, rebounds, report, applied);
    float inverse_mass_first = physics_entity_simulated_get(first) &&
        system_contact_response_check(first, second)
        ? 1.0f / mass[first] : 0.0f;
    float inverse_mass_second = physics_entity_simulated_get(second) &&
        system_contact_response_check(second, first)
        ? 1.0f / mass[second] : 0.0f;
    float inverse_inertia_first = 0.0f;
    float inverse_inertia_second = 0.0f;
    ContactInfo result = {
        .detected = true,
        .normal = overlap.normal,
        .depth = overlap.depth
    };
    ContactManifold manifold = {0};

    if(!(first_particle && second_particle)) {
        manifold = contact_manifold_polygon_get(
            world_hit_boxes[first], world_hit_boxes[second], overlap.normal);
    }
    if(manifold.count == 0) {
        manifold.points[0] = system_collision_contact_point(first, second, overlap);
        manifold.count = 1;
    }
    result.point_count = manifold.count;
    for(uint8_t i = 0; i < manifold.count; i += 1) {
        result.points[i].feature_id = manifold.feature_ids[i];
        result.points[i].position = manifold.points[i];
    }
    if(inverse_mass_first + inverse_mass_second <= 0.0f) return result;
    if(!physics_entity_nonrotating_check(first) && inverse_mass_first > 0.0f) {
        inverse_inertia_first = physics_inverse_inertia_by_index_get(first);
    }
    if(!physics_entity_nonrotating_check(second) && inverse_mass_second > 0.0f) {
        inverse_inertia_second = physics_inverse_inertia_by_index_get(second);
    }
    float initial_rebounds[PHYSICS_CONTACT_POINT_MAX];
    for(uint8_t i=0;i<result.point_count;++i)
        /* A retained one-point target already describes the impact. */
        initial_rebounds[i] = result.point_count == 1 && previous_contact != NULL &&
            previous_contact->point_count == 1 ? previous_rebounds[0] :
            system_contact_rebound_get(first, second, result.points[i].position,
                overlap.normal, restitution_enabled);
    for(uint8_t i = 0; i < result.point_count; i += 1) {
        const ContactPointInfo *previous_point = NULL;
        const float *previous_rebound = &initial_rebounds[i];
        if(previous_contact != NULL) {
            for(uint8_t previous = 0; previous < previous_contact->point_count;
                    previous += 1) {
                if((result.point_count == 1 && previous_contact->point_count == 1) ||
                        (result.points[i].feature_id != 0 &&
                        result.points[i].feature_id ==
                            previous_contact->points[previous].feature_id)) {
                    previous_point = &previous_contact->points[previous];
                    previous_rebound = &previous_rebounds[previous];
                    break;
                }
            }
        }
        bool committed = system_contact_point_solve(
            first, second, overlap, result.points[i].position, restitution_enabled,
            inverse_mass_first, inverse_mass_second,
            inverse_inertia_first, inverse_inertia_second,
            previous_point, previous_rebound, &rebounds[i],
            &result.points[i].relative_velocity,
            &result.points[i].normal_impulse,
            &result.points[i].friction_impulse, report, first_particle || second_particle);
        if(committed) *applied = true;
    }
    return result;
}

static ContactInfo system_resolve_collision_manifold(
    Entity first,
    Entity second,
    const ContactManifold *manifold,
    bool restitution_enabled,
    const ContactInfo *previous_contact, const float *previous_rebounds,
    float *rebounds, Report *report, bool *applied
) {
    ContactInfo result;
    OverlapInfo overlap;

    if(manifold == NULL || manifold->count == 0) return (ContactInfo){0};
    overlap = (OverlapInfo){
        .detected = true,
        .normal = manifold->normal,
        .depth = manifold->depth
    };
    {
        float inverse_mass_first = physics_entity_simulated_get(first) &&
        system_contact_response_check(first, second)
            ? 1.0f / mass[first] : 0.0f;
        float inverse_mass_second = physics_entity_simulated_get(second) &&
        system_contact_response_check(second, first)
            ? 1.0f / mass[second] : 0.0f;
        float inverse_inertia_first = 0.0f;
        float inverse_inertia_second = 0.0f;

        if(!physics_entity_nonrotating_check(first) && inverse_mass_first > 0.0f) {
            inverse_inertia_first = physics_inverse_inertia_by_index_get(first);
        }
        if(!physics_entity_nonrotating_check(second) && inverse_mass_second > 0.0f) {
            inverse_inertia_second = physics_inverse_inertia_by_index_get(second);
        }
        result = (ContactInfo){
            .detected = true,
            .normal = overlap.normal,
            .depth = overlap.depth,
            .point_count = manifold->count
        };
        bool compact_velocity = physics_step_entity_components_check(first, ROHR_PARTICLE) ||
            physics_step_entity_components_check(second, ROHR_PARTICLE);
        const ContactPointInfo *previous_points[PHYSICS_CONTACT_POINT_MAX] = {0};
        float initial_rebounds[PHYSICS_CONTACT_POINT_MAX];
        /* Match retained targets before any point changes velocity. New targets
         * must all observe the incoming motion, including the second point. */
        for(uint8_t i = 0; i < manifold->count; ++i) {
            result.points[i].feature_id = manifold->feature_ids[i];
            result.points[i].position = manifold->points[i];
            if(previous_contact != NULL) {
                for(uint8_t previous = 0; previous < previous_contact->point_count; ++previous) {
                    if(manifold->feature_ids[i] != 0 && manifold->feature_ids[i] ==
                            previous_contact->points[previous].feature_id) {
                        previous_points[i] = &previous_contact->points[previous];
                        initial_rebounds[i] = previous_rebounds[previous];
                        break;
                    }
                }
            }
            if(previous_points[i] == NULL)
                initial_rebounds[i] = system_contact_rebound_get(first, second,
                    manifold->points[i], overlap.normal, restitution_enabled);
        }
        for(uint8_t i = 0; i < manifold->count; ++i) {
            bool committed = system_contact_point_solve(
                first, second, overlap, result.points[i].position,
                restitution_enabled, inverse_mass_first, inverse_mass_second,
                inverse_inertia_first, inverse_inertia_second, previous_points[i],
                &initial_rebounds[i], &rebounds[i],
                &result.points[i].relative_velocity,
                &result.points[i].normal_impulse,
                &result.points[i].friction_impulse, report, compact_velocity);
            if(committed) *applied = true;
        }
    }
    return result;
}

static bool broadphase_refit, broadphase_changed[MAX_ENTITIES];
static uint32_t broadphase_query_order[MAX_ENTITIES];
/* Prepared query sources retain collider iteration order for this gather. */
static EntityIndex broadphase_query_indices[MAX_ENTITIES];
static uint32_t broadphase_query_count;
/* Only complete synchronous substeps can rely on engine pose commits.
 * Manual stages still scan current poses, including direct-array edits. */
static bool broadphase_tracking;
static uint32_t broadphase_pose_pending[(MAX_ENTITIES + 31) / 32];
static uint32_t broadphase_pose_order[MAX_ENTITIES];
static EntityIndex broadphase_pose_indices[MAX_ENTITIES];
static uint32_t broadphase_pose_count;

void physics_rigid_gather_tracking_set(bool enabled) {
    broadphase_tracking = enabled;
    memset(broadphase_pose_pending, 0, sizeof(broadphase_pose_pending));
}

void physics_rigid_gather_pose_dirty_add(EntityIndex index) {
    if(!broadphase_tracking || !physics_step_entities_active_check() ||
            index >= MAX_ENTITIES ||
            !physics_step_entity_components_check(index, ROHR_HIT_BOX)) return;
    uint32_t order = broadphase_pose_order[index];
    broadphase_pose_pending[order / 32] |= UINT32_C(1) << (31 - order % 32);
}

static void system_broadphase_pose_sources_prepare(void) {
    broadphase_pose_count = 0;
    uint32_t count = physics_step_collider_iteration_count_get();
    for(uint32_t word = 0; word < (count + 31) / 32; ++word) {
        uint32_t pending = broadphase_pose_pending[word];
        broadphase_pose_pending[word] = 0;
        while(pending) {
            unsigned bit = (unsigned)SDL_MostSignificantBitIndex32(pending);
            uint32_t order = word * 32 + 31 - bit;
            pending &= ~(UINT32_C(1) << bit);
            EntityIndex index;
            if(physics_step_collider_index_at(order, &index))
                broadphase_pose_indices[broadphase_pose_count++] = index;
        }
    }
}

/* Valid only with the matching broadphase leaf: refreshed on build/refit and
 * borrowed during a gather, whose callbacks never mutate poses or geometry. */
static PhysicsParticleCircle broadphase_circles[MAX_ENTITIES];
/* One-gather scratch storage; circles occupy one cell whose width is at least
 * the largest diameter. Testing the eight neighboring cells is conservative. */
#define PARTICLE_GRID_BUCKETS 4096
static bool particle_grid_active;
static EntityIndex particle_grid_indices[MAX_ENTITIES];
static uint32_t particle_grid_count;
static AABB particle_grid_bounds[MAX_ENTITIES];
static int particle_grid_heads[PARTICLE_GRID_BUCKETS], particle_grid_next[MAX_ENTITIES];
static int particle_grid_x[MAX_ENTITIES], particle_grid_y[MAX_ENTITIES];
static float particle_grid_width;

static unsigned system_particle_grid_bucket_get(int x, int y) {
    return ((uint32_t)x * UINT32_C(73856093) ^ (uint32_t)y * UINT32_C(19349663)) &
        (PARTICLE_GRID_BUCKETS - 1);
}

typedef struct SystemBroadphasePair {
    Entity first, second;
    EntityIndex first_index, second_index;
} SystemBroadphasePair;

typedef struct SystemBroadphaseQuery {
    Entity source;
    EntityIndex source_index;
    Report *report;
    bool refit;
    bool particle_tree_query;
    unsigned particle_count;
    SystemBroadphasePair particles[4];
} SystemBroadphaseQuery;

static bool system_broadphase_pair_record(
    const SystemBroadphasePair *pair, OverlapInfo overlap, Report *report
) {
    if(!overlap.detected) return true;
    physics_report_counter_add(report, ROHR_REPORT_OVERLAP_COUNT, 1);
    bool responds = physics_step_entity_components_check(pair->first_index, ROHR_COLLISION) &&
        physics_step_entity_components_check(pair->second_index, ROHR_COLLISION) &&
        physics_collision_response_between_by_index_check(pair->first_index, pair->second_index);
    bool appended = physics_simple_baseline_active_check() ?
        contact_constraint_list_rigid_append(&physics_step_contact_constraints,
            pair->first, pair->second, pair->first_index, pair->second_index, overlap, responds) :
        contact_constraint_list_append(&physics_step_contact_constraints,
        (SystemContactConstraint){
        .type = SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR,
        .value.rigid = {
            .first = pair->first,
            .second = pair->second,
            .first_index = pair->first_index,
            .second_index = pair->second_index,
            .overlap = overlap,
            .responds = responds
        }
    });
    if(!appended)
        physics_report_issue_add(report, ROHR_PHYSICS_CONTACT_GATHER, ROHR_REPORT_SEVERITY_FATAL,
            ERROR_MEMORY_POOL_ALLOCATION_FAILED, ROHR_REPORT_UPDATE_ABORTED,
            pair->first, pair->second, "could not allocate rigid contact constraint");
    return appended;
}

static bool system_broadphase_particles_flush(SystemBroadphaseQuery *query) {
    unsigned count = query->particle_count;
    if(count == 0) return true;
    query->particle_count = 0;
    physics_report_counter_add(query->report, ROHR_REPORT_CANDIDATE_COUNT, count);
    physics_report_counter_add(query->report, ROHR_REPORT_NARROWPHASE_TEST_COUNT, count);
    uint64_t started = SDL_GetPerformanceCounter();
    PhysicsParticleCircle first[4], second[4];
    OverlapInfo overlaps[4];
    for(unsigned i = 0; i < count; ++i) {
        first[i] = broadphase_circles[query->particles[i].first_index];
        second[i] = broadphase_circles[query->particles[i].second_index];
    }
    if(count == 4) physics_particle_circles_overlap_get(first, second, overlaps);
    else for(unsigned i = 0; i < count; ++i)
        overlaps[i] = physics_particle_circle_overlap_get(first[i], second[i]);
    physics_report_counter_add(query->report, ROHR_REPORT_NARROWPHASE_NS,
        (uint64_t)(physics_rigid_elapsed_ms(started) * 1000000.0));
    /* Publish in original traversal order, including sensors. All lanes above
     * were tested even if a later allocation failure stops publication. */
    for(unsigned i = 0; i < count; ++i)
        if(!system_broadphase_pair_record(&query->particles[i], overlaps[i], query->report))
            return false;
    return true;
}

static bool system_broadphase_pair_apply(Entity target, void *context) {
    SystemBroadphaseQuery *query = context;
    EntityIndex target_index;
    if(query == NULL || target == query->source ||
            (!query->refit && !query->particle_tree_query && target < query->source) ||
            !entity_index_get(target, &target_index) ||
            !physics_step_entity_alive_check(target_index)) return true;
    /* Grid particles query polygons; polygon sources must not visit those
     * mixed pairs again. */
    if(particle_grid_active && !query->particle_tree_query &&
            physics_step_entity_components_check(target_index, ROHR_PARTICLE)) return true;
    /* Bounds and poses stay fixed throughout this gather. If both endpoints
     * moved, the earlier query already visited this pair, including misses.
     * Use iteration order, not handle order: reused slots can reorder handles.
     * A later gather must test the pair again after either endpoint moves. */
    if(query->refit && broadphase_changed[target_index] &&
            broadphase_query_order[target_index] <
                broadphase_query_order[query->source_index]) return true;
    SystemBroadphasePair pair = target < query->source ?
        (SystemBroadphasePair){target, query->source, target_index, query->source_index} :
        (SystemBroadphasePair){query->source, target, query->source_index, target_index};
    if(contact_constraint_list_rigid_pair_check(&physics_step_contact_constraints,
            pair.first, pair.second)) return true;
    if(physics_step_entity_components_check(pair.first_index, ROHR_HIT_BOX | ROHR_PARTICLE) &&
            physics_step_entity_components_check(pair.second_index, ROHR_HIT_BOX | ROHR_PARTICLE) &&
            world_hit_boxes[pair.first_index].amount_of_vertices != 0 &&
            world_hit_boxes[pair.second_index].amount_of_vertices != 0) {
        query->particles[query->particle_count++] = pair;
        return query->particle_count < 4 || system_broadphase_particles_flush(query);
    }
    /* Mixed geometry and invalidated leaves keep the original path. Flush
     * pending circles first so no contact is reordered across a polygon pair. */
    if(!system_broadphase_particles_flush(query)) return false;
    physics_report_counter_add(query->report, ROHR_REPORT_CANDIDATE_COUNT, 1);
    physics_report_counter_add(query->report, ROHR_REPORT_NARROWPHASE_TEST_COUNT, 1);
    uint64_t started = SDL_GetPerformanceCounter();
    OverlapInfo overlap = system_entity_overlap_get(pair.first_index, pair.second_index);
    physics_report_counter_add(query->report, ROHR_REPORT_NARROWPHASE_NS,
        (uint64_t)(physics_rigid_elapsed_ms(started) * 1000000.0));
    return system_broadphase_pair_record(&pair, overlap, query->report);
}

void physics_rigid_contact_point_impulses_accumulate(
    ContactInfo *current,
    const ContactInfo *previous
) {
    bool matched[PHYSICS_CONTACT_POINT_MAX] = {0};

    if(current == NULL || previous == NULL) return;
    for(uint8_t previous_index = 0;
            previous_index < previous->point_count;
            previous_index += 1) {
        uint8_t nearest = PHYSICS_CONTACT_POINT_MAX;
        float nearest_distance = FLT_MAX;

        for(uint8_t current_index = 0;
                current_index < current->point_count;
                current_index += 1) {
            Vec2D delta;
            float distance;

            if(matched[current_index]) continue;
            if(previous->points[previous_index].feature_id != 0 &&
                    previous->points[previous_index].feature_id ==
                        current->points[current_index].feature_id) {
                nearest = current_index;
                nearest_distance = 0.0f;
                break;
            }
            delta = math_vector_subtract(
                current->points[current_index].position,
                previous->points[previous_index].position);
            distance = math_dot_product(delta, delta);
            if(distance < nearest_distance) {
                nearest = current_index;
                nearest_distance = distance;
            }
        }
        if(nearest >= current->point_count) continue;
        matched[nearest] = true;
        current->points[nearest].normal_impulse.x +=
            previous->points[previous_index].normal_impulse.x;
        current->points[nearest].normal_impulse.y +=
            previous->points[previous_index].normal_impulse.y;
        current->points[nearest].friction_impulse.x +=
            previous->points[previous_index].friction_impulse.x;
        current->points[nearest].friction_impulse.y +=
            previous->points[previous_index].friction_impulse.y;
    }
}

static ContactManifoldSet system_rigid_manifolds_get(
    SystemContactConstraint *constraint
) {
    EntityIndex bodies[] = {constraint->value.rigid.first_index,
        constraint->value.rigid.second_index};
    bool unchanged = constraint->value.rigid.block_geometry_valid;
    for(unsigned side = 0; side < 2 && unchanged; ++side) {
        Position previous = constraint->value.rigid.block_positions[side];
        unchanged = previous.x == positions[bodies[side]].x &&
            previous.y == positions[bodies[side]].y &&
            constraint->value.rigid.block_orientations[side] == orientations[bodies[side]];
    }
    if(!unchanged) {
        constraint->value.rigid.block_geometry = contact_manifold_set_polygon_get(
            world_hit_boxes[bodies[0]], world_hit_boxes[bodies[1]]);
        for(unsigned side = 0; side < 2; ++side) {
            constraint->value.rigid.block_positions[side] = positions[bodies[side]];
            constraint->value.rigid.block_orientations[side] = orientations[bodies[side]];
        }
        constraint->value.rigid.block_geometry_valid = true;
    }
    return constraint->value.rigid.block_geometry;
}

static bool system_rigid_contact_identity_check(const SystemContactConstraint *constraint) {
    Entity first, second;
    return constraint != NULL && constraint->type == SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR &&
        physics_step_entity_from_index_get(constraint->value.rigid.first_index, &first) &&
        first == constraint->value.rigid.first &&
        physics_step_entity_from_index_get(constraint->value.rigid.second_index, &second) &&
        second == constraint->value.rigid.second;
}

static void system_rigid_contact_constraint_solve(
    SystemContactConstraint *constraint,
    float position_fraction, Report *report
) {
    ContactInfo representative = {0};
    ContactManifoldSet manifolds;
    uint8_t previous_contact_count;
    EntityIndex first;
    EntityIndex second;
    OverlapInfo overlap;
    bool responds;
    bool first_solve;
    bool normal_applied = false;

    /* Manual callers may delete and reuse either slot between stages. */
    if(!system_rigid_contact_identity_check(constraint)) return;
    first = constraint->value.rigid.first_index;
    second = constraint->value.rigid.second_index;
    bool particles = physics_step_entity_components_check(first, ROHR_PARTICLE) &&
        physics_step_entity_components_check(second, ROHR_PARTICLE);
    PhysicsParticleCircle first_circle, second_circle;
    bool geometry_valid = particles ?
        system_particle_contact_circle_get(first, &first_circle, report) &&
            system_particle_contact_circle_get(second, &second_circle, report) :
        physics_step_hitbox_update(first, report) && physics_step_hitbox_update(second, report);
    if(!geometry_valid) {
        constraint->value.rigid.overlap = (OverlapInfo){0};
        constraint->value.rigid.contact = (ContactInfo){0};
        return;
    }
    responds = constraint->value.rigid.responds;
    if(responds && !particles) {
        /* The manifold already performs SAT. Do not repeat that work just to
         * check overlap; retain SAT fallback for overlap without clip points. */
        manifolds = system_rigid_manifolds_get(constraint);
        overlap = manifolds.count ? (OverlapInfo){.detected = true} :
            system_entity_overlap_get(first, second);
    } else if(particles) {
        overlap = physics_particle_circle_overlap_get(first_circle, second_circle);
    } else overlap = system_entity_overlap_get(first, second);
    if(!overlap.detected) return;
    if(!responds) {
        constraint->value.rigid.manifold_contact_count = 0;
        constraint->value.rigid.contact = (ContactInfo){0};
        constraint->value.rigid.overlap = overlap;
        return;
    }
    first_solve = !constraint->value.rigid.solved;
    previous_contact_count = constraint->value.rigid.manifold_contact_count;
    if(particles) {
        /* Circle pairs have one contact. Keep its previous target separate from
         * the output, but avoid preparing eight polygon manifolds on every
         * particle solve. The previous contact is read before it is replaced. */
        float previous_rebounds[PHYSICS_CONTACT_POINT_MAX];
        for(uint8_t p = 0; p < PHYSICS_CONTACT_POINT_MAX; ++p)
            previous_rebounds[p] = constraint->value.rigid.rebound_speeds[0][p];
        representative = system_resolve_collision(
            first, second, overlap, first_solve,
            previous_contact_count > 0 ? &constraint->value.rigid.manifold_contacts[0] : NULL,
            previous_rebounds, constraint->value.rigid.rebound_speeds[0], report, &normal_applied);
        constraint->value.rigid.manifold_contact_count = representative.detected;
        if(representative.detected)
            constraint->value.rigid.manifold_contacts[0] = representative;
        constraint->value.rigid.contact = representative;
        constraint->value.rigid.overlap = overlap;
        constraint->value.rigid.solved |= normal_applied;
        if(responds) {
            overlap.depth *= position_fraction;
            system_separate_entities_tuned(first, second, overlap, report);
            physics_step_hitbox_dirty_add(first);
            physics_step_hitbox_dirty_add(second);
        }
        return;
    }
    ContactInfo previous_contacts[CONTACT_MANIFOLD_MAX];
    float previous_rebounds[CONTACT_MANIFOLD_MAX][PHYSICS_CONTACT_POINT_MAX];
    bool previous_contact_matched[CONTACT_MANIFOLD_MAX] = {0};
    for(uint8_t i = 0; i < previous_contact_count; ++i) {
        previous_contacts[i] = constraint->value.rigid.manifold_contacts[i];
        for(uint8_t p = 0; p < PHYSICS_CONTACT_POINT_MAX; ++p)
            previous_rebounds[i][p] = constraint->value.rigid.rebound_speeds[i][p];
    }
    constraint->value.rigid.manifold_contact_count = 0;
    for(uint8_t i = 0; responds && i < manifolds.count; i += 1) {
        const ContactInfo *previous = NULL;
        const float *rebounds = NULL;
        ContactInfo contact;

        for(uint8_t candidate = 0; candidate < previous_contact_count;
                candidate += 1) {
            if(previous_contact_matched[candidate] ||
                    math_dot_product(previous_contacts[candidate].normal,
                        manifolds.values[i].normal) < 0.98f) continue;
            previous = &previous_contacts[candidate];
            rebounds = previous_rebounds[candidate];
            previous_contact_matched[candidate] = true;
            break;
        }
        contact = system_resolve_collision_manifold(
            first, second, &manifolds.values[i], first_solve, previous, rebounds,
            constraint->value.rigid.rebound_speeds[constraint->value.rigid.manifold_contact_count],
            report, &normal_applied);

        constraint->value.rigid.manifold_contacts[
            constraint->value.rigid.manifold_contact_count++] = contact;
        if(!representative.detected || contact.depth > representative.depth)
            representative = contact;
        overlap = (OverlapInfo){
            .detected = true,
            .normal = manifolds.values[i].normal,
            .depth = manifolds.values[i].depth * position_fraction
        };
        system_separate_entities_tuned(first, second, overlap, report);
        (void)physics_step_hitbox_update(first, report);
        (void)physics_step_hitbox_update(second, report);
    }
    if(!responds) representative = (ContactInfo){0};
    constraint->value.rigid.contact = representative;
    constraint->value.rigid.overlap = representative.detected
        ? (OverlapInfo){true, representative.normal, representative.depth}
        : overlap;
    constraint->value.rigid.solved |= normal_applied;
    if(responds) {
        if(first_solve) physics_report_counter_add(report, ROHR_REPORT_CONTACT_COUNT, 1);
        physics_step_hitbox_dirty_add(first);
        physics_step_hitbox_dirty_add(second);
    }
}

static AABB system_broadphase_bounds_get(EntityIndex index) {
    /* Read only the live vertices; do not copy a full fixed-capacity Shape. */
    const Shape *shape = &world_hit_boxes[index];
    AABB bounds = {.min_x = FLT_MAX, .max_x = -FLT_MAX,
        .min_y = FLT_MAX, .max_y = -FLT_MAX};
    if(shape->amount_of_vertices == 0) bounds = (AABB){0};
    for(uint16_t n = 0; n < shape->amount_of_vertices; ++n) {
        Vec2D vertex = shape->vertices[n];
        if(vertex.x < bounds.min_x) bounds.min_x = vertex.x;
        if(vertex.x > bounds.max_x) bounds.max_x = vertex.x;
        if(vertex.y < bounds.min_y) bounds.min_y = vertex.y;
        if(vertex.y > bounds.max_y) bounds.max_y = vertex.y;
    }

    if(physics_step_entity_components_check(index, ROHR_PARTICLE)) {
        Position center = physics_particle_world_origin_by_index_get(index);
        float radius = physics_particle_radius_by_index_get(index);
        broadphase_circles[index] = (PhysicsParticleCircle){center, radius};
        bounds.min_x = fminf(bounds.min_x, center.x - radius);
        bounds.max_x = fmaxf(bounds.max_x, center.x + radius);
        bounds.min_y = fminf(bounds.min_y, center.y - radius);
        bounds.max_y = fmaxf(bounds.max_y, center.y + radius);
    }
    return bounds;
}

static uint32_t broadphase_leaves[MAX_ENTITIES];
static Position broadphase_positions[MAX_ENTITIES];
static Orientation broadphase_orientations[MAX_ENTITIES];

static void system_broadphase_build(Report *report) {
    bool refit = broadphase_reuse_allowed && physics_step_contact_constraints.solving &&
        physics_step_contact_constraints.rigid_gathered;
    broadphase_refit = refit;
    /* Preserve tree contact order for beam-coupled scenes: the grid's changed
     * order regressed node overlap without a worthwhile measured car saving.
     * Manual gathers also always retain the complete tree. */
    particle_grid_active = physics_simple_baseline_active_check() && !refit &&
        soft_body_beams_pool.count == 0;
    particle_grid_count = 0;
    particle_grid_width = 1.0f;
    if(particle_grid_active) {
        double diameter_sum = 0;
        uint32_t count = 0;
        for(uint32_t n = 0; n < physics_step_collider_iteration_count_get(); ++n) {
            EntityIndex index;
            if(!physics_step_collider_index_at(n, &index) ||
                    !physics_step_entity_components_check(index, ROHR_HIT_BOX | ROHR_PARTICLE)) continue;
            double diameter = 2.0 * physics_particle_radius_by_index_get(index);
            diameter_sum += diameter;
            particle_grid_width = fmaxf(particle_grid_width, (float)diameter);
            ++count;
        }
        /* A large outlier makes a single-resolution grid scan dense cells.
         * Choose ordinary tree traversal if the largest diameter exceeds four
         * times the mean. This changes neither shapes nor response effort. */
        if(!count || !isfinite(diameter_sum) || !isfinite(particle_grid_width) ||
                particle_grid_width > 4.0 * diameter_sum / count)
            particle_grid_active = false;
    }
    broadphase_query_count = 0;
    memset(broadphase_changed, 0, sizeof(broadphase_changed));
    if(refit) {
        bool tracked = broadphase_tracking && physics_step_entities_active_check();
        if(tracked) system_broadphase_pose_sources_prepare();
        uint32_t count = tracked ? broadphase_pose_count : physics_step_collider_iteration_count_get();
        for(uint32_t n = 0; n < count; ++n) {
            EntityIndex i;
            if(tracked) i = broadphase_pose_indices[n];
            else if(!physics_step_collider_index_at(n, &i)) continue;
            if(broadphase_leaves[i] == AABB_TREE_NODE_INVALID) continue;
            if(positions[i].x == broadphase_positions[i].x &&
                    positions[i].y == broadphase_positions[i].y &&
                    orientations[i] == broadphase_orientations[i]) continue;
            if(!physics_step_hitbox_update(i, report)) continue;
            (void)aabb_tree_leaf_bounds_set(&physics_broadphase_tree,
                broadphase_leaves[i], system_broadphase_bounds_get(i));
            broadphase_positions[i] = positions[i];
            broadphase_orientations[i] = orientations[i];
            broadphase_changed[i] = true;
            broadphase_query_order[i] = tracked ? broadphase_pose_order[i] : n;
            broadphase_query_indices[broadphase_query_count++] = i;
        }
        return;
    }
    memset(broadphase_pose_pending, 0, sizeof(broadphase_pose_pending));
    aabb_tree_clear(&physics_broadphase_tree);
    memset(broadphase_leaves, 0xff, sizeof(broadphase_leaves));
    for(uint32_t alive_position = 0;
            alive_position < physics_step_collider_iteration_count_get();
            alive_position += 1) {
        EntityIndex index;
        Entity entity;

        if(!physics_step_collider_index_at(alive_position, &index) ||
                !physics_step_entity_components_check(index, ROHR_HIT_BOX) ||
                !physics_step_entity_from_index_get(index, &entity)) continue;
        broadphase_pose_order[index] = alive_position;
        if(!physics_step_hitbox_update(index, report)) continue;
        physics_report_counter_add(report, ROHR_REPORT_COLLIDER_COUNT, 1);
        AABB bounds = system_broadphase_bounds_get(index);
        if(particle_grid_active && physics_step_entity_components_check(index, ROHR_PARTICLE)) {
            particle_grid_indices[particle_grid_count++] = index;
            particle_grid_bounds[index] = bounds;
            broadphase_query_indices[broadphase_query_count++] = index;
            continue;
        }
        uint32_t leaf = (uint32_t)physics_broadphase_tree.count;
        EngineResult result = aabb_tree_insert(
            &physics_broadphase_tree, entity, bounds);
        if(result.kind == ERROR_RESULT_ERROR) {
            physics_report_issue_add(report, ROHR_PHYSICS_BROADPHASE, ROHR_REPORT_SEVERITY_FATAL,
                result.result.error, ROHR_REPORT_UPDATE_ABORTED, entity, 0,
                "could not build broadphase tree");
            return;
        }
        broadphase_leaves[index] = leaf;
        broadphase_query_indices[broadphase_query_count++] = index;
        broadphase_positions[index] = positions[index];
        broadphase_orientations[index] = orientations[index];
    }
    broadphase_reuse_allowed = physics_step_contact_constraints.solving;
}

/* Maximum eligible-order key in each current subtree. An unchanged target
 * uses UINT32_MAX during refits, because only its moved partner will query. */
static uint32_t broadphase_subtree_limits[2 * MAX_ENTITIES];
static uint32_t system_broadphase_limit_prepare(uint32_t index) {
    const AABBTreeNode *node = &physics_broadphase_tree.nodes[index];
    uint32_t limit;
    if(aabb_tree_node_leaf_check(node)) {
        limit = node->entity;
        if(broadphase_refit) {
            EntityIndex entity_index;
            limit = entity_index_get(node->entity, &entity_index) && broadphase_changed[entity_index]
                ? broadphase_query_order[entity_index] + 1 : UINT32_MAX;
        }
    } else {
        uint32_t left = system_broadphase_limit_prepare(node->left);
        uint32_t right = system_broadphase_limit_prepare(node->right);
        limit = left > right ? left : right;
    }
    broadphase_subtree_limits[index] = limit;
    return limit;
}

static bool system_broadphase_query(AABB bounds, SystemBroadphaseQuery *query) {
    if(physics_broadphase_tree.root == AABB_TREE_NODE_INVALID) return false;
    uint32_t threshold = query->refit ? broadphase_query_order[query->source_index] + 1 : query->source;
    size_t count = 0;
    physics_broadphase_tree.query_stack[count++] = physics_broadphase_tree.root;
    while(count) {
        uint32_t index = physics_broadphase_tree.query_stack[--count];
        if(!query->particle_tree_query && broadphase_subtree_limits[index] <= threshold) continue;
        const AABBTreeNode *node = &physics_broadphase_tree.nodes[index];
        if(!math_aabb_overlap(bounds, node->bounds)) continue;
        if(aabb_tree_node_leaf_check(node)) {
            if(!system_broadphase_pair_apply(node->entity, query)) return false;
        } else {
            physics_broadphase_tree.query_stack[count++] = node->left;
            physics_broadphase_tree.query_stack[count++] = node->right;
        }
    }
    return true;
}

static void system_particle_grid_gather(SystemBroadphaseQuery *query) {
    memset(particle_grid_heads, 0xff, sizeof(particle_grid_heads));
    bool coordinates_valid = true;
    for(uint32_t n = 0; n < particle_grid_count; ++n) {
        EntityIndex index = particle_grid_indices[n];
        PhysicsParticleCircle circle = broadphase_circles[index];
        double cell_x = floor((double)circle.center.x / particle_grid_width);
        double cell_y = floor((double)circle.center.y / particle_grid_width);
        if(!isfinite(cell_x) || !isfinite(cell_y) ||
                cell_x < INT_MIN + 1 || cell_x > INT_MAX - 1 ||
                cell_y < INT_MIN + 1 || cell_y > INT_MAX - 1) {
            coordinates_valid = false;
            break;
        }
        int x = (int)cell_x, y = (int)cell_y;
        particle_grid_x[n] = x; particle_grid_y[n] = y;
        unsigned bucket = system_particle_grid_bucket_get(x, y);
        particle_grid_next[n] = particle_grid_heads[bucket];
        particle_grid_heads[bucket] = (int)n;
    }
    if(!coordinates_valid) {
        /* Malformed/extreme offsets must not overflow a cell coordinate or
         * silently omit a pair. Keep the validated geometry's scalar fallback. */
        for(uint32_t n = 0; n < particle_grid_count; ++n)
            for(uint32_t m = n + 1; m < particle_grid_count; ++m) {
                EntityIndex a = particle_grid_indices[n], b = particle_grid_indices[m];
                Entity first, second;
                if(!physics_step_entity_from_index_get(a, &first) ||
                        !physics_step_entity_from_index_get(b, &second)) continue;
                query->particles[query->particle_count++] = first < second ?
                    (SystemBroadphasePair){first, second, a, b} :
                    (SystemBroadphasePair){second, first, b, a};
                if(query->particle_count == 4 && !system_broadphase_particles_flush(query)) return;
            }
        (void)system_broadphase_particles_flush(query);
        return;
    }
    for(uint32_t n = 0; n < particle_grid_count; ++n) {
        EntityIndex first_index = particle_grid_indices[n];
        Entity first;
        if(!physics_step_entity_from_index_get(first_index, &first)) continue;
        PhysicsParticleCircle a = broadphase_circles[first_index];
        for(int dx = -1; dx <= 1; ++dx) for(int dy = -1; dy <= 1; ++dy) {
            int x = particle_grid_x[n] + dx, y = particle_grid_y[n] + dy;
            unsigned bucket = system_particle_grid_bucket_get(x, y);
            for(int other = particle_grid_heads[bucket]; other >= 0; other = particle_grid_next[other]) {
                if(particle_grid_x[other] != x || particle_grid_y[other] != y) continue;
                EntityIndex second_index = particle_grid_indices[other];
                Entity second;
                if(!physics_step_entity_from_index_get(second_index, &second) || second <= first) continue;
                PhysicsParticleCircle b = broadphase_circles[second_index];
                float radius = a.radius + b.radius;
                if(fabsf(a.center.x - b.center.x) > radius || fabsf(a.center.y - b.center.y) > radius) continue;
                query->particles[query->particle_count++] = (SystemBroadphasePair){
                    first, second, first_index, second_index};
                if(query->particle_count == 4 && !system_broadphase_particles_flush(query)) return;
            }
        }
    }
    (void)system_broadphase_particles_flush(query);
}

static void system_broadphase_collisions_apply(Report *report) {
    SystemBroadphaseQuery query = {.report = report, .refit = broadphase_refit};
    if(broadphase_query_count && physics_broadphase_tree.root != AABB_TREE_NODE_INVALID)
        (void)system_broadphase_limit_prepare(physics_broadphase_tree.root);
    for(uint32_t n = 0; n < broadphase_query_count; ++n) {
        EntityIndex index = broadphase_query_indices[n];
        Entity entity;
        if(!physics_step_entity_from_index_get(index, &entity)) continue;
        query.source = entity;
        query.source_index = index;
        query.particle_tree_query = particle_grid_active &&
            physics_step_entity_components_check(index, ROHR_PARTICLE);
        (void)system_broadphase_query(
            query.particle_tree_query ? particle_grid_bounds[index] :
                physics_broadphase_tree.nodes[broadphase_leaves[index]].bounds, &query);
        if(report->has_fatal) return;
    }
    (void)system_broadphase_particles_flush(&query);
    if(particle_grid_active && !report->has_fatal) system_particle_grid_gather(&query);
}

static void system_angle_locks_apply(Report *report) {
    for(uint32_t alive_position = 0; alive_position < entity_alive_count_get();
            alive_position += 1) {
        EntityIndex entity;
        if(!physics_step_alive_index_at(alive_position, &entity)) {
            continue;
        }

        if(!physics_entity_movable_get(entity)) {
            continue;
        }

        if(!physics_step_entity_components_check(entity, ROHR_ANGLE_LOCK)) {
            continue;
        }

        if(!isfinite(angle_locks[entity].min) || !isfinite(angle_locks[entity].max)) {
            physics_operation_reject(report, ROHR_PHYSICS_ANGLE_LOCK, ERROR_ENGINE_STATE_INVALID, physics_operation_entity_get(entity), 0);
            continue;
        }
        PhysicsChange changes[] = {
            physics_change_get(entity, PHYSICS_CHANGE_POSITION | PHYSICS_CHANGE_ORIENTATION | PHYSICS_CHANGE_ANGULAR_VELOCITY | PHYSICS_CHANGE_ANGULAR_ACCELERATION | PHYSICS_CHANGE_TORQUE_ACCELERATION)
        };
        Orientation min = angle_locks[entity].min;
        Orientation max = angle_locks[entity].max;

        if(min > max) {
            Orientation temp = min;
            min = max;
            max = temp;
        }

        if(fabsf(max - min) <= 0) {
            physics_change_orientation_set(&changes[0], min);
            changes[0].angular_velocity = 0.0f;
            changes[0].angular_acceleration = 0.0f;
            changes[0].torque_acceleration = 0.0f;
            (void)physics_changes_commit(changes, 1, report, ROHR_PHYSICS_ANGLE_LOCK,
            physics_operation_entity_get(entity), 0);
            continue;
        }

        if(changes[0].orientation < min) {
            physics_change_orientation_set(&changes[0], min);

            if(changes[0].angular_velocity < 0.0f) {
                changes[0].angular_velocity = 0.0f;
            }

            if(changes[0].angular_acceleration < 0.0f) {
                changes[0].angular_acceleration = 0.0f;
            }

            if(changes[0].torque_acceleration < 0.0f) {
                changes[0].torque_acceleration = 0.0f;
            }

        }

        if(changes[0].orientation > max) {
            physics_change_orientation_set(&changes[0], max);

            if(changes[0].angular_velocity > 0.0f) {
                changes[0].angular_velocity = 0.0f;
            }

            if(changes[0].angular_acceleration > 0.0f) {
                changes[0].angular_acceleration = 0.0f;
            }

            if(changes[0].torque_acceleration > 0.0f) {
                changes[0].torque_acceleration = 0.0f;
            }

        }
        (void)physics_changes_commit(changes, 1, report, ROHR_PHYSICS_ANGLE_LOCK,
            physics_operation_entity_get(entity), 0);
    }
}

static void system_axis_locks_apply(Report *report) {
    for(uint32_t alive_position = 0; alive_position < entity_alive_count_get();
            alive_position += 1) {
        EntityIndex entity;
        if(!physics_step_alive_index_at(alive_position, &entity)) {
            continue;
        }

        if(!physics_entity_movable_get(entity)) {
            continue;
        }

        if(!physics_step_entity_components_check(entity, ROHR_AXIS_LOCK)) {
            continue;
        }

        PhysicsChange changes[] = {
            physics_change_get(entity, PHYSICS_CHANGE_POSITION | PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ACCELERATION | PHYSICS_CHANGE_FORCE_ACCELERATION)
        };
        Axis axis = axis_locks[entity].axis;

        float mag = math_axis_magnitude(axis);

        if(mag <= 0) {
            continue;
        }

        axis.x /= mag;
        axis.y /= mag;

        Position point_on_axis = axis_locks[entity].point_on_axis;

        Vec2D relative = {
            .x = changes[0].position.x - point_on_axis.x,
            .y = changes[0].position.y - point_on_axis.y
        };

        float distance_along_axis = math_dot_product(relative, axis);

        changes[0].position.x = point_on_axis.x + axis.x * distance_along_axis;
        changes[0].position.y = point_on_axis.y + axis.y * distance_along_axis;


        Vec2D radius = math_vector_rotate(physics_com_local_by_index_get(entity),
            changes[0].orientation);
        float omega = physics_entity_nonrotating_check(entity) ?
            0.0f : changes[0].angular_velocity;
        Vec2D rotational = math_angular_velocity_cross_vec(omega, radius);
        Velocity origin_velocity = math_vector_subtract(changes[0].velocity, rotational);
        origin_velocity = math_project_onto_axis(origin_velocity, axis);
        changes[0].velocity = (Velocity){origin_velocity.x + rotational.x,
            origin_velocity.y + rotational.y};

        changes[0].acceleration = math_project_onto_axis(changes[0].acceleration, axis);
        changes[0].force_acceleration = math_project_onto_axis(changes[0].force_acceleration, axis);

        (void)physics_changes_commit(changes, 1, report, ROHR_PHYSICS_AXIS_LOCK,
            physics_operation_entity_get(entity), 0);
    }
}

static void system_transform_locks_apply(Report *report) {
    for(uint32_t alive_position = 0; alive_position < entity_alive_count_get();
            alive_position += 1) {
        EntityIndex driven;
        if(!physics_step_alive_index_at(alive_position, &driven)) {
            continue;
        }

        if(!physics_entity_movable_get(driven)) {
            continue;
        }

        if(!physics_step_entity_components_check(driven, ROHR_TRANSFORM_LOCK)) {
            continue;
        }

        Entity driver = transform_locks[driven].driver;
        EntityIndex driver_index;

        if(!entity_index_get(driver, &driver_index) || !physics_step_entity_alive_check(driver_index)) {
            physics_operation_reject(report, ROHR_PHYSICS_TRANSFORM_LOCK,
                ERROR_ENGINE_INVALID_ENTITY, physics_operation_entity_get(driven), driver);
            continue;
        }

        PhysicsChange changes[] = {
            physics_change_get(driven, (transform_locks[driven].lock_position ? PHYSICS_CHANGE_POSITION : 0) |
            (transform_locks[driven].lock_orientation ? PHYSICS_CHANGE_ORIENTATION : 0) |
            (transform_locks[driven].inherit_velocity ? PHYSICS_CHANGE_VELOCITY : 0) |
            (transform_locks[driven].inherit_velocity && transform_locks[driven].lock_orientation ? PHYSICS_CHANGE_ANGULAR_VELOCITY : 0))
        };
        Vec2D world_offset = math_vector_rotate(
            transform_locks[driven].local_offset,
            orientations[driver_index]
        );

        if(transform_locks[driven].lock_position) {
            changes[0].position.x = positions[driver_index].x + world_offset.x;
            changes[0].position.y = positions[driver_index].y + world_offset.y;
        }

        if(transform_locks[driven].lock_orientation) {
            float orientation = orientations[driver_index] + transform_locks[driven].local_angle;
            if(transform_locks[driven].lock_position) changes[0].orientation = orientation;
            else physics_change_orientation_set(&changes[0], orientation);
        }


        if(transform_locks[driven].inherit_velocity) {
            if(transform_locks[driven].lock_orientation)
                changes[0].angular_velocity = physics_entity_nonrotating_check(driven) ?
                    0.0f : angular_velocities[driver_index];
            Vec2D driver_com_offset = math_vector_rotate(
                physics_com_local_by_index_get(driver_index), orientations[driver_index]);
            Vec2D driven_com_offset = math_vector_rotate(
                physics_com_local_by_index_get(driven), changes[0].orientation);
            Vec2D driver_origin_velocity = math_vector_subtract(velocities[driver_index],
                math_angular_velocity_cross_vec(angular_velocities[driver_index],
                    driver_com_offset));
            Vec2D driven_rotational = math_angular_velocity_cross_vec(
                changes[0].angular_velocity, driven_com_offset);
            changes[0].velocity = (Velocity){driver_origin_velocity.x + driven_rotational.x,
                driver_origin_velocity.y + driven_rotational.y};

            Vec2D rotational_velocity = math_angular_velocity_cross_vec(
                angular_velocities[driver_index],
                world_offset
            );

            changes[0].velocity.x += rotational_velocity.x;
            changes[0].velocity.y += rotational_velocity.y;

        }
        (void)physics_changes_commit(changes, 1, report, ROHR_PHYSICS_TRANSFORM_LOCK,
            physics_operation_entity_get(driven), driver);
    }
}

void physics_rigid_transform_constraints_apply(Report *report) {
    system_angle_locks_apply(report);
    system_axis_locks_apply(report);
    system_transform_locks_apply(report);
}

EngineResult physics_rigid_integrate(double dt, Report *report) {
    system_loads_apply(report, false);
    system_loads_apply(report, true);
    physics_rigid_motion_integrate(dt, report);
    return error_result_value(true);
}

double physics_rigid_integrate_adaptive(double dt, Report *report) {
    system_loads_apply(report, false);
    system_loads_apply(report, true);
    dt = physics_joint_duration_get(dt);
    dt = physics_soft_duration_get(dt);
    dt = physics_rigid_sweep_duration_get(dt, report);
    dt = physics_soft_sweep_duration_get(dt, report);
    if(report != NULL && report->has_fatal) return dt;
    physics_rigid_motion_integrate(dt, report);
    return dt;
}

void physics_rigid_motion_integrate(double dt, Report *report) {
    physics_joint_motion_begin(dt, report);
    physics_soft_motion_begin(dt, report);
    system_velocities_update(dt, report);
    system_angular_velocities_update(dt, report);
    system_angular_velocity_maximums_apply(report);
    system_orientations_update(dt, report);
    system_positions_update(dt, report);
    physics_soft_motion_end(dt, report);
    physics_joint_motion_end();
    system_angle_locks_apply(report);
    system_axis_locks_apply(report);
    system_transform_locks_apply(report);
}

void physics_rigid_accelerations_clear(void) {
    system_force_torque_accelerations_clear();
}

void physics_rigid_constraints_gather(Report *report) {
    uint64_t started;
    started = SDL_GetPerformanceCounter();
    system_broadphase_build(report);
    physics_step_contact_constraints.rigid_gathered = true;
    physics_report_counter_add(report, ROHR_REPORT_BROADPHASE_BUILD_NS,
        (uint64_t)(physics_rigid_elapsed_ms(started) * 1000000.0));
    physics_report_counter_add(report, ROHR_REPORT_TREE_NODES_CURRENT, physics_broadphase_tree.count);
    physics_report_counter_add(report, ROHR_REPORT_TREE_HEIGHT_PEAK,
        physics_broadphase_tree.root == AABB_TREE_NODE_INVALID ? 0 :
        (uint64_t)physics_broadphase_tree.nodes[physics_broadphase_tree.root].height);
    if(report->has_fatal) return;
    system_broadphase_collisions_apply(report);
    physics_step_contact_constraints.rigid_dirty = false;
}

void physics_rigid_contact_constraint_solve(
    SystemContactConstraint *constraint,
    float position_fraction, Report *report
) {
#if defined(ROHR_PHYSICS_PARTICLE_PASSES) && ROHR_PHYSICS_PARTICLE_PASSES > 0
    if(!physics_particle_solver_pass_check(constraint, &position_fraction)) return;
#endif
    system_rigid_contact_constraint_solve(constraint, position_fraction, report);
}

void physics_rigid_contact_constraint_finalize(
    const SystemContactConstraint *constraint, Report *report
) {
    if(!system_rigid_contact_identity_check(constraint) || !constraint->value.rigid.overlap.detected) return;
    physics_step_interaction_by_index_record(
        constraint->value.rigid.first_index,
        constraint->value.rigid.second_index,
        constraint->value.rigid.overlap,
        constraint->value.rigid.contact,
        PHYSICS_INTERACTION_OVERLAP |
            (constraint->value.rigid.responds
                ? PHYSICS_INTERACTION_CONTACT : 0), report);
}
