/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_PHYSICS_OPERATION_H
#define ROHR_PHYSICS_OPERATION_H

#include "physics/physics_internal.h"

/* Only the fields owned by this operation are validated and committed. */
enum {
    PHYSICS_CHANGE_POSITION = 1u << 0,
    PHYSICS_CHANGE_ORIENTATION = 1u << 1,
    PHYSICS_CHANGE_VELOCITY = 1u << 2,
    PHYSICS_CHANGE_ANGULAR_VELOCITY = 1u << 3,
    PHYSICS_CHANGE_ACCELERATION = 1u << 4,
    PHYSICS_CHANGE_ANGULAR_ACCELERATION = 1u << 5,
    PHYSICS_CHANGE_FORCE_ACCELERATION = 1u << 6,
    PHYSICS_CHANGE_TORQUE_ACCELERATION = 1u << 7
};
typedef struct PhysicsChange {
    EntityIndex index;
    unsigned fields;
    Position position;
    Orientation orientation;
    Velocity velocity;
    AngularVelocity angular_velocity;
    Acceleration acceleration;
    AngularAcceleration angular_acceleration;
    Acceleration force_acceleration;
    AngularAcceleration torque_acceleration;
} PhysicsChange;

/* One rigid-body change or a coupled pair of beam endpoint changes. */
typedef struct PhysicsSourceChanges {
    PhysicsChange values[2];
    size_t count;
} PhysicsSourceChanges;

EngineResult physics_source_change_get(Entity source, bool torque,
    PhysicsSourceChanges *changes, bool *active);

bool physics_vector_finite_check(Vec2D value);
PhysicsChange physics_change_get(EntityIndex index, unsigned fields);
void physics_change_orientation_set(PhysicsChange *change, float orientation);
bool physics_changes_commit(PhysicsChange *changes, size_t count, Report *report,
    PhysicsReportStage stage, Entity source, Entity target);
void physics_operation_reject(Report *report, PhysicsReportStage stage,
    EngineError code, Entity source, Entity target);
Entity physics_operation_entity_get(EntityIndex index);

#endif
