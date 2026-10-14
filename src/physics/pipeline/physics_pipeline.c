/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics.h"
#include "physics/physics_step_internal.h"
#include <SDL3/SDL_timer.h>
#include <math.h>

Report physics_pipeline_substep(double dt) {
    uint64_t started = SDL_GetPerformanceCounter();
    Report report = physics_report_create(ROHR_PHYSICS_SUBSTEP_REPORT);
    Report child;
    if(!isfinite(dt) || dt < 0.0) {
        physics_report_issue_add(&report, ROHR_PHYSICS_ARGUMENTS, ROHR_REPORT_SEVERITY_FATAL,
            ERROR_ENGINE_STATE_INVALID, ROHR_REPORT_UPDATE_ABORTED, 0, 0,
            "substep duration must be finite and nonnegative");
        goto done;
    }
    physics_pipeline_substep_begin();
    physics_pipeline_accelerations_clear();
    physics_pipeline_gravity_apply();
    child = physics_pipeline_forces_apply();
    (void)report_merge(&report, &child);
    if(report.has_fatal) goto done;
    child = physics_pipeline_integrate(dt);
    (void)report_merge(&report, &child);
    if(report.has_fatal) goto done;
    child = physics_pipeline_contacts_gather();
    (void)report_merge(&report, &child);
    if(report.has_fatal) goto done;
    child = physics_pipeline_joints_gather();
    (void)report_merge(&report, &child);
    if(report.has_fatal) goto done;
    child = physics_pipeline_constraints_solve(physics_solver_iterations_get());
    (void)report_merge(&report, &child);
done:
    physics_report_finish(&report, started);
    return report;
}

Report physics_pipeline_update_internal(double dt, bool publish) {
    uint64_t started = SDL_GetPerformanceCounter();
    Report report = physics_report_create(ROHR_PHYSICS_REPORT);
    uint32_t substeps = physics_substeps_get();
    if(!isfinite(dt) || dt < 0.0) {
        physics_report_issue_add(&report, ROHR_PHYSICS_ARGUMENTS, ROHR_REPORT_SEVERITY_FATAL,
            ERROR_ENGINE_STATE_INVALID, ROHR_REPORT_UPDATE_ABORTED, 0, 0,
            "physics duration must be finite and nonnegative");
        goto done;
    }
    physics_pipeline_step_begin();
    for(uint32_t substep = 0; substep < substeps; substep += 1) {
        Report child;
        physics_report_counter_add(&report, ROHR_REPORT_SUBSTEPS_ATTEMPTED_COUNT, 1);
        physics_report_context_set(true, substep);
        child = physics_pipeline_substep(dt / (double)substeps);
        physics_report_context_set(false, 0);
        (void)report_merge(&report, &child);
        if(report.has_fatal) break;
        physics_report_counter_add(&report, ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT, 1);
    }
done:
    if(publish) physics_report_finish(&report, started);
    else report.header.total_ms = physics_step_elapsed_ms(started);
    return report;
}

Report physics_pipeline_update(double dt) {
    return physics_pipeline_update_internal(dt, true);
}
