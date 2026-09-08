/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics.h"

#include "physics/physics_step_internal.h"

#include <SDL3/SDL_timer.h>

EngineResult physics_pipeline_substep(double dt) {
    EngineResult result;
    physics_pipeline_substep_begin();
    physics_pipeline_accelerations_clear();
    physics_pipeline_gravity_apply();
    physics_pipeline_forces_apply();
    result = physics_pipeline_integrate(dt);
    if(result.kind == ERROR_RESULT_ERROR) return result;
    physics_pipeline_contacts_gather();
    physics_pipeline_joints_gather();
    physics_pipeline_constraints_solve(physics_solver_iterations_get());
    return error_result_value(true);
}

EngineResult physics_pipeline_update(double dt) {
    uint64_t started =
        physics_update_report_enabled ? SDL_GetPerformanceCounter() : 0;
    uint32_t substeps = physics_substeps_get();
    double substep_dt = dt / (double)substeps;
    EngineResult result = error_result_value(true);

    physics_pipeline_step_begin();
    for(uint32_t substep = 0; substep < substeps; substep += 1) {
        result = physics_pipeline_substep(substep_dt);
        if(result.kind == ERROR_RESULT_ERROR) break;
    }
    if(physics_update_report_enabled) {
        physics_update_report.total_ms = physics_step_elapsed_ms(started);
    }
    return result;
}
