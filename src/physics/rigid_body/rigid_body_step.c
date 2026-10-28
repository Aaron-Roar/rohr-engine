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
#include <float.h>
#include <math.h>

#define RIGID_CONTACT_PENETRATION_SLOP 0.01f
#define RIGID_CONTACT_RELATIVE_SLOP 0.0001f
#define RIGID_CONTACT_POSITION_CORRECTION_MAX 5.0f

static double physics_rigid_elapsed_ms(uint64_t start) {
    return (double)(SDL_GetPerformanceCounter() - start) * 1000.0 /
        (double)SDL_GetPerformanceFrequency();
}

bool physics_step_hitbox_update(EntityIndex i, Report *report) {
    if(!entity_index_alive_check(i) || !entity_index_components_check(i, ROHR_HIT_BOX)) return false;
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
    if(entity_index_components_check(i, ROHR_PARTICLE)) {
        Position center = physics_particle_world_origin_by_index_get(i);
        float radius = physics_particle_radius_by_index_get(i);
        valid = valid && physics_vector_finite_check(center) && isfinite(radius) && radius >= 0 &&
            isfinite(center.x - radius) && isfinite(center.x + radius) &&
            isfinite(center.y - radius) && isfinite(center.y + radius);
    }
    if(!valid) {
        /* Invalidate derived geometry, never reuse an obsolete collider. */
        world_hit_boxes[i] = (Shape){0};
        physics_operation_reject(report, ROHR_PHYSICS_GEOMETRY, ERROR_ENGINE_STATE_INVALID,
            physics_operation_entity_get(i), 0);
        return false;
    }
    world_hit_boxes[i] = next;
    return next.amount_of_vertices != 0;
}

static void system_positions_update(double dt, Report *report) {
    for(uint32_t n = 0; n < entity_alive_count_get(); ++n) {
        EntityIndex i;
        if(!physics_step_alive_index_at(n, &i) || !physics_entity_movable_get(i)) continue;
        if(physics_joint_motion_integrated_check(i)) continue;
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
                entity_index_components_check(i, ROHR_SOFT_BODY) ||
                physics_entity_nonrotating_check(i)) continue;
        if(physics_joint_motion_integrated_check(i)) continue;
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
                entity_index_components_check(i, ROHR_SOFT_BODY)) continue;
        if(physics_joint_motion_integrated_check(i)) continue;
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
            if(physics_joint_motion_integrated_check(i)) continue;
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
                !entity_index_components_check(index, ROHR_GRAVITY)) continue;
        PhysicsChange change = physics_change_get(index, PHYSICS_CHANGE_FORCE_ACCELERATION);
        change.force_acceleration.x += gravity.x;
        change.force_acceleration.y += gravity.y;
        (void)physics_changes_commit(&change, 1, report, ROHR_PHYSICS_FORCE_CONTRIBUTION,
            physics_operation_entity_get(index), 0);
    }
}

OverlapInfo system_entity_overlap_get(Entity entity_1, Entity entity_2) {
    if(!entity_index_components_check(entity_1, ROHR_HIT_BOX) ||
            !entity_index_components_check(entity_2, ROHR_HIT_BOX)) return (OverlapInfo){0};
    Shape shape1 = world_hit_boxes[entity_1];
    Shape shape2 = world_hit_boxes[entity_2];
    if(entity_index_components_check(entity_1, ROHR_PARTICLE) && entity_index_components_check(entity_2, ROHR_PARTICLE)) {
        return physics_particle_entities_overlap_get(entity_1, entity_2);
    }
    return physics_sat_overlap_get(shape1, shape2);
}

/* Masks affect only this pair's effective mass, never the stored motion mode. */
static bool system_contact_response_check(EntityIndex first, EntityIndex second) {
    return physics_collision_response_to_check(physics_operation_entity_get(first),
        physics_operation_entity_get(second));
}

/* Measure thickness in local space so a distant world origin does not consume
 * the precision needed by a small collider. The fixed tolerance is an upper
 * bound, not permission for a thin body to disappear into its support. */
static float system_contact_thickness_get(EntityIndex entity, Vec2D normal) {
    if(entity_index_components_check(entity, ROHR_PARTICLE))
        return 2.0f * physics_particle_radius_by_index_get(entity);
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
        dynamic_1 && entity_index_components_check(entity_1, ROHR_MASS) &&
            mass[entity_1] > 0.0f
        ? 1.0f / mass[entity_1]
        : 0.0f;

    float inv_mass_2 =
        dynamic_2 && entity_index_components_check(entity_2, ROHR_MASS) &&
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
    Shape shape_1 = world_hit_boxes[entity_1];
    Shape shape_2 = world_hit_boxes[entity_2];

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
        entity_index_components_check(entity_1, ROHR_PARTICLE) &&
        entity_index_components_check(entity_2, ROHR_PARTICLE);
    bool entity_1_particle = particle_pair;
    bool entity_2_particle = particle_pair;

    if(entity_1_particle) {
        Vec1D r1 = physics_particle_radius_by_index_get(entity_1);
        point_1 = system_particle_edge_get(entity_1, normal, r1);
    } else {
        point_1 = system_support_point_average(shape_1, normal);
    }
    if(entity_2_particle) {
        Vec1D r2 = physics_particle_radius_by_index_get(entity_2);
        point_2 = system_particle_edge_get(entity_2, opposite_normal, r2);
    } else {
        point_2 = system_support_point_average(shape_2, opposite_normal);
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

Vec2D system_friction_impulse_apply(
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
    Vec2D previous_impulse, Report *report
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

    PhysicsChange changes[] = {
        physics_change_get(entity_1, PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY),
        physics_change_get(entity_2, PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY)
    };
    changes[0].velocity.x -= friction_impulse.x * inv_mass_1;
    changes[0].velocity.y -= friction_impulse.y * inv_mass_1;
    changes[1].velocity.x += friction_impulse.x * inv_mass_2;
    changes[1].velocity.y += friction_impulse.y * inv_mass_2;

    changes[0].angular_velocity += math_radians_to_degrees(math_cross_2d(r1, friction_impulse) * inv_inertia_1);
    changes[1].angular_velocity -= math_radians_to_degrees(math_cross_2d(r2, friction_impulse) * inv_inertia_2);
    if(!physics_changes_commit(changes, 2, report, ROHR_PHYSICS_FRICTION_IMPULSE,
            physics_operation_entity_get(entity_1), physics_operation_entity_get(entity_2))) return previous_impulse;
    return (Vec2D){
        tangent.x * impulse_magnitude,
        tangent.y * impulse_magnitude
    };
}

static float system_contact_rebound_get(EntityIndex first,EntityIndex second,
        Position point,Vec2D normal,bool enabled) {
    if(!enabled)return 0;
    EntityIndex bodies[]={first,second};Velocity v[2]={{0,0},{0,0}};
    for(unsigned i=0;i<2;++i)if(physics_entity_movable_get(bodies[i])) {
        EntityIndex body=bodies[i];
        Vec2D angular=math_angular_velocity_cross_vec(physics_entity_nonrotating_check(body)?0:angular_velocities[body],
            math_vector_subtract(point,physics_com_world_by_index_get(body)));
        v[i]=(Velocity){velocities[body].x+angular.x,velocities[body].y+angular.y};
    }
    float normal_velocity=math_dot_product(math_vector_subtract(v[1],v[0]),normal);
    return -fminf(restitutions[first],restitutions[second])*fminf(normal_velocity,0);
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
    Vec2D *friction_impulse, Report *report
) {
    if(normal_impulse != NULL) *normal_impulse = previous ? previous->normal_impulse : (Vec2D){0};
    if(friction_impulse != NULL) *friction_impulse = previous ? previous->friction_impulse : (Vec2D){0};
    bool moving_first = physics_entity_movable_get(first);
    bool moving_second = physics_entity_movable_get(second);
    Vec2D first_offset = math_vector_subtract(point, physics_com_world_by_index_get(first));
    Vec2D second_offset = math_vector_subtract(point, physics_com_world_by_index_get(second));
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
    if(!physics_changes_commit(changes, 2, report, ROHR_PHYSICS_NORMAL_IMPULSE,
            physics_operation_entity_get(first), physics_operation_entity_get(second))) return false;
    if(normal_impulse != NULL) *normal_impulse = (Vec2D){
        overlap.normal.x * accumulated_impulse_magnitude,
        overlap.normal.y * accumulated_impulse_magnitude
    };
    if(friction_impulse != NULL) {
        *friction_impulse = system_friction_impulse_apply(
            first, second, overlap, first_offset, second_offset,
            accumulated_impulse_magnitude,
            inverse_mass_first, inverse_mass_second,
            inverse_inertia_first, inverse_inertia_second,
            previous == NULL ? (Vec2D){0} : previous->friction_impulse, report);
    }
    return true;
}

ContactInfo system_resolve_collision(
    Entity first,
    Entity second,
    OverlapInfo overlap,
    bool restitution_enabled,
    const ContactInfo *previous_contact, const float *previous_rebounds,
    float *rebounds, Report *report, bool *applied
) {
    bool first_particle = entity_index_components_check(first, ROHR_PARTICLE);
    bool second_particle = entity_index_components_check(second, ROHR_PARTICLE);
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
        initial_rebounds[i]=system_contact_rebound_get(first,second,result.points[i].position,overlap.normal,restitution_enabled);
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
            &result.points[i].friction_impulse, report);
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
        float initial_rebounds[PHYSICS_CONTACT_POINT_MAX];
        for(uint8_t i=0;i<manifold->count;++i)
            initial_rebounds[i]=system_contact_rebound_get(first,second,manifold->points[i],overlap.normal,restitution_enabled);
        for(uint8_t i = 0; i < manifold->count; i += 1) {
            const ContactPointInfo *previous_point = NULL;
            const float *previous_rebound = &initial_rebounds[i];
            result.points[i].feature_id = manifold->feature_ids[i];
            result.points[i].position = manifold->points[i];
            if(previous_contact != NULL) {
                for(uint8_t previous = 0;
                        previous < previous_contact->point_count;
                        previous += 1) {
                    if(result.points[i].feature_id != 0 &&
                            result.points[i].feature_id ==
                                previous_contact->points[previous].feature_id) {
                        previous_point = &previous_contact->points[previous];
                        previous_rebound = &previous_rebounds[previous];
                        break;
                    }
                }
            }
            bool committed = system_contact_point_solve(
                first, second, overlap, result.points[i].position,
                restitution_enabled, inverse_mass_first, inverse_mass_second,
                inverse_inertia_first, inverse_inertia_second, previous_point,
                previous_rebound, &rebounds[i],
                &result.points[i].relative_velocity,
                &result.points[i].normal_impulse,
                &result.points[i].friction_impulse, report);
            if(committed) *applied = true;
        }
    }
    return result;
}

typedef struct SystemBroadphaseQuery {
    Entity source;
    EntityIndex source_index;
    Report *report;
} SystemBroadphaseQuery;

static bool system_broadphase_pair_apply(Entity target, void *context) {
    SystemBroadphaseQuery *query = context;
    EntityIndex target_index;
    OverlapInfo overlap;
    bool responds;
    uint64_t started;

    if(query == NULL || target <= query->source ||
            !entity_index_get(target, &target_index) ||
            !entity_index_alive_check(target_index)) return true;
    /* Position correction can introduce another support contact during the
     * solve. Refresh candidates without duplicating existing constraints or
     * losing their accumulated impulses. */
    for(size_t i = 0; i < physics_step_contact_constraints.count; ++i) {
        const SystemContactConstraint *c = &physics_step_contact_constraints.values[i];
        if(c->type == SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR &&
                c->value.rigid.first == query->source && c->value.rigid.second == target) return true;
    }
    physics_report_counter_add(query->report, ROHR_REPORT_CANDIDATE_COUNT, 1);
    physics_report_counter_add(query->report, ROHR_REPORT_NARROWPHASE_TEST_COUNT, 1);
    started = SDL_GetPerformanceCounter();
    overlap = system_entity_overlap_get(query->source_index, target_index);
    physics_report_counter_add(query->report, ROHR_REPORT_NARROWPHASE_NS,
        (uint64_t)(physics_rigid_elapsed_ms(started) * 1000000.0));
    if(!overlap.detected) return true;
    physics_report_counter_add(query->report, ROHR_REPORT_OVERLAP_COUNT, 1);
    responds = entity_index_components_check(query->source_index, ROHR_COLLISION) &&
        entity_index_components_check(target_index, ROHR_COLLISION) &&
        physics_collision_response_between_check(query->source, target);
    bool appended = contact_constraint_list_append(&physics_step_contact_constraints,
        (SystemContactConstraint){
        .type = SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR,
        .value.rigid = {
            .first = query->source,
            .second = target,
            .first_index = query->source_index,
            .second_index = target_index,
            .overlap = overlap,
            .responds = responds
        }
    });
    if(!appended)
        physics_report_issue_add(query->report, ROHR_PHYSICS_CONTACT_GATHER, ROHR_REPORT_SEVERITY_FATAL,
            ERROR_MEMORY_POOL_ALLOCATION_FAILED, ROHR_REPORT_UPDATE_ABORTED,
            query->source, target, "could not allocate rigid contact constraint");
    return appended;
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

static void system_rigid_contact_constraint_solve(
    SystemContactConstraint *constraint,
    float position_fraction, Report *report
) {
    ContactInfo representative = {0};
    ContactInfo previous_contacts[CONTACT_MANIFOLD_MAX] = {0};
    float previous_rebounds[CONTACT_MANIFOLD_MAX][PHYSICS_CONTACT_POINT_MAX] = {{0}};
    bool previous_contact_matched[CONTACT_MANIFOLD_MAX] = {0};
    ContactManifoldSet manifolds;
    uint8_t previous_contact_count;
    EntityIndex first;
    EntityIndex second;
    OverlapInfo overlap;
    bool responds;
    bool first_solve;
    bool normal_applied = false;

    if(constraint == NULL) return;
    first = constraint->value.rigid.first_index;
    second = constraint->value.rigid.second_index;
    if(!physics_step_hitbox_update(first, report) || !physics_step_hitbox_update(second, report)) {
        constraint->value.rigid.overlap = (OverlapInfo){0};
        constraint->value.rigid.contact = (ContactInfo){0};
        return;
    }
    overlap = system_entity_overlap_get(first, second);
    if(!overlap.detected) return;
    responds = constraint->value.rigid.responds;
    first_solve = !constraint->value.rigid.solved;
    previous_contact_count = constraint->value.rigid.manifold_contact_count;
    for(uint8_t i = 0; i < constraint->value.rigid.manifold_contact_count; i += 1) {
        previous_contacts[i] = constraint->value.rigid.manifold_contacts[i];
        for(uint8_t p = 0; p < PHYSICS_CONTACT_POINT_MAX; ++p)
            previous_rebounds[i][p] = constraint->value.rigid.rebound_speeds[i][p];
    }
    if(entity_index_components_check(first, ROHR_PARTICLE) &&
            entity_index_components_check(second, ROHR_PARTICLE)) {
        representative = responds
            ? system_resolve_collision(
                first, second, overlap, first_solve,
                previous_contact_count > 0 ? &previous_contacts[0] : NULL,
                previous_rebounds[0], constraint->value.rigid.rebound_speeds[0], report, &normal_applied)
            : (ContactInfo){0};
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
    manifolds = contact_manifold_set_polygon_get(
        world_hit_boxes[first], world_hit_boxes[second]);
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
    AABB bounds = math_aabb_create(world_hit_boxes[index]);

    if(entity_index_components_check(index, ROHR_PARTICLE)) {
        Position center = physics_particle_world_origin_by_index_get(index);
        float radius = physics_particle_radius_by_index_get(index);
        bounds.min_x = fminf(bounds.min_x, center.x - radius);
        bounds.max_x = fmaxf(bounds.max_x, center.x + radius);
        bounds.min_y = fminf(bounds.min_y, center.y - radius);
        bounds.max_y = fmaxf(bounds.max_y, center.y + radius);
    }
    return bounds;
}

static void system_broadphase_build(Report *report) {
    aabb_tree_clear(&physics_broadphase_tree);
    for(uint32_t alive_position = 0;
            alive_position < entity_alive_count_get();
            alive_position += 1) {
        EntityIndex index;
        Entity entity;

        if(!physics_step_alive_index_at(alive_position, &index) ||
                !entity_index_components_check(index, ROHR_HIT_BOX) ||
                !physics_step_entity_from_index_get(index, &entity)) continue;
        if(!physics_step_hitbox_update(index, report)) continue;
        physics_report_counter_add(report, ROHR_REPORT_COLLIDER_COUNT, 1);
        EngineResult result = aabb_tree_insert(
            &physics_broadphase_tree, entity, system_broadphase_bounds_get(index));
        if(result.kind == ERROR_RESULT_ERROR) {
            physics_report_issue_add(report, ROHR_PHYSICS_BROADPHASE, ROHR_REPORT_SEVERITY_FATAL,
                result.result.error, ROHR_REPORT_UPDATE_ABORTED, entity, 0,
                "could not build broadphase tree");
            return;
        }
    }
}

static void system_broadphase_collisions_apply(Report *report) {
    for(uint32_t alive_position = 0;
            alive_position < entity_alive_count_get();
            alive_position += 1) {
        EntityIndex index;
        Entity entity;
        SystemBroadphaseQuery query;

        if(!physics_step_alive_index_at(alive_position, &index) ||
                !entity_index_components_check(index, ROHR_HIT_BOX) ||
                !physics_step_entity_from_index_get(index, &entity)) continue;
        if(world_hit_boxes[index].amount_of_vertices == 0) continue;
        query = (SystemBroadphaseQuery){
            .source = entity,
            .source_index = index,
            .report = report
        };
        (void)aabb_tree_query(
            &physics_broadphase_tree,
            system_broadphase_bounds_get(index),
            system_broadphase_pair_apply,
            &query
        );
        if(report->has_fatal) return;
    }
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

        if(!entity_index_components_check(entity, ROHR_ANGLE_LOCK)) {
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

        if(!entity_index_components_check(entity, ROHR_AXIS_LOCK)) {
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

        if(!entity_index_components_check(driven, ROHR_TRANSFORM_LOCK)) {
            continue;
        }

        Entity driver = transform_locks[driven].driver;
        EntityIndex driver_index;

        if(!entity_index_get(driver, &driver_index) || !entity_index_alive_check(driver_index)) {
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
    dt = physics_rigid_sweep_duration_get(dt, report);
    if(report != NULL && report->has_fatal) return dt;
    physics_rigid_motion_integrate(dt, report);
    return dt;
}

void physics_rigid_motion_integrate(double dt, Report *report) {
    physics_joint_motion_begin(dt, report);
    system_velocities_update(dt, report);
    system_angular_velocities_update(dt, report);
    system_angular_velocity_maximums_apply(report);
    system_orientations_update(dt, report);
    system_positions_update(dt, report);
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
    physics_step_contact_constraints.rigid_gathered = true;
    started = SDL_GetPerformanceCounter();
    system_broadphase_build(report);
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
    system_rigid_contact_constraint_solve(constraint, position_fraction, report);
}

void physics_rigid_contact_constraint_finalize(
    const SystemContactConstraint *constraint, Report *report
) {
    if(constraint == NULL || !constraint->value.rigid.overlap.detected) return;
    physics_step_interaction_by_index_record(
        constraint->value.rigid.first_index,
        constraint->value.rigid.second_index,
        constraint->value.rigid.overlap,
        constraint->value.rigid.contact,
        PHYSICS_INTERACTION_OVERLAP |
            (constraint->value.rigid.responds
                ? PHYSICS_INTERACTION_CONTACT : 0), report);
}
