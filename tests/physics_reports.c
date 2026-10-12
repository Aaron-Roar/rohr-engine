/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define OK(value) assert(!rohr_error_check(value))

#ifdef ROHR_TEST_ALLOCATION_WRAP
static bool fail_allocations;
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
void *__real_malloc(size_t);
void *__wrap_realloc(void *ptr, size_t size) {
    return fail_allocations ? NULL : __real_realloc(ptr, size);
}
void *__wrap_calloc(size_t count, size_t size) {
    return fail_allocations ? NULL : __real_calloc(count, size);
}
void *__wrap_malloc(size_t size) {
    return fail_allocations ? NULL : __real_malloc(size);
}
#endif

static uint64_t metric(const Report *report, ReportId owner, ReportMetricId id) {
    for(size_t i = 0; i < report->statistic_count; i++)
        if(report->statistics[i].report_id == owner && report->statistics[i].metric_id == id)
            return report->statistics[i].value;
    return 0;
}
static const ReportEntry *entry(const Report *report, ReportId id) {
    for(size_t i = 0; i < report->entry_count; i++)
        if(report->entries[i].header.id == id) return &report->entries[i];
    return NULL;
}
static Entity body(float x, bool collision) {
    EntityResult result = rohr_entity_add(); OK(result);
    Entity e = result.result.value;
    OK(rohr_physics_position_set(e, (Position){x, 0}));
    OK(rohr_physics_dynamic_set(e));
    OK(rohr_physics_mass_set(e, 1));
    if(collision) {
        OK(rohr_physics_hitbox_set(e, rohr_math_square_create(2, 2)));
        OK(rohr_entity_components_add(e, ROHR_COLLISION));
    }
    return e;
}
static Position position(Entity e) {
    PositionResult result = rohr_physics_position_get(e); OK(result);
    return result.result.value;
}
static Entity joint_make(Entity a, Entity b, bool spring) {
    EntityResult joint = rohr_entity_add(); OK(joint);
    JointAnchorIdResult first = rohr_physics_joint_anchor_create(a, (Vec2D){0}); OK(first);
    JointAnchorIdResult second = rohr_physics_joint_anchor_create(b, (Vec2D){0}); OK(second);
    if(spring) {
        OK(rohr_physics_joint_spring_set(joint.result.value,
            first.result.value, second.result.value, 2, 1, 0));
    } else OK(rohr_physics_joint_pin_set(joint.result.value, first.result.value, second.result.value));
    return joint.result.value;
}

static void no_work_and_arguments(void) {
    OK(rohr_engine_start());
    Entity e = body(0, false);
    OK(rohr_physics_velocity_set(e, (Velocity){2, 0}));
    Report report = rohr_physics_update(0);
    assert(report.header.status == ROHR_REPORT_NOT_RUN && report.entry_count == 0);
    report = rohr_physics_dt_update(0);
    assert(report.header.status == ROHR_REPORT_NOT_RUN);
    report = rohr_physics_dt_update(-1);
    assert(report.header.status == ROHR_REPORT_NOT_RUN);
    report = rohr_physics_pipeline_integrate(NAN);
    assert(report.has_fatal && report.first_fatal.internal_stage == ROHR_PHYSICS_ARGUMENTS);
    report = rohr_system_physics_update(INFINITY);
    assert(report.has_fatal && report.entry_count == 0 && rohr_report_valid_check(&report));
    report = rohr_physics_pipeline_substep(-1);
    assert(report.has_fatal && position(e).x == 0);
    report = rohr_physics_pipeline_constraints_solve(0);
    assert(report.header.status == ROHR_REPORT_NOT_RUN);
    /* Direct pipeline zero-duration calls still gather/solve contacts. */
    report = rohr_system_physics_update(0);
    assert(report.header.status == ROHR_REPORT_COMPLETE && report.entry_count == 6);
    rohr_engine_stop();
}

static void composition_and_joints(void) {
    OK(rohr_engine_start());
    Entity a = body(0, false);
    OK(rohr_physics_velocity_set(a, (Velocity){2, 0}));
    OK(rohr_physics_substeps_set(3));
    Report full = rohr_physics_dt_update(.3);
    assert(rohr_report_valid_check(&full) && fabsf(position(a).x - .6f) < .0001f);
    assert(metric(&full, ROHR_PHYSICS_REPORT, ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT) == 3);
    assert(metric(&full, ROHR_PHYSICS_INTEGRATION_REPORT, ROHR_REPORT_PROCESSED_COUNT) == 3);
    assert(entry(&full, ROHR_PHYSICS_INTEGRATION_REPORT)->invocation_count == 3);
    assert(entry(&full, ROHR_PHYSICS_SUBSTEP_REPORT)->invocation_count == 3);
    OK(rohr_physics_position_set(a, (Position){0}));
    Report combined = rohr_report_create();
    OK(rohr_report_header_set(&combined, (ReportHeader){ROHR_PHYSICS_REPORT, ROHR_REPORT_COMPLETE, 42}));
    rohr_physics_pipeline_step_begin();
    for(unsigned i = 0; i < 3; i++) {
        Report child = physics_pipeline_substep(.1);
        assert(child.header.status == ROHR_REPORT_COMPLETE);
        OK(rohr_report_merge(&combined, &child));
    }
    assert(fabsf(position(a).x - .6f) < .0001f && combined.header.total_ms == 42);
    assert(metric(&combined, ROHR_PHYSICS_INTEGRATION_REPORT, ROHR_REPORT_PROCESSED_COUNT) == 3);
    OK(rohr_physics_position_set(a, (Position){0}));
    OK(rohr_physics_dt_per_tick_set(.1));
    Report tick = rohr_physics_update(3);
    assert(tick.header.status == ROHR_REPORT_COMPLETE && fabsf(position(a).x - .6f) < .0001f);
    Entity b = body(4, false);
    (void)joint_make(a, b, false);
    rohr_physics_pipeline_substep_begin();
    Report joint_report = rohr_physics_pipeline_joints_gather();
    assert(joint_report.header.id == ROHR_PHYSICS_JOINTS_REPORT && joint_report.header.status == ROHR_REPORT_COMPLETE);
    Report solved = rohr_physics_pipeline_constraints_solve(8);
    assert(solved.header.status == ROHR_REPORT_COMPLETE);
    assert(fabsf(position(a).x - position(b).x) < .0001f);
    assert(metric(&joint_report, ROHR_PHYSICS_JOINTS_REPORT, ROHR_REPORT_PROCESSED_COUNT) == 1);
    assert(joint_report.entry_count == 0 && solved.entry_count == 0);
    rohr_engine_stop();
    assert(rohr_report_valid_check(&full) && rohr_report_valid_check(&joint_report));
}

static void warning_and_fatal(void) {
    OK(rohr_engine_start());
    Entity a = body(0, false), b = body(2, false);
    Entity spring = joint_make(a, b, true);
    EntityIndexResult index = rohr_entity_index_get(spring); OK(index);
    /* Fault injection: simulate a runtime-broken joint without setter rejection. */
    joints[index.result.value].a = UINT32_MAX;
    OK(rohr_physics_velocity_set(a, (Velocity){1, 0}));
    Report report = rohr_system_physics_update(.25);
    assert(report.header.status == ROHR_REPORT_COMPLETE && !report.has_fatal);
    assert(report.diagnostic_count == 1 && report.diagnostics[0].severity == ROHR_REPORT_SEVERITY_WARNING);
    assert(report.diagnostics[0].source.id == spring && report.diagnostics[0].has_substep);
    assert(position(a).x == .25f); /* Warning did not prevent unrelated integration. */
    assert(rohr_error_check(rohr_entity_index_get(spring)));
    OK(rohr_physics_position_set(a, (Position){ROHR_WORLD_COORDINATE_MAX, 0}));
    OK(rohr_physics_substeps_set(4));
    report = rohr_physics_dt_update(1);
    assert(report.header.status == ROHR_REPORT_FAILED && report.has_fatal);
    assert(report.first_fatal.source.id == a && report.first_fatal.has_substep);
    assert(report.first_fatal.first_substep == 0 && report.first_fatal.last_substep == 0);
    assert(report.first_fatal.recovery == ROHR_REPORT_ENTITY_QUARANTINED);
    assert(metric(&report, ROHR_PHYSICS_INTEGRATION_REPORT, ROHR_REPORT_QUARANTINED_COUNT) == 1);
    assert(metric(&report, ROHR_PHYSICS_REPORT, ROHR_REPORT_SUBSTEPS_ATTEMPTED_COUNT) == 1);
    assert(metric(&report, ROHR_PHYSICS_REPORT, ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT) == 0);
    assert(entry(&report, ROHR_PHYSICS_CONTACTS_REPORT) == NULL);
    Report saved = report;
    OK(rohr_entity_delete(a));
    (void)body(0, false); /* Reuse may not alter copied evidence. */
    report = rohr_system_physics_update(.1);
    assert(report.header.status == ROHR_REPORT_COMPLETE);
    rohr_engine_stop();
    OK(rohr_engine_start());
    assert(rohr_report_valid_check(&saved) && saved.first_fatal.source.id == a);
    assert(strstr(saved.first_fatal.detail, "quarantined") != NULL);
    rohr_engine_stop();
}

static void diagnostic_overflow(void) {
    OK(rohr_engine_start());
    Entity first = 0;
    for(unsigned i = 0; i < ROHR_REPORT_DIAGNOSTIC_CAPACITY + 4; i++) {
        Entity e = body(ROHR_WORLD_COORDINATE_MAX, false);
        if(i == 0) first = e;
        OK(rohr_physics_velocity_set(e, (Velocity){1, 0}));
    }
    Report report = rohr_system_physics_update(1);
    assert(report.has_fatal && report.first_fatal.source.id == first);
    assert(report.diagnostic_count == ROHR_REPORT_DIAGNOSTIC_CAPACITY);
    assert(report.dropped_diagnostic_occurrences == 4);
    assert(metric(&report, ROHR_PHYSICS_INTEGRATION_REPORT, ROHR_REPORT_QUARANTINED_COUNT)
        == ROHR_REPORT_DIAGNOSTIC_CAPACITY + 4);
    assert(rohr_report_valid_check(&report));
    rohr_engine_stop();
}

static void contact_metrics(void) {
    OK(rohr_engine_start());
    (void)body(0, true); (void)body(1, true);
    Report report = rohr_system_physics_update(0);
    assert(report.header.status == ROHR_REPORT_COMPLETE);
    assert(metric(&report, ROHR_PHYSICS_CONTACTS_REPORT, ROHR_REPORT_COLLIDER_COUNT) == 2);
    assert(metric(&report, ROHR_PHYSICS_CONTACTS_REPORT, ROHR_REPORT_CANDIDATE_COUNT) == 1);
    assert(metric(&report, ROHR_PHYSICS_CONTACTS_REPORT, ROHR_REPORT_NARROWPHASE_TEST_COUNT) == 1);
    assert(metric(&report, ROHR_PHYSICS_CONTACTS_REPORT, ROHR_REPORT_OVERLAP_COUNT) == 1);
    assert(metric(&report, ROHR_PHYSICS_CONSTRAINTS_REPORT, ROHR_REPORT_CONTACT_COUNT) == 1);
    assert(metric(&report, ROHR_PHYSICS_CONTACTS_REPORT, ROHR_REPORT_TREE_NODES_CURRENT) == 3);
    assert(metric(&report, ROHR_PHYSICS_CONTACTS_REPORT, ROHR_REPORT_TREE_HEIGHT_PEAK) == 1);
    rohr_engine_stop();
}

#ifdef ROHR_TEST_ALLOCATION_WRAP
static void allocation_failures(void) {
    OK(rohr_engine_start());
    Entity a = body(0, true), b = body(1, true);
    (void)joint_make(a, b, false);
    fail_allocations = true;
    Report report = rohr_physics_pipeline_contacts_gather();
    fail_allocations = false;
    assert(report.has_fatal && report.first_fatal.internal_stage == ROHR_PHYSICS_BROADPHASE);
    assert(report.first_fatal.source.id == a);
    physics_pipeline_substep_begin();
    report = physics_pipeline_contacts_gather();
    assert(!report.has_fatal);
    contact_constraint_list_destroy(&physics_step_contact_constraints);
    fail_allocations = true;
    report = physics_pipeline_contacts_gather();
    fail_allocations = false;
    assert(report.has_fatal && report.first_fatal.internal_stage == ROHR_PHYSICS_CONTACT_GATHER);
    assert(report.first_fatal.source.id == a && report.first_fatal.target.id == b);
    joint_constraint_list_destroy(&physics_step_joint_constraints);
    fail_allocations = true;
    report = physics_pipeline_joints_gather();
    fail_allocations = false;
    assert(report.has_fatal && report.first_fatal.internal_stage == ROHR_PHYSICS_JOINT_GATHER);
    rohr_engine_stop();

    OK(rohr_engine_start());
    EntityResult soft = rohr_physics_soft_body_create(); OK(soft);
    EntityResult left = rohr_physics_soft_body_node_create(soft.result.value, (Position){-10, 0}, 1, 1); OK(left);
    EntityResult right = rohr_physics_soft_body_node_create(soft.result.value, (Position){10, 0}, 1, 1); OK(right);
    OK(rohr_physics_static_set(left.result.value));
    OK(rohr_physics_static_set(right.result.value));
    EntityResult beam = rohr_physics_soft_body_beam_create(soft.result.value,
        left.result.value, right.result.value, 0, 0); OK(beam);
    OK(rohr_physics_soft_body_beam_collision_thickness_set(beam.result.value, .1f));
    ParticleConfig config = rohr_physics_particle_config_default_get();
    config.position = (Position){0, .5f}; config.radius = 1; config.gravity_enabled = false;
    EntityResult particle = rohr_physics_particle_create(config); OK(particle);
    physics_pipeline_substep_begin();
    report = physics_pipeline_contacts_gather();
    assert(!report.has_fatal && physics_step_contact_constraints.count > 0);
    contact_constraint_list_destroy(&physics_step_contact_constraints);
    fail_allocations = true;
    report = physics_pipeline_contacts_gather();
    fail_allocations = false;
    assert(report.has_fatal && report.first_fatal.internal_stage == ROHR_PHYSICS_CONTACT_GATHER);
    assert(report.first_fatal.source.id == beam.result.value);
    assert(report.first_fatal.target.id == particle.result.value);
    rohr_engine_stop();

    OK(rohr_engine_start());
    for(int i = 0; i < 14; i++) {
        Entity e = body(0, true);
        OK(rohr_physics_static_set(e));
    }
    physics_pipeline_step_begin();
    physics_pipeline_substep_begin();
    report = physics_pipeline_contacts_gather();
    assert(!report.has_fatal);
    fail_allocations = true;
    report = physics_pipeline_constraints_solve(1);
    fail_allocations = false;
    assert(report.has_fatal && report.first_fatal.internal_stage == ROHR_PHYSICS_FINALIZATION);
    assert(report.first_fatal.code == ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    assert(rohr_report_valid_check(&report));
    rohr_engine_stop();
}
#endif

int main(void) {
    no_work_and_arguments();
    composition_and_joints();
    warning_and_fatal();
    contact_metrics();
    diagnostic_overflow();
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_failures();
#endif
    OK(rohr_engine_start());
    for(int i = 0; i < 100; i++) (void)body((float)i, false);
    clock_t started = clock();
    for(int i = 0; i < 100; i++) {
        Report report = rohr_physics_dt_update(.001);
        assert(report.header.status == ROHR_REPORT_COMPLETE);
    }
    printf("100 physics updates / 100 bodies: %.3f ms CPU (informational)\n",
        1000.0 * (double)(clock() - started) / CLOCKS_PER_SEC);
    rohr_engine_stop();
    return 0;
}
