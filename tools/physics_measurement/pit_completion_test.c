/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#define main particle_pit_main
#include "particle_pit.c"
#undef main
#include <assert.h>

int main(void) {
    Pit *p = calloc(1, sizeof(*p));
    assert(p != NULL);
    p->count = 1; p->limit = 60; p->headless = true;
    assert(result_check(rohr_engine_start()));
    assert(result_check(rohr_engine_time_per_tick_set(1.0/240)));
    assert(result_check(rohr_physics_substeps_set(4)));
    assert(result_check(rohr_physics_solver_iterations_set(8)));
    assert(result_check(rohr_physics_gravity_set((Acceleration){0,-9.8f})));
    rohr_physics_engine_time_per_tick_use();
    assert(scene_create(p));
    assert(result_check(rohr_physics_position_set(p->particles[0], (Position){200,-20})));
    double first_depth = 0;
    for(unsigned i = 0; i < p->limit; ++i) {
        assert(step_check(p));
        if(i == 0) first_depth = p->maximum_depth;
    }
    assert(p->completed_frames == 60 && !p->numerical_valid && p->escape_samples == 60);
    assert(p->maximum_depth > first_depth + 4);
    summary_print(p); /* Finite failures finish and retain full-run worst values. */

    p->limit = 1;
    assert(scene_create(p));
    assert(result_check(rohr_entity_delete(p->particles[0])));
    assert(!step_check(p) && p->completed_frames == 0);
    assert(strcmp(p->stop_reason, "particle_state_unavailable") == 0);
    summary_print(p); p->particles[0] = 0;

    assert(scene_create(p));
    EntityIndexResult index = rohr_entity_index_get(p->particles[0]);
    assert(!rohr_error_check(index));
    positions[index.result.value].x = NAN;
    assert(!step_check(p) && p->completed_frames == 0);
    summary_print(p);

    assert(scene_create(p));
    assert(step_check(p) && p->completed_frames == p->limit);
    /* A failure after the last physics step must not become a completed run. */
    p->stop_reason = "graphics_failed";
    summary_print(p);
    rohr_engine_stop(); free(p);
    return 0;
}
