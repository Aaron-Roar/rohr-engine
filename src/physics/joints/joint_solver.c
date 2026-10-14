/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics/physics_step_internal.h"
#include "physics/physics_internal.h"
#include "systems.h"
#include "core/engine_internal.h"
#include "math2d.h"
#include "physics/physics_operation.h"
#include <math.h>

static void system_unavailable_joint_skip(EntityIndex index, Entity target,
        EngineError code, Report *report) {
    Entity source = 0;
    (void)physics_step_entity_from_index_get(index, &source);
    physics_report_issue_add(report, ROHR_PHYSICS_JOINT_TARGET, ROHR_REPORT_SEVERITY_WARNING,
        code, ROHR_REPORT_CONTRIBUTION_SKIPPED, source, target,
        "joint endpoint or anchor unavailable; operation skipped; definition retained");
    physics_report_counter_add(report, ROHR_REPORT_SKIPPED_COUNT, 1);

}

static Velocity system_point_velocity(Entity entity, Vec2D world_offset) {
    EntityIndex index;

    if(!entity_index_get(entity, &index) || !entity_index_alive_check(index)) {
        return (Velocity){0};
    }
    if(!physics_entity_movable_get(index)) {
        return (Velocity){0};
    }
    Vec2D angular_part = math_angular_velocity_cross_vec(
        entity_index_components_check(index, ROHR_PARTICLE) ? 0.0f : angular_velocities[index],
        world_offset
    );

    return (Velocity){
        .x = velocities[index].x + angular_part.x,
        .y = velocities[index].y + angular_part.y
    };
}
/* Legacy pin loads retain their one-tick lifetime. Prepare both endpoints
 * before creating anything; allocation failure releases this operation's
 * temporary entities without undoing earlier operations. */
static void system_joint_load_pair_add(Entity a, Entity b, Position point_a,
        Position point_b, Force force, Report *report, Entity source) {
    Entity targets_pair[] = {a, b};
    Position points[] = {point_a, point_b};
    Force values[] = {force, {-force.x, -force.y}};
    Torque torque[2];
    bool active[2] = {false, false};
    Entity created[4];
    size_t count = 0;
    EngineError failure = ERROR_NONE;
    for(size_t n = 0; n < 2; ++n) {
        EntityIndex i;
        if(!entity_index_get(targets_pair[n], &i) || !physics_entity_simulated_get(i)) continue;
        active[n] = true;
        torque[n] = -math_cross_2d(math_vector_subtract(points[n],
            physics_com_world_by_index_get(i)), values[n]);
        if(!physics_vector_finite_check(values[n]) || !isfinite(torque[n])) {
            physics_operation_reject(report, ROHR_PHYSICS_FORCE_CONTRIBUTION,
                ERROR_ENGINE_STATE_INVALID, source, targets_pair[n]);
            return;
        }
    }
    for(size_t n = 0; n < 2; ++n) {
        if(!active[n]) continue;
        EntityResult force_result = physics_force_create(targets_pair[n], values[n]);
        if(force_result.kind == ERROR_RESULT_ERROR) { failure = force_result.result.error; break; }
        created[count++] = force_result.result.value;
        EntityResult torque_result = physics_torque_create(targets_pair[n], torque[n]);
        if(torque_result.kind == ERROR_RESULT_ERROR) { failure = torque_result.result.error; break; }
        created[count++] = torque_result.result.value;
    }
    for(size_t n = 0; failure == ERROR_NONE && n < count; ++n) {
        EngineResult result = entity_life_time_set(created[n], 0.0, engine_tick_get() + 1);
        if(result.kind == ERROR_RESULT_ERROR) failure = result.result.error;
    }
    if(failure != ERROR_NONE) {
        for(size_t n = 0; n < count; ++n) (void)entity_delete(created[n]);
        physics_report_issue_add(report, ROHR_PHYSICS_CONSTRAINT_RESPONSE, ROHR_REPORT_SEVERITY_FATAL,
            failure, ROHR_REPORT_UPDATE_ABORTED, source, b, "could not allocate paired joint contributions");
    }
}

static float system_joint_inverse_inertia(EntityIndex index) {
    return physics_inverse_inertia_by_index_get(index);
}

static void system_rigid_anchor_axis_solve(
        EntityIndex a,
        EntityIndex b,
        Vec2D offset_a,
        Vec2D offset_b,
        Vec2D position_error,
        Vec2D velocity_error,
        Vec2D axis, Report *report, Entity source
) {
    float inverse_mass_a = physics_entity_simulated_get(a) ? 1.0f / mass[a] : 0.0f;
    float inverse_mass_b = physics_entity_simulated_get(b) ? 1.0f / mass[b] : 0.0f;
    float inverse_inertia_a = system_joint_inverse_inertia(a);
    float inverse_inertia_b = system_joint_inverse_inertia(b);
    float lever_a = math_cross_2d(offset_a, axis);
    float lever_b = math_cross_2d(offset_b, axis);
    float effective_inverse_mass = inverse_mass_a + inverse_mass_b +
        lever_a * lever_a * inverse_inertia_a + lever_b * lever_b * inverse_inertia_b;
    float position_impulse;
    float velocity_impulse;

    if(effective_inverse_mass <= 0.0f) return;
    position_impulse = math_dot_product(position_error, axis) / effective_inverse_mass;
    velocity_impulse = math_dot_product(velocity_error, axis) / effective_inverse_mass;
    {
        PhysicsChange changes[] = {
            physics_change_get(a, PHYSICS_CHANGE_POSITION | PHYSICS_CHANGE_ORIENTATION),
            physics_change_get(b, PHYSICS_CHANGE_POSITION | PHYSICS_CHANGE_ORIENTATION)
        };
        changes[0].position.x += axis.x * position_impulse * inverse_mass_a;
        changes[0].position.y += axis.y * position_impulse * inverse_mass_a;
        physics_change_orientation_set(&changes[0], changes[0].orientation - math_radians_to_degrees(lever_a * position_impulse * inverse_inertia_a));
        changes[1].position.x -= axis.x * position_impulse * inverse_mass_b;
        changes[1].position.y -= axis.y * position_impulse * inverse_mass_b;
        physics_change_orientation_set(&changes[1], changes[1].orientation + math_radians_to_degrees(lever_b * position_impulse * inverse_inertia_b));
        (void)physics_changes_commit(changes, 2, report, ROHR_PHYSICS_JOINT_POSITION, source, physics_operation_entity_get(b));
    }
    {
        PhysicsChange changes[] = {
            physics_change_get(a, PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY),
            physics_change_get(b, PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY)
        };
        changes[0].velocity.x += axis.x * velocity_impulse * inverse_mass_a;
        changes[0].velocity.y += axis.y * velocity_impulse * inverse_mass_a;
        changes[0].angular_velocity -= math_radians_to_degrees(lever_a * velocity_impulse * inverse_inertia_a);
        changes[1].velocity.x -= axis.x * velocity_impulse * inverse_mass_b;
        changes[1].velocity.y -= axis.y * velocity_impulse * inverse_mass_b;
        changes[1].angular_velocity += math_radians_to_degrees(lever_b * velocity_impulse * inverse_inertia_b);
        (void)physics_changes_commit(changes, 2, report, ROHR_PHYSICS_JOINT_VELOCITY, source, physics_operation_entity_get(b));
    }
}

static void system_pin_joint_apply(Entity joint_entity, Report *report) {
    Joint joint = joints[joint_entity];

    Entity a = joint.a;
    Entity b = joint.b;
    EntityIndex a_index;
    EntityIndex b_index;

    if(!entity_index_get(a, &a_index) || !entity_index_alive_check(a_index) || !entity_index_get(b, &b_index) || !entity_index_alive_check(b_index)) {
        system_unavailable_joint_skip(joint_entity,
            (!entity_index_get(a, &a_index) || !entity_index_alive_check(a_index)) ? a : b,
            ERROR_ENGINE_INVALID_ENTITY, report);
        return;
    }

    if(a_index == b_index) return;
    Vec2D offset_a;
    Vec2D offset_b;
    Position world_anchor_a;
    Position world_anchor_b;

    if(joint.anchor_a != JOINT_ANCHOR_INVALID && joint.anchor_b != JOINT_ANCHOR_INVALID) {
        JointAnchorPositionResult anchor_a = physics_joint_anchor_world_position_get(joint.anchor_a);
        JointAnchorPositionResult anchor_b = physics_joint_anchor_world_position_get(joint.anchor_b);
        if(anchor_a.kind == ERROR_RESULT_ERROR || anchor_b.kind == ERROR_RESULT_ERROR) {
            system_unavailable_joint_skip(joint_entity,
                anchor_a.kind == ERROR_RESULT_ERROR ? a : b,
                anchor_a.kind == ERROR_RESULT_ERROR ? anchor_a.result.error : anchor_b.result.error,
                report);
            return;
        }
        world_anchor_a = anchor_a.result.value;
        world_anchor_b = anchor_b.result.value;
        offset_a = math_vector_subtract(world_anchor_a, positions[a_index]);
        offset_b = math_vector_subtract(world_anchor_b, positions[b_index]);
    } else {
        offset_a = math_vector_rotate(joint.local_anchor_a, orientations[a_index]);
        offset_b = math_vector_rotate(joint.local_anchor_b, orientations[b_index]);
        world_anchor_a = (Position){positions[a_index].x + offset_a.x, positions[a_index].y + offset_a.y};
        world_anchor_b = (Position){positions[b_index].x + offset_b.x, positions[b_index].y + offset_b.y};
    }

    offset_a = math_vector_subtract(world_anchor_a, physics_com_world_by_index_get(a_index));
    offset_b = math_vector_subtract(world_anchor_b, physics_com_world_by_index_get(b_index));
    Vec2D error = {
        .x = world_anchor_b.x - world_anchor_a.x,
        .y = world_anchor_b.y - world_anchor_a.y
    };

    Velocity velocity_a = system_point_velocity(a, offset_a);
    Velocity velocity_b = system_point_velocity(b, offset_b);

    Vec2D relative_velocity = {
        .x = velocity_b.x - velocity_a.x,
        .y = velocity_b.y - velocity_a.y
    };

    if(joint.anchor_a != JOINT_ANCHOR_INVALID && joint.anchor_b != JOINT_ANCHOR_INVALID) {
        system_rigid_anchor_axis_solve(a_index, b_index, offset_a, offset_b,
            error, relative_velocity, (Vec2D){1.0f, 0.0f}, report, physics_operation_entity_get(joint_entity));
        system_rigid_anchor_axis_solve(a_index, b_index, offset_a, offset_b,
            error, relative_velocity, (Vec2D){0.0f, 1.0f}, report, physics_operation_entity_get(joint_entity));
        return;
    }

    Force force_on_a = {
        .x = joint.stiffness * error.x + joint.damping * relative_velocity.x,
        .y = joint.stiffness * error.y + joint.damping * relative_velocity.y
    };

    Force force_on_b = {
        .x = -force_on_a.x,
        .y = -force_on_a.y
    };

    Entity source = physics_operation_entity_get(joint_entity);
    if(!physics_vector_finite_check(force_on_a) ||
            !isfinite(math_cross_2d(offset_a, force_on_a)) ||
            !isfinite(math_cross_2d(offset_b, force_on_b))) {
        physics_operation_reject(report, ROHR_PHYSICS_FORCE_CONTRIBUTION,
            ERROR_ENGINE_STATE_INVALID, source, b);
        return;
    }
    system_joint_load_pair_add(a, b, world_anchor_a, world_anchor_b, force_on_a, report, source);
}

static void system_weld_joint_apply(Entity joint_entity, Report *report) {
    Joint joint = joints[joint_entity];
    EntityIndex a_index;
    EntityIndex b_index;
    float inverse_inertia_a = 0.0f;
    float inverse_inertia_b = 0.0f;
    float inverse_inertia_sum;
    float angle_error;
    float angular_velocity_error;

    system_pin_joint_apply(joint_entity, report);
    if(report != NULL && report->has_fatal) return;
    if(!entity_index_get(joint.a, &a_index) || !entity_index_alive_check(a_index) ||
            !entity_index_get(joint.b, &b_index) || !entity_index_alive_check(b_index)) return;
    if(a_index == b_index) return;
    inverse_inertia_a = physics_inverse_inertia_by_index_get(a_index);
    inverse_inertia_b = physics_inverse_inertia_by_index_get(b_index);
    inverse_inertia_sum = inverse_inertia_a + inverse_inertia_b;
    if(inverse_inertia_sum <= 0.0f) return;
    angle_error = (orientations[b_index] - orientations[a_index]) - joint.rest_angle;
    angular_velocity_error = angular_velocities[b_index] - angular_velocities[a_index];
    {
        PhysicsChange changes[] = {
            physics_change_get(a_index, PHYSICS_CHANGE_ORIENTATION),
            physics_change_get(b_index, PHYSICS_CHANGE_ORIENTATION)
        };
        physics_change_orientation_set(&changes[0], changes[0].orientation + angle_error * inverse_inertia_a / inverse_inertia_sum);
        physics_change_orientation_set(&changes[1], changes[1].orientation - angle_error * inverse_inertia_b / inverse_inertia_sum);
        (void)physics_changes_commit(changes, 2, report, ROHR_PHYSICS_JOINT_POSITION,
                physics_operation_entity_get(joint_entity), joint.b);
    }
    {
        PhysicsChange changes[] = {
            physics_change_get(a_index, PHYSICS_CHANGE_ANGULAR_VELOCITY),
            physics_change_get(b_index, PHYSICS_CHANGE_ANGULAR_VELOCITY)
        };
        changes[0].angular_velocity += angular_velocity_error * inverse_inertia_a / inverse_inertia_sum;
        changes[1].angular_velocity -= angular_velocity_error * inverse_inertia_b / inverse_inertia_sum;
        (void)physics_changes_commit(changes, 2, report, ROHR_PHYSICS_JOINT_VELOCITY,
                physics_operation_entity_get(joint_entity), joint.b);
    }
}

static void system_spring_joint_apply(Entity joint_entity, Report *report) {
        Joint joint = joints[joint_entity];
        if(joint.type != JOINT_SPRING) {
            return;
        }

        Entity a = joint.a;
        Entity b = joint.b;
        EntityIndex a_index;
        EntityIndex b_index;

        if(!entity_index_get(a, &a_index) || !entity_index_alive_check(a_index) || !entity_index_get(b, &b_index) || !entity_index_alive_check(b_index)) {
            system_unavailable_joint_skip(joint_entity,
                (!entity_index_get(a, &a_index) || !entity_index_alive_check(a_index)) ? a : b,
                ERROR_ENGINE_INVALID_ENTITY, report);
            return;
        }

        if(a_index == b_index) return;
        Vec2D offset_a;
        Vec2D offset_b;
        Position world_anchor_a;
        Position world_anchor_b;
        if(joint.anchor_a != JOINT_ANCHOR_INVALID && joint.anchor_b != JOINT_ANCHOR_INVALID) {
            JointAnchorPositionResult anchor_a = physics_joint_anchor_world_position_get(joint.anchor_a);
            JointAnchorPositionResult anchor_b = physics_joint_anchor_world_position_get(joint.anchor_b);
            if(anchor_a.kind == ERROR_RESULT_ERROR || anchor_b.kind == ERROR_RESULT_ERROR) {
                system_unavailable_joint_skip(joint_entity,
                    anchor_a.kind == ERROR_RESULT_ERROR ? a : b,
                    anchor_a.kind == ERROR_RESULT_ERROR ? anchor_a.result.error : anchor_b.result.error,
                    report);
                return;
            }
            world_anchor_a = anchor_a.result.value;
            world_anchor_b = anchor_b.result.value;
            offset_a = math_vector_subtract(world_anchor_a, positions[a_index]);
            offset_b = math_vector_subtract(world_anchor_b, positions[b_index]);
        } else {
            offset_a = math_vector_rotate(joint.local_anchor_a, orientations[a_index]);
            offset_b = math_vector_rotate(joint.local_anchor_b, orientations[b_index]);
            world_anchor_a = (Position){positions[a_index].x + offset_a.x, positions[a_index].y + offset_a.y};
            world_anchor_b = (Position){positions[b_index].x + offset_b.x, positions[b_index].y + offset_b.y};
        }

        offset_a = math_vector_subtract(world_anchor_a, physics_com_world_by_index_get(a_index));
        offset_b = math_vector_subtract(world_anchor_b, physics_com_world_by_index_get(b_index));
        Vec2D delta = {
            .x = world_anchor_b.x - world_anchor_a.x,
            .y = world_anchor_b.y - world_anchor_a.y
        };

        float length = math_vector_magnitude(delta);

        if(length <= 0.0) {
            return;
        }

        Vec2D normal = {
            .x = delta.x / length,
            .y = delta.y / length
        };

        Velocity velocity_a = system_point_velocity(a, offset_a);
        Velocity velocity_b = system_point_velocity(b, offset_b);

        Vec2D relative_velocity = {
            .x = velocity_b.x - velocity_a.x,
            .y = velocity_b.y - velocity_a.y
        };

        float relative_speed = math_dot_product(relative_velocity, normal);

        float stretch = length - joint.rest_length;

        float force_magnitude =
            joint.stiffness * stretch +
            joint.damping * relative_speed;

        Force force_on_a = {
            .x = normal.x * force_magnitude,
            .y = normal.y * force_magnitude
        };

        Force force_on_b = {
            .x = -force_on_a.x,
            .y = -force_on_a.y
        };

        Vec2D r_a = {
            .x = world_anchor_a.x - physics_com_world_by_index_get(a_index).x,
            .y = world_anchor_a.y - physics_com_world_by_index_get(a_index).y
        };

        Vec2D r_b = {
            .x = world_anchor_b.x - physics_com_world_by_index_get(b_index).x,
            .y = world_anchor_b.y - physics_com_world_by_index_get(b_index).y
        };

        Torque torque_on_a = -math_cross_2d(r_a, force_on_a);
        Torque torque_on_b = -math_cross_2d(r_b, force_on_b);

        PhysicsChange changes[] = {
            physics_change_get(a_index, 0), physics_change_get(b_index, 0)
        };
        Force pair_forces[] = {force_on_a, force_on_b};
        Torque pair_torques[] = {torque_on_a, torque_on_b};
        for(size_t n = 0; n < 2; ++n) {
            EntityIndex i = changes[n].index;
            if(!physics_entity_simulated_get(i) || !entity_index_components_check(i, ROHR_MASS) || mass[i] == 0) continue;
            changes[n].fields = PHYSICS_CHANGE_FORCE_ACCELERATION | PHYSICS_CHANGE_TORQUE_ACCELERATION;
            changes[n].force_acceleration.x += pair_forces[n].x / mass[i];
            changes[n].force_acceleration.y += pair_forces[n].y / mass[i];
            changes[n].torque_acceleration += math_radians_to_degrees(pair_torques[n] *
                physics_inverse_inertia_by_index_get(i));
        }
        (void)physics_changes_commit(changes, 2, report, ROHR_PHYSICS_SPRING_CONTRIBUTION,
            physics_operation_entity_get(joint_entity), b);

}
static void system_joint_spring_forces_apply(Report *report) {
    for(Entity joint_entity = 0; joint_entity < MAX_ENTITIES; joint_entity += 1) {
        if(!entity_index_alive_check(joint_entity) ||
                !entity_index_components_check(joint_entity, ROHR_JOINT) ||
                joints[joint_entity].type != JOINT_SPRING) continue;
        system_spring_joint_apply(joint_entity, report);
    }
}

static void physics_step_joint_constraints_gather(Report *report) {
    joint_constraint_list_clear(&physics_step_joint_constraints);
    for(EntityIndex joint_entity = 0;
            joint_entity < joints_pool.capacity;
            joint_entity += 1) {
        if(!joints_pool.used[joint_entity] ||
                !entity_index_alive_check(joint_entity) ||
                !entity_index_components_check(joint_entity, ROHR_JOINT)) continue;
        if(joints[joint_entity].type == JOINT_PIN ||
                joints[joint_entity].type == JOINT_WELD) {
            Entity source = 0;
            (void)physics_step_entity_from_index_get(joint_entity, &source);
            physics_report_counter_add(report, ROHR_REPORT_PROCESSED_COUNT, 1);
            if(!joint_constraint_list_append(&physics_step_joint_constraints, joint_entity)) {
                physics_report_issue_add(report, ROHR_PHYSICS_JOINT_GATHER, ROHR_REPORT_SEVERITY_FATAL,
                    ERROR_MEMORY_POOL_ALLOCATION_FAILED, ROHR_REPORT_UPDATE_ABORTED,
                    source, 0, "could not allocate joint constraint");
                return;
            }
        }
    }
}

static void physics_step_joint_constraints_apply(Report *report) {
    for(size_t i = 0; i < physics_step_joint_constraints.count; i += 1) {
        EntityIndex joint_entity = physics_step_joint_constraints.values[i];
        if(report != NULL && report->has_fatal) return;

        if(!entity_index_alive_check(joint_entity) ||
                !entity_index_components_check(joint_entity, ROHR_JOINT)) continue;
        if(joints[joint_entity].type == JOINT_PIN) {
            system_pin_joint_apply(joint_entity, report);
        } else if(joints[joint_entity].type == JOINT_WELD) {
            system_weld_joint_apply(joint_entity, report);
        }
    }
}

void physics_joint_spring_forces_apply(Report *report) {
    system_joint_spring_forces_apply(report);
}

void physics_joint_constraints_gather(Report *report) {
    physics_step_joint_constraints_gather(report);
}

void physics_joint_constraints_solve(Report *report) {
    physics_step_joint_constraints_apply(report);
    physics_step_hitbox_dirty_flush(report);
}
