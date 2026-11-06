/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include "physics/particles/particle_solver.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef ROHR_TEST_PARTICLE_PASSES
#define ROHR_TEST_PARTICLE_PASSES 0
#endif
#ifndef ROHR_TEST_COMPACT_PARTICLES
#define ROHR_TEST_COMPACT_PARTICLES 0
#endif
#define OK(value) assert(!rohr_error_check(value))
#define PAIRS 48
static unsigned mismatches;

#ifdef ROHR_TEST_PARTICLE_WRAP
static bool reject_scratch, rejecting;
static bool inject_extreme_velocity;
static EntityIndex extreme_index;
static unsigned prepared, rejected;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__wrap_malloc(size_t size) { return rejecting ? NULL : __real_malloc(size); }
void *__wrap_calloc(size_t n, size_t size) { return rejecting ? NULL : __real_calloc(n, size); }
bool __real_physics_particle_solver_prepare_check(ContactConstraintList *, uint32_t, Report *);
bool __wrap_physics_particle_solver_prepare_check(ContactConstraintList *list,
        uint32_t iterations, Report *report) {
    rejecting = reject_scratch;
    bool active = __real_physics_particle_solver_prepare_check(list, iterations, report);
    rejecting = false;
    if(inject_extreme_velocity && list == &physics_step_contact_constraints)
        velocities[extreme_index] = (Velocity){1e38f, 1e38f};
    prepared += active;
    rejected += reject_scratch && !active && report && report->diagnostic_count > 0;
    return active;
}
#endif

typedef struct Sample {
    Position position[2];
    Velocity velocity[2];
    OverlapInfo overlap;
    ContactInfo contact;
} Sample;

static void report_check(Report report, bool allocation_failure) {
    assert(!report.has_fatal && report.header.status == ROHR_REPORT_COMPLETE);
    assert(rohr_report_valid_check(&report));
    if(allocation_failure && ROHR_TEST_COMPACT_PARTICLES)
        assert(report.diagnostic_count > 0);
}

static Entity pair_body(unsigned pair, unsigned side, unsigned round) {
    ParticleDescription d = rohr_physics_particle_description_default_get();
    d.position = (Position){pair * 20.0f + side * 4.0f, side ? 1 : 0};
    d.radius = 2.5f + round * .1f;
    d.velocity = (Velocity){side ? -3 : 3, side ? -7 : 7};
    d.mass_value = side ? 1 + round : 100;
    d.gravity_enabled = false;
    d.friction = round ? .25f : .8f; d.restitution = round ? .1f : .5f;
    /* Sparse fallback cases occur between independent compact runs. */
    if(pair % 11 == 0) d.local_origin = (Position){.3f, -.2f};
    d.static_body = pair % 7 == 0 && side == 1;
    d.collision_enabled = pair % 13 != 0;
    d.collision_filter.category = side ? 2 : 1;
    d.collision_filter.responds_to = pair % 5 == 0 && side == 0
        ? 0 : ROHR_COLLISION_CATEGORY_ALL;
    EntityResult result = rohr_physics_particle_add(d); OK(result);
    return result.result.value;
}

static void scene_sample(bool manual, bool allocation_failure, uint32_t iterations,
        Sample samples[2][PAIRS]) {
    OK(rohr_engine_start());
    OK(rohr_physics_solver_iterations_set(iterations));
    Entity bodies[PAIRS][2];
    for(unsigned pair = 0; pair < PAIRS; ++pair)
        for(unsigned side = 0; side < 2; ++side) bodies[pair][side] = pair_body(pair, side, 0);
    for(unsigned round = 0; round < 2; ++round) {
        if(round) {
            /* Reuse slots and mutate geometry/materials between invocations. */
            for(unsigned pair = 0; pair < PAIRS; ++pair)
                for(unsigned side = 0; side < 2; ++side) {
                    Entity old = bodies[pair][side]; OK(rohr_entity_delete(old));
                    bodies[pair][side] = pair_body(pair, side, round);
                    assert(old != bodies[pair][side]);
                }
        }
        rohr_physics_pipeline_step_begin();
#ifdef ROHR_TEST_PARTICLE_WRAP
        reject_scratch = allocation_failure;
        EntityIndexResult extreme = rohr_entity_index_get(bodies[1][0]); OK(extreme);
        extreme_index = extreme.result.value;
#endif
        if(manual) {
            rohr_physics_pipeline_substep_begin();
            report_check(rohr_physics_pipeline_contacts_gather(), false);
            uint32_t passes = ROHR_TEST_PARTICLE_PASSES && ROHR_TEST_PARTICLE_PASSES < iterations
                ? ROHR_TEST_PARTICLE_PASSES : iterations;
            report_check(rohr_physics_pipeline_constraints_solve(passes), false);
        } else report_check(rohr_physics_pipeline_substep(0), allocation_failure);
#ifdef ROHR_TEST_PARTICLE_WRAP
        reject_scratch = false;
#endif
        assert(!physics_particle_solver_active_check(&physics_step_contact_constraints));
        assert(!physics_step_contact_constraints.particle_passes);
        for(unsigned pair = 0; pair < PAIRS; ++pair) {
            Sample *s = &samples[round][pair];
            for(unsigned side = 0; side < 2; ++side) {
                EntityIndexResult index = rohr_entity_index_get(bodies[pair][side]); OK(index);
                s->position[side] = positions[index.result.value];
                s->velocity[side] = velocities[index.result.value];
                Shape expected = physics_shape_world_translate(hit_boxes[index.result.value],
                    positions[index.result.value], orientations[index.result.value]);
                for(unsigned v = 0; v < expected.amount_of_vertices; ++v) {
                    mismatches += expected.vertices[v].x != world_hit_boxes[index.result.value].vertices[v].x;
                    mismatches += expected.vertices[v].y != world_hit_boxes[index.result.value].vertices[v].y;
                }
            }
            s->overlap = rohr_physics_overlap_get(bodies[pair][0], bodies[pair][1]);
            s->contact = rohr_physics_contact_get(bodies[pair][0], bodies[pair][1]);
        }
    }
    rohr_engine_stop();
    assert(!physics_step_contact_constraints.particle_solver);
}

static void close_check(float a, float b) {
    mismatches += !isfinite(a) || !isfinite(b) || fabsf(a - b) > .00002f;
}

static void samples_compare(Sample a[2][PAIRS], Sample b[2][PAIRS]) {
    for(unsigned round = 0; round < 2; ++round) for(unsigned pair = 0; pair < PAIRS; ++pair) {
        const Sample *x = &a[round][pair], *y = &b[round][pair];
        for(unsigned side = 0; side < 2; ++side) {
            close_check(x->position[side].x, y->position[side].x);
            close_check(x->position[side].y, y->position[side].y);
            close_check(x->velocity[side].x, y->velocity[side].x);
            close_check(x->velocity[side].y, y->velocity[side].y);
        }
        mismatches += x->overlap.detected != y->overlap.detected;
        mismatches += x->contact.detected != y->contact.detected;
        if(x->contact.detected && y->contact.detected) {
            close_check(x->contact.normal.x, y->contact.normal.x);
            close_check(x->contact.normal.y, y->contact.normal.y);
            close_check(x->contact.depth, y->contact.depth);
            close_check(x->contact.points[0].normal_impulse.x, y->contact.points[0].normal_impulse.x);
            close_check(x->contact.points[0].normal_impulse.y, y->contact.points[0].normal_impulse.y);
            close_check(x->contact.points[0].friction_impulse.x, y->contact.points[0].friction_impulse.x);
            close_check(x->contact.points[0].friction_impulse.y, y->contact.points[0].friction_impulse.y);
        }
    }
}

static void scratch_lifetime_check(void) {
    OK(rohr_engine_start());
    for(unsigned pair = 0; pair < PAIRS; ++pair)
        for(unsigned side = 0; side < 2; ++side) (void)pair_body(pair, side, 0);
    for(unsigned cycle = 0; cycle < 1000; ++cycle) {
        /* Overlap remains within the slop, retaining contacts through teardown. */
        rohr_physics_pipeline_step_begin();
        report_check(rohr_physics_pipeline_substep(0), false);
        if(ROHR_TEST_COMPACT_PARTICLES)
            assert(physics_step_contact_constraints.particle_solver != NULL);
        contact_constraint_list_destroy(&physics_step_contact_constraints);
        assert(!physics_step_contact_constraints.particle_solver);
        assert(!physics_step_contact_constraints.values);
    }
    rohr_engine_stop();
}

int main(void) {
    const uint32_t iterations[] = {1, 2, 4, 8};
    for(unsigned i = 0; i < sizeof(iterations) / sizeof(*iterations); ++i) {
        Sample manual[2][PAIRS], complete[2][PAIRS];
        scene_sample(true, false, iterations[i], manual);
        scene_sample(false, false, iterations[i], complete);
        samples_compare(manual, complete);
#ifdef ROHR_TEST_PARTICLE_WRAP
        scene_sample(false, true, iterations[i], complete);
        samples_compare(manual, complete);
#endif
    }
    scratch_lifetime_check();
#ifdef ROHR_TEST_PARTICLE_WRAP
    if(ROHR_TEST_COMPACT_PARTICLES) assert(prepared > 0 && rejected == 8);
    /* Finite velocities whose intermediate impulse/friction calculations
     * overflow must retain checked commits and let unaffected pairs finish. */
    Sample fallback[2][PAIRS], compact[2][PAIRS];
    inject_extreme_velocity = true;
    scene_sample(false, true, 4, fallback);
    scene_sample(false, false, 4, compact);
    inject_extreme_velocity = false;
    samples_compare(fallback, compact);
#endif
    printf("particle_solver endpoint=complete ownership_cycles=1000 mismatches=%u\n", mismatches);
    return mismatches ? EXIT_FAILURE : EXIT_SUCCESS;
}
