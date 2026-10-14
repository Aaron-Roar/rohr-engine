/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics.h"
#include "physics/constraints/constraint_solver.h"
#include "physics/physics_step_internal.h"
#include <SDL3/SDL_timer.h>
#include <math.h>

void physics_pipeline_step_begin(void) {
    physics_interactions_step_begin();
}

void physics_pipeline_substep_begin(void) {
    contact_constraint_list_clear(&physics_step_contact_constraints);
    joint_constraint_list_clear(&physics_step_joint_constraints);
}

void physics_pipeline_accelerations_clear(void) {
    physics_rigid_accelerations_clear();
}

void physics_pipeline_gravity_apply(void) {
    physics_rigid_gravity_apply(physics_gravity_get());
}

Report physics_pipeline_forces_apply(void) {
    uint64_t started = SDL_GetPerformanceCounter();
    Report report = physics_report_create(ROHR_PHYSICS_LOADS_REPORT);
    physics_joint_spring_forces_apply(&report);
    physics_soft_body_beams_apply();
    physics_report_finish(&report, started);
    return report;
}

Report physics_pipeline_integrate(double dt) {
    uint64_t started = SDL_GetPerformanceCounter();
    Report report = physics_report_create(ROHR_PHYSICS_INTEGRATION_REPORT);
    if(!isfinite(dt) || dt < 0.0) {
        physics_report_issue_add(&report, ROHR_PHYSICS_ARGUMENTS, ROHR_REPORT_SEVERITY_FATAL,
            ERROR_ENGINE_STATE_INVALID, ROHR_REPORT_UPDATE_ABORTED, 0, 0,
            "integration duration must be finite and nonnegative");
    } else {
        EngineResult result = physics_rigid_integrate(dt, &report);
        if(result.kind == ERROR_RESULT_ERROR && !report.has_fatal)
            physics_report_issue_add(&report, ROHR_PHYSICS_INTEGRATION, ROHR_REPORT_SEVERITY_FATAL,
                result.result.error, ROHR_REPORT_UPDATE_ABORTED, 0, 0, NULL);
    }
    physics_report_finish(&report, started);
    return report;
}

Report physics_pipeline_contacts_gather(void) {
    uint64_t started = SDL_GetPerformanceCounter();
    Report report = physics_report_create(ROHR_PHYSICS_CONTACTS_REPORT);
    physics_rigid_constraints_gather(&report);
    if(!report.has_fatal) physics_soft_body_constraints_gather(&report);
    uint64_t excluded_ns = 0;
    for(size_t i = 0; i < report.statistic_count; i += 1)
        if(report.statistics[i].metric_id == ROHR_REPORT_BROADPHASE_BUILD_NS ||
                report.statistics[i].metric_id == ROHR_REPORT_NARROWPHASE_NS)
            excluded_ns += report.statistics[i].value;
    uint64_t elapsed_ns = (uint64_t)(physics_step_elapsed_ms(started) * 1000000.0);
    physics_report_counter_add(&report, ROHR_REPORT_BROADPHASE_QUERY_NS,
        elapsed_ns > excluded_ns ? elapsed_ns - excluded_ns : 0);
    physics_report_finish(&report, started);
    return report;
}

Report physics_pipeline_joints_gather(void) {
    uint64_t started = SDL_GetPerformanceCounter();
    Report report = physics_report_create(ROHR_PHYSICS_JOINTS_REPORT);
    physics_joint_constraints_gather(&report);
    physics_report_finish(&report, started);
    return report;
}

Report physics_pipeline_constraints_solve(uint32_t iterations) {
    uint64_t started = SDL_GetPerformanceCounter();
    Report report = physics_report_create(ROHR_PHYSICS_CONSTRAINTS_REPORT);
    if(iterations != 0) {
        constraint_solver_run(
            &physics_step_contact_constraints,
            iterations,
            physics_pipeline_contact_constraints_solve,
            physics_pipeline_joint_constraints_solve,
            physics_pipeline_contact_constraints_finalize,
            &report);
    } else report.header.status = ROHR_REPORT_NOT_RUN;
    physics_report_finish(&report, started);
    return report;
}
