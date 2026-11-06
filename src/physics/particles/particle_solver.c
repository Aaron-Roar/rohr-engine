/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "particle_solver.h"
#include "physics/physics_step_internal.h"
#include "physics/physics_operation.h"
#include <math.h>
#include <stdlib.h>

#ifndef ROHR_PHYSICS_COMPACT_PARTICLES
#define ROHR_PHYSICS_COMPACT_PARTICLES 0
#endif
#ifndef ROHR_PHYSICS_PARTICLE_PASSES
#define ROHR_PHYSICS_PARTICLE_PASSES 0
#endif

typedef struct ParticleRow {
    size_t source;
    EntityIndex first, second;
    float radius_first, radius_second;
    float inverse_first, inverse_second, inverse_sum;
    float friction, restitution, slop, rebound;
    bool moving_first, moving_second, solved;
    OverlapInfo overlap;
    ContactInfo contact;
} ParticleRow;

typedef struct ParticleEntry {
    size_t index;
    bool compact;
} ParticleEntry;

typedef struct PhysicsParticleSolver {
    ParticleRow *rows;
    ParticleEntry *entries;
    size_t *remaining;
    size_t capacity, row_count, entry_count, remaining_count;
    uint32_t iteration, passes;
    bool active;
} PhysicsParticleSolver;

static bool row_prepare_check(ParticleRow *row, size_t source,
        const SystemContactConstraint *constraint, Report *report) {
    if(constraint->type != SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR ||
            !constraint->value.rigid.responds) return false;
    EntityIndex a = constraint->value.rigid.first_index;
    EntityIndex b = constraint->value.rigid.second_index;
    EntityIndex ids[] = {a, b};
    for(unsigned side = 0; side < 2; ++side) {
        EntityIndex i = ids[side];
        if(!physics_step_entity_components_check(i, ROHR_PARTICLE | ROHR_HIT_BOX) ||
                !physics_entity_nonrotating_check(i) ||
                (entity_mask[i] & ROHR_TRANSFORM_LOCK) ||
                i >= particle_geometries_pool.capacity || !particle_geometries_pool.used[i] ||
                particle_geometries_pool.objects[i].local_origin.x != 0 ||
                particle_geometries_pool.objects[i].local_origin.y != 0 ||
                !isfinite(restitutions[i]) || !isfinite(frictions[i]) ||
                frictions[i] < 0 || !physics_step_hitbox_update(i, report)) return false;
    }
    float ra = physics_particle_radius_by_index_get(a);
    float rb = physics_particle_radius_by_index_get(b);
    float ia = physics_entity_simulated_get(a) &&
        physics_collision_response_to_by_index_check(a, b) ? 1.0f / mass[a] : 0;
    float ib = physics_entity_simulated_get(b) &&
        physics_collision_response_to_by_index_check(b, a) ? 1.0f / mass[b] : 0;
    float friction = sqrtf(frictions[a] * frictions[b]);
    if(!isfinite(ia + ib) || ia + ib <= 0 || !isfinite(friction) ||
            !isfinite(ra + rb) || ra < 0 || rb < 0 ||
            ra + rb > 1e10f) return false;
    *row = (ParticleRow){.source = source, .first = a, .second = b,
        .radius_first = ra, .radius_second = rb,
        .inverse_first = ia, .inverse_second = ib, .inverse_sum = ia + ib,
        .friction = friction, .restitution = fminf(restitutions[a], restitutions[b]),
        .slop = fminf(.01f, .0001f * fminf(2 * ra, 2 * rb)),
        .moving_first = physics_entity_movable_get(a),
        .moving_second = physics_entity_movable_get(b),
        .solved = constraint->value.rigid.solved,
        .overlap = constraint->value.rigid.overlap,
        .contact = constraint->value.rigid.contact,
        .rebound = constraint->value.rigid.rebound_speeds[0][0]};
    return true;
}

static bool scratch_reserve_check(PhysicsParticleSolver *solver, size_t count) {
    if(count <= solver->capacity) return true;
    size_t capacity = solver->capacity ? solver->capacity : 64;
    while(capacity < count && capacity <= SIZE_MAX / 2) capacity *= 2;
    count = capacity < count ? count : capacity;
    if(count > SIZE_MAX / sizeof(ParticleRow) ||
            count > SIZE_MAX / sizeof(ParticleEntry) || count > SIZE_MAX / sizeof(size_t))
        return false;
    ParticleRow *rows = malloc(count * sizeof(*rows));
    ParticleEntry *entries = malloc(count * sizeof(*entries));
    size_t *remaining = malloc(count * sizeof(*remaining));
    if(!rows || !entries || !remaining) {
        free(rows); free(entries); free(remaining);
        return false;
    }
    free(solver->rows); free(solver->entries); free(solver->remaining);
    solver->rows = rows; solver->entries = entries; solver->remaining = remaining;
    solver->capacity = count;
    return true;
}

bool physics_particle_solver_prepare_check(ContactConstraintList *list,
        uint32_t iterations, Report *report) {
    if(list == NULL) return false;
    if(list->particle_solver) list->particle_solver->active = false;
    list->particle_passes = 0;
    list->particle_iteration = 0;
    if(ROHR_PHYSICS_PARTICLE_PASSES && physics_simple_baseline_active_check() &&
            list == &physics_step_contact_constraints && physics_step_entities_active_check())
        list->particle_passes = ROHR_PHYSICS_PARTICLE_PASSES < iterations
            ? ROHR_PHYSICS_PARTICLE_PASSES : iterations;
    /* Small sets retain the existing polygon block solver and its ordering.
     * Manual solves cannot assume that preparation remains immutable. */
    if(!ROHR_PHYSICS_COMPACT_PARTICLES || !physics_simple_baseline_active_check() ||
            list != &physics_step_contact_constraints || list->count <= 32 ||
            !physics_step_entities_active_check() || !iterations) return false;
    if(!list->particle_solver) list->particle_solver = calloc(1, sizeof(*list->particle_solver));
    PhysicsParticleSolver *solver = list->particle_solver;
    if(!solver || !scratch_reserve_check(solver, list->count)) {
        physics_report_issue_add(report, ROHR_PHYSICS_NORMAL_IMPULSE,
            ROHR_REPORT_SEVERITY_WARNING, ERROR_MEMORY_POOL_ALLOCATION_FAILED,
            ROHR_REPORT_CONTRIBUTION_SKIPPED, 0, 0,
            "compact contact preparation unavailable; using the existing solver");
        return false;
    }
    solver->row_count = solver->entry_count = solver->remaining_count = 0;
    solver->iteration = 0;
    solver->passes = list->particle_passes ? list->particle_passes : iterations;
    for(size_t i = 0; i < list->count; ++i) {
        const SystemContactConstraint *constraint = &list->values[i];
        if(constraint->type != SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR) continue;
        bool compact = row_prepare_check(&solver->rows[solver->row_count], i, constraint, report);
        solver->entries[solver->entry_count++] = (ParticleEntry){
            compact ? solver->row_count++ : i, compact};
        if(!compact) solver->remaining[solver->remaining_count++] = i;
    }
    solver->active = solver->row_count != 0;
    return solver->active;
}

bool physics_particle_solver_active_check(const ContactConstraintList *list) {
    return list && list->particle_solver && list->particle_solver->active;
}

/* The baseline's experimental effort applies equally to compact and fallback
 * particle pairs. Small lists and failed scratch allocation use this same
 * policy. Manual staged solves never install it. */
bool physics_particle_solver_pass_check(const SystemContactConstraint *constraint,
        float *position_fraction) {
    const ContactConstraintList *list = &physics_step_contact_constraints;
    if(!list->particle_passes || !constraint ||
            constraint->type != SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR ||
            !constraint->value.rigid.responds) return true;
    EntityIndex a = constraint->value.rigid.first_index;
    EntityIndex b = constraint->value.rigid.second_index;
    if(!physics_step_entity_components_check(a, ROHR_PARTICLE) ||
            !physics_step_entity_components_check(b, ROHR_PARTICLE) ||
            !physics_entity_nonrotating_check(a) || !physics_entity_nonrotating_check(b))
        return true;
    if(list->particle_iteration >= list->particle_passes) return false;
    *position_fraction = 1.0f / (float)(list->particle_passes - list->particle_iteration);
    return true;
}

static void row_store(ParticleRow *row, SystemContactConstraint *constraint) {
    constraint->value.rigid.overlap = row->overlap;
    constraint->value.rigid.contact = row->contact;
    constraint->value.rigid.solved = row->solved;
    constraint->value.rigid.manifold_contact_count = row->contact.detected;
    if(row->contact.detected) constraint->value.rigid.manifold_contacts[0] = row->contact;
    constraint->value.rigid.rebound_speeds[0][0] = row->rebound;
}

static void row_fallback(ParticleRow *row, ContactConstraintList *list,
        float fraction, Report *report) {
    SystemContactConstraint *constraint = &list->values[row->source];
    row_store(row, constraint);
    physics_rigid_contact_constraint_solve(constraint, fraction, report);
    row->overlap = constraint->value.rigid.overlap;
    row->contact = constraint->value.rigid.contact;
    row->solved = constraint->value.rigid.solved;
    row->rebound = constraint->value.rigid.rebound_speeds[0][0];
}

static void row_solve(ParticleRow *row, ContactConstraintList *list,
        float fraction, Report *report) {
    EntityIndex a = row->first, b = row->second;
    Position pa = positions[a], pb = positions[b];
    if(!physics_world_position_check(pa) || !physics_world_position_check(pb) ||
            fabsf(pa.x) > 1e10f || fabsf(pa.y) > 1e10f ||
            fabsf(pb.x) > 1e10f || fabsf(pb.y) > 1e10f) {
        row_fallback(row, list, fraction, report);
        return;
    }
    float dx = pb.x - pa.x, dy = pb.y - pa.y;
    float distance2 = dx * dx + dy * dy;
    float radius = row->radius_first + row->radius_second;
    if(distance2 >= radius * radius) return;
    float distance = sqrtf(distance2);
    Vec2D normal = distance > .00001f ? (Vec2D){dx / distance, dy / distance} : (Vec2D){1, 0};
    OverlapInfo overlap = {true, normal, radius - distance};
    Velocity relative = {
        (row->moving_second ? velocities[b].x : 0) - (row->moving_first ? velocities[a].x : 0),
        (row->moving_second ? velocities[b].y : 0) - (row->moving_first ? velocities[a].y : 0)};
    float along = relative.x * normal.x + relative.y * normal.y;
    float previous = row->contact.detected
        ? row->contact.points[0].normal_impulse.x * normal.x +
            row->contact.points[0].normal_impulse.y * normal.y : 0;
    float rebound = row->contact.detected ? row->rebound :
        (row->solved ? 0 : -row->restitution * fminf(along, 0));
    float delta = (rebound - along) / row->inverse_sum;
    if(!isfinite(delta) || !isfinite(previous + delta) ||
            !isfinite(angular_velocities[a]) || !isfinite(angular_velocities[b])) {
        row_fallback(row, list, fraction, report);
        return;
    }
    float accumulated = fmaxf(previous + delta, 0);
    delta = accumulated - previous;
    Vec2D impulse = {normal.x * delta, normal.y * delta};
    PhysicsVelocityChange changes[] = {
        {a, {velocities[a].x - impulse.x * row->inverse_first,
             velocities[a].y - impulse.y * row->inverse_first}, angular_velocities[a]},
        {b, {velocities[b].x + impulse.x * row->inverse_second,
             velocities[b].y + impulse.y * row->inverse_second}, angular_velocities[b]}};
    if(!physics_vector_finite_check(changes[0].velocity) ||
            !physics_vector_finite_check(changes[1].velocity)) {
        row_fallback(row, list, fraction, report);
        return;
    }
    (void)physics_velocity_pair_commit(changes, report, ROHR_PHYSICS_NORMAL_IMPULSE);
    Vec2D friction = row->contact.detected ? row->contact.points[0].friction_impulse : (Vec2D){0};
    Velocity after = {
        (row->moving_second ? velocities[b].x : 0) - (row->moving_first ? velocities[a].x : 0),
        (row->moving_second ? velocities[b].y : 0) - (row->moving_first ? velocities[a].y : 0)};
    float after_along = after.x * normal.x + after.y * normal.y;
    Vec2D tangent = {after.x - normal.x * after_along, after.y - normal.y * after_along};
    float magnitude = sqrtf(tangent.x * tangent.x + tangent.y * tangent.y);
    if(!(magnitude <= .000001f)) {
        tangent.x /= magnitude; tangent.y /= magnitude;
        delta = -(after.x * tangent.x + after.y * tangent.y) / row->inverse_sum;
        float old = friction.x * tangent.x + friction.y * tangent.y;
        float maximum = fabsf(accumulated) * row->friction;
        if(isfinite(magnitude) && isfinite(delta) && isfinite(old) && isfinite(maximum)) {
            float next = fmaxf(-maximum, fminf(old + delta, maximum));
            delta = next - old;
            impulse = (Vec2D){tangent.x * delta, tangent.y * delta};
            changes[0].velocity.x -= impulse.x * row->inverse_first;
            changes[0].velocity.y -= impulse.y * row->inverse_first;
            changes[1].velocity.x += impulse.x * row->inverse_second;
            changes[1].velocity.y += impulse.y * row->inverse_second;
            if(physics_velocity_pair_commit(changes, report, ROHR_PHYSICS_FRICTION_IMPULSE))
                friction = (Vec2D){tangent.x * next, tangent.y * next};
        } else physics_operation_reject(report, ROHR_PHYSICS_FRICTION_IMPULSE,
            ERROR_ENGINE_STATE_INVALID, physics_operation_entity_get(a), physics_operation_entity_get(b));
    }
    Position ca = {pa.x + normal.x * row->radius_first, pa.y + normal.y * row->radius_first};
    Position cb = {pb.x + -normal.x * row->radius_second, pb.y + -normal.y * row->radius_second};
    Position point = row->moving_first && !row->moving_second ? ca :
        row->moving_second && !row->moving_first ? cb :
        (Position){(ca.x + cb.x) * .5f, (ca.y + cb.y) * .5f};
    row->contact = (ContactInfo){.detected = true, .normal = normal, .depth = overlap.depth,
        .point_count = 1, .points = {{.position = point, .relative_velocity = relative,
            .normal_impulse = {normal.x * accumulated, normal.y * accumulated},
            .friction_impulse = friction}}};
    row->overlap = overlap;
    row->rebound = rebound;
    row->solved = true;
    float correction = fminf(fmaxf(overlap.depth * fraction - row->slop, 0), 5);
    if(correction > 0) {
        Vec2D move = {normal.x * correction, normal.y * correction};
        float share_a = row->inverse_first / row->inverse_sum;
        float share_b = row->inverse_second / row->inverse_sum;
        PhysicsChange position_changes[] = {
            {.index = a, .fields = PHYSICS_CHANGE_POSITION,
             .position = {pa.x - move.x * share_a, pa.y - move.y * share_a}},
            {.index = b, .fields = PHYSICS_CHANGE_POSITION,
             .position = {pb.x + move.x * share_b, pb.y + move.y * share_b}}};
        (void)physics_changes_commit(position_changes, 2, report, ROHR_PHYSICS_POSITION_CORRECTION,
            physics_operation_entity_get(a), physics_operation_entity_get(b));
    }
    physics_step_hitbox_dirty_add(a);
    physics_step_hitbox_dirty_add(b);
}

void physics_particle_solver_solve(ContactConstraintList *list,
        float position_fraction, Report *report) {
    PhysicsParticleSolver *solver = list->particle_solver;
    if(solver->iteration < solver->passes) {
        float fraction = 1.0f / (float)(solver->passes - solver->iteration);
        for(size_t i = 0; i < solver->entry_count && (!report || !report->has_fatal); ++i) {
            ParticleEntry entry = solver->entries[i];
            if(entry.compact) row_solve(&solver->rows[entry.index], list, fraction, report);
            else physics_rigid_contact_constraint_solve(&list->values[entry.index], position_fraction, report);
        }
    } else {
        for(size_t i = 0; i < solver->remaining_count && (!report || !report->has_fatal); ++i)
            physics_rigid_contact_constraint_solve(&list->values[solver->remaining[i]], position_fraction, report);
    }
    ++solver->iteration;
}

void physics_particle_solver_finish(ContactConstraintList *list) {
    if(!list) return;
    list->particle_passes = 0;
    list->particle_iteration = 0;
    if(!physics_particle_solver_active_check(list)) return;
    PhysicsParticleSolver *solver = list->particle_solver;
    for(size_t i = 0; i < solver->row_count; ++i)
        row_store(&solver->rows[i], &list->values[solver->rows[i].source]);
    solver->active = false;
}

void physics_particle_solver_destroy(ContactConstraintList *list) {
    if(!list || !list->particle_solver) return;
    PhysicsParticleSolver *solver = list->particle_solver;
    free(solver->rows); free(solver->entries); free(solver->remaining);
    free(solver);
    list->particle_solver = NULL;
}
