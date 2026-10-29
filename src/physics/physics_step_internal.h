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
/* Internal synchronous verification hook. Caller owns context, must unregister
 * before releasing it, and must not mutate physics from the callback. */
void physics_substep_observer_set(void (*observer)(double, void *), void *context);
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
bool physics_step_hitbox_update(EntityIndex index, Report *report);
void physics_step_hitbox_dirty_flush(Report *report);
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
void physics_rigid_motion_integrate(double dt, Report *report);
double physics_rigid_integrate_adaptive(double dt, Report *report);
double physics_rigid_sweep_duration_get(double dt, Report *report);
Report physics_pipeline_integrate_adaptive(double *dt);
void physics_rigid_transform_constraints_apply(Report *report);
void physics_rigid_accelerations_clear(void);
void physics_rigid_gravity_apply(Acceleration gravity, Report *report);
void physics_rigid_constraints_gather(Report *report);
float physics_rigid_contact_slop_get(EntityIndex first, EntityIndex second, Vec2D normal);
bool physics_rigid_contact_block_solve_check(ContactConstraintList *list,
    float position_fraction, Report *report, bool *converged);
Vec2D system_friction_impulse_apply(Entity first, Entity second, OverlapInfo overlap,
    Vec2D first_offset, Vec2D second_offset, float normal_impulse,
    float inverse_mass_first, float inverse_mass_second,
    float inverse_inertia_first, float inverse_inertia_second,
    Vec2D previous_impulse, Report *report);
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
void physics_joint_motion_begin(double dt, Report *report);
void physics_joint_motion_enabled_set(bool enabled);
bool physics_joint_motion_integrated_check(EntityIndex index);
void physics_joint_motion_end(void);
double physics_joint_duration_get(double dt);

void physics_soft_body_beams_apply(Report *report);
void physics_soft_motion_enabled_set(bool enabled);
void physics_soft_motion_begin(double dt, Report *report);
void physics_soft_motion_end(double dt, Report *report);
bool physics_soft_motion_integrated_check(EntityIndex index);
double physics_soft_duration_get(double dt);
double physics_soft_sweep_duration_get(double dt, Report *report);
void physics_beam_pairs_gather(Report *report);
void physics_beam_pair_solve(SystemBeamPair *pair, float fraction, Report *report);
void physics_beam_pair_finalize(const SystemBeamPair *pair, Report *report);
void physics_soft_body_constraints_gather(Report *report);
void physics_soft_body_constraint_solve(
    SystemContactConstraint *constraint,
    float position_fraction, Report *report
);
void physics_soft_body_constraint_finalize(
    const SystemContactConstraint *constraint, Report *report
);

#endif
