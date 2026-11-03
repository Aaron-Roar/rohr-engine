/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include <assert.h>
#include <math.h>

#define OK(value) assert(!rohr_error_check(value))

static Entity body(float x) {
    EntityResult result = rohr_entity_add(); OK(result);
    Entity entity = result.result.value;
    OK(rohr_physics_position_set(entity, (Position){x, 0}));
    OK(rohr_physics_dynamic_set(entity));
    OK(rohr_physics_mass_set(entity, 1));
    return entity;
}

static JointAnchorId anchor(Entity entity) {
    JointAnchorIdResult result = rohr_physics_joint_anchor_add(
        (JointAnchorDescription){.entity = entity}); OK(result);
    return result.result.value;
}

static JointDescription description(JointType type, JointAnchorId a, JointAnchorId b) {
    return (JointDescription){.type = type, .anchor_a = a, .anchor_b = b,
        .rest_length_explicit = true, .rest_angle_explicit = true};
}

static Entity joint(JointType type, JointAnchorId a, JointAnchorId b) {
    EntityResult result = rohr_physics_joint_add(description(type, a, b)); OK(result);
    return result.result.value;
}

static void gather(void) {
    Report report = rohr_physics_pipeline_joints_gather();
    assert(!report.has_fatal && rohr_report_valid_check(&report));
}

static Report solve(void) {
    Report report = physics_report_create(ROHR_PHYSICS_CONSTRAINTS_REPORT_ID);
    physics_joint_constraints_solve(&report);
    assert(!report.has_fatal);
    return report;
}

static float x_get(Entity entity) {
    PositionResult result = rohr_physics_position_get(entity); OK(result);
    return result.result.value.x;
}

static void reused_joint_slot(void) {
    OK(rohr_engine_start());
    Entity a = body(0), b = body(10);
    JointAnchorId aa = anchor(a), bb = anchor(b);
    Entity old = joint(JOINT_PIN, aa, bb);
    EntityIndex index = rohr_entity_index_get(old).result.value;
    gather();
    OK(rohr_entity_delete(old));
    Entity replacement = joint(JOINT_WELD, aa, bb);
    assert(replacement != old && rohr_entity_index_get(replacement).result.value == index);
    (void)solve();
    assert(x_get(a) == 0 && x_get(b) == 10);
    gather();
    (void)solve();
    assert(fabsf(x_get(a) - x_get(b)) < .00001f);
    rohr_engine_stop();
}

static void current_motion_and_definition(void) {
    OK(rohr_engine_start());
    Entity a = body(0), b = body(10), c = body(20);
    JointAnchorId aa = anchor(a), bb = anchor(b);
    Entity pin = joint(JOINT_PIN, aa, bb);
    gather();
    /* Retained identity does not freeze an anchor's body, offset or pose. */
    OK(rohr_physics_joint_anchor_description_set(aa,
        (JointAnchorDescription){.entity = c, .local_offset = {2, 0}}));
    OK(rohr_physics_position_set(b, (Position){14, 0}));
    (void)solve();
    assert(x_get(a) == 0 && fabsf(x_get(c) + 2 - x_get(b)) < .00001f);
    OK(rohr_physics_joint_description_set(pin, description(JOINT_SPRING, aa, bb)));
    OK(rohr_physics_position_set(b, (Position){30, 0}));
    float c_before = x_get(c);
    (void)solve();
    assert(x_get(b) == 30 && x_get(c) == c_before);
    gather(); /* Springs never enter the pin/weld list. */
    assert(physics_step_joint_constraints.count == 0);
    OK(rohr_physics_joint_description_set(pin, description(JOINT_WELD, aa, bb)));
    (void)solve();
    assert(x_get(b) == 30 && x_get(c) == c_before);
    gather();
    assert(physics_step_joint_constraints.count == 1);
    (void)solve();
    assert(fabsf(x_get(c) + 2 - x_get(b)) < .00001f);
    rohr_engine_stop();
}

static void disconnected_anchor(void) {
    OK(rohr_engine_start());
    Entity a = body(0), b = body(10);
    JointAnchorId aa = anchor(a), bb = anchor(b);
    Entity pin = joint(JOINT_PIN, aa, bb);
    gather();
    OK(rohr_physics_joint_anchor_remove(aa));
    JointAnchorId replacement = anchor(a);
    assert(aa != replacement);
    Report report = solve();
    assert(report.diagnostic_count == 1 && report.diagnostics[0].source.id == pin);
    assert(x_get(a) == 0 && x_get(b) == 10);
    OK(rohr_physics_joint_description_set(pin, description(JOINT_PIN, replacement, bb)));
    (void)solve();
    assert(fabsf(x_get(a) - x_get(b)) < .00001f);
    OK(rohr_entity_delete(b));
    Entity other = body(50);
    float before = x_get(a);
    report = solve();
    assert(report.diagnostic_count == 1 && x_get(a) == before && x_get(other) == 50);
    rohr_engine_stop();
}

static void gathering_replaces_membership(void) {
    OK(rohr_engine_start());
    Entity pin = joint(JOINT_PIN, 0, 0);
    Entity weld = joint(JOINT_WELD, 0, 0);
    (void)joint(JOINT_SPRING, 0, 0);
    gather();
    assert(physics_step_joint_constraints.count == 2);
    gather();
    assert(physics_step_joint_constraints.count == 2);
    Report report = solve();
    assert(report.diagnostic_count == 2);
    assert(report.diagnostics[0].source.id == pin && report.diagnostics[1].source.id == weld);
    OK(rohr_entity_delete(pin));
    gather();
    assert(physics_step_joint_constraints.count == 1);
    report = solve();
    assert(report.diagnostic_count == 1 && report.diagnostics[0].source.id == weld);
    rohr_engine_stop();
}

int main(void) {
    reused_joint_slot();
    current_motion_and_definition();
    disconnected_anchor();
    gathering_replaces_membership();
    return 0;
}
