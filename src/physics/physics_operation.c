/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics_operation.h"
#include "physics_step_internal.h"
#include "math2d.h"
#include <math.h>

bool physics_vector_finite_check(Vec2D value) {
    return isfinite(value.x) && isfinite(value.y);
}

Entity physics_operation_entity_get(EntityIndex index) {
    Entity entity = 0;
    (void)physics_step_entity_from_index_get(index, &entity);
    return entity;
}

void physics_operation_reject(Report *report, PhysicsReportStage stage,
        EngineError code, Entity source, Entity target) {
    physics_report_counter_add(report, ROHR_REPORT_SKIPPED_COUNT, 1);
    physics_report_issue_add(report, stage, ROHR_REPORT_SEVERITY_WARNING, code,
        ROHR_REPORT_CONTRIBUTION_SKIPPED, source, target,
        "operation rejected; prior state retained; retry on next execution");
}

PhysicsChange physics_change_get(EntityIndex i, unsigned fields) {
    return (PhysicsChange){.index = i, .fields = fields,
        .position = positions[i], .orientation = orientations[i],
        .velocity = velocities[i], .angular_velocity = angular_velocities[i],
        .acceleration = accelerations[i], .angular_acceleration = angular_accelerations[i],
        .force_acceleration = force_accelerations[i],
        .torque_acceleration = torque_angular_accelerations[i]};
}

void physics_change_orientation_set(PhysicsChange *c, float orientation) {
    if(c->orientation == orientation) return;
    Vec2D local = physics_com_local_by_index_get(c->index);
    Vec2D before = math_vector_rotate(local, c->orientation);
    Vec2D after = math_vector_rotate(local, orientation);
    c->position = (Position){(c->position.x + before.x) - after.x,
        (c->position.y + before.y) - after.y};
    c->orientation = orientation;
    c->fields |= PHYSICS_CHANGE_POSITION | PHYSICS_CHANGE_ORIENTATION;
}

bool physics_changes_commit(PhysicsChange *changes, size_t count, Report *report,
        PhysicsReportStage stage, Entity source, Entity target) {
    for(size_t n = 0; n < count; ++n) {
        const PhysicsChange *c = &changes[n];
        unsigned f = c->fields;
        if((f & PHYSICS_CHANGE_POSITION) && !physics_world_position_check(c->position)) {
            physics_operation_reject(report, stage, ERROR_ENGINE_POSITION_OUT_OF_RANGE, source, target);
            return false;
        }
        if(((f & PHYSICS_CHANGE_ORIENTATION) && !isfinite(c->orientation)) ||
           ((f & PHYSICS_CHANGE_VELOCITY) && !physics_vector_finite_check(c->velocity)) ||
           ((f & PHYSICS_CHANGE_ANGULAR_VELOCITY) && !isfinite(c->angular_velocity)) ||
           ((f & PHYSICS_CHANGE_ACCELERATION) && !physics_vector_finite_check(c->acceleration)) ||
           ((f & PHYSICS_CHANGE_ANGULAR_ACCELERATION) && !isfinite(c->angular_acceleration)) ||
           ((f & PHYSICS_CHANGE_FORCE_ACCELERATION) && !physics_vector_finite_check(c->force_acceleration)) ||
           ((f & PHYSICS_CHANGE_TORQUE_ACCELERATION) && !isfinite(c->torque_acceleration))) {
            physics_operation_reject(report, stage, ERROR_ENGINE_STATE_INVALID, source, target);
            return false;
        }
    }
    for(size_t n = 0; n < count; ++n) {
        const PhysicsChange *c = &changes[n];
        EntityIndex i = c->index;
        unsigned f = c->fields;
        if(f & PHYSICS_CHANGE_POSITION) positions[i] = c->position;
        if(f & PHYSICS_CHANGE_ORIENTATION) orientations[i] = c->orientation;
        if(f & PHYSICS_CHANGE_VELOCITY) velocities[i] = c->velocity;
        if(f & PHYSICS_CHANGE_ANGULAR_VELOCITY) angular_velocities[i] = c->angular_velocity;
        if(f & PHYSICS_CHANGE_ACCELERATION) accelerations[i] = c->acceleration;
        if(f & PHYSICS_CHANGE_ANGULAR_ACCELERATION) angular_accelerations[i] = c->angular_acceleration;
        if(f & PHYSICS_CHANGE_FORCE_ACCELERATION) force_accelerations[i] = c->force_acceleration;
        if(f & PHYSICS_CHANGE_TORQUE_ACCELERATION) torque_angular_accelerations[i] = c->torque_acceleration;
        if(f & (PHYSICS_CHANGE_POSITION | PHYSICS_CHANGE_ORIENTATION))
            physics_step_hitbox_dirty_add(i);
    }
    return true;
}
