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
    JointAnchorIdResult first = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = a, .local_offset = (Vec2D){0}}); OK(first);
    JointAnchorIdResult second = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = b, .local_offset = (Vec2D){0}}); OK(second);
    if(spring) {
        OK(rohr_physics_joint_description_set(joint.result.value, (JointDescription){.type = JOINT_SPRING, .anchor_a = first.result.value, .anchor_b = second.result.value, .rest_length = 2, .rest_length_explicit = true, .stiffness = 1, .damping = 0}));
    } else OK(rohr_physics_joint_description_set(joint.result.value, (JointDescription){.type = JOINT_PIN, .anchor_a = first.result.value, .anchor_b = second.result.value}));
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
    assert(metric(&full, ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT) == 3);
    assert(metric(&full, ROHR_PHYSICS_INTEGRATION_REPORT_ID, ROHR_REPORT_PROCESSED_COUNT) == 3);
    assert(entry(&full, ROHR_PHYSICS_INTEGRATION_REPORT_ID)->invocation_count == 3);
    assert(entry(&full, ROHR_PHYSICS_SUBSTEP_REPORT_ID)->invocation_count == 3);
    OK(rohr_physics_position_set(a, (Position){0}));
    Report combined = rohr_report_create();
    OK(rohr_report_header_set(&combined, (ReportHeader){ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_COMPLETE, 42}));
    rohr_physics_pipeline_step_begin();
    for(unsigned i = 0; i < 3; i++) {
        Report child = physics_pipeline_substep(.1);
        assert(child.header.status == ROHR_REPORT_COMPLETE);
        OK(rohr_report_merge(&combined, &child));
    }
    assert(fabsf(position(a).x - .6f) < .0001f && combined.header.total_ms == 42);
    assert(metric(&combined, ROHR_PHYSICS_INTEGRATION_REPORT_ID, ROHR_REPORT_PROCESSED_COUNT) == 3);
    OK(rohr_physics_position_set(a, (Position){0}));
    OK(rohr_physics_dt_per_tick_set(.1));
    Report tick = rohr_physics_update(3);
    assert(tick.header.status == ROHR_REPORT_COMPLETE && fabsf(position(a).x - .6f) < .0001f);
    assert(rohr_report_valid_check(&tick));
    assert(metric(&tick, ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT) == 9);
    assert(entry(&tick, ROHR_PHYSICS_INTEGRATION_REPORT_ID)->invocation_count == 9);
    ReportResult latest = rohr_report_get(ROHR_PHYSICS_REPORT); OK(latest);
    assert(memcmp(&latest.result.value, &tick, sizeof(tick)) == 0);
    Entity b = body(4, false);
    (void)joint_make(a, b, false);
    Entity unrelated_first = body(100, true), unrelated_second = body(101, true);
    rohr_physics_pipeline_substep_begin();
    Report joint_report = rohr_physics_pipeline_joints_gather();
    assert(joint_report.header.id == ROHR_PHYSICS_JOINTS_REPORT_ID && joint_report.header.status == ROHR_REPORT_COMPLETE);
    Report solved = rohr_physics_pipeline_constraints_solve(8);
    assert(solved.header.status == ROHR_REPORT_COMPLETE);
    assert(fabsf(position(a).x - position(b).x) < .0001f);
    assert(metric(&joint_report, ROHR_PHYSICS_JOINTS_REPORT_ID, ROHR_REPORT_PROCESSED_COUNT) == 1);
    assert(joint_report.entry_count == 0 && solved.entry_count == 0);
    assert(position(unrelated_first).x == 100 && position(unrelated_second).x == 101);
    assert(rohr_physics_contact_count_get(unrelated_first) == 0);
    rohr_engine_stop();
    assert(rohr_report_valid_check(&full) && rohr_report_valid_check(&joint_report));
}

static void delayed_frame_soft_body(void) {
    Position expected[2];
    for(unsigned batched = 0; batched < 2; batched++) {
        OK(rohr_engine_start());
        OK(rohr_physics_dt_per_tick_set(1.0 / 120.0));
        EntityResult soft = rohr_physics_soft_body_add((SoftBodyDescription){0}); OK(soft);
        Entity nodes[2];
        for(unsigned i = 0; i < 2; i++) {
            EntityResult node = rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
                .owner = soft.result.value, .position = {20.0f * i, 0}, .mass = 1, .radius = 1});
            OK(node); nodes[i] = node.result.value;
        }
        OK(rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
            .owner = soft.result.value, .node_a = nodes[0], .node_b = nodes[1],
            .stiffness = 800, .damping = 15}));
        OK(rohr_physics_velocity_set(nodes[1], (Velocity){10, 0}));
        /* Two seconds without rendering, as when a compositor suspends frames. */
        for(unsigned i = 0; i < (batched ? 1u : 240u); i++) {
            Report report = rohr_physics_update(batched ? 240 : 1);
            assert(report.header.status == ROHR_REPORT_COMPLETE && !report.has_fatal);
            assert(report.diagnostic_count == 0 && rohr_report_valid_check(&report));
        }
        for(unsigned i = 0; i < 2; i++) {
            Position actual = position(nodes[i]);
            assert(isfinite(actual.x) && isfinite(actual.y) && fabsf(actual.x) < 100);
            if(batched) {
                assert(fabsf(actual.x - expected[i].x) < .0001f);
                assert(fabsf(actual.y - expected[i].y) < .0001f);
            } else expected[i] = actual;
        }
        assert(fabsf(position(nodes[1]).x - position(nodes[0]).x - 20) < .1f);
        rohr_engine_stop();
    }
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
    OK(rohr_entity_index_get(spring));
    joints[index.result.value].a = a; /* Developer repairs the retained definition. */
    OK(rohr_entity_delete(spring));
    OK(rohr_physics_position_set(a, (Position){ROHR_WORLD_COORDINATE_MAX, 0}));
    OK(rohr_physics_substeps_set(4));
    report = rohr_physics_dt_update(1);
    assert(report.header.status == ROHR_REPORT_COMPLETE && !report.has_fatal);
    assert(report.diagnostics[0].source.id == a && report.diagnostics[0].has_substep);
    assert(report.diagnostics[0].first_substep == 0 && report.diagnostics[0].last_substep == 3);
    assert(report.diagnostics[0].occurrence_count == 4);
    assert(report.diagnostics[0].recovery == ROHR_REPORT_CONTRIBUTION_SKIPPED);
    assert(metric(&report, ROHR_PHYSICS_INTEGRATION_REPORT_ID, ROHR_REPORT_SKIPPED_COUNT) == 4);
    assert(metric(&report, ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_SUBSTEPS_ATTEMPTED_COUNT) == 4);
    assert(metric(&report, ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT) == 4);
    assert(entry(&report, ROHR_PHYSICS_CONTACTS_REPORT_ID) != NULL);
    Report saved = report;
    OK(rohr_entity_delete(a));
    (void)body(0, false); /* Reuse may not alter copied evidence. */
    report = rohr_system_physics_update(.1);
    assert(report.header.status == ROHR_REPORT_COMPLETE);
    rohr_engine_stop();
    OK(rohr_engine_start());
    assert(rohr_report_valid_check(&saved) && saved.diagnostics[0].source.id == a);
    assert(strstr(saved.diagnostics[0].detail, "retained") != NULL);
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
    assert(!report.has_fatal && report.diagnostics[0].source.id == first);
    assert(report.diagnostic_count == ROHR_REPORT_DIAGNOSTIC_CAPACITY);
    assert(report.dropped_diagnostic_occurrences == 4);
    assert(metric(&report, ROHR_PHYSICS_INTEGRATION_REPORT_ID, ROHR_REPORT_SKIPPED_COUNT)
        == ROHR_REPORT_DIAGNOSTIC_CAPACITY + 4);
    assert(rohr_report_valid_check(&report));
    rohr_engine_stop();
}

static void contact_metrics(void) {
    OK(rohr_engine_start());
    (void)body(0, true); (void)body(1, true);
    Report report = rohr_system_physics_update(0);
    assert(report.header.status == ROHR_REPORT_COMPLETE);
    assert(metric(&report, ROHR_PHYSICS_CONTACTS_REPORT_ID, ROHR_REPORT_COLLIDER_COUNT) == 2);
    assert(metric(&report, ROHR_PHYSICS_CONTACTS_REPORT_ID, ROHR_REPORT_CANDIDATE_COUNT) == 1);
    assert(metric(&report, ROHR_PHYSICS_CONTACTS_REPORT_ID, ROHR_REPORT_NARROWPHASE_TEST_COUNT) == 1);
    assert(metric(&report, ROHR_PHYSICS_CONTACTS_REPORT_ID, ROHR_REPORT_OVERLAP_COUNT) == 1);
    assert(metric(&report, ROHR_PHYSICS_CONSTRAINTS_REPORT_ID, ROHR_REPORT_CONTACT_COUNT) == 1);
    assert(metric(&report, ROHR_PHYSICS_CONTACTS_REPORT_ID, ROHR_REPORT_TREE_NODES_CURRENT) == 3);
    assert(metric(&report, ROHR_PHYSICS_CONTACTS_REPORT_ID, ROHR_REPORT_TREE_HEIGHT_PEAK) == 1);
    rohr_engine_stop();
}

static void contact_cache_invalidation(void) {
    OK(rohr_engine_start());
    Entity first = body(0, true), moving = body(1.5f, true), second = body(10, true);
    OK(rohr_physics_static_set(first));
    OK(rohr_physics_static_set(second));
    physics_pipeline_substep_begin();
    Report report = rohr_physics_pipeline_contacts_gather();
    assert(!report.has_fatal);
    report = rohr_physics_pipeline_constraints_solve(8);
    assert(!report.has_fatal && position(moving).x > 1.999f);

    /* Repeated manual solves must discover a newly touching neighbor after a
     * public transform edit, even without another explicit gather call. */
    OK(rohr_physics_position_set(moving, (Position){9.25f, 0}));
    report = rohr_physics_pipeline_constraints_solve(8);
    assert(!report.has_fatal && position(moving).x < 8.001f);

    /* Shape edits can change contact geometry without changing the pose. */
    OK(rohr_physics_hitbox_set(moving, rohr_math_square_create(4, 2)));
    report = rohr_physics_pipeline_constraints_solve(8);
    assert(!report.has_fatal && position(moving).x < 7.001f);
    assert(position(first).x == 0 && position(second).x == 10);

    EntityIndexResult index = rohr_entity_index_get(moving); OK(index);
    OK(rohr_physics_velocity_set(moving, (Velocity){1, 0}));
    report = rohr_physics_pipeline_constraints_solve(8);
    assert(!report.has_fatal && fabsf(velocities[index.result.value].x) < .0001f);
    OK(rohr_physics_velocity_set(moving, (Velocity){-3, 0}));
    report = rohr_physics_pipeline_constraints_solve(8);
    assert(!report.has_fatal && velocities[index.result.value].x == -3);
    rohr_engine_stop();
}

#ifdef ROHR_TEST_ALLOCATION_WRAP
static void aborted_update_state(bool batched) {
    OK(rohr_engine_start());
    Entity a = body(0, true), b = body(1, true), healthy = body(100, false);
    OK(rohr_physics_velocity_set(healthy, (Velocity){2, 0}));
    OK(rohr_physics_substeps_set(3));
    OK(rohr_physics_dt_per_tick_set(3));
    EntityInfoFlags fields = ROHR_ENTITY_INFO_PHYSICS_POSITION | ROHR_ENTITY_INFO_PHYSICS_CONTACTS;
    EntityInfoResult before = rohr_entity_info_get(healthy, fields); OK(before);
    fail_allocations = true;
    Report failed = batched ? rohr_physics_update(3) : rohr_physics_dt_update(3);
    fail_allocations = false;
    assert(failed.has_fatal && failed.header.status == ROHR_REPORT_FAILED);
    assert(failed.first_fatal.report_id == ROHR_PHYSICS_CONTACTS_REPORT_ID);
    assert(failed.first_fatal.internal_stage == ROHR_PHYSICS_BROADPHASE);
    assert(failed.first_fatal.code == ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    assert(failed.first_fatal.recovery == ROHR_REPORT_UPDATE_ABORTED);
    assert(failed.first_fatal.occurrence_count == 1 && failed.first_fatal.has_substep);
    assert(failed.first_fatal.first_substep == 0 && failed.first_fatal.last_substep == 0);
    assert(metric(&failed, ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_SUBSTEPS_ATTEMPTED_COUNT) == 1);
    assert(metric(&failed, ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT) == 0);
    assert(entry(&failed, ROHR_PHYSICS_JOINTS_REPORT_ID) == NULL);
    assert(entry(&failed, ROHR_PHYSICS_CONSTRAINTS_REPORT_ID) == NULL);
    /* A fatal gather aborts later work; it does not undo completed integration. */
    assert(position(healthy).x == 102 && position(a).x == 0 && position(b).x == 1);
    EntityInfoResult after = entity_info_get(healthy, fields); OK(after);
    assert(after.result.value.position.x == 102 && before.result.value.position.x == 100);
    assert(after.result.value.contact_count == 0 && physics_contact_count_get(a) == 0);
    EntityInfoIssueCountResult issues = entity_info_issues_count_get(&after.result.value, fields); OK(issues);
    assert(issues.result.value == 0); /* Valid state can accompany an execution failure. */
    ReportResult stored = report_get(ROHR_PHYSICS_REPORT); OK(stored);
    assert(memcmp(&stored.result.value, &failed, sizeof(failed)) == 0);
    Report recovered = physics_dt_update(3);
    assert(!recovered.has_fatal && recovered.diagnostic_count == 0);
    assert(metric(&recovered, ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT) == 3);
    assert(position(healthy).x == 108 && after.result.value.position.x == 102);
    assert(memcmp(&stored.result.value, &failed, sizeof(failed)) == 0);
    rohr_engine_stop(); OK(rohr_engine_start());
    assert(before.result.value.position.x == 100 && after.result.value.position.x == 102);
    assert(report_valid_check(&failed) && failed.first_fatal.source.id == a);
    OK(entity_info_release(&before.result.value)); OK(entity_info_release(&after.result.value));
    rohr_engine_stop();
}

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
    /* With value storage already reserved, fail only the new pair-index
     * allocation. No constraint may be published before both stores exist. */
    contact_constraint_list_destroy(&physics_step_contact_constraints);
    OK(contact_constraint_list_init(&physics_step_contact_constraints, 1));
    fail_allocations = true;
    report = physics_pipeline_contacts_gather();
    fail_allocations = false;
    assert(report.has_fatal && report.first_fatal.internal_stage == ROHR_PHYSICS_CONTACT_GATHER);
    assert(physics_step_contact_constraints.count == 0 &&
        !contact_constraint_list_rigid_pair_check(&physics_step_contact_constraints, a, b));
    report = physics_pipeline_contacts_gather();
    assert(!report.has_fatal && physics_step_contact_constraints.count == 1 &&
        contact_constraint_list_rigid_pair_check(&physics_step_contact_constraints, a, b));
    joint_constraint_list_destroy(&physics_step_joint_constraints);
    fail_allocations = true;
    report = physics_pipeline_joints_gather();
    fail_allocations = false;
    assert(report.has_fatal && report.first_fatal.internal_stage == ROHR_PHYSICS_JOINT_GATHER);
    rohr_engine_stop();

    OK(rohr_engine_start());
    EntityResult soft = rohr_physics_soft_body_add((SoftBodyDescription){0}); OK(soft);
    EntityResult left = rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = soft.result.value,
        .position = (Position){-10, 0},
        .mass = 1,
        .radius = 1
    }); OK(left);
    EntityResult right = rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = soft.result.value,
        .position = (Position){10, 0},
        .mass = 1,
        .radius = 1
    }); OK(right);
    OK(rohr_physics_static_set(left.result.value));
    OK(rohr_physics_static_set(right.result.value));
    EntityResult beam = rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.enabled = true, .thickness = 2,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = soft.result.value,
        .node_a = left.result.value,
        .node_b = right.result.value,
        .stiffness = 0,
        .damping = 0
    }); OK(beam);
    OK(rohr_physics_soft_body_beam_collision_thickness_set(beam.result.value, .1f));
    ParticleDescription config = rohr_physics_particle_description_default_get();
    config.position = (Position){0, .5f}; config.radius = 1; config.gravity_enabled = false;
    EntityResult particle = rohr_physics_particle_add(config); OK(particle);
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
    delayed_frame_soft_body();
    warning_and_fatal();
    contact_metrics();
    contact_cache_invalidation();
    diagnostic_overflow();
#ifdef ROHR_TEST_ALLOCATION_WRAP
    aborted_update_state(false);
    aborted_update_state(true);
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
