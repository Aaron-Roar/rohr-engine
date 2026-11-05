/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics.h"
#include "physics/physics_step_internal.h"
#include <SDL3/SDL_timer.h>
#include <math.h>

static void (*substep_observer)(double, void *);
static void *substep_observer_context;
static bool simple_baseline_active;

#ifndef ROHR_PHYSICS_SIMPLE_BASELINE
#define ROHR_PHYSICS_SIMPLE_BASELINE 0
#endif

bool physics_simple_baseline_active_check(void) {
    return simple_baseline_active;
}

void physics_substep_observer_set(void (*observer)(double, void *), void *context) {
    substep_observer = observer;
    substep_observer_context = context;
}

static Report physics_pipeline_substep_internal(double *duration, bool adaptive) {
    double dt = *duration;
    uint64_t started = SDL_GetPerformanceCounter();
    Report report = physics_report_create(ROHR_PHYSICS_SUBSTEP_REPORT_ID);
    Report child;
    if(!isfinite(dt) || dt < 0.0) {
        physics_report_issue_add(&report, ROHR_PHYSICS_ARGUMENTS, ROHR_REPORT_SEVERITY_FATAL,
            ERROR_ENGINE_STATE_INVALID, ROHR_REPORT_UPDATE_ABORTED, 0, 0,
            "substep duration must be finite and nonnegative");
        goto done;
    }
    simple_baseline_active = ROHR_PHYSICS_SIMPLE_BASELINE;
    physics_pipeline_substep_begin();
    physics_step_entities_begin();
    physics_rigid_gather_tracking_set(true);
    physics_step_hitbox_cache_begin();
    physics_pipeline_accelerations_clear();
    physics_rigid_gravity_apply(physics_gravity_get(), &report);
    child = physics_pipeline_forces_apply();
    (void)report_merge(&report, &child);
    if(report.has_fatal) goto done;
    /* Only complete substeps have prepared spring loads. Standalone integration
     * must not invent forces or constraint work for a manually composed pass. */
    /* The baseline uses ordinary semi-implicit integration of prepared loads.
     * Joint/elastic special integration and swept adaptive selection belong to
     * the comparison control until their cost and protection are reintroduced. */
    physics_joint_motion_enabled_set(!simple_baseline_active);
    physics_soft_motion_enabled_set(!simple_baseline_active);
    child = adaptive ? physics_pipeline_integrate_adaptive(duration) : physics_pipeline_integrate(dt);
    physics_joint_motion_enabled_set(false);
    physics_soft_motion_enabled_set(false);
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
    simple_baseline_active = false;
    physics_rigid_gather_tracking_set(false);
    physics_step_entities_end();
    physics_step_hitbox_cache_end();
    physics_report_finish(&report, started);
    return report;
}

Report physics_pipeline_substep(double dt) {
    return physics_pipeline_substep_internal(&dt, false);
}

Report physics_pipeline_update_internal(double dt, bool publish) {
    uint64_t started = SDL_GetPerformanceCounter();
    Report report = physics_report_create(ROHR_PHYSICS_REPORT_ID);
    uint32_t substeps = physics_substeps_get();
    if(!isfinite(dt) || dt < 0.0) {
        physics_report_issue_add(&report, ROHR_PHYSICS_ARGUMENTS, ROHR_REPORT_SEVERITY_FATAL,
            ERROR_ENGINE_STATE_INVALID, ROHR_REPORT_UPDATE_ABORTED, 0, 0,
            "physics duration must be finite and nonnegative");
        goto done;
    }
    physics_pipeline_step_begin();
    uint32_t completed = 0;
    for(uint32_t substep = 0; substep < substeps; substep += 1) {
        double remaining = dt / (double)substeps;
        do {
            double duration = remaining;
            Report child;
            physics_report_counter_add(&report, ROHR_REPORT_SUBSTEPS_ATTEMPTED_COUNT, 1);
            physics_report_context_set(true, completed);
            child = physics_pipeline_substep_internal(&duration, !ROHR_PHYSICS_SIMPLE_BASELINE);
            physics_report_context_set(false, 0);
            (void)report_merge(&report, &child);
            if(report.has_fatal) goto done;
            if(remaining > 0 && (!(duration > 0) || duration > remaining ||
                    remaining - duration == remaining)) {
                physics_report_issue_add(&report, ROHR_PHYSICS_INTEGRATION, ROHR_REPORT_SEVERITY_FATAL,
                    ERROR_ENGINE_STATE_INVALID, ROHR_REPORT_UPDATE_ABORTED, 0, 0,
                    "adaptive substep could not advance simulation time");
                goto done;
            }
            physics_report_counter_add(&report, ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT, 1);
            ++completed;
            if(substep_observer != NULL) substep_observer(duration, substep_observer_context);
            remaining -= duration;
        } while(remaining > 0);
    }
done:
    if(publish) physics_report_finish(&report, started);
    else report.header.total_ms = physics_step_elapsed_ms(started);
    return report;
}

Report physics_pipeline_update(double dt) {
    return physics_pipeline_update_internal(dt, true);
}
