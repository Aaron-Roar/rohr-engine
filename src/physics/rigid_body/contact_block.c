/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics/physics_step_internal.h"
#include "physics/physics_operation.h"
#include "systems.h"
#include <float.h>
#include <math.h>
#include <string.h>

/* Bounded reciprocal polygon contact sets are solved without allocation.
 * Unsupported or oversized sets retain the iterative solver. Extended-precision
 * QR preserves support across extreme mass ratios; ordinary weights can use
 * the sparse double-precision path with QR fallback for dependent rows. */
#define BLOCK_PAIR_MAX 32
#define BLOCK_BODY_MAX 32
#define BLOCK_ROW_MAX 96
#define BLOCK_DOF_MAX (3 * BLOCK_BODY_MAX)
#define BLOCK_DEGREES (180.0 / 3.14159265358979323846)
_Static_assert(BLOCK_ROW_MAX <= 128, "active-row masks contain two 64-bit words");

typedef struct BlockRow {
    long double a[BLOCK_DOF_MAX];
    long double scale, friction;
    long double velocity_rhs;
    long double position_rhs;
    double position_tolerance;
    unsigned pair, manifold, point;
    unsigned slots[2];
    bool tangent, sliding, active_hint;
} BlockRow;

typedef struct ContactBlock {
    EntityIndex bodies[BLOCK_BODY_MAX];
    Position centers[BLOCK_BODY_MAX];
    long double weights[BLOCK_DOF_MAX];
    BlockRow rows[BLOCK_ROW_MAX];
    ContactInfo contacts[BLOCK_PAIR_MAX][CONTACT_MANIFOLD_MAX];
    unsigned contact_counts[BLOCK_PAIR_MAX];
    unsigned body_count, row_count;
} ContactBlock;

/* Validate each body once while checking/preparing a block. Contact constraints
 * separately retain pose-checked manifolds for this solver invocation. */
typedef struct BlockGeometry {
    EntityIndex bodies[BLOCK_BODY_MAX];
    unsigned body_count;
} BlockGeometry;

static bool block_geometry_prepare_check(BlockGeometry *geometry,
    SystemContactConstraint *constraint, Report *report) {
    EntityIndex first = constraint->value.rigid.first_index, second = constraint->value.rigid.second_index;
    EntityIndex bodies[] = {first, second};
    bool unchanged = constraint->value.rigid.block_geometry_valid;
    for(unsigned side = 0; side < 2 && unchanged; ++side) {
        Position previous = constraint->value.rigid.block_positions[side];
        unchanged &= previous.x == positions[bodies[side]].x && previous.y == positions[bodies[side]].y &&
            constraint->value.rigid.block_orientations[side] == orientations[bodies[side]];
    }
    if(unchanged) return true;
    for(unsigned side = 0; side < 2; ++side) {
        bool found = false;
        for(unsigned k = 0; k < geometry->body_count; ++k) found |= geometry->bodies[k] == bodies[side];
        if(found) continue;
        if(geometry->body_count == BLOCK_BODY_MAX || !physics_step_hitbox_update(bodies[side], report)) return false;
        geometry->bodies[geometry->body_count++] = bodies[side];
    }
    constraint->value.rigid.block_geometry = contact_manifold_set_polygon_get(world_hit_boxes[first], world_hit_boxes[second]);
    for(unsigned side = 0; side < 2; ++side) {
        constraint->value.rigid.block_positions[side] = positions[bodies[side]];
        constraint->value.rigid.block_orientations[side] = orientations[bodies[side]];
    }
    constraint->value.rigid.block_geometry_valid = true;
    return true;
}

/* Share the active-set logic across double Cholesky, double QR, and extended
 * precision QR. QR avoids squaring the condition number of the Jacobian. */
#define PROJECTION_REAL long double
#define PROJECTION_NAME block_project_precise_check
#define PROJECTION_SQRT sqrtl
#include "contact_projection.h"
#undef PROJECTION_REAL
#undef PROJECTION_NAME
#undef PROJECTION_SQRT

#define PROJECTION_REAL double
#define PROJECTION_NAME block_project_stable_check
#define PROJECTION_SQRT sqrt
#include "contact_projection.h"
#undef PROJECTION_REAL
#undef PROJECTION_NAME
#undef PROJECTION_SQRT
#define PROJECTION_REAL double
#define PROJECTION_NAME block_project_fast_check
#define PROJECTION_SQRT sqrt
#define PROJECTION_GRAM
#include "contact_projection.h"
#undef PROJECTION_GRAM
#undef PROJECTION_REAL
#undef PROJECTION_NAME
#undef PROJECTION_SQRT

static bool block_project_check(ContactBlock *block, bool position, bool sticking,
    long double *change, long double *impulses) {
    bool result = false;
    if(block->row_count > 16) {
        long double low = LDBL_MAX, high = 0;
        for(unsigned k = 0; k < 3 * block->body_count; ++k) {
            if(block->weights[k] <= 0) continue;
            low = fminl(low, block->weights[k]);
            high = fmaxl(high, block->weights[k]);
        }
        if(high <= 1000 * low) {
            result = block_project_fast_check(block, position, sticking, true, change, impulses);
            if(!result) result = block_project_fast_check(block, position, sticking, false, change, impulses);
            if(!result) result = block_project_stable_check(block, position, sticking, false, change, impulses);
        }
    }
    if(!result) result = block_project_precise_check(block, position, sticking, false, change, impulses);
    if(result)
        for(unsigned i = 0; i < block->row_count; ++i)
            if(!block->rows[i].tangent) block->rows[i].active_hint = impulses[i] > 0;
    return result;
}

static unsigned block_body_add(ContactBlock *block, EntityIndex body) {
    for(unsigned i = 0; i < block->body_count; ++i)
        if(block->bodies[i] == body) return i;
    if(block->body_count == BLOCK_BODY_MAX) return BLOCK_BODY_MAX;
    unsigned slot = block->body_count++;
    block->bodies[slot] = body;
    block->centers[slot] = physics_com_world_by_index_get(body);
    for(unsigned axis = 0; axis < 3; ++axis) block->weights[3 * slot + axis] = 0;
    if(physics_entity_simulated_get(body)) {
        block->weights[3 * slot] = block->weights[3 * slot + 1] = sqrtl(1.0 / mass[body]);
        if(!physics_entity_nonrotating_check(body))
            block->weights[3 * slot + 2] = sqrtl(physics_inverse_inertia_by_index_get(body));
    }
    return slot;
}

static Velocity block_point_velocity_get(const ContactBlock *block, unsigned slot, Position point) {
    EntityIndex body = block->bodies[slot];
    if(!physics_entity_movable_get(body)) return (Velocity){0};
    Vec2D angular = math_angular_velocity_cross_vec(
        physics_entity_nonrotating_check(body) ? 0 : angular_velocities[body],
        math_vector_subtract(point, block->centers[slot]));
    return (Velocity){velocities[body].x + angular.x, velocities[body].y + angular.y};
}

/* Penetration repair is geometric, not an impulse. Give movable bodies equal
 * translational weight while retaining their shape's rotational resistance.
 * Using physical mass here can repair a microscopic wedge overlap by moving
 * a light support many body widths instead of lifting its heavy load slightly.
 * Velocity response continues to use the actual mass and inertia. */
static void block_position_metric_set(ContactBlock *block) {
    long double factors[BLOCK_DOF_MAX] = {0};
    for(unsigned i = 0; i < block->body_count; ++i) {
        long double factor = sqrtl(mass[block->bodies[i]]);
        for(unsigned axis = 0; axis < 3; ++axis) {
            factors[3 * i + axis] = factor;
            block->weights[3 * i + axis] *= factor;
        }
    }
    for(unsigned i = 0; i < block->row_count; ++i) {
        BlockRow *row = &block->rows[i];
        long double norm = 0;
        for(unsigned side = 0; side < 2; ++side)
            for(unsigned axis = 0; axis < 3; ++axis) {
                unsigned k = 3 * row->slots[side] + axis;
                row->a[k] *= factors[k];
                norm += row->a[k] * row->a[k];
            }
        norm = sqrtl(norm);
        for(unsigned side = 0; side < 2; ++side)
            for(unsigned axis = 0; axis < 3; ++axis) row->a[3 * row->slots[side] + axis] /= norm;
        row->scale *= norm;
        row->position_rhs /= norm;
    }
}

static bool block_prepare_check(ContactBlock *block, ContactConstraintList *list,
    BlockGeometry *geometry, Report *report) {
    if(!list->count || list->count > BLOCK_PAIR_MAX) return false;
    block->body_count = block->row_count = 0;
    for(unsigned i = 0; i < list->count; ++i) {
        block->contact_counts[i] = 0;
        SystemContactConstraint *c = &list->values[i];
        if(c->type != SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR) return false;
        EntityIndex first = c->value.rigid.first_index, second = c->value.rigid.second_index;
        if(entity_index_components_check(first, ROHR_PARTICLE) ||
                entity_index_components_check(second, ROHR_PARTICLE)) return false;
        if(!c->value.rigid.responds) continue;
        /* A single shared mass metric requires reciprocal dynamic response. */
        if(!physics_collision_response_to_check(c->value.rigid.first, c->value.rigid.second) ||
                !physics_collision_response_to_check(c->value.rigid.second, c->value.rigid.first))
            return false;
        if(!isfinite(mass[first]) || !isfinite(mass[second]) ||
                mass[first] <= 0 || mass[second] <= 0 ||
                !isfinite(restitutions[first]) || !isfinite(restitutions[second]) ||
                !isfinite(frictions[first]) || !isfinite(frictions[second]) ||
                frictions[first] < 0 || frictions[second] < 0) return false;
        if(!block_geometry_prepare_check(geometry, c, report)) return false;
        unsigned slots[] = {block_body_add(block, first), block_body_add(block, second)};
        if(slots[0] == BLOCK_BODY_MAX || slots[1] == BLOCK_BODY_MAX) return false;
        EntityIndex bodies[] = {first, second};
        ContactManifoldSet manifolds = c->value.rigid.block_geometry;
        block->contact_counts[i] = manifolds.count;
        for(unsigned m = 0; m < manifolds.count; ++m) {
            ContactManifold *manifold = &manifolds.values[m];
            ContactInfo *contact = &block->contacts[i][m];
            *contact = (ContactInfo){.detected = true, .normal = manifold->normal,
                .depth = manifold->depth, .point_count = manifold->count};
            for(unsigned p = 0; p < manifold->count; ++p) {
                if(block->row_count == BLOCK_ROW_MAX) return false;
                BlockRow *row = &block->rows[block->row_count];
                *row = (BlockRow){.active_hint = true};
                row->slots[0] = slots[0]; row->slots[1] = slots[1];
                row->pair = i; row->manifold = m; row->point = p;
                ContactPointInfo *point = &contact->points[p];
                point->position = manifold->points[p];
                point->feature_id = manifold->feature_ids[p];
                long double vn = 0;
                for(unsigned side = 0; side < 2; ++side) {
                    EntityIndex body = bodies[side];
                    unsigned slot = slots[side];
                    long double sign = side ? 1 : -1;
                    Vec2D r = math_vector_subtract(point->position, block->centers[slot]);
                    long double j[] = {sign * manifold->normal.x, sign * manifold->normal.y,
                        -sign * ((long double)r.x * manifold->normal.y - (long double)r.y * manifold->normal.x)};
                    if(physics_entity_movable_get(body)) {
                        vn += j[0] * velocities[body].x + j[1] * velocities[body].y;
                        if(!physics_entity_nonrotating_check(body))
                            vn += j[2] * angular_velocities[body] / BLOCK_DEGREES;
                    }
                    for(unsigned axis = 0; axis < 3; ++axis)
                        row->a[3 * slot + axis] = j[axis] * block->weights[3 * slot + axis];
                }
                long double norm = 0;
                for(unsigned side = 0; side < 2; ++side)
                    for(unsigned axis = 0; axis < 3; ++axis) {
                        long double value = row->a[3 * slots[side] + axis];
                        norm += value * value;
                    }
                if(norm <= 0) continue;
                row->scale = sqrtl(norm);
                for(unsigned side = 0; side < 2; ++side)
                    for(unsigned axis = 0; axis < 3; ++axis) row->a[3 * slots[side] + axis] /= row->scale;
                long double rebound = !c->value.rigid.solved && vn < -1 ?
                    -fmin(restitutions[first], restitutions[second]) * vn : 0;
                row->velocity_rhs = (rebound - vn) / row->scale;
                float slop = physics_rigid_contact_slop_get(first, second, manifold->normal);
                long double correction = manifold->point_depths[p] - slop;
                row->position_tolerance = fmin(slop, 4 * FLT_EPSILON *
                    fmax(1, fmax(fabs(point->position.x), fabs(point->position.y))));
                /* Connected contacts are repaired together. Full correction
                 * leaves later passes to catch newly touching neighbors. */
                row->position_rhs = correction / row->scale;
                point->relative_velocity = math_vector_subtract(
                    block_point_velocity_get(block, slots[1], point->position),
                    block_point_velocity_get(block, slots[0], point->position));
                ++block->row_count;
            }
            if(block->row_count == BLOCK_ROW_MAX) return false;
            BlockRow *row = &block->rows[block->row_count];
            *row = (BlockRow){0};
            row->slots[0] = slots[0]; row->slots[1] = slots[1];
            row->pair = i; row->manifold = m; row->tangent = true;
            row->friction = sqrtl((long double)frictions[first] * frictions[second]);
            Position point = manifold->points[0];
            if(manifold->count == 2) {
                point.x = 0.5f * (point.x + manifold->points[1].x);
                point.y = 0.5f * (point.y + manifold->points[1].y);
            }
            long double tx = -manifold->normal.y, ty = manifold->normal.x, vt = 0;
            for(unsigned side = 0; side < 2; ++side) {
                EntityIndex body = bodies[side];
                unsigned slot = slots[side];
                long double sign = side ? 1 : -1;
                Vec2D r = math_vector_subtract(point, block->centers[slot]);
                long double j[] = {sign * tx, sign * ty, -sign * (r.x * ty - r.y * tx)};
                if(physics_entity_movable_get(body)) {
                    vt += j[0] * velocities[body].x + j[1] * velocities[body].y;
                    if(!physics_entity_nonrotating_check(body))
                        vt += j[2] * angular_velocities[body] / BLOCK_DEGREES;
                }
                for(unsigned axis = 0; axis < 3; ++axis)
                    row->a[3 * slot + axis] = j[axis] * block->weights[3 * slot + axis];
            }
            long double norm = 0;
            row->sliding = fabsl(vt) > 1e-6;
            for(unsigned side = 0; side < 2; ++side)
                for(unsigned axis = 0; axis < 3; ++axis) {
                    long double value = row->a[3 * slots[side] + axis];
                    norm += value * value;
                }
            if(norm > 0) {
                row->scale = sqrtl(norm);
                for(unsigned side = 0; side < 2; ++side)
                    for(unsigned axis = 0; axis < 3; ++axis) row->a[3 * slots[side] + axis] /= row->scale;
                row->velocity_rhs = -vt / row->scale;
                ++block->row_count;
            }
        }
    }
    return true;
}

/* Degenerate sticking geometry retains the normal-only fallback. */
static bool block_friction_project_check(ContactBlock *block,
    bool position, long double *change, long double *impulses) {
    if(block_project_check(block, position, true, change, impulses)) return true;
    for(unsigned i = 0; i < block->row_count; ++i)
        if(block->rows[i].tangent) block->rows[i].sliding = true;
    return block_project_check(block, position, false, change, impulses);
}

/* A completed contact pass needs no additional impulse when all current
 * normal constraints hold. Recheck geometry and velocity every time: this is
 * convergence within a substep, not sleeping or a persistent motion lock. */
static bool block_satisfied_check(ContactConstraintList *list, BlockGeometry *geometry, Report *report) {
    if(!list->count || list->count > BLOCK_PAIR_MAX) return false;
    for(unsigned i = 0; i < list->count; ++i) {
        SystemContactConstraint *c = &list->values[i];
        if(c->type != SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR) return false;
        if(c->value.rigid.responds && !c->value.rigid.solved) return false;
    }
    for(unsigned i = 0; i < list->count; ++i) {
        SystemContactConstraint *c = &list->values[i];
        if(!c->value.rigid.responds) continue;
        EntityIndex first = c->value.rigid.first_index, second = c->value.rigid.second_index;
        if(entity_index_components_check(first, ROHR_PARTICLE) ||
                entity_index_components_check(second, ROHR_PARTICLE) ||
                !physics_collision_response_to_check(c->value.rigid.first, c->value.rigid.second) ||
                !physics_collision_response_to_check(c->value.rigid.second, c->value.rigid.first) ||
                !isfinite(mass[first]) || !isfinite(mass[second]) || mass[first] <= 0 || mass[second] <= 0 ||
                !isfinite(restitutions[first]) || !isfinite(restitutions[second]) ||
                !isfinite(frictions[first]) || !isfinite(frictions[second]) ||
                frictions[first] < 0 || frictions[second] < 0) return false;
        EntityIndex bodies[] = {first, second};
        if(!block_geometry_prepare_check(geometry, c, report)) return false;
        ContactManifoldSet manifolds = c->value.rigid.block_geometry;
        Position centers[] = {physics_com_world_by_index_get(first), physics_com_world_by_index_get(second)};
        for(unsigned m = 0; m < manifolds.count; ++m) {
            ContactManifold *manifold = &manifolds.values[m];
            double slop = physics_rigid_contact_slop_get(first, second, manifold->normal);
            for(unsigned p = 0; p < manifold->count; ++p) {
                Position point = manifold->points[p];
                double precision = fmin(slop, 4 * FLT_EPSILON * fmax(1, fmax(fabs(point.x), fabs(point.y))));
                if(manifold->point_depths[p] - slop > precision) return false;
                long double vn = 0, velocity_scale = 0;
                for(unsigned side = 0; side < 2; ++side) {
                    EntityIndex body = bodies[side];
                    if(!physics_entity_movable_get(body)) continue;
                    long double sign = side ? 1 : -1;
                    Vec2D r = math_vector_subtract(point, centers[side]);
                    long double x = sign * manifold->normal.x * velocities[body].x;
                    long double y = sign * manifold->normal.y * velocities[body].y;
                    long double rotation = !physics_entity_nonrotating_check(body) ?
                        -sign * ((long double)r.x * manifold->normal.y -
                            (long double)r.y * manifold->normal.x) * angular_velocities[body] / BLOCK_DEGREES : 0;
                    vn += x + y + rotation;
                    velocity_scale += fabsl(x) + fabsl(y) + fabsl(rotation);
                }
                if(!isfinite(vn) || !isfinite(velocity_scale) ||
                        vn < -fmaxl(1e-6, 8 * FLT_EPSILON * velocity_scale)) return false;
            }
        }
    }
    return true;
}

static bool block_solve_check(ContactConstraintList *list,
    Report *report, bool *converged) {
    BlockGeometry geometry;
    geometry.body_count = 0;
    *converged = block_satisfied_check(list, &geometry, report);
    if(*converged) return true;
    ContactBlock block;
    long double velocity[BLOCK_DOF_MAX], position[BLOCK_DOF_MAX];
    long double impulses[BLOCK_ROW_MAX], unused[BLOCK_ROW_MAX];
    if(!block_prepare_check(&block, list, &geometry, report)) return false;
    if(!block_friction_project_check(&block, false, velocity, impulses)) return false;
    bool velocity_sticking[BLOCK_ROW_MAX];
    for(unsigned i = 0; i < block.row_count; ++i) velocity_sticking[i] = !block.rows[i].sliding;
    long double velocity_weights[BLOCK_DOF_MAX];
    memcpy(velocity_weights, block.weights, 3 * block.body_count * sizeof(*velocity_weights));
    bool position_satisfied = true;
    for(unsigned i = 0; i < block.row_count; ++i) {
        BlockRow *row = &block.rows[i];
        if(row->tangent) continue;
        if(row->position_rhs * row->scale > row->position_tolerance) position_satisfied = false;
    }
    if(position_satisfied) memset(position, 0, 3 * block.body_count * sizeof(*position));
    else {
        block_position_metric_set(&block);
        if(!block_friction_project_check(&block, true, position, unused)) return false;
    }
    PhysicsChange changes[BLOCK_BODY_MAX];
    for(unsigned i = 0; i < block.body_count; ++i) {
        changes[i] = physics_change_get(block.bodies[i], PHYSICS_CHANGE_POSITION |
            PHYSICS_CHANGE_ORIENTATION | PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY);
        changes[i].velocity.x += velocity[3 * i] * velocity_weights[3 * i];
        changes[i].velocity.y += velocity[3 * i + 1] * velocity_weights[3 * i + 1];
        changes[i].angular_velocity += velocity[3 * i + 2] * velocity_weights[3 * i + 2] * BLOCK_DEGREES;
        physics_change_orientation_set(&changes[i], changes[i].orientation +
            position[3 * i + 2] * block.weights[3 * i + 2] * BLOCK_DEGREES);
        changes[i].position.x += position[3 * i] * block.weights[3 * i];
        changes[i].position.y += position[3 * i + 1] * block.weights[3 * i + 1];
    }
    Entity source = list->values[0].value.rigid.first;
    Entity target = list->values[0].value.rigid.second;
    for(unsigned i = 0; i < block.body_count; ++i)
        changes[i].fields = PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY;
    if(!physics_changes_commit(changes, block.body_count, report, ROHR_PHYSICS_NORMAL_IMPULSE, source, target))
        return true;
    for(unsigned i = 0; i < block.body_count; ++i)
        changes[i].fields = PHYSICS_CHANGE_POSITION | PHYSICS_CHANGE_ORIENTATION;
    (void)physics_changes_commit(changes, block.body_count, report,
        ROHR_PHYSICS_POSITION_CORRECTION, source, target);
    for(unsigned i = 0; i < block.row_count; ++i) {
        BlockRow *row = &block.rows[i];
        if(row->tangent) continue;
        SystemContactConstraint *c = &list->values[row->pair];
        ContactInfo *contact = &block.contacts[row->pair][row->manifold];
        ContactPointInfo *point = &contact->points[row->point];
        point->normal_impulse = (Vec2D){contact->normal.x * impulses[i], contact->normal.y * impulses[i]};
        for(unsigned m = 0; m < c->value.rigid.manifold_contact_count; ++m) {
            ContactInfo *old = &c->value.rigid.manifold_contacts[m];
            if(math_dot_product(old->normal, contact->normal) < 0.98f) continue;
            for(unsigned p = 0; p < old->point_count; ++p)
                if(old->points[p].feature_id == point->feature_id) {
                    point->normal_impulse.x += old->points[p].normal_impulse.x;
                    point->normal_impulse.y += old->points[p].normal_impulse.y;
                    point->friction_impulse = old->points[p].friction_impulse;
                    break;
                }
        }
        EntityIndex first = c->value.rigid.first_index, second = c->value.rigid.second_index;
        float im1 = physics_entity_simulated_get(first) ? 1.0f / mass[first] : 0;
        float im2 = physics_entity_simulated_get(second) ? 1.0f / mass[second] : 0;
        bool sticking = false;
        for(unsigned j = 0; j < block.row_count; ++j)
            if(block.rows[j].tangent && block.rows[j].pair == row->pair &&
                    block.rows[j].manifold == row->manifold && velocity_sticking[j]) {
                sticking = true;
                point->friction_impulse.x -= contact->normal.y * impulses[j] / contact->point_count;
                point->friction_impulse.y += contact->normal.x * impulses[j] / contact->point_count;
            }
        if(!sticking) {
            /* Block impulses are incremental, including their coupled static
             * friction. Do not independently unwind an earlier tangent impulse
             * when switching to sliding: its balancing normal impulse is still
             * applied. Bound each new friction impulse by this solve's normal
             * increment, and accumulate only the reported result. */
            Vec2D friction = system_friction_impulse_apply(first, second,
                (OverlapInfo){true, contact->normal, contact->depth},
                math_vector_subtract(point->position, physics_com_world_by_index_get(first)),
                math_vector_subtract(point->position, physics_com_world_by_index_get(second)),
                fmaxl(0, impulses[i]), im1, im2,
                im1 > 0 && !physics_entity_nonrotating_check(first) ? physics_inverse_inertia_by_index_get(first) : 0,
                im2 > 0 && !physics_entity_nonrotating_check(second) ? physics_inverse_inertia_by_index_get(second) : 0,
                (Vec2D){0}, report);
            point->friction_impulse.x += friction.x;
            point->friction_impulse.y += friction.y;
        }
    }
    for(unsigned i = 0; i < list->count; ++i) {
        SystemContactConstraint *c = &list->values[i];
        if(!block.contact_counts[i]) continue;
        c->value.rigid.manifold_contact_count = block.contact_counts[i];
        c->value.rigid.contact = block.contacts[i][0];
        for(unsigned m = 0; m < block.contact_counts[i]; ++m) {
            c->value.rigid.manifold_contacts[m] = block.contacts[i][m];
            if(block.contacts[i][m].depth > c->value.rigid.contact.depth)
                c->value.rigid.contact = block.contacts[i][m];
        }
        ContactInfo *contact = &c->value.rigid.contact;
        c->value.rigid.overlap = (OverlapInfo){true, contact->normal, contact->depth};
        if(!c->value.rigid.solved) physics_report_counter_add(report, ROHR_REPORT_CONTACT_COUNT, 1);
        c->value.rigid.solved = true;
    }
    return true;
}

bool physics_rigid_contact_block_solve_check(ContactConstraintList *list,
    float position_fraction, Report *report, bool *converged) {
    *converged = false;
    if(list->count <= 8) return block_solve_check(list, report, converged);
    if(list->count > BLOCK_PAIR_MAX) return false;
    unsigned group[BLOCK_PAIR_MAX];
    for(unsigned i = 0; i < list->count; ++i) {
        if(list->values[i].type != SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR) return false;
        group[i] = i;
    }
    /* Only a body changed by response couples two contact sets. Static and
     * prescribed-motion bodies may be shared by independent sets. */
    for(unsigned i = 0; i < list->count; ++i) {
        SystemContactConstraint *a = &list->values[i];
        if(!a->value.rigid.responds) continue;
        EntityIndex bodies[] = {a->value.rigid.first_index, a->value.rigid.second_index};
        for(unsigned j = 0; j < i; ++j) {
            SystemContactConstraint *b = &list->values[j];
            if(!b->value.rigid.responds || group[i] == group[j]) continue;
            bool connected = false;
            for(unsigned side = 0; side < 2; ++side)
                if(physics_entity_simulated_get(bodies[side]) &&
                        (bodies[side] == b->value.rigid.first_index ||
                         bodies[side] == b->value.rigid.second_index)) connected = true;
            if(!connected) continue;
            unsigned old = group[i], replacement = group[j];
            for(unsigned k = 0; k <= i; ++k)
                if(group[k] == old) group[k] = replacement;
        }
    }
    bool single = true;
    for(unsigned i = 1; i < list->count; ++i) single &= group[i] == group[0];
    if(single) return block_solve_check(list, report, converged);
    *converged = true;
    SystemContactConstraint values[BLOCK_PAIR_MAX];
    unsigned indices[BLOCK_PAIR_MAX];
    for(unsigned i = 0; i < list->count; ++i) {
        if(group[i] != i) continue;
        ContactConstraintList subset = {.values = values, .capacity = BLOCK_PAIR_MAX};
        for(unsigned j = 0; j < list->count; ++j)
            if(group[j] == i && list->values[j].value.rigid.responds) {
                indices[subset.count] = j;
                values[subset.count++] = list->values[j];
            }
        if(!subset.count) continue;
        bool subset_converged = false;
        if(!block_solve_check(&subset, report, &subset_converged))
            for(unsigned j = 0; j < subset.count && (!report || !report->has_fatal); ++j)
                physics_rigid_contact_constraint_solve(&values[j], position_fraction, report);
        *converged &= subset_converged;
        for(unsigned j = 0; j < subset.count; ++j) list->values[indices[j]] = values[j];
        if(report && report->has_fatal) break;
    }
    return true;
}
