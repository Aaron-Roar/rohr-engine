/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics/physics_step_internal.h"
#include "physics/physics_internal.h"
#include "systems.h"
#include "math2d.h"
#include "physics/physics_operation.h"
#include <math.h>
#include <float.h>

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
        physics_entity_nonrotating_check(index) ? 0.0f : angular_velocities[index],
        world_offset
    );

    return (Velocity){
        .x = velocities[index].x + angular_part.x,
        .y = velocities[index].y + angular_part.y
    };
}
/* Solve the anchor axes together: an off-center impulse changes both axes
 * through rotation. Welds add the angular row to that same effective-mass
 * system, rather than undoing the anchor correction in a separate pass. */
static bool joint_matrix_solve(double matrix[3][3], double rhs[3], unsigned count) {
    for(unsigned col = 0; col < count; ++col) {
        unsigned pivot = col;
        for(unsigned row = col + 1; row < count; ++row)
            if(fabs(matrix[row][col]) > fabs(matrix[pivot][col])) pivot = row;
        if(!isfinite(matrix[pivot][col]) || fabs(matrix[pivot][col]) < 1e-18) return false;
        if(pivot != col) {
            for(unsigned j = 0; j < count; ++j) {
                double swap = matrix[col][j]; matrix[col][j] = matrix[pivot][j]; matrix[pivot][j] = swap;
            }
            double swap = rhs[col]; rhs[col] = rhs[pivot]; rhs[pivot] = swap;
        }
        double diagonal = matrix[col][col];
        for(unsigned j = col; j < count; ++j) matrix[col][j] /= diagonal;
        rhs[col] /= diagonal;
        for(unsigned row = 0; row < count; ++row) {
            if(row == col) continue;
            double factor = matrix[row][col];
            for(unsigned j = col; j < count; ++j) matrix[row][j] -= factor * matrix[col][j];
            rhs[row] -= factor * rhs[col];
        }
    }
    for(unsigned row = 0; row < count; ++row) if(!isfinite(rhs[row])) return false;
    return true;
}

static void system_rigid_joint_apply(EntityIndex joint_entity, Report *report) {
    Joint joint = joints[joint_entity];
    EntityIndex bodies[2];
    if(!entity_index_get(joint.a, &bodies[0]) || !entity_index_alive_check(bodies[0]) ||
            !entity_index_get(joint.b, &bodies[1]) || !entity_index_alive_check(bodies[1])) {
        system_unavailable_joint_skip(joint_entity, joint.b, ERROR_ENGINE_INVALID_ENTITY, report);
        return;
    }
    if(bodies[0] == bodies[1]) return;
    double weights[6] = {0};
    for(unsigned side = 0; side < 2; ++side) {
        EntityIndex body = bodies[side];
        if(!physics_entity_simulated_get(body)) continue;
        weights[3 * side] = weights[3 * side + 1] = 1.0 / mass[body];
        weights[3 * side + 2] = physics_inverse_inertia_by_index_get(body);
    }
    unsigned count = joint.type == JOINT_WELD && weights[2] + weights[5] > 0 ? 3 : 2;
    for(unsigned phase = 0; phase < 2; ++phase) {
        JointAnchorPositionResult anchors[] = {
            physics_joint_anchor_world_position_get(joint.anchor_a),
            physics_joint_anchor_world_position_get(joint.anchor_b)
        };
        for(unsigned side = 0; side < 2; ++side)
            if(error_check(anchors[side])) {
                system_unavailable_joint_skip(joint_entity, side ? joint.b : joint.a,
                    anchors[side].result.error, report);
                return;
            }
        Vec2D r[2];
        for(unsigned side = 0; side < 2; ++side)
            r[side] = math_vector_subtract(anchors[side].result.value,
                physics_com_world_by_index_get(bodies[side]));
        double jacobian[3][6] = {
            {-1, 0, -r[0].y, 1, 0, r[1].y},
            {0, -1, r[0].x, 0, 1, -r[1].x},
            {0, 0, -1, 0, 0, 1}
        };
        double rhs[3] = {
            (double)anchors[1].result.value.x - anchors[0].result.value.x,
            (double)anchors[1].result.value.y - anchors[0].result.value.y,
            remainder((double)orientations[bodies[1]] - orientations[bodies[0]] - joint.rest_angle, 360) *
                (3.14159265358979323846 / 180)
        };
        if(phase) {
            double velocity[6] = {0};
            for(unsigned side = 0; side < 2; ++side) {
                EntityIndex body = bodies[side];
                if(!physics_entity_movable_get(body)) continue;
                velocity[3 * side] = velocities[body].x;
                velocity[3 * side + 1] = velocities[body].y;
                velocity[3 * side + 2] = physics_entity_nonrotating_check(body) ? 0 :
                    angular_velocities[body] * (3.14159265358979323846 / 180);
            }
            for(unsigned row = 0; row < count; ++row) {
                rhs[row] = 0;
                for(unsigned k = 0; k < 6; ++k) rhs[row] += jacobian[row][k] * velocity[k];
            }
            /* Retain the operation's float-state overflow/retry contract even
             * though the effective-mass calculation uses double precision. */
            for(unsigned row = 0; row < count; ++row)if(!isfinite(rhs[row])||fabs(rhs[row])>FLT_MAX) {
                physics_operation_reject(report,ROHR_PHYSICS_JOINT_VELOCITY,
                    ERROR_ENGINE_STATE_INVALID,physics_operation_entity_get(joint_entity),joint.b);
                return;
            }
        }
        double matrix[3][3] = {{0}};
        for(unsigned row = 0; row < count; ++row)
            for(unsigned col = 0; col < count; ++col)
                for(unsigned k = 0; k < 6; ++k)
                    matrix[row][col] += jacobian[row][k] * weights[k] * jacobian[col][k];
        if(!joint_matrix_solve(matrix, rhs, count)) return;
        PhysicsChange changes[2];
        for(unsigned side = 0; side < 2; ++side) {
            changes[side] = physics_change_get(bodies[side], phase ?
                PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY :
                PHYSICS_CHANGE_POSITION | PHYSICS_CHANGE_ORIENTATION);
            double delta[3] = {0};
            for(unsigned axis = 0; axis < 3; ++axis)
                for(unsigned row = 0; row < count; ++row)
                    delta[axis] -= weights[3 * side + axis] * jacobian[row][3 * side + axis] * rhs[row];
            if(phase) {
                changes[side].velocity.x += delta[0]; changes[side].velocity.y += delta[1];
                changes[side].angular_velocity += delta[2] * (180 / 3.14159265358979323846);
            } else {
                physics_change_orientation_set(&changes[side], changes[side].orientation +
                    delta[2] * (180 / 3.14159265358979323846));
                changes[side].position.x += delta[0]; changes[side].position.y += delta[1];
            }
        }
        (void)physics_changes_commit(changes, 2, report,
                phase ? ROHR_PHYSICS_JOINT_VELOCITY : ROHR_PHYSICS_JOINT_POSITION,
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
        {
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

        /* The poses have not changed since resolving the point velocities. */
        Torque torque_on_a = -math_cross_2d(offset_a, force_on_a);
        Torque torque_on_b = -math_cross_2d(offset_b, force_on_b);

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
    /* Keep index order (and therefore accumulation/report order), but do not
     * scan unused global entity capacity or resolve non-joint slots. No cache
     * survives a call, so standalone preparation observes edits immediately. */
    for(EntityIndex joint_entity = 0; joint_entity < joints_pool.capacity; joint_entity += 1) {
        /* Membership, rather than pool usage, preserves diagnostics for a
         * manually enabled joint component without a stored description. */
        if(joint_entity >= entity_mask_pool.capacity ||
                !(entity_mask[joint_entity] & ROHR_JOINT) ||
                !entity_index_alive_check(joint_entity) ||
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
            system_rigid_joint_apply(joint_entity, report);
        } else if(joints[joint_entity].type == JOINT_WELD) {
            system_rigid_joint_apply(joint_entity, report);
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
