/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "runtime_fixture.h"
#ifndef ROHR_INCOMPLETE_JOINT_CHECKS_H
#define ROHR_INCOMPLETE_JOINT_CHECKS_H
#include "rohr.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>

#define OK(call) assert(!rohr_error_check(call))
static const JointType joint_types[] = {JOINT_PIN, JOINT_WELD, JOINT_SPRING};
static const char *joint_names[] = {"pin", "weld", "spring"};
typedef struct DraftJoints { Entity handles[3][4]; } DraftJoints;

static Entity draft_member(Entity root, const char *name) {
    EntityResult result = fixture_entity_get(root, name); OK(result);
    return result.result.value;
}
static DraftJoints draft_members(Entity root) {
    DraftJoints value = {0};
    for(unsigned kind = 0; kind < 3; ++kind) for(unsigned endpoints = 0; endpoints < 4; ++endpoints) {
        char name[32]; snprintf(name, sizeof(name), "%s_%u", joint_names[kind], endpoints);
        value.handles[kind][endpoints] = draft_member(root, name);
    }
    return value;
}
static void draft_checks(DraftJoints value, Entity owner) {
    for(unsigned kind = 0; kind < 3; ++kind) for(unsigned endpoints = 0; endpoints < 4; ++endpoints) {
        Entity entity = value.handles[kind][endpoints];
        assert(rohr_entity_alive_check(entity));
        JointDescriptionResult got = rohr_physics_joint_description_get(entity); OK(got);
        JointDescription d = got.result.value;
        assert(d.owner == owner && d.type == joint_types[kind]);
        assert((d.anchor_a != 0) == ((endpoints & 1) != 0));
        assert((d.anchor_b != 0) == ((endpoints & 2) != 0));
        assert(d.stiffness == 7 && d.damping == 2 && d.rest_length == 3 && d.rest_angle == 15);
        if(d.anchor_a) OK(rohr_physics_joint_anchor_description_get(d.anchor_a));
        if(d.anchor_b) OK(rohr_physics_joint_anchor_description_get(d.anchor_b));
    }
    Report report = rohr_system_physics_update(1.0 / 120.0);
    assert(!report.has_fatal && report.header.status == ROHR_REPORT_COMPLETE);
    for(unsigned kind = 0; kind < 3; ++kind) for(unsigned endpoints = 0; endpoints < 3; ++endpoints) {
        bool found = false;
        for(size_t i = 0; i < report.diagnostic_count; ++i)
            if(report.diagnostics[i].source.id == value.handles[kind][endpoints] &&
                    report.diagnostics[i].recovery == ROHR_REPORT_CONTRIBUTION_SKIPPED &&
                    report.diagnostics[i].severity == ROHR_REPORT_SEVERITY_WARNING) found = true;
        assert(found);
    }
    for(unsigned kind = 0; kind < 3; ++kind) {
        JointDescriptionResult complete = rohr_physics_joint_description_get(value.handles[kind][3]); OK(complete);
        for(unsigned endpoints = 0; endpoints < 3; ++endpoints) {
            Entity entity = value.handles[kind][endpoints];
            OK(rohr_physics_joint_description_set(entity, complete.result.value));
            assert(rohr_entity_alive_check(entity));
            JointDescriptionResult got = rohr_physics_joint_description_get(entity); OK(got);
            assert(got.result.value.anchor_a == complete.result.value.anchor_a &&
                got.result.value.anchor_b == complete.result.value.anchor_b);
        }
    }
    report = rohr_system_physics_update(1.0 / 120.0);
    assert(!report.has_fatal && report.diagnostic_count == 0);
}
#endif
