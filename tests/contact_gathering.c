/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include <assert.h>
#include <math.h>
#include <string.h>

#define OK(value) assert(!rohr_error_check(value))

#ifdef ROHR_TEST_CONTACT_APPEND_WRAP
static bool fail_append;
static unsigned append_attempts;
static Entity failed_first, failed_second;
bool __real_contact_constraint_list_append(ContactConstraintList *, SystemContactConstraint);
bool __wrap_contact_constraint_list_append(ContactConstraintList *list, SystemContactConstraint value) {
    if(fail_append && ++append_attempts == 2) {
        failed_first = value.value.rigid.first;
        failed_second = value.value.rigid.second;
        return false;
    }
    return __real_contact_constraint_list_append(list, value);
}
#endif

static Entity particle(Position position) {
    EntityResult result = rohr_physics_particle_add((ParticleDescription){
        .position = position, .radius = 1, .mass_value = 1,
        .collision_enabled = false});
    OK(result);
    return result.result.value;
}

static uint64_t candidates(const Report *report) {
    uint64_t count = 0;
    for(size_t i = 0; i < report->statistic_count; ++i)
        if(report->statistics[i].metric_id == ROHR_REPORT_CANDIDATE_COUNT)
            count += report->statistics[i].value;
    return count;
}

static Report gather(void) {
    Report report = physics_report_create(ROHR_PHYSICS_CONTACTS_REPORT_ID);
    physics_rigid_constraints_gather(&report);
    assert(!report.has_fatal);
    return report;
}

static void refit_pairs(bool reuse_slot) {
    OK(rohr_engine_start());
    Entity first = particle((Position){0, 0});
    Entity second = particle((Position){1.5f, 1.5f});
    if(reuse_slot) {
        OK(rohr_entity_delete(first));
        first = particle((Position){0, 0});
    }
    EntityIndex a = rohr_entity_index_get(first).result.value;
    EntityIndex b = rohr_entity_index_get(second).result.value;
    physics_step_entities_begin();
    physics_step_hitbox_cache_begin();
    physics_step_contact_constraints.solving = true;
    Report report = gather();
    assert(candidates(&report) == 1 && physics_step_contact_constraints.count == 0);

    /* Overlapping bounds, disjoint circles: both moved, one narrowphase test.
     * Direct pose edits model solver corrections inside this frozen topology. */
    positions[a].x += .125f;
    positions[b].x += .125f;
    report = gather();
    assert(candidates(&report) == 1 && physics_step_contact_constraints.count == 0);
    report = gather();
    assert(candidates(&report) == 0);
    positions[b].x -= .125f;
    report = gather();
    assert(candidates(&report) == 1 && physics_step_contact_constraints.count == 0);

    /* A previous miss must not suppress a new sensor overlap. Both endpoints
     * move, then only one moves; retained constraints keep canonical handles. */
    positions[a] = (Position){.25f, .25f};
    positions[b] = (Position){1.25f, 1.25f};
    report = gather();
    assert(candidates(&report) == 1 && physics_step_contact_constraints.count == 1);
    assert(physics_step_contact_constraints.values[0].value.rigid.first ==
        (first < second ? first : second));
    assert(!physics_step_contact_constraints.values[0].value.rigid.responds);
    positions[b].x -= .125f;
    report = gather();
    assert(candidates(&report) == 0 && physics_step_contact_constraints.count == 1);
    physics_step_entities_end();
    physics_step_hitbox_cache_end();
    physics_step_contact_constraints.solving = false;

    /* Manual gathering must rebuild after shape/pose edits and deletion. */
    OK(rohr_physics_position_set(second, (Position){10, 10}));
    physics_pipeline_substep_begin();
    report = rohr_physics_pipeline_contacts_gather();
    assert(!report.has_fatal && physics_step_contact_constraints.count == 0);
    OK(rohr_entity_delete(second));
    second = particle((Position){.5f, .5f});
    physics_pipeline_substep_begin();
    report = rohr_physics_pipeline_contacts_gather();
    assert(!report.has_fatal && physics_step_contact_constraints.count == 1);
    rohr_engine_stop();
}

static void same_float(float first, float second) {
    assert(memcmp(&first, &second, sizeof(first)) == 0);
}

static void particle_batch_parity(void) {
    uint32_t random = 37;
    for(unsigned sample = 0; sample < 4096; ++sample) {
        PhysicsParticleCircle first[4], second[4];
        OverlapInfo batch[4];
        for(unsigned i = 0; i < 4; ++i) {
            float values[6];
            for(unsigned j = 0; j < 6; ++j) {
                random = random * UINT32_C(1664525) + UINT32_C(1013904223);
                values[j] = (float)(random % 20001) * .01f - 100;
            }
            first[i] = (PhysicsParticleCircle){{values[0], values[1]}, fabsf(values[2])};
            second[i] = (PhysicsParticleCircle){{values[3], values[4]}, fabsf(values[5])};
            if(sample < 8) {
                first[i] = (PhysicsParticleCircle){{0, -0.0f}, 1};
                float distance[] = {0, 0.00001f, nextafterf(.00001f, 0),
                    nextafterf(.00001f, 1), 2, nextafterf(2, 0), nextafterf(2, 3), 1000000};
                second[i] = (PhysicsParticleCircle){{distance[sample], 0}, 1};
            }
        }
        physics_particle_circles_overlap_get(first, second, batch);
        for(unsigned i = 0; i < 4; ++i) {
            OverlapInfo scalar = physics_particle_circle_overlap_get(first[i], second[i]);
            assert(batch[i].detected == scalar.detected);
            same_float(batch[i].depth, scalar.depth);
            same_float(batch[i].normal.x, scalar.normal.x);
            same_float(batch[i].normal.y, scalar.normal.y);
        }
    }
}

static void particle_gather_oracle(void) {
    OK(rohr_engine_start());
    Entity entities[12];
    EntityIndex indices[12];
    for(unsigned i = 0; i < 12; ++i) {
        entities[i] = particle((Position){(float)(i % 4) * .5f, (float)(i / 4) * .5f});
        indices[i] = rohr_entity_index_get(entities[i]).result.value;
    }
    /* Dense queries exercise full batches and tails. Repeat after edits with
     * no pose change to catch inappropriate reuse of cached circle geometry. */
    for(unsigned iteration = 0; iteration < 3; ++iteration) {
        if(iteration == 1) OK(rohr_physics_particle_radius_set(entities[3], .1f));
        if(iteration == 2) OK(rohr_physics_particle_origin_set(entities[6], (Position){10, 0}));
        physics_pipeline_substep_begin();
        Report report = rohr_physics_pipeline_contacts_gather();
        assert(!report.has_fatal);
        size_t expected = 0;
        for(unsigned a = 0; a < 12; ++a) for(unsigned b = a + 1; b < 12; ++b) {
            OverlapInfo overlap = physics_particle_entities_overlap_get(indices[a], indices[b]);
            assert(contact_constraint_list_rigid_pair_check(&physics_step_contact_constraints,
                entities[a], entities[b]) == overlap.detected);
            expected += overlap.detected;
        }
        assert(physics_step_contact_constraints.count == expected);
        for(size_t i = 0; i < expected; ++i) {
            const SystemContactConstraint *constraint = &physics_step_contact_constraints.values[i];
            OverlapInfo scalar = physics_particle_entities_overlap_get(
                constraint->value.rigid.first_index, constraint->value.rigid.second_index);
            assert(!constraint->value.rigid.responds);
            same_float(constraint->value.rigid.overlap.depth, scalar.depth);
            same_float(constraint->value.rigid.overlap.normal.x, scalar.normal.x);
            same_float(constraint->value.rigid.overlap.normal.y, scalar.normal.y);
        }
    }
    rohr_engine_stop();
}

#ifdef ROHR_TEST_CONTACT_APPEND_WRAP
static void particle_batch_failure(void) {
    OK(rohr_engine_start());
    for(unsigned i = 0; i < 12; ++i)
        (void)particle((Position){(float)(i % 4) * .5f, (float)(i / 4) * .5f});
    physics_pipeline_substep_begin();
    append_attempts = 0;
    fail_append = true;
    Report report = rohr_physics_pipeline_contacts_gather();
    fail_append = false;
    assert(report.has_fatal && rohr_report_valid_check(&report));
    assert(report.first_fatal.source.id == failed_first &&
        report.first_fatal.target.id == failed_second);
    /* Four checks finished before publication. The second append fails;
     * retain the first constraint and publish neither remaining lane. */
    assert(append_attempts == 2 && physics_step_contact_constraints.count == 1);
    assert(candidates(&report) == 4);
    rohr_engine_stop();
}
#endif

int main(void) {
    particle_batch_parity();
    refit_pairs(false);
    refit_pairs(true);
    particle_gather_oracle();
#ifdef ROHR_TEST_CONTACT_APPEND_WRAP
    particle_batch_failure();
#endif
    return 0;
}
