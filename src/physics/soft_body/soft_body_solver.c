/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics/physics_step_internal.h"
#include "physics/physics_internal.h"
#include "physics/soft_body/soft_body.h"
#include "systems.h"
#include "math2d.h"
#include "physics/physics_operation.h"
#include <math.h>

static void system_soft_body_beams_apply(Report *report) {
    for(EntityIndex beam_index = 0; beam_index < soft_body_beams_pool.capacity; beam_index += 1) {
        SoftBodyBeam beam;
        EntityIndex a;
        EntityIndex b;
        Vec2D delta;
        Vec2D normal;
        Vec2D relative_velocity;
        float length;
        float relative_speed;
        float force_magnitude;
        Vec2D force;

        if(!soft_body_beams_pool.used[beam_index] || !physics_step_entity_alive_check(beam_index) ||
                !physics_step_entity_components_check(beam_index, ROHR_SOFT_BODY_BEAM)) continue;
        beam = soft_body_beams[beam_index];
        if(beam.node_a == ENTITY_INVALID || beam.node_b == ENTITY_INVALID) {
            physics_report_issue_add(report, ROHR_PHYSICS_BEAM_CONTRIBUTION,
                ROHR_REPORT_SEVERITY_WARNING, ERROR_ENGINE_STATE_INVALID,
                ROHR_REPORT_CONTRIBUTION_SKIPPED, physics_operation_entity_get(beam_index), 0,
                "beam endpoint missing; operation skipped; definition retained");
            physics_report_counter_add(report, ROHR_REPORT_SKIPPED_COUNT, 1);
            continue;
        }
        if(!entity_index_get(beam.node_a, &a) || !physics_step_entity_alive_check(a) ||
                !entity_index_get(beam.node_b, &b) || !physics_step_entity_alive_check(b) ||
                mass[a] <= 0.0f || mass[b] <= 0.0f) {
            physics_operation_reject(report, ROHR_PHYSICS_BEAM_CONTRIBUTION,
                ERROR_ENGINE_STATE_INVALID, physics_operation_entity_get(beam_index), beam.node_b);
            continue;
        }
        delta = math_vector_subtract(positions[b], positions[a]);
        length = math_vector_magnitude(delta);
        if(length <= 0.0001f) continue;
        normal = (Vec2D){delta.x / length, delta.y / length};
        relative_velocity = math_vector_subtract(velocities[b], velocities[a]);
        relative_speed = math_dot_product(relative_velocity, normal);
        force_magnitude = beam.stiffness * (length - beam.rest_length) + beam.damping * relative_speed;
        force = (Vec2D){normal.x * force_magnitude, normal.y * force_magnitude};
        PhysicsChange changes[] = {
            physics_change_get(a, PHYSICS_CHANGE_FORCE_ACCELERATION),
            physics_change_get(b, PHYSICS_CHANGE_FORCE_ACCELERATION)
        };
        changes[0].force_acceleration.x += force.x / mass[a];
        changes[0].force_acceleration.y += force.y / mass[a];
        changes[1].force_acceleration.x -= force.x / mass[b];
        changes[1].force_acceleration.y -= force.y / mass[b];
        (void)physics_changes_commit(changes, 2, report, ROHR_PHYSICS_BEAM_CONTRIBUTION,
            physics_operation_entity_get(beam_index), beam.node_b);
    }
}

static bool system_soft_boundary_node_handles(
        EntityIndex node_index, EntityIndex rigid_index, EntityIndex beam_index) {
    return physics_step_entity_components_check(node_index, ROHR_COLLISION) &&
        physics_step_entity_components_check(rigid_index, ROHR_COLLISION) &&
        physics_collision_response_between_by_index_check(node_index, rigid_index) &&
        physics_collision_response_to_by_index_check(node_index, rigid_index) ==
            physics_collision_response_to_by_index_check(beam_index, rigid_index) &&
        physics_collision_response_to_by_index_check(rigid_index, node_index) ==
            physics_collision_response_to_by_index_check(rigid_index, beam_index);
}

static Shape system_soft_boundary_pair_shape_get(
        SystemSoftBoundaryQuery *query, Entity rigid) {
    float start_exclusion = 0.0f;
    float end_exclusion = 0.0f;

    if(query == NULL) return (Shape){0};
    if(query->exclude_node_a && query->a < soft_body_nodes_pool.capacity &&
            soft_body_nodes_pool.used[query->a])
        start_exclusion = soft_body_nodes[query->a].radius;
    if(query->exclude_node_b && query->b < soft_body_nodes_pool.capacity &&
            soft_body_nodes_pool.used[query->b])
        end_exclusion = soft_body_nodes[query->b].radius;
    /* Cutouts cannot affect a target entirely between the endpoint disks.
     * Avoid decomposing those irrelevant concavities: their triangle contacts
     * can otherwise choose a surface far from an interior impact. */
    EntityIndex rigid_index;
    Vec2D direction = math_vector_subtract(query->end, query->start);
    float length = math_vector_magnitude(direction);
    if(length > 0.0001f && entity_index_get(rigid, &rigid_index)) {
        direction.x /= length;
        direction.y /= length;
        Projection target = math_project_shape_on_axis(world_hit_boxes[rigid_index], direction);
        float start = math_dot_product(query->start, direction);
        if(target.min > start + start_exclusion) start_exclusion = 0.0f;
        if(target.max < start + length - end_exclusion) end_exclusion = 0.0f;
    }
    return soft_body_boundary_shape_create(query->start, query->end,
        query->radius, start_exclusion, end_exclusion);
}

static bool system_soft_boundary_overlap_get(
        SystemSoftBoundaryQuery *query, Shape rigid_shape) {
    ContactManifoldSet manifolds;
    const ContactManifold *best = NULL;
    Vec2D edge;
    float edge_length_squared;

    if(query == NULL || query->shape.amount_of_vertices == 0) return false;
    manifolds = contact_manifold_set_polygon_get(query->shape, rigid_shape);
    for(uint8_t i = 0; i < manifolds.count; i += 1)
        if(manifolds.values[i].count > 0 &&
                (best == NULL || manifolds.values[i].depth > best->depth))
            best = &manifolds.values[i];
    if(best == NULL) return false;
    query->overlap = (OverlapInfo){
        .detected = true,
        .normal = best->normal,
        .depth = best->depth
    };
    query->contact_position = (Position){0};
    for(uint8_t i = 0; i < best->count; i += 1) {
        query->contact_position.x += best->points[i].x;
        query->contact_position.y += best->points[i].y;
    }
    query->contact_position.x /= (float)best->count;
    query->contact_position.y /= (float)best->count;
    edge = math_vector_subtract(query->end, query->start);
    edge_length_squared = math_dot_product(edge, edge);
    if(edge_length_squared <= 0.0001f) return false;
    query->t = fmaxf(0.0f, fminf(1.0f,
        math_dot_product(math_vector_subtract(
            query->contact_position, query->start), edge) /
            edge_length_squared));
    return true;
}

static bool system_soft_boundary_pair_apply(Entity rigid_entity, SystemSoftBoundaryQuery *query, Report *report) {
    EntityIndex rigid;
    OverlapInfo overlap;
    float weight_a;
    float weight_b;
    float inverse_mass_a;
    float inverse_mass_b;
    float inverse_mass_rigid;
    float inverse_mass_edge;
    float inverse_mass_sum;
    Vec2D relative_velocity;
    float normal_velocity;
    ContactInfo contact;
    bool overlapping;

    if(query == NULL) return true;
    if(!physics_world_position_check(positions[query->a]) ||
            !physics_world_position_check(positions[query->b]) ||
            !isfinite(query->radius) || query->radius < 0) {
        physics_operation_reject(report, ROHR_PHYSICS_GEOMETRY, ERROR_ENGINE_STATE_INVALID,
            physics_operation_entity_get(query->beam_index), rigid_entity);
        query->overlap = (OverlapInfo){0};
        query->contact = (ContactInfo){0};
        return true;
    }
    if(!query->solving) {
        if(rigid_entity == query->node_a || rigid_entity == query->node_b ||
                !entity_index_get(rigid_entity, &rigid) || !physics_step_entity_alive_check(rigid) ||
                !physics_step_entity_components_check(rigid, ROHR_HIT_BOX) ||
                world_hit_boxes[rigid].amount_of_vertices == 0) return true;
        SystemContactConstraint *existing = NULL;
        for(size_t i = 0; i < physics_step_contact_constraints.count; ++i) {
            SystemContactConstraint *c = &physics_step_contact_constraints.values[i];
            if(c->type != SYSTEM_CONTACT_CONSTRAINT_SOFT_BOUNDARY ||
                    c->value.soft.beam_index != query->beam_index ||
                    c->value.soft.rigid != rigid) continue;
            /* Retain impact targets and accumulated impulses. A sensor-only
             * overlap may become a response contact after another correction. */
            if(c->value.soft.responds ||
                    !physics_step_entity_components_check(rigid, ROHR_COLLISION) ||
                    !physics_collision_response_between_by_index_check(query->beam_index, rigid)) return true;
            existing = c;
            break;
        }
        query->shape = system_soft_boundary_pair_shape_get(query, rigid_entity);
        if(!system_soft_boundary_overlap_get(query, world_hit_boxes[rigid]))
            return true;
        /* Gather the full geometric overlap independently of masks. Only the
         * response shape has cutouts for equivalent endpoint contacts. */
        query->exclude_node_a = system_soft_boundary_node_handles(
            query->a, rigid, query->beam_index);
        query->exclude_node_b = system_soft_boundary_node_handles(
            query->b, rigid, query->beam_index);
        SystemSoftBoundaryQuery response = *query;
        response.shape = system_soft_boundary_pair_shape_get(&response, rigid_entity);
        query->responds = physics_step_entity_components_check(rigid, ROHR_COLLISION) &&
            physics_collision_response_between_by_index_check(query->beam_index, rigid) &&
            system_soft_boundary_overlap_get(&response, world_hit_boxes[rigid]);
        if(query->responds) {
            query->overlap = response.overlap;
            query->contact_position = response.contact_position;
            query->t = response.t;
        }
        query->rigid_entity = rigid_entity;
        query->rigid = rigid;
        query->solving = true;
        {
            bool appended = true;
            if(existing != NULL) {
                if(query->responds) existing->value.soft = *query;
            } else appended = contact_constraint_list_append(
                &physics_step_contact_constraints,
                (SystemContactConstraint){
                    .type = SYSTEM_CONTACT_CONSTRAINT_SOFT_BOUNDARY,
                    .value.soft = *query
                });
            query->solving = false;
            query->exclude_node_a = false;
            query->exclude_node_b = false;
            if(!appended) {
                Entity beam = 0;
                (void)physics_step_entity_from_index_get(query->beam_index, &beam);
                physics_report_issue_add(report, ROHR_PHYSICS_CONTACT_GATHER, ROHR_REPORT_SEVERITY_FATAL,
                    ERROR_MEMORY_POOL_ALLOCATION_FAILED, ROHR_REPORT_UPDATE_ABORTED,
                    beam, rigid_entity, "could not allocate soft boundary contact constraint");
            }
            return appended;
        }
    }
    rigid_entity = query->rigid_entity;
    rigid = query->rigid;
    if(!query->responds) return true;
    /* A saved manual-stage constraint can outlive its target. Check identity
     * once before using indices so slot reuse cannot revive a stale handle. */
    Entity current_rigid;
    if(!physics_step_entity_from_index_get(rigid, &current_rigid) ||
            current_rigid != rigid_entity) return true;
    if(!physics_step_entity_alive_check(query->a) ||
            !physics_step_entity_alive_check(query->b) ||
            !physics_step_entity_alive_check(query->beam_index) ||
            !physics_step_entity_alive_check(rigid) ||
            query->beam_index >= soft_body_beams_pool.capacity ||
            !soft_body_beams_pool.used[query->beam_index] ||
            !soft_body_beams[query->beam_index].collision_enabled ||
            !physics_step_entity_components_check(rigid, ROHR_HIT_BOX | ROHR_COLLISION)) return true;
    if(!physics_step_hitbox_update(rigid, report)) {
        query->overlap = (OverlapInfo){0};
        query->contact = (ContactInfo){0};
        return true;
    }
    bool geometry_unchanged = query->geometry_valid &&
        query->start.x == positions[query->a].x && query->start.y == positions[query->a].y &&
        query->end.x == positions[query->b].x && query->end.y == positions[query->b].y &&
        query->geometry_rigid_position.x == positions[rigid].x &&
        query->geometry_rigid_position.y == positions[rigid].y &&
        query->geometry_rigid_orientation == orientations[rigid];
    if(!geometry_unchanged) {
        query->start = positions[query->a];
        query->end = positions[query->b];
        query->shape = system_soft_boundary_pair_shape_get(query, rigid_entity);
        query->geometry_overlapping = system_soft_boundary_overlap_get(query, world_hit_boxes[rigid]);
        query->geometry_rigid_position = positions[rigid];
        query->geometry_rigid_orientation = orientations[rigid];
        query->geometry_valid = true;
    }
    overlapping = query->geometry_overlapping;
    if(!overlapping && !query->solved) return true;
    overlap = query->overlap;
    if(!overlapping) overlap.depth = 0.0f;
    weight_a = 1.0f - query->t;
    weight_b = query->t;
    bool edge_responds = physics_collision_response_to_by_index_check(query->beam_index, rigid);
    bool rigid_responds = physics_collision_response_to_by_index_check(rigid, query->beam_index);
    if(!edge_responds && !rigid_responds) return true;
    inverse_mass_a = edge_responds && physics_entity_simulated_get(query->a) &&
            physics_step_entity_components_check(query->a, ROHR_MASS) && mass[query->a] > 0.0f
        ? 1.0f / mass[query->a] : 0.0f;
    inverse_mass_b = edge_responds && physics_entity_simulated_get(query->b) &&
            physics_step_entity_components_check(query->b, ROHR_MASS) && mass[query->b] > 0.0f
        ? 1.0f / mass[query->b] : 0.0f;
    inverse_mass_rigid = rigid_responds && physics_entity_simulated_get(rigid) &&
            physics_step_entity_components_check(rigid, ROHR_MASS) && mass[rigid] > 0.0f
        ? 1.0f / mass[rigid] : 0.0f;
    inverse_mass_edge = weight_a * weight_a * inverse_mass_a +
        weight_b * weight_b * inverse_mass_b;
    inverse_mass_sum = inverse_mass_edge + inverse_mass_rigid;
    if(inverse_mass_sum <= 0.0f) return true;

    if(!query->solved) {
        query->velocity_normal = overlap.normal;
        query->velocity_t = query->t;
        query->rigid_offset = math_vector_subtract(query->contact_position,
            physics_com_world_by_index_get(rigid));
    }
    weight_a = 1.0f - query->velocity_t;
    weight_b = query->velocity_t;
    inverse_mass_edge = weight_a * weight_a * inverse_mass_a +
        weight_b * weight_b * inverse_mass_b;
    inverse_mass_sum = inverse_mass_edge + inverse_mass_rigid;
    Vec2D normal = query->velocity_normal;
    Vec2D tangent = {-normal.y, normal.x};
    float inverse_inertia_rigid = inverse_mass_rigid > 0.0f
        ? physics_inverse_inertia_by_index_get(rigid) : 0.0f;
    float normal_lever = math_cross_2d(query->rigid_offset, normal);
    float tangent_lever = math_cross_2d(query->rigid_offset, tangent);
    float normal_denominator = inverse_mass_sum +
        normal_lever * normal_lever * inverse_inertia_rigid;
    float tangent_denominator = inverse_mass_sum +
        tangent_lever * tangent_lever * inverse_inertia_rigid;
    Position rigid_com = physics_com_world_by_index_get(rigid);
    Position contact_position = {rigid_com.x + query->rigid_offset.x,
        rigid_com.y + query->rigid_offset.y};
    Velocity rigid_point_velocity = physics_point_velocity_by_index_get(rigid,
        contact_position);
    Velocity node_velocity_a = physics_point_velocity_by_index_get(query->a,
        positions[query->a]);
    Velocity node_velocity_b = physics_point_velocity_by_index_get(query->b,
        positions[query->b]);
    relative_velocity = (Vec2D){
        rigid_point_velocity.x - node_velocity_a.x * weight_a - node_velocity_b.x * weight_b,
        rigid_point_velocity.y - node_velocity_a.y * weight_a - node_velocity_b.y * weight_b
    };
    normal_velocity = math_dot_product(relative_velocity, normal);
    bool previously_solved = query->solved;
    bool normal_applied = false;
    if(!query->solved) {
        float restitution = fminf(soft_body_beams[query->beam_index].restitution,
            restitutions_pool.used[rigid] ? restitutions[rigid] : 0.0f);
        query->rebound_speed = isfinite(normal_velocity) &&
                isfinite(soft_body_beams[query->beam_index].restitution) &&
                (!restitutions_pool.used[rigid] || isfinite(restitutions[rigid])) ?
            -restitution * fminf(normal_velocity, 0.0f) : NAN;
    }
    contact = (ContactInfo){
        .detected = true,
        .normal = normal,
        .depth = overlap.depth,
        .points = {{.position = contact_position,
            .relative_velocity = query->solved
                ? query->contact.points[0].relative_velocity :
                    (physics_vector_finite_check(relative_velocity) ? relative_velocity : (Velocity){0})}},
        .point_count = 1
    };
    if(!isfinite(normal_denominator) || normal_denominator > 0.0f) {
        float previous_impulse = query->normal_impulse;
        float candidate = previous_impulse + (query->rebound_speed - normal_velocity) / normal_denominator;
        float next_impulse = isfinite(candidate) && isfinite(normal_denominator) ? fmaxf(0.0f, candidate) : NAN;
        float impulse_delta = next_impulse - previous_impulse;
        Vec2D impulse = {normal.x * impulse_delta, normal.y * impulse_delta};
        PhysicsChange changes[] = {
            physics_change_get(query->a, PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY),
            physics_change_get(query->b, PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY),
            physics_change_get(rigid, PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY)
        };
        changes[0].velocity.x -= impulse.x * weight_a * inverse_mass_a;
        changes[0].velocity.y -= impulse.y * weight_a * inverse_mass_a;
        changes[1].velocity.x -= impulse.x * weight_b * inverse_mass_b;
        changes[1].velocity.y -= impulse.y * weight_b * inverse_mass_b;
        changes[2].velocity.x += impulse.x * inverse_mass_rigid;
        changes[2].velocity.y += impulse.y * inverse_mass_rigid;
        changes[2].angular_velocity -= math_radians_to_degrees(
            normal_lever * impulse_delta * inverse_inertia_rigid);
        if(physics_changes_commit(changes, 3, report, ROHR_PHYSICS_NORMAL_IMPULSE,
            physics_operation_entity_get(query->beam_index), rigid_entity)) {
            query->normal_impulse = next_impulse;
            normal_applied = true;
        }
    }
    if(!isfinite(tangent_denominator) || tangent_denominator > 0.0f) {
        float friction = sqrtf(soft_body_beams[query->beam_index].friction *
            (frictions_pool.used[rigid] ? frictions[rigid] : 0.0f));
        float maximum_friction = query->normal_impulse * friction;
        float previous_impulse = query->tangent_impulse;
        rigid_point_velocity = physics_point_velocity_by_index_get(rigid, contact_position);
        node_velocity_a = physics_point_velocity_by_index_get(query->a, positions[query->a]);
        node_velocity_b = physics_point_velocity_by_index_get(query->b, positions[query->b]);
        relative_velocity = (Vec2D){
            rigid_point_velocity.x - node_velocity_a.x * weight_a - node_velocity_b.x * weight_b,
            rigid_point_velocity.y - node_velocity_a.y * weight_a - node_velocity_b.y * weight_b
        };
        float candidate = previous_impulse - math_dot_product(relative_velocity, tangent) / tangent_denominator;
        float next_impulse = isfinite(candidate) && isfinite(maximum_friction) && isfinite(tangent_denominator) ?
            fmaxf(-maximum_friction, fminf(maximum_friction, candidate)) : NAN;
        float impulse_delta = next_impulse - previous_impulse;
        Vec2D impulse = {tangent.x * impulse_delta, tangent.y * impulse_delta};
        PhysicsChange changes[] = {
            physics_change_get(query->a, PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY),
            physics_change_get(query->b, PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY),
            physics_change_get(rigid, PHYSICS_CHANGE_VELOCITY | PHYSICS_CHANGE_ANGULAR_VELOCITY)
        };
        changes[0].velocity.x -= impulse.x * weight_a * inverse_mass_a;
        changes[0].velocity.y -= impulse.y * weight_a * inverse_mass_a;
        changes[1].velocity.x -= impulse.x * weight_b * inverse_mass_b;
        changes[1].velocity.y -= impulse.y * weight_b * inverse_mass_b;
        changes[2].velocity.x += impulse.x * inverse_mass_rigid;
        changes[2].velocity.y += impulse.y * inverse_mass_rigid;
        changes[2].angular_velocity -= math_radians_to_degrees(
            tangent_lever * impulse_delta * inverse_inertia_rigid);
        if(physics_changes_commit(changes, 3, report, ROHR_PHYSICS_FRICTION_IMPULSE,
            physics_operation_entity_get(query->beam_index), rigid_entity))
            query->tangent_impulse = next_impulse;
    }
    contact.points[0].normal_impulse = (Vec2D){
        normal.x * query->normal_impulse, normal.y * query->normal_impulse};
    contact.points[0].friction_impulse = (Vec2D){
        tangent.x * query->tangent_impulse, tangent.y * query->tangent_impulse};
    query->contact = contact;
    query->solved = previously_solved || normal_applied;
    if(!query->solved) query->rebound_speed = 0;

    /* Current geometry drives separation; the impact geometry above drives
     * velocity throughout the solve, even if correction removes the overlap. */
    weight_a = 1.0f - query->t;
    weight_b = query->t;
    /* Overlap repair is geometric. Physical inverse mass remains above for
     * impulses; equal movable weights avoid repeatedly burying a light beam
     * beneath a heavy load supported by another collider. */
    inverse_mass_a = inverse_mass_a > 0.0f ? 1.0f : 0.0f;
    inverse_mass_b = inverse_mass_b > 0.0f ? 1.0f : 0.0f;
    inverse_mass_rigid = inverse_mass_rigid > 0.0f ? 1.0f : 0.0f;
    inverse_mass_sum = weight_a * weight_a * inverse_mass_a +
        weight_b * weight_b * inverse_mass_b + inverse_mass_rigid;
    PhysicsChange changes[] = {
        physics_change_get(query->a, PHYSICS_CHANGE_POSITION),
        physics_change_get(query->b, PHYSICS_CHANGE_POSITION),
        physics_change_get(rigid, PHYSICS_CHANGE_POSITION)
    };
    changes[0].position.x -= overlap.normal.x * overlap.depth *
        query->position_fraction *
        weight_a * inverse_mass_a / inverse_mass_sum;
    changes[0].position.y -= overlap.normal.y * overlap.depth *
        query->position_fraction *
        weight_a * inverse_mass_a / inverse_mass_sum;
    changes[1].position.x -= overlap.normal.x * overlap.depth *
        query->position_fraction *
        weight_b * inverse_mass_b / inverse_mass_sum;
    changes[1].position.y -= overlap.normal.y * overlap.depth *
        query->position_fraction *
        weight_b * inverse_mass_b / inverse_mass_sum;
    changes[2].position.x += overlap.normal.x * overlap.depth *
        query->position_fraction *
        inverse_mass_rigid / inverse_mass_sum;
    changes[2].position.y += overlap.normal.y * overlap.depth *
        query->position_fraction *
        inverse_mass_rigid / inverse_mass_sum;

    (void)physics_changes_commit(changes, 3, report, ROHR_PHYSICS_POSITION_CORRECTION,
            physics_operation_entity_get(query->beam_index), rigid_entity);
    return true;
}

typedef struct SoftBoundaryGatherContext {
    SystemSoftBoundaryQuery *query;
    Report *report;
    bool unchanged_beam;
} SoftBoundaryGatherContext;

static bool boundary_gather_cached;
static Position boundary_gather_starts[MAX_ENTITIES], boundary_gather_ends[MAX_ENTITIES];
static Position boundary_gather_rigid_positions[MAX_ENTITIES];
static Orientation boundary_gather_rigid_orientations[MAX_ENTITIES];
static bool boundary_gather_rigid_changed[MAX_ENTITIES];

void physics_soft_body_gather_cache_reset(void) { boundary_gather_cached = false; }

static bool system_soft_boundary_gather(Entity entity, void *context) {
    SoftBoundaryGatherContext *gather = context;
    EntityIndex rigid;
    if(gather->unchanged_beam && entity_index_get(entity, &rigid) &&
            !boundary_gather_rigid_changed[rigid]) return true;
    return system_soft_boundary_pair_apply(entity, gather->query, gather->report);
}

static void system_soft_body_boundary_collisions_apply(Report *report) {
    bool cached = physics_step_contact_constraints.solving && boundary_gather_cached;
    bool rigid_changed = false;
    if(cached) {
        for(uint32_t n = 0; n < physics_step_collider_iteration_count_get(); ++n) {
            EntityIndex i;
            if(!physics_step_collider_index_at(n, &i)) continue;
            boundary_gather_rigid_changed[i] =
                boundary_gather_rigid_positions[i].x != positions[i].x ||
                boundary_gather_rigid_positions[i].y != positions[i].y ||
                boundary_gather_rigid_orientations[i] != orientations[i];
            rigid_changed |= boundary_gather_rigid_changed[i] &&
                physics_step_entity_components_check(i, ROHR_HIT_BOX);
        }
    }
    for(EntityIndex beam_index = 0; beam_index < soft_body_beams_pool.capacity; ++beam_index) {
        if(!soft_body_beams_pool.used[beam_index] || !physics_step_entity_alive_check(beam_index)) continue;
        {
            EntityIndex a;
            EntityIndex b;
            SoftBodyBeam beam;
            SystemSoftBoundaryQuery query;

            beam = soft_body_beams[beam_index];
            if(!beam.collision_enabled ||
                    !entity_index_get(beam.node_a, &a) || !physics_step_entity_alive_check(a) ||
                    !entity_index_get(beam.node_b, &b) || !physics_step_entity_alive_check(b) ||
                    !soft_body_nodes_pool.used[a] || !soft_body_nodes_pool.used[b]) continue;
            bool unchanged = cached &&
                boundary_gather_starts[beam_index].x == positions[a].x &&
                boundary_gather_starts[beam_index].y == positions[a].y &&
                boundary_gather_ends[beam_index].x == positions[b].x &&
                boundary_gather_ends[beam_index].y == positions[b].y;
            boundary_gather_starts[beam_index] = positions[a];
            boundary_gather_ends[beam_index] = positions[b];
            if(unchanged && !rigid_changed) continue;
            query = (SystemSoftBoundaryQuery){
                .node_a = beam.node_a,
                .node_b = beam.node_b,
                .beam_index = beam_index,
                .a = a,
                .b = b,
                .start = positions[a],
                .end = positions[b],
                .radius = beam.collision_thickness * 0.5f
            };
            query.shape = soft_body_boundary_shape_create(
                query.start, query.end, query.radius, 0.0f, 0.0f);
            if(query.shape.amount_of_vertices == 0) continue;
            SoftBoundaryGatherContext context = {&query, report, unchanged};
            (void)aabb_tree_query(&physics_broadphase_tree,
                math_aabb_create(query.shape), system_soft_boundary_gather, &context);
            if(report->has_fatal) return;
        }
    }
    for(uint32_t n = 0; n < physics_step_collider_iteration_count_get(); ++n) {
        EntityIndex i;
        if(!physics_step_collider_index_at(n, &i)) continue;
        boundary_gather_rigid_positions[i] = positions[i];
        boundary_gather_rigid_orientations[i] = orientations[i];
    }
    boundary_gather_cached = physics_step_contact_constraints.solving;
}

void physics_soft_body_beams_apply(Report *report) {
    system_soft_body_beams_apply(report);
}

void physics_soft_body_constraints_gather(Report *report) {
    system_soft_body_boundary_collisions_apply(report);
    if(!report->has_fatal) physics_beam_pairs_gather(report);
}

void physics_soft_body_constraint_solve(
    SystemContactConstraint *constraint,
    float position_fraction, Report *report
) {
    if(constraint == NULL) return;
    /* Soft endpoints can be shared by several contacts. Apply separation on
     * every pass so later contacts do not leave most of their correction until
     * the final pass, when earlier contacts can no longer respond. */
    constraint->value.soft.position_fraction = 1.0f;
    (void)position_fraction;
    (void)system_soft_boundary_pair_apply(
        constraint->value.soft.rigid_entity,
        &constraint->value.soft, report);
}

void physics_soft_body_constraint_finalize(
    const SystemContactConstraint *constraint, Report *report
) {
    const SystemSoftBoundaryQuery *soft;

    if(constraint == NULL) return;
    soft = &constraint->value.soft;
    if(!soft->overlap.detected) return;
    physics_step_interaction_by_index_record(
        soft->beam_index, soft->rigid, soft->overlap, soft->contact,
        PHYSICS_INTERACTION_OVERLAP |
            (soft->contact.detected ? PHYSICS_INTERACTION_CONTACT : 0), report);
}
