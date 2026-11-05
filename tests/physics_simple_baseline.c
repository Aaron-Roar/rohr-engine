/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include <assert.h>
#include <math.h>

#define OK(value) assert(!rohr_error_check(value))
static double observed_time;
static unsigned observed_steps;

static void observe(double dt, void *context) {
    (void)context;
    /* Complete-substep policy must be released before caller code executes. */
    assert(!physics_simple_baseline_active_check());
    observed_time += dt;
    ++observed_steps;
}

static Entity body(float x, float speed) {
    ParticleDescription d = rohr_physics_particle_description_default_get();
    d.position = (Position){x, 0}; d.velocity = (Velocity){speed, 0};
    d.radius = 2.5f; d.mass_value = 2; d.gravity_enabled = false;
    EntityResult e = rohr_physics_particle_add(d); OK(e);
    return e.result.value;
}

static void report_check(Report r) {
    assert(!r.has_fatal && r.header.status == ROHR_REPORT_COMPLETE);
    assert(rohr_report_valid_check(&r));
}

static void prepared_composition_check(bool manual) {
    OK(rohr_engine_start());
    Entity e = body(0, 0);
    EntityResult source = rohr_physics_force_add((ForceDescription){
        .target = e, .magnitude = 12,
        .location.reference = {.kind = ROHR_FORCE_COM_POSITION, .entity = e}});
    OK(source);
    rohr_physics_pipeline_step_begin();
    if(manual) {
        rohr_physics_pipeline_substep_begin();
        rohr_physics_pipeline_accelerations_clear();
        rohr_physics_pipeline_gravity_apply();
        report_check(rohr_physics_pipeline_forces_apply());
        report_check(rohr_physics_pipeline_integrate(.25));
        report_check(rohr_physics_pipeline_contacts_gather());
        report_check(rohr_physics_pipeline_joints_gather());
        report_check(rohr_physics_pipeline_constraints_solve(4));
    } else report_check(rohr_physics_pipeline_substep(.25));
    EntityIndexResult index = rohr_entity_index_get(e); OK(index);
    assert(velocities[index.result.value].y == 1.5f);
    assert(positions[index.result.value].y == .375f);
    OK(rohr_entity_delete(source.result.value));
    /* Integration does not own/delete a persistent source, and a fresh force
     * preparation does not reapply it after explicit source destruction. */
    report_check(rohr_physics_pipeline_substep(.25));
    assert(velocities[index.result.value].y == 1.5f);
    assert(positions[index.result.value].y == .75f);
    rohr_engine_stop();
}

int main(void) {
    prepared_composition_check(false);
    prepared_composition_check(true);
    OK(rohr_engine_start());
    OK(rohr_physics_substeps_set(4)); OK(rohr_physics_solver_iterations_set(4));
    Entity a = body(-20, 1000), b = body(20, -1000);
    physics_substep_observer_set(observe, NULL);
    Report r = rohr_physics_dt_update(.1f); report_check(r);
    assert(observed_steps == 4 && fabs(observed_time - (double).1f) < 1e-12);
    unsigned completed = 0;
    for(size_t i = 0; i < r.statistic_count; ++i)
        if(r.statistics[i].metric_id == ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT)
            completed += (unsigned)r.statistics[i].value;
    assert(completed == observed_steps);
    physics_substep_observer_set(NULL, NULL);
    r = rohr_physics_pipeline_substep(NAN); assert(r.has_fatal);
    assert(!physics_simple_baseline_active_check());
    /* Failed complete calls cannot leave manual gathering in bounded mode.
     * Deletion/reuse must not revive a gathered pair under a different handle. */
    OK(rohr_physics_position_set(a, (Position){0, 0}));
    OK(rohr_physics_position_set(b, (Position){4, 0}));
    rohr_physics_pipeline_step_begin();
    report_check(rohr_physics_pipeline_contacts_gather());
    OK(rohr_entity_delete(b));
    Entity replacement = body(100, 0);
    assert(replacement != b);
    report_check(rohr_physics_pipeline_constraints_solve(4));
    assert(!rohr_physics_contact_check(a, replacement));
    OK(rohr_physics_position_set(replacement, (Position){4, 0}));
    report_check(rohr_physics_pipeline_contacts_gather());
    report_check(rohr_physics_pipeline_constraints_solve(4));
    assert(rohr_physics_contact_check(a, replacement));
    rohr_engine_stop();
    assert(!physics_simple_baseline_active_check());
    return 0;
}
