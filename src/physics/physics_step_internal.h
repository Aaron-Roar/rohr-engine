/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_PHYSICS_STEP_INTERNAL_H
#define ROHR_PHYSICS_STEP_INTERNAL_H

#include "entity_components.h"
#include "physics/broadphase/aabb_tree.h"
#include "physics/collision/contact_constraint.h"
#include "physics/collision/interaction_set.h"
#include "physics/joints/joint_constraint.h"

extern AABBTree physics_broadphase_tree;
/** Internal collection helpers; NULL is allowed for legacy report-free helpers. */
Report physics_report_create(ReportId id);
Report physics_pipeline_update_internal(double dt, bool publish);
void physics_report_context_set(bool active, uint32_t substep);
void physics_report_counter_add(Report *report, ReportMetricId metric, uint64_t value);
void physics_report_issue_add(Report *report, uint32_t stage, ReportSeverity severity,
    EngineError code, ReportRecovery recovery, Entity source, Entity target, const char *detail);
void physics_report_finish(Report *report, uint64_t started);
void physics_report_substep_set(Report *report, uint32_t substep);
void physics_hitbox_animation_bindings_report_update(Report *report);
extern ContactConstraintList physics_step_contact_constraints;
extern JointConstraintList physics_step_joint_constraints;

void physics_interactions_step_begin(void);
bool physics_step_entity_from_index_get(EntityIndex index, Entity *entity);
bool physics_step_alive_index_at(uint32_t alive_position, EntityIndex *index);
void physics_step_interaction_by_index_record(
    EntityIndex entity_1,
    EntityIndex entity_2,
    OverlapInfo overlap,
    ContactInfo contact,
    PhysicsInteractionFlags flags, Report *report
);
void physics_step_hitbox_dirty_add(EntityIndex index);
void physics_step_hitbox_dirty_flush(void);
void physics_step_entity_by_index_delete(EntityIndex index);
void physics_step_transform_lock_by_index_remove(EntityIndex index);
double physics_step_elapsed_ms(uint64_t start);
void physics_pipeline_contact_constraints_solve(
    ContactConstraintList *constraints,
    float position_fraction,
    void *context
);
void physics_pipeline_contact_constraints_finalize(
    ContactConstraintList *constraints,
    void *context
);
void physics_pipeline_joint_constraints_solve(void *context);

EngineResult physics_rigid_integrate(double dt, Report *report);
void physics_rigid_transform_constraints_apply(void);
void physics_rigid_accelerations_clear(void);
void physics_rigid_gravity_apply(Acceleration gravity);
void physics_rigid_constraints_gather(Report *report);
void physics_rigid_contact_constraint_solve(
    SystemContactConstraint *constraint,
    float position_fraction, Report *report
);
void physics_rigid_contact_constraint_finalize(
    const SystemContactConstraint *constraint, Report *report
);
void physics_rigid_contact_point_impulses_accumulate(
    ContactInfo *current,
    const ContactInfo *previous
);

void physics_joint_spring_forces_apply(Report *report);
void physics_joint_constraints_gather(Report *report);
void physics_joint_constraints_solve(Report *report);

void physics_soft_body_beams_apply(void);
void physics_soft_body_constraints_gather(Report *report);
void physics_soft_body_constraint_solve(
    SystemContactConstraint *constraint,
    float position_fraction, Report *report
);
void physics_soft_body_constraint_finalize(
    const SystemContactConstraint *constraint, Report *report
);

#endif
