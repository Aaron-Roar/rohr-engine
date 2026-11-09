/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include "physics/physics_internal.h"
#include <assert.h>
#include <math.h>

#define OK(value) assert(!rohr_error_check(value))
#ifndef ROHR_TEST_PARTICLE_PASSES
#define ROHR_TEST_PARTICLE_PASSES 0
#endif
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

static void baseline_report_assert(Report r) {
    if(r.has_fatal || r.header.status != ROHR_REPORT_COMPLETE)
        (void)rohr_report_console_write(&r);
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
        baseline_report_assert(rohr_physics_pipeline_forces_apply());
        baseline_report_assert(rohr_physics_pipeline_integrate(.25));
        baseline_report_assert(rohr_physics_pipeline_contacts_gather());
        baseline_report_assert(rohr_physics_pipeline_joints_gather());
        baseline_report_assert(rohr_physics_pipeline_constraints_solve(4));
    } else baseline_report_assert(rohr_physics_pipeline_substep(.25));
    EntityIndexResult index = rohr_entity_index_get(e); OK(index);
    assert(velocities[index.result.value].y == 1.5f);
    assert(positions[index.result.value].y == .375f);
    OK(rohr_entity_delete(source.result.value));
    /* Integration does not own/delete a persistent source, and a fresh force
     * preparation does not reapply it after explicit source destruction. */
    baseline_report_assert(rohr_physics_pipeline_substep(.25));
    assert(velocities[index.result.value].y == 1.5f);
    assert(positions[index.result.value].y == .75f);
    rohr_engine_stop();
}

/* Compare the complete candidate's particle path with ordinary manual response
 * at the same particle effort. Offset circles exercise deferred polygon refresh;
 * masks/static bodies verify that stored motion mode never replaces pair mass. */
static void particle_pair_sample(bool manual, unsigned mode, float tangent,
        uint32_t iterations, float state[8]) {
    OK(rohr_engine_start());
    OK(rohr_physics_solver_iterations_set(iterations));
    Entity entities[2];
    for(unsigned side = 0; side < 2; ++side) {
        ParticleDescription d = rohr_physics_particle_description_default_get();
        d.position = (Position){side ? 4 : 0, side ? 1 : 0};
        d.local_origin = (Position){.3f, -.2f};
        d.radius = 2.5f; d.rigid_vertices = 32;
        d.velocity = (Velocity){side ? -3 : 3, side ? -tangent : tangent};
        d.mass_value = side ? 1 : 100; d.friction = .8f; d.restitution = .5f;
        d.static_body = mode == 1 && side == 1;
        d.collision_enabled = mode != 3;
        d.collision_filter.category = side ? 2 : 1;
        d.collision_filter.responds_to = mode == 2 && side == 0 ? 0 : ROHR_COLLISION_CATEGORY_ALL;
        EntityResult result = rohr_physics_particle_add(d); OK(result);
        entities[side] = result.result.value;
    }
    rohr_physics_pipeline_step_begin();
    if(manual) {
        rohr_physics_pipeline_substep_begin();
        baseline_report_assert(rohr_physics_pipeline_contacts_gather());
        uint32_t passes = ROHR_TEST_PARTICLE_PASSES && ROHR_TEST_PARTICLE_PASSES < iterations
            ? ROHR_TEST_PARTICLE_PASSES : iterations;
        baseline_report_assert(rohr_physics_pipeline_constraints_solve(passes));
    } else baseline_report_assert(rohr_physics_pipeline_substep(0));
    for(unsigned side = 0; side < 2; ++side) {
        EntityIndexResult result = rohr_entity_index_get(entities[side]); OK(result);
        EntityIndex i = result.result.value;
        state[4*side] = positions[i].x; state[4*side+1] = positions[i].y;
        state[4*side+2] = velocities[i].x; state[4*side+3] = velocities[i].y;
        Shape expected = physics_shape_world_translate(hit_boxes[i], positions[i], orientations[i]);
        assert(expected.amount_of_vertices == world_hit_boxes[i].amount_of_vertices);
        for(unsigned v = 0; v < expected.amount_of_vertices; ++v) {
            assert(expected.vertices[v].x == world_hit_boxes[i].vertices[v].x);
            assert(expected.vertices[v].y == world_hit_boxes[i].vertices[v].y);
        }
    }
    rohr_engine_stop();
}

static void particle_pair_check(void) {
    const uint32_t iterations[] = {1, 2, 4, 8};
    for(unsigned effort = 0; effort < sizeof(iterations)/sizeof(*iterations); ++effort)
    for(unsigned mode = 0; mode < 4; ++mode) for(unsigned tangent = 0; tangent < 2; ++tangent) {
        float manual[8], complete[8];
        particle_pair_sample(true, mode, tangent * 7, iterations[effort], manual);
        particle_pair_sample(false, mode, tangent * 7, iterations[effort], complete);
        for(unsigned i = 0; i < 8; ++i) assert(fabsf(manual[i] - complete[i]) < .00001f);
    }
}

static void particle_discovery_check(void) {
    OK(rohr_engine_start());
    OK(rohr_physics_substeps_set(4));
    Entity entities[128];
    uint32_t seed = 713;
    for(unsigned n = 0; n < 128; ++n) {
        ParticleDescription d = rohr_physics_particle_description_default_get();
        seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
        d.position.x = (float)(seed % 1000) / 20 - 25;
        seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
        d.position.y = (float)(seed % 1000) / 20 - 25;
        d.radius = .25f + (n % 17) * .5f;
        d.local_origin = (Position){(n % 3) * .1f, -.2f};
        d.collision_enabled = false;
        EntityResult result = rohr_physics_particle_add(d); OK(result);
        entities[n] = result.result.value;
    }
    /* Include polygons on both sides of the particle handle range. */
    for(unsigned n = 0; n < 3; ++n) {
        EntityResult result = rohr_entity_add(); OK(result);
        Entity e = result.result.value;
        OK(rohr_physics_position_set(e, (Position){(float)n * 5 - 5, 0}));
        OK(rohr_physics_hitbox_set(e, rohr_math_square_create(3, 40)));
        OK(rohr_physics_static_set(e));
        OK(rohr_physics_collision_response_none_set(e));
        OK(rohr_entity_delete(entities[n])); entities[n] = e;
    }
    for(unsigned round = 0; round < 4; ++round) {
        if(round == 1) for(unsigned n = 4; n < 14; ++n) {
            Entity old = entities[n]; OK(rohr_entity_delete(old));
            entities[n] = body((float)n - 10, 0);
            assert(old != entities[n]);
            OK(rohr_physics_collision_response_none_set(entities[n]));
        }
        if(round == 2) {
            /* A changing outlier must not leave the grid's size stale. */
            OK(rohr_physics_particle_radius_set(entities[30], 100));
            OK(rohr_physics_particle_radius_set(entities[31], .05f));
        }
        if(round == 3) for(unsigned n = 20; n < 22; ++n) {
            EntityIndexResult index = rohr_entity_index_get(entities[n]); OK(index);
            /* Exercise the guarded cell-coordinate fallback with corrupted
             * but finite offsets; do not convert an out-of-range float to int. */
            particle_geometries_pool.objects[index.result.value].local_origin = (Position){1e20f, 1e20f};
        }
        rohr_physics_pipeline_step_begin();
        baseline_report_assert(rohr_physics_pipeline_substep(0));
        for(unsigned n = 0; n < 128; ++n) for(unsigned m = n + 1; m < 128; ++m) {
            EntityIndexResult a = rohr_entity_index_get(entities[n]), b = rohr_entity_index_get(entities[m]);
            OK(a); OK(b);
            bool particles = physics_step_entity_components_check(a.result.value, ROHR_PARTICLE) &&
                physics_step_entity_components_check(b.result.value, ROHR_PARTICLE);
            bool expected = particles ?
                physics_particle_entities_overlap_get(a.result.value, b.result.value).detected :
                rohr_physics_sat_overlap_get(world_hit_boxes[a.result.value],
                    world_hit_boxes[b.result.value]).detected;
            assert(rohr_physics_overlap_check(entities[n], entities[m]) == expected);
            assert(!rohr_physics_contact_check(entities[n], entities[m]));
        }
    }
    rohr_engine_stop();
}

static void particle_distant_discovery_check(void) {
    OK(rohr_engine_start());
    Entity entities[12];
    for(unsigned n = 0; n < 12; ++n) {
        ParticleDescription d = rohr_physics_particle_description_default_get();
        /* Radius 2.5 gives width 5. Cells separated by 4096 in either axis
         * alias in the current hash, but retain distinct spatial identities.
         * Each distant group contains an overlapping pair and a separated one. */
        float offset = ((float)(n / 3) - 2) * 4096 * 5;
        d.position = (Position){offset + (n % 3 == 1 ? 4 : n % 3 == 2 ? 20 : 0), offset};
        d.radius = 2.5f; d.gravity_enabled = false; d.collision_enabled = false;
        EntityResult result = rohr_physics_particle_add(d); OK(result);
        entities[n] = result.result.value;
    }
    for(unsigned round = 0; round < 2; ++round) {
        if(round) for(unsigned n = 0; n < 12; ++n) {
            EntityIndexResult index = rohr_entity_index_get(entities[n]); OK(index);
            Position position = positions[index.result.value];
            position.y -= 500000;
            OK(rohr_physics_position_set(entities[n], position));
        }
        rohr_physics_pipeline_step_begin();
        baseline_report_assert(rohr_physics_pipeline_substep(0));
        for(unsigned n = 0; n < 12; ++n) for(unsigned m = n + 1; m < 12; ++m) {
            bool expected = n / 3 == m / 3 && n % 3 == 0 && m % 3 == 1;
            assert(rohr_physics_overlap_check(entities[n], entities[m]) == expected);
            assert(!rohr_physics_contact_check(entities[n], entities[m]));
        }
    }
    rohr_engine_stop();
}

int main(void) {
    particle_pair_check();
    particle_discovery_check();
    particle_distant_discovery_check();
    prepared_composition_check(false);
    prepared_composition_check(true);
    OK(rohr_engine_start());
    assert(rohr_physics_substeps_get() == 4);
    assert(rohr_physics_solver_iterations_get() == 4);
    assert(rohr_engine_time_per_tick_get() == 1.0 / 60.0);
    Entity a = body(-20, 1000), b = body(20, -1000);
    physics_substep_observer_set(observe, NULL);
    Report r = rohr_physics_dt_update(.1f); baseline_report_assert(r);
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
    baseline_report_assert(rohr_physics_pipeline_contacts_gather());
    OK(rohr_entity_delete(b));
    Entity replacement = body(100, 0);
    assert(replacement != b);
    baseline_report_assert(rohr_physics_pipeline_constraints_solve(4));
    assert(!rohr_physics_contact_check(a, replacement));
    OK(rohr_physics_position_set(replacement, (Position){4, 0}));
    baseline_report_assert(rohr_physics_pipeline_contacts_gather());
    baseline_report_assert(rohr_physics_pipeline_constraints_solve(4));
    assert(rohr_physics_contact_check(a, replacement));
    rohr_engine_stop();
    assert(!physics_simple_baseline_active_check());
    return 0;
}
