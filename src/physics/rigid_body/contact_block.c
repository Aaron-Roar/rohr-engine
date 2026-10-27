/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics/physics_step_internal.h"
#include "physics/physics_operation.h"
#include "systems.h"
#include <math.h>
#include <string.h>

/* Small contact sets can be solved together without allocating. Larger sets,
 * soft boundaries and directional response retain the iterative solver. QR of
 * the mass-weighted Jacobian avoids squaring its condition number, which is
 * essential when a light body transmits a heavy body's load to a static body. */
#define BLOCK_PAIR_MAX 8
#define BLOCK_BODY_MAX (2 * BLOCK_PAIR_MAX)
#define BLOCK_ROW_MAX 16
#define BLOCK_DOF_MAX (3 * BLOCK_BODY_MAX)
#define BLOCK_DEGREES (180.0 / 3.14159265358979323846)

typedef struct BlockRow {
    long double a[BLOCK_DOF_MAX];
    long double scale;
    long double velocity_rhs;
    long double position_rhs;
    unsigned pair, manifold, point;
    bool tangent;
} BlockRow;

typedef struct ContactBlock {
    EntityIndex bodies[BLOCK_BODY_MAX];
    long double weights[BLOCK_DOF_MAX];
    BlockRow rows[BLOCK_ROW_MAX];
    ContactInfo contacts[BLOCK_PAIR_MAX][CONTACT_MANIFOLD_MAX];
    unsigned contact_counts[BLOCK_PAIR_MAX];
    unsigned body_count, row_count;
} ContactBlock;

/* Project zero onto A*x >= b. Active rows are orthogonalized twice; solving
 * the triangular systems gives both the minimum change and the nonnegative
 * contact impulses. Degenerate/inconsistent sets use the existing solver. */
static bool block_project_check(const ContactBlock *block, bool position, bool sticking,
    long double *change, long double *impulses) {
    long double q[BLOCK_ROW_MAX][BLOCK_DOF_MAX] = {{0}};
    long double l[BLOCK_ROW_MAX][BLOCK_ROW_MAX] = {{0}};
    long double y[BLOCK_ROW_MAX], lambda[BLOCK_ROW_MAX];
    unsigned active[BLOCK_ROW_MAX], count = 0;
    unsigned dofs = 3 * block->body_count;
    if(sticking)
        for(unsigned i = 0; i < block->row_count; ++i)
            if(block->rows[i].tangent) active[count++] = i;
    memset(change, 0, dofs * sizeof(*change));
    memset(impulses, 0, block->row_count * sizeof(*impulses));
    for(unsigned pass = 0; pass < 4 * BLOCK_ROW_MAX * BLOCK_ROW_MAX; ++pass) {
        if(count) {
            memset(l, 0, sizeof(l));
            for(unsigned i = 0; i < count; ++i) {
                const BlockRow *row = &block->rows[active[i]];
                memcpy(q[i], row->a, dofs * sizeof(long double));
                for(unsigned repeat = 0; repeat < 2; ++repeat) {
                    for(unsigned j = 0; j < i; ++j) {
                        long double dot = 0;
                        for(unsigned k = 0; k < dofs; ++k) dot += q[i][k] * q[j][k];
                        l[i][j] += dot;
                        for(unsigned k = 0; k < dofs; ++k) q[i][k] -= dot * q[j][k];
                    }
                }
                long double length = 0;
                for(unsigned k = 0; k < dofs; ++k) length += q[i][k] * q[i][k];
                length = sqrtl(length);
                if(!isfinite(length) || length < 1e-12) return false;
                l[i][i] = length;
                for(unsigned k = 0; k < dofs; ++k) q[i][k] /= length;
                y[i] = position ? row->position_rhs : row->velocity_rhs;
                for(unsigned j = 0; j < i; ++j) y[i] -= l[i][j] * y[j];
                y[i] /= length;
            }
            int remove = -1;
            long double lowest = -1e-12;
            for(unsigned reverse = count; reverse > 0; --reverse) {
                unsigned i = reverse - 1;
                lambda[i] = y[i];
                for(unsigned j = i + 1; j < count; ++j)
                    lambda[i] -= l[j][i] * lambda[j];
                lambda[i] /= l[i][i];
                if(!block->rows[active[i]].tangent && lambda[i] < lowest) {
                    lowest = lambda[i]; remove = (int)i;
                }
            }
            if(remove >= 0) {
                for(unsigned i = (unsigned)remove; i + 1 < count; ++i)
                    active[i] = active[i + 1];
                --count;
                memset(change, 0, dofs * sizeof(*change));
                continue;
            }
            memset(change, 0, dofs * sizeof(*change));
            for(unsigned i = 0; i < count; ++i)
                for(unsigned k = 0; k < dofs; ++k) change[k] += q[i][k] * y[i];
        }
        int add = -1;
        long double largest = 1e-12;
        for(unsigned i = 0; i < block->row_count; ++i) {
            if(block->rows[i].tangent) continue;
            bool present = false;
            for(unsigned j = 0; j < count; ++j) present |= active[j] == i;
            if(present) continue;
            const BlockRow *row = &block->rows[i];
            long double violation = position ? row->position_rhs : row->velocity_rhs;
            for(unsigned k = 0; k < dofs; ++k) violation -= row->a[k] * change[k];
            if(violation > largest) { largest = violation; add = (int)i; }
        }
        if(add < 0) {
            for(unsigned i = 0; i < count; ++i)
                impulses[active[i]] = lambda[i] / block->rows[active[i]].scale;
            for(unsigned k = 0; k < dofs; ++k)
                if(!isfinite(change[k])) return false;
            return true;
        }
        if(count == BLOCK_ROW_MAX) return false;
        active[count++] = (unsigned)add;
    }
    return false;
}

static unsigned block_body_add(ContactBlock *block, EntityIndex body) {
    for(unsigned i = 0; i < block->body_count; ++i)
        if(block->bodies[i] == body) return i;
    unsigned slot = block->body_count++;
    block->bodies[slot] = body;
    if(physics_entity_simulated_get(body)) {
        block->weights[3 * slot] = block->weights[3 * slot + 1] = sqrtl(1.0 / mass[body]);
        if(!physics_entity_nonrotating_check(body))
            block->weights[3 * slot + 2] = sqrtl(physics_inverse_inertia_by_index_get(body));
    }
    return slot;
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
        for(unsigned k = 0; k < 3 * block->body_count; ++k) {
            row->a[k] *= factors[k];
            norm += row->a[k] * row->a[k];
        }
        norm = sqrtl(norm);
        for(unsigned k = 0; k < 3 * block->body_count; ++k) row->a[k] /= norm;
        row->scale *= norm;
        row->position_rhs /= norm;
    }
}

static bool block_prepare_check(ContactBlock *block, ContactConstraintList *list,
    Report *report) {
    if(!list->count || list->count > BLOCK_PAIR_MAX) return false;
    for(unsigned i = 0; i < list->count; ++i) {
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
                frictions[first] < 0 || frictions[second] < 0 ||
                !physics_step_hitbox_update(first, report) ||
                !physics_step_hitbox_update(second, report)) return false;
        unsigned slots[] = {block_body_add(block, first), block_body_add(block, second)};
        EntityIndex bodies[] = {first, second};
        ContactManifoldSet manifolds = contact_manifold_set_polygon_get(
            world_hit_boxes[first], world_hit_boxes[second]);
        block->contact_counts[i] = manifolds.count;
        for(unsigned m = 0; m < manifolds.count; ++m) {
            ContactManifold *manifold = &manifolds.values[m];
            ContactInfo *contact = &block->contacts[i][m];
            *contact = (ContactInfo){.detected = true, .normal = manifold->normal,
                .depth = manifold->depth, .point_count = manifold->count};
            for(unsigned p = 0; p < manifold->count; ++p) {
                if(block->row_count == BLOCK_ROW_MAX) return false;
                BlockRow *row = &block->rows[block->row_count];
                row->pair = i; row->manifold = m; row->point = p;
                ContactPointInfo *point = &contact->points[p];
                point->position = manifold->points[p];
                point->feature_id = manifold->feature_ids[p];
                long double vn = 0;
                for(unsigned side = 0; side < 2; ++side) {
                    EntityIndex body = bodies[side];
                    unsigned slot = slots[side];
                    long double sign = side ? 1 : -1;
                    Vec2D r = math_vector_subtract(point->position, physics_com_world_by_index_get(body));
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
                for(unsigned k = 0; k < 3 * block->body_count; ++k)
                    norm += row->a[k] * row->a[k];
                if(norm <= 0) continue;
                row->scale = sqrtl(norm);
                for(unsigned k = 0; k < 3 * block->body_count; ++k) row->a[k] /= row->scale;
                long double rebound = !c->value.rigid.solved && vn < -1 ?
                    -fmin(restitutions[first], restitutions[second]) * vn : 0;
                row->velocity_rhs = (rebound - vn) / row->scale;
                long double correction = manifold->point_depths[p] -
                    physics_rigid_contact_slop_get(first, second, manifold->normal);
                /* Connected contacts are repaired together. Full correction
                 * leaves later passes to catch newly touching neighbors. */
                row->position_rhs = correction / row->scale;
                point->relative_velocity = math_vector_subtract(
                    physics_point_velocity_by_index_get(second, point->position),
                    physics_point_velocity_by_index_get(first, point->position));
                ++block->row_count;
            }
            if(block->row_count == BLOCK_ROW_MAX) return false;
            BlockRow *row = &block->rows[block->row_count];
            row->pair = i; row->manifold = m; row->tangent = true;
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
                Vec2D r = math_vector_subtract(point, physics_com_world_by_index_get(body));
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
            for(unsigned k = 0; k < 3 * block->body_count; ++k) norm += row->a[k] * row->a[k];
            if(norm > 0) {
                row->scale = sqrtl(norm);
                for(unsigned k = 0; k < 3 * block->body_count; ++k) row->a[k] /= row->scale;
                row->velocity_rhs = -vt / row->scale;
                ++block->row_count;
            }
        }
    }
    return true;
}

bool physics_rigid_contact_block_solve_check(ContactConstraintList *list,
    Report *report) {
    ContactBlock block = {0};
    long double velocity[BLOCK_DOF_MAX], position[BLOCK_DOF_MAX];
    long double impulses[BLOCK_ROW_MAX], unused[BLOCK_ROW_MAX];
    if(!block_prepare_check(&block, list, report)) return false;
    bool sticking = block_project_check(&block, false, true, velocity, impulses);
    for(unsigned i = 0; i < block.row_count && sticking; ++i) {
        BlockRow *row = &block.rows[i];
        if(!row->tangent) continue;
        long double normal = 0;
        for(unsigned j = 0; j < block.row_count; ++j)
            if(!block.rows[j].tangent && block.rows[j].pair == row->pair &&
                    block.rows[j].manifold == row->manifold) normal += impulses[j];
        SystemContactConstraint *c = &list->values[row->pair];
        long double mu = sqrtl((long double)frictions[c->value.rigid.first_index] * frictions[c->value.rigid.second_index]);
        if(fabsl(impulses[i]) > mu * normal + 1e-12) sticking = false;
    }
    if(!sticking && !block_project_check(&block, false, false, velocity, impulses)) return false;
    ContactBlock position_block = block;
    block_position_metric_set(&position_block);
    bool position_sticking = sticking && block_project_check(&position_block, true, true, position, unused);
    for(unsigned i = 0; i < position_block.row_count && position_sticking; ++i) {
        BlockRow *row = &position_block.rows[i];
        if(!row->tangent) continue;
        long double normal = 0;
        for(unsigned j = 0; j < block.row_count; ++j)
            if(!block.rows[j].tangent && block.rows[j].pair == row->pair &&
                    block.rows[j].manifold == row->manifold) normal += unused[j];
        SystemContactConstraint *c = &list->values[row->pair];
        long double mu = sqrtl((long double)frictions[c->value.rigid.first_index] * frictions[c->value.rigid.second_index]);
        if(fabsl(unused[i]) > mu * normal + 1e-12) position_sticking = false;
    }
    if(!position_sticking && !block_project_check(&position_block, true, false, position, unused)) return false;
    PhysicsChange changes[BLOCK_BODY_MAX];
    for(unsigned i = 0; i < block.body_count; ++i) {
        changes[i] = physics_change_get(block.bodies[i], PHYSICS_CHANGE_POSITION |
            PHYSICS_CHANGE_ORIENTATION | PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY);
        changes[i].velocity.x += velocity[3 * i] * block.weights[3 * i];
        changes[i].velocity.y += velocity[3 * i + 1] * block.weights[3 * i + 1];
        changes[i].angular_velocity += velocity[3 * i + 2] * block.weights[3 * i + 2] * BLOCK_DEGREES;
        physics_change_orientation_set(&changes[i], changes[i].orientation +
            position[3 * i + 2] * position_block.weights[3 * i + 2] * BLOCK_DEGREES);
        changes[i].position.x += position[3 * i] * position_block.weights[3 * i];
        changes[i].position.y += position[3 * i + 1] * position_block.weights[3 * i + 1];
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
        if(sticking) {
            for(unsigned j = 0; j < block.row_count; ++j)
                if(block.rows[j].tangent && block.rows[j].pair == row->pair &&
                        block.rows[j].manifold == row->manifold) {
                    point->friction_impulse.x -= contact->normal.y * impulses[j] / contact->point_count;
                    point->friction_impulse.y += contact->normal.x * impulses[j] / contact->point_count;
                }
        } else {
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
        physics_step_hitbox_dirty_add(c->value.rigid.first_index);
        physics_step_hitbox_dirty_add(c->value.rigid.second_index);
    }
    return true;
}
