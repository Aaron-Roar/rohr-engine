/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include "physics/physics_operation.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#define OK(value) assert(!rohr_error_check(value))

static Entity particle(float x, bool sensor) {
    ParticleDescription description = rohr_physics_particle_description_default_get();
    description.position = (Position){x, 0};
    description.radius = 1;
    description.mass_value = 1;
    description.gravity_enabled = false;
    description.collision_enabled = !sensor;
    EntityResult result = rohr_physics_particle_add(description); OK(result);
    return result.result.value;
}

static void same_float(float first, float second) {
    if(memcmp(&first, &second, sizeof(first)) != 0)
        fprintf(stderr, "rigid contact mismatch actual=%a expected=%a\n", first, second);
    assert(memcmp(&first, &second, sizeof(first)) == 0);
}

static void manual_circle_edits(void) {
    OK(rohr_engine_start());
    Entity a = particle(0, false), b = particle(1.5f, false);
    OK(rohr_physics_static_set(a));
    OK(rohr_physics_static_set(b));
    EntityIndex ai = rohr_entity_index_get(a).result.value;
    EntityIndex bi = rohr_entity_index_get(b).result.value;
    physics_pipeline_substep_begin();
    assert(!rohr_physics_pipeline_contacts_gather().has_fatal);
    for(unsigned edit = 0; edit < 4; ++edit) {
        if(edit == 1) OK(rohr_physics_particle_radius_set(a, 2));
        if(edit == 2) OK(rohr_physics_particle_origin_set(a, (Position){.25f, .5f}));
        if(edit == 3) OK(rohr_physics_position_set(b, (Position){1.25f, .25f}));
        OverlapInfo expected = physics_particle_entities_overlap_get(ai, bi);
        assert(expected.detected);
        Report report = rohr_physics_pipeline_constraints_solve(8);
        assert(!report.has_fatal && physics_step_contact_constraints.count == 1);
        OverlapInfo actual = physics_step_contact_constraints.values[0].value.rigid.overlap;
        assert(actual.detected);
        same_float(actual.normal.x, expected.normal.x);
        same_float(actual.normal.y, expected.normal.y);
        same_float(actual.depth, expected.depth);
    }
    rohr_engine_stop();
}

typedef struct Snapshot {
    Position position_values[3];
    Velocity velocity_values[3];
    float angular[3];
    float normal_impulse[3][2], friction_impulse[3][2], rebound[3];
} Snapshot;

static void sequential_response(bool cold, bool rotating, Snapshot snapshots[16]) {
    OK(rohr_engine_start());
    Entity entities[3];
    EntityIndex indices[3];
    for(unsigned i = 0; i < 3; ++i) {
        if(rotating && i == 2) {
            EntityResult result = rohr_entity_add(); OK(result);
            entities[i] = result.result.value;
            OK(rohr_physics_hitbox_set(entities[i], rohr_math_square_create(2, 2)));
            OK(rohr_physics_particle_radius_set(entities[i], 1));
            OK(rohr_physics_position_set(entities[i], (Position){(float)i * .75f, 0}));
            OK(rohr_physics_dynamic_set(entities[i]));
            OK(rohr_entity_components_add(entities[i], ROHR_COLLISION));
            OK(rohr_physics_orientation_set(entities[i], 30));
        } else entities[i] = particle((float)i * .75f, false);
        indices[i] = rohr_entity_index_get(entities[i]).result.value;
        OK(rohr_physics_mass_set(entities[i], i == 2 ? 40 : 1));
        OK(rohr_physics_velocity_set(entities[i], (Velocity){3 - 2*(float)i, (float)i}));
        OK(rohr_physics_particle_origin_set(entities[i], (Position){.125f, .25f}));
    }
    physics_pipeline_substep_begin();
    assert(!rohr_physics_pipeline_contacts_gather().has_fatal);
    assert(physics_step_contact_constraints.count == 3);
    Report report = physics_report_create(ROHR_PHYSICS_CONSTRAINTS_REPORT_ID);
    physics_step_hitbox_cache_begin();
    for(unsigned pass = 0; pass < 16; ++pass) {
        if(rotating && pass == 8) {
            /* Public calls may change COM without changing the pose. A new
             * manual invocation must not inherit the previous center cache. */
            physics_step_hitbox_cache_end();
            OK(rohr_physics_center_of_mass_local_position_set(entities[2], (Position){.5f, -.25f}));
            OK(rohr_physics_particle_radius_set(entities[2], 1.5f));
            physics_step_hitbox_cache_begin();
        }
        for(size_t i = 0; i < physics_step_contact_constraints.count; ++i) {
            if(cold) {
                physics_step_hitbox_cache_end();
                physics_step_hitbox_cache_begin();
            }
            physics_rigid_contact_constraint_solve(&physics_step_contact_constraints.values[i],
                1.0f / (float)(16-pass), &report);
            assert(!report.has_fatal);
        }
        Snapshot *s = &snapshots[pass];
        *s = (Snapshot){0};
        for(unsigned i = 0; i < 3; ++i) {
            s->position_values[i] = positions[indices[i]];
            s->velocity_values[i] = velocities[indices[i]];
            s->angular[i] = angular_velocities[indices[i]];
            const SystemContactConstraint *c = &physics_step_contact_constraints.values[i];
            s->normal_impulse[i][0] = c->value.rigid.contact.points[0].normal_impulse.x;
            s->normal_impulse[i][1] = c->value.rigid.contact.points[0].normal_impulse.y;
            s->friction_impulse[i][0] = c->value.rigid.contact.points[0].friction_impulse.x;
            s->friction_impulse[i][1] = c->value.rigid.contact.points[0].friction_impulse.y;
            s->rebound[i] = c->value.rigid.rebound_speeds[0][0];
        }
    }
    physics_step_hitbox_cache_end();
    rohr_engine_stop();
}

static void stale_contact_identity(void) {
    OK(rohr_engine_start());
    Entity old = particle(0, false), other = particle(1.5f, false);
    EntityIndex slot = rohr_entity_index_get(old).result.value;
    physics_pipeline_substep_begin();
    assert(!physics_pipeline_contacts_gather().has_fatal);
    assert(physics_step_contact_constraints.count == 1);
    SystemContactConstraint saved = physics_step_contact_constraints.values[0];
    OK(rohr_entity_delete(old));
    Entity replacement = particle(0, false);
    assert(replacement != old && rohr_entity_index_get(replacement).result.value == slot);
    for(unsigned cached = 0; cached < 2; ++cached) {
        if(cached) physics_step_entities_begin();
        SystemContactConstraint before;
        memcpy(&before, &saved, sizeof(before));
        Report report = physics_report_create(ROHR_PHYSICS_CONSTRAINTS_REPORT_ID);
        physics_rigid_contact_constraint_solve(&saved, 1, &report);
        assert(!report.has_fatal && memcmp(&before, &saved, sizeof(before)) == 0);
        same_float(rohr_physics_position_get(replacement).result.value.x, 0);
        same_float(rohr_physics_position_get(other).result.value.x, 1.5f);
        physics_pipeline_step_begin();
        physics_rigid_contact_constraint_finalize(&saved, &report);
        assert(!report.has_fatal && !rohr_physics_overlap_check(replacement, other));
        physics_step_entities_end();
    }
    rohr_engine_stop();
}

static void retained_particle_rebound(void) {
    OK(rohr_engine_start());
    Entity a = particle(0, false), b = particle(1.5f, false);
    OK(rohr_physics_restitution_set(a, .5f));
    OK(rohr_physics_restitution_set(b, .5f));
    OK(rohr_physics_friction_set(a, 0));
    OK(rohr_physics_friction_set(b, 0));
    OK(rohr_physics_velocity_set(a, (Velocity){10, 0}));
    OK(rohr_physics_velocity_set(b, (Velocity){-10, 0}));
    physics_pipeline_substep_begin();
    assert(!physics_pipeline_contacts_gather().has_fatal);
    assert(physics_step_contact_constraints.count == 1);
    SystemContactConstraint *contact = &physics_step_contact_constraints.values[0];
    Report report = physics_report_create(ROHR_PHYSICS_CONSTRAINTS_REPORT_ID);
    physics_rigid_contact_constraint_solve(contact, 0, &report);
    same_float(velocities[rohr_entity_index_get(a).result.value].x, -5);
    same_float(velocities[rohr_entity_index_get(b).result.value].x, 5);
    same_float(contact->value.rigid.rebound_speeds[0][0], 10);
    /* Another constraint can change the velocities before this pair's next
     * pass. Its rebound target must still describe the original impact. */
    OK(rohr_physics_velocity_set(a, (Velocity){20, 0}));
    OK(rohr_physics_velocity_set(b, (Velocity){-20, 0}));
    physics_rigid_contact_constraint_solve(contact, 0, &report);
    same_float(velocities[rohr_entity_index_get(a).result.value].x, -5);
    same_float(velocities[rohr_entity_index_get(b).result.value].x, 5);
    same_float(contact->value.rigid.rebound_speeds[0][0], 10);
    assert(!report.has_fatal && report.diagnostic_count == 0);
    rohr_engine_stop();
}

static void particle_velocity_commit(void) {
    OK(rohr_engine_start());
    Entity entities[] = {particle(0, false), particle(1.5f, false)};
    EntityIndex indices[] = {rohr_entity_index_get(entities[0]).result.value,
        rohr_entity_index_get(entities[1]).result.value};
    /* Cover success, no change, and rejection on either side of the pair.
     * The compact path must preserve the generic atomicity and convergence
     * tracking contract, including invalid angular state on particles. */
    for(unsigned scenario = 0; scenario < 6; ++scenario) {
        Velocity expected[2];
        float expected_angular[2];
        bool expected_commit = false, expected_unchanged = false;
        unsigned expected_diagnostics = 0;
        for(unsigned compact = 0; compact < 2; ++compact) {
            PhysicsChange generic[2];
            PhysicsVelocityChange pair[2];
            for(unsigned i = 0; i < 2; ++i) {
                velocities[indices[i]] = (Velocity){(float)i, -(float)i};
                angular_velocities[indices[i]] = 0;
                generic[i] = physics_change_get(indices[i],
                    PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY);
                if(scenario != 1) generic[i].velocity.x += 3;
                if(scenario == 2 + i) generic[i].velocity.y = INFINITY;
                if(scenario == 4 + i) generic[i].angular_velocity = NAN;
                pair[i] = (PhysicsVelocityChange){indices[i], generic[i].velocity,
                    generic[i].angular_velocity};
            }
            Report report = physics_report_create(ROHR_PHYSICS_CONSTRAINTS_REPORT_ID);
            physics_motion_tracking_begin();
            bool committed = compact
                ? physics_velocity_pair_commit(pair, &report, ROHR_PHYSICS_NORMAL_IMPULSE)
                : physics_changes_commit(generic, 2, &report, ROHR_PHYSICS_NORMAL_IMPULSE,
                    entities[0], entities[1]);
            bool unchanged = physics_motion_tracking_end_check();
            if(!compact) {
                expected_commit = committed;
                expected_unchanged = unchanged;
                expected_diagnostics = report.diagnostic_count;
            } else {
                assert(committed == expected_commit && unchanged == expected_unchanged);
                assert(report.diagnostic_count == expected_diagnostics);
            }
            assert(committed == (scenario < 2));
            assert(unchanged == (scenario != 0));
            if(!committed) {
                assert(report.diagnostic_count == 1);
                assert(report.diagnostics[0].internal_stage == ROHR_PHYSICS_NORMAL_IMPULSE);
                assert(report.diagnostics[0].source.id == entities[0]);
                assert(report.diagnostics[0].target.id == entities[1]);
            }
            for(unsigned i = 0; i < 2; ++i) {
                if(!compact) {
                    expected[i] = velocities[indices[i]];
                    expected_angular[i] = angular_velocities[indices[i]];
                } else {
                    same_float(velocities[indices[i]].x, expected[i].x);
                    same_float(velocities[indices[i]].y, expected[i].y);
                    same_float(angular_velocities[indices[i]], expected_angular[i]);
                }
                same_float(positions[indices[i]].x, i * 1.5f);
                same_float(positions[indices[i]].y, 0);
            }
        }
    }
    rohr_engine_stop();
}

int main(void) {
    manual_circle_edits();
    retained_particle_rebound();
    particle_velocity_commit();
    Snapshot cached[16], cold[16];
    for(unsigned rotating = 0; rotating < 2; ++rotating) {
        sequential_response(false, rotating, cached);
        sequential_response(true, rotating, cold);
        assert(memcmp(cached, cold, sizeof(cached)) == 0);
    }
    stale_contact_identity();
    return 0;
}
