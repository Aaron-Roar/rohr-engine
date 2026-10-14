/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "entity_components.h"
#include "core/engine_internal.h"
#include "core/report_internal.h"
#include "systems.h"
#include "console.h"
#include "physics/broadphase/aabb_tree.h"
#include "physics/collision/contact_constraint.h"
#include "physics/constraints/constraint_solver.h"
#include "physics/collision/contact_manifold.h"
#include "physics/joints/joint_constraint.h"
#include "physics/soft_body/soft_body.h"
#include "physics/physics_step_internal.h"
#include "math2d.h"
#include "engine.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <time.h>

Shape system_generate_global_hitbox(Entity entity);

AABBTree physics_broadphase_tree = {.root = AABB_TREE_NODE_INVALID};
ContactConstraintList physics_step_contact_constraints;
JointConstraintList physics_step_joint_constraints;
static bool system_hitbox_dirty[MAX_ENTITIES];
static EntityIndex system_hitbox_dirty_entities[MAX_ENTITIES];
static size_t system_hitbox_dirty_count;

double physics_step_elapsed_ms(uint64_t start) {
    return (double)(SDL_GetPerformanceCounter() - start) * 1000.0 /
        (double)SDL_GetPerformanceFrequency();
}

Report physics_report_create(ReportId id) {
    Report report = report_create();
    report.header = (ReportHeader){id, ROHR_REPORT_COMPLETE, 0};
    return report;
}

void physics_report_counter_add(Report *report, ReportMetricId metric, uint64_t value) {
    if(report == NULL) return;
    ReportAggregation rule = metric == ROHR_REPORT_TREE_HEIGHT_PEAK
        ? ROHR_REPORT_AGGREGATION_MAX : metric == ROHR_REPORT_TREE_NODES_CURRENT
        ? ROHR_REPORT_AGGREGATION_LATEST : ROHR_REPORT_AGGREGATION_SUM;
    (void)report_statistic_add(report, (ReportStatistic){.report_id = report->header.id, .metric_id = metric, .aggregation = rule, .value = value});
}

void physics_report_issue_add(Report *report, uint32_t stage, ReportSeverity severity,
        EngineError code, ReportRecovery recovery, Entity source, Entity target, const char *detail) {
    if(report == NULL) return;
    ReportDiagnostic diagnostic = {
        .report_id = report->header.id, .internal_stage = stage,
        .severity = severity, .code = code, .recovery = recovery,
        .source = {source ? ROHR_REPORT_OBJECT_ENTITY : ROHR_REPORT_OBJECT_NONE, source},
        .target = {target ? ROHR_REPORT_OBJECT_ENTITY : ROHR_REPORT_OBJECT_NONE, target},
        .occurrence_count = 1
    };
    (void)report_diagnostic_detail_set(&diagnostic, detail ? detail : error_code_message_get(code));
    (void)report_diagnostic_add(report, diagnostic);
}

static bool physics_report_context_active;
static uint32_t physics_report_context_substep;
void physics_report_context_set(bool active, uint32_t substep) {
    physics_report_context_active = active;
    physics_report_context_substep = substep;
}

void physics_report_finish(Report *report, uint64_t started) {
    report->header.total_ms = physics_step_elapsed_ms(started);
    if(physics_report_context_active)
        physics_report_substep_set(report, physics_report_context_substep);
    report_publish_internal(report);
}

void physics_report_substep_set(Report *report, uint32_t substep) {
    for(size_t i = 0; i < report->diagnostic_count; i += 1) {
        report->diagnostics[i].has_substep = true;
        report->diagnostics[i].first_substep = substep;
        report->diagnostics[i].last_substep = substep;
    }
    if(report->has_fatal) {
        report->first_fatal.has_substep = true;
        report->first_fatal.first_substep = substep;
        report->first_fatal.last_substep = substep;
    }
}

EngineResult physics_broadphase_init(void) {
    EngineResult result = aabb_tree_init(&physics_broadphase_tree, 0);

    if(result.kind == ERROR_RESULT_ERROR) return result;
    result = contact_constraint_list_init(&physics_step_contact_constraints, 64);
    if(result.kind == ERROR_RESULT_ERROR) {
        aabb_tree_destroy(&physics_broadphase_tree);
        return result;
    }
    result = joint_constraint_list_init(&physics_step_joint_constraints, 16);
    if(result.kind == ERROR_RESULT_ERROR) {
        contact_constraint_list_destroy(&physics_step_contact_constraints);
        aabb_tree_destroy(&physics_broadphase_tree);
        return result;
    }
    return result;
}

void physics_broadphase_destroy(void) {
    aabb_tree_destroy(&physics_broadphase_tree);
    contact_constraint_list_destroy(&physics_step_contact_constraints);
    joint_constraint_list_destroy(&physics_step_joint_constraints);
}

bool physics_step_entity_from_index_get(EntityIndex index, Entity *entity) {
    EntityResult result = entity_from_index_get(index);

    if(entity == NULL || result.kind == ERROR_RESULT_ERROR) {
        return false;
    }
    *entity = result.result.value;
    return true;
}

bool physics_step_alive_index_at(uint32_t alive_position, EntityIndex *index) {
    EntityResult result;

    if(index == NULL) {
        return false;
    }
    result = entity_alive_at_get(alive_position);
    if(result.kind == ERROR_RESULT_ERROR) {
        return false;
    }
    return entity_index_get(result.result.value, index) && entity_index_alive_check(*index);
}

void physics_step_interaction_by_index_record(
    EntityIndex entity_1,
    EntityIndex entity_2,
    OverlapInfo overlap,
    ContactInfo contact,
    PhysicsInteractionFlags flags, Report *report
) {
    Entity entity_1_id;
    Entity entity_2_id;

    if(!physics_step_entity_from_index_get(entity_1, &entity_1_id) ||
            !physics_step_entity_from_index_get(entity_2, &entity_2_id)) return;
    EngineResult result = physics_interaction_record(
        entity_1_id, entity_2_id, overlap, contact, flags
    );
    if(result.kind == ERROR_RESULT_ERROR)
        physics_report_issue_add(report, ROHR_PHYSICS_FINALIZATION, ROHR_REPORT_SEVERITY_FATAL,
            result.result.error, ROHR_REPORT_UPDATE_ABORTED, entity_1_id, entity_2_id,
            "could not publish physics interaction");
}

static void system_generate_global_hitbox_by_index(EntityIndex index) {
    Entity entity;

    if(!physics_step_entity_from_index_get(index, &entity)) {
        return;
    }
    system_generate_global_hitbox(entity);
}

void physics_step_hitbox_dirty_add(EntityIndex index) {
    if(index >= MAX_ENTITIES || system_hitbox_dirty[index]) return;
    system_hitbox_dirty[index] = true;
    system_hitbox_dirty_entities[system_hitbox_dirty_count++] = index;
}

void physics_step_hitbox_dirty_flush(void) {
    for(size_t i = 0; i < system_hitbox_dirty_count; i += 1) {
        EntityIndex index = system_hitbox_dirty_entities[i];

        system_generate_global_hitbox_by_index(index);
        system_hitbox_dirty[index] = false;
    }
    system_hitbox_dirty_count = 0;
}

void physics_step_entity_by_index_delete(EntityIndex index) {
    Entity entity;

    if(!physics_step_entity_from_index_get(index, &entity)) {
        return;
    }
    entity_delete(entity);
}

void physics_step_transform_lock_by_index_remove(EntityIndex index) {
    Entity entity;

    if(!physics_step_entity_from_index_get(index, &entity)) {
        return;
    }
    physics_transform_lock_remove(entity);
}

void physics_pipeline_contact_constraints_solve(
    ContactConstraintList *constraints,
    float position_fraction,
    void *context
) {
    for(size_t i = 0; i < constraints->count; i += 1) {
        if(context != NULL && ((Report *)context)->has_fatal) break;
        SystemContactConstraint *constraint = &constraints->values[i];

        if(constraint->type == SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR) {
            physics_rigid_contact_constraint_solve(constraint, position_fraction, context);
        } else if(constraint->type == SYSTEM_CONTACT_CONSTRAINT_SOFT_BOUNDARY) {
            physics_soft_body_constraint_solve(constraint, position_fraction, context);
        }
    }
}

void physics_pipeline_contact_constraints_finalize(
    ContactConstraintList *constraints,
    void *context
) {
    for(size_t i = 0; i < constraints->count; i += 1) {
        if(context != NULL && ((Report *)context)->has_fatal) break;
        SystemContactConstraint *constraint = &constraints->values[i];

        if(constraint->type == SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR) {
            physics_rigid_contact_constraint_finalize(constraint, context);
        } else if(constraint->type == SYSTEM_CONTACT_CONSTRAINT_SOFT_BOUNDARY) {
            physics_soft_body_constraint_finalize(constraint, context);
        }
    }
}

void physics_pipeline_joint_constraints_solve(void *context) {
    if(context != NULL && ((Report *)context)->has_fatal) return;
    physics_joint_constraints_solve(context);
    physics_rigid_transform_constraints_apply();
    physics_step_hitbox_dirty_flush();
}

Report system_physics_update(double dt) {
    uint64_t started = SDL_GetPerformanceCounter();
    Report preparation = physics_report_create(ROHR_PHYSICS_REPORT);
    Report report;
    if(isfinite(dt) && dt >= 0.0) physics_hitbox_animation_bindings_report_update(&preparation);
    report = physics_pipeline_update_internal(dt, false);
    /* Preparation is internal: copy its issues, not a second physics invocation. */
    for(size_t i = 0; i < preparation.diagnostic_count; i += 1)
        (void)report_diagnostic_add(&report, preparation.diagnostics[i]);
    for(size_t i = 0; i < preparation.statistic_count; i += 1)
        (void)report_statistic_add(&report, preparation.statistics[i]);
    report.dropped_diagnostic_occurrences += preparation.dropped_diagnostic_occurrences;
    physics_report_finish(&report, started);
    return report;
}

void print_entity_movement(Entity entity) {
    console_write(LOG_ENGINE, "---Movement Log---\n");
    console_write(LOG_ENGINE, "Entity: %d\n", entity);
    console_write(LOG_ENGINE, "Position: {x: %f, y: %f}\n", positions[entity].x, positions[entity].y);
    console_write(LOG_ENGINE, "Velocity: {x: %f, y: %f}\n", velocities[entity].x, velocities[entity].y);
    console_write(LOG_ENGINE, "Acceleration: {x: %f, y: %f}\n", accelerations[entity].x, accelerations[entity].y);
    console_write(LOG_ENGINE, "---Movement Log---\n");
}
