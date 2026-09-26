/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics/physics_step_internal.h"
#include "physics/physics_internal.h"
#include "physics/soft_body/soft_body.h"
#include "systems.h"
#include "math2d.h"
#include <math.h>

static void system_soft_body_beams_apply(void) {
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

        if(!soft_body_beams_pool.used[beam_index] || !entity_index_alive_check(beam_index) ||
                !entity_index_components_check(beam_index, ROHR_SOFT_BODY_BEAM)) continue;
        beam = soft_body_beams[beam_index];
        if(!entity_index_get(beam.node_a, &a) || !entity_index_alive_check(a) ||
                !entity_index_get(beam.node_b, &b) || !entity_index_alive_check(b) ||
                mass[a] <= 0.0f || mass[b] <= 0.0f) continue;
        delta = math_vector_subtract(positions[b], positions[a]);
        length = math_vector_magnitude(delta);
        if(length <= 0.0001f) continue;
        normal = (Vec2D){delta.x / length, delta.y / length};
        relative_velocity = math_vector_subtract(velocities[b], velocities[a]);
        relative_speed = math_dot_product(relative_velocity, normal);
        force_magnitude = beam.stiffness * (length - beam.rest_length) + beam.damping * relative_speed;
        force = (Vec2D){normal.x * force_magnitude, normal.y * force_magnitude};
        force_accelerations[a].x += force.x / mass[a];
        force_accelerations[a].y += force.y / mass[a];
        force_accelerations[b].x -= force.x / mass[b];
        force_accelerations[b].y -= force.y / mass[b];
    }
}

static bool system_soft_boundary_filter_allows(
        const SystemSoftBoundaryQuery *query, EntityIndex rigid) {
    CollisionFilterConfig rigid_filter = physics_collision_filter_config_default_get();
    const SoftBodyBeam *beam;

    if(query == NULL || query->beam_index >= soft_body_beams_pool.capacity ||
            !soft_body_beams_pool.used[query->beam_index]) return false;
    beam = &soft_body_beams[query->beam_index];
    if(entity_index_components_check(rigid, ROHR_COLLISION_FILTER) &&
            rigid < collision_filters_pool.capacity &&
            collision_filters_pool.used[rigid])
        rigid_filter = collision_filters[rigid];
    return (beam->collides_with & rigid_filter.category) != 0 &&
        (rigid_filter.collides_with & beam->category) != 0;
}

static bool system_soft_boundary_node_handles(
        Entity node, EntityIndex node_index, Entity rigid) {
    EntityIndex rigid_index;

    return entity_index_get(rigid, &rigid_index) &&
        entity_index_components_check(node_index, ROHR_COLLISION) &&
        entity_index_components_check(rigid_index, ROHR_COLLISION) &&
        physics_collision_between_check(node, rigid);
}

static Shape system_soft_boundary_pair_shape_get(
        SystemSoftBoundaryQuery *query, Entity rigid) {
    float start_exclusion = 0.0f;
    float end_exclusion = 0.0f;

    if(query == NULL) return (Shape){0};
    if(!query->solving) {
        query->exclude_node_a = system_soft_boundary_node_handles(
            query->node_a, query->a, rigid);
        query->exclude_node_b = system_soft_boundary_node_handles(
            query->node_b, query->b, rigid);
    }
    if(query->exclude_node_a && query->a < soft_body_nodes_pool.capacity &&
            soft_body_nodes_pool.used[query->a])
        start_exclusion = soft_body_nodes[query->a].radius;
    if(query->exclude_node_b && query->b < soft_body_nodes_pool.capacity &&
            soft_body_nodes_pool.used[query->b])
        end_exclusion = soft_body_nodes[query->b].radius;
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

static bool system_soft_boundary_pair_apply(Entity rigid_entity, void *context) {
    SystemSoftBoundaryQuery *query = context;
    EntityIndex rigid;
    OverlapInfo overlap;
    Vec2D edge;
    float t;
    float weight_a;
    float weight_b;
    float inverse_mass_a;
    float inverse_mass_b;
    float inverse_mass_rigid;
    float inverse_mass_edge;
    float inverse_mass_sum;
    Vec2D relative_velocity;
    float normal_velocity;
    float restitution;
    float impulse_magnitude;
    ContactInfo contact;
    ContactInfo previous_contact;

    if(query == NULL) return true;
    if(!query->solving) {
        if(rigid_entity == query->node_a || rigid_entity == query->node_b ||
                !entity_index_get(rigid_entity, &rigid) || !entity_index_alive_check(rigid) ||
                !entity_index_components_check(rigid, ROHR_HIT_BOX | ROHR_COLLISION) ||
                !system_soft_boundary_filter_allows(query, rigid)) return true;
        query->shape = system_soft_boundary_pair_shape_get(query, rigid_entity);
        if(!system_soft_boundary_overlap_get(query, world_hit_boxes[rigid]))
            return true;
        query->rigid_entity = rigid_entity;
        query->rigid = rigid;
        query->solving = true;
        {
            bool appended = contact_constraint_list_append(
                &physics_step_contact_constraints,
                (SystemContactConstraint){
                    .type = SYSTEM_CONTACT_CONSTRAINT_SOFT_BOUNDARY,
                    .value.soft = *query
                });
            query->solving = false;
            return appended;
        }
    }
    rigid_entity = query->rigid_entity;
    rigid = query->rigid;
    if(!entity_index_alive_check(query->a) ||
            !entity_index_alive_check(query->b) ||
            !entity_index_alive_check(query->beam_index) ||
            !entity_index_alive_check(rigid) ||
            query->beam_index >= soft_body_beams_pool.capacity ||
            !soft_body_beams_pool.used[query->beam_index] ||
            !soft_body_beams[query->beam_index].collision_enabled ||
            !entity_index_components_check(rigid, ROHR_HIT_BOX | ROHR_COLLISION)) return true;
    query->start = positions[query->a];
    query->end = positions[query->b];
    query->shape = system_soft_boundary_pair_shape_get(query, rigid_entity);
    if(!system_soft_boundary_overlap_get(query, world_hit_boxes[rigid])) return true;
    overlap = query->overlap;
    edge = math_vector_subtract(query->end, query->start);
    t = query->t;
    previous_contact = query->contact;
    weight_a = 1.0f - t;
    weight_b = t;
    inverse_mass_a = physics_entity_simulated_get(query->a) &&
            entity_index_components_check(query->a, ROHR_MASS) && mass[query->a] > 0.0f
        ? 1.0f / mass[query->a] : 0.0f;
    inverse_mass_b = physics_entity_simulated_get(query->b) &&
            entity_index_components_check(query->b, ROHR_MASS) && mass[query->b] > 0.0f
        ? 1.0f / mass[query->b] : 0.0f;
    inverse_mass_rigid = physics_entity_simulated_get(rigid) &&
            entity_index_components_check(rigid, ROHR_MASS) && mass[rigid] > 0.0f
        ? 1.0f / mass[rigid] : 0.0f;
    inverse_mass_edge = weight_a * weight_a * inverse_mass_a +
        weight_b * weight_b * inverse_mass_b;
    inverse_mass_sum = inverse_mass_edge + inverse_mass_rigid;
    if(inverse_mass_sum <= 0.0f) return true;

    positions[query->a].x -= overlap.normal.x * overlap.depth *
        query->position_fraction *
        weight_a * inverse_mass_a / inverse_mass_sum;
    positions[query->a].y -= overlap.normal.y * overlap.depth *
        query->position_fraction *
        weight_a * inverse_mass_a / inverse_mass_sum;
    positions[query->b].x -= overlap.normal.x * overlap.depth *
        query->position_fraction *
        weight_b * inverse_mass_b / inverse_mass_sum;
    positions[query->b].y -= overlap.normal.y * overlap.depth *
        query->position_fraction *
        weight_b * inverse_mass_b / inverse_mass_sum;
    positions[rigid].x += overlap.normal.x * overlap.depth *
        query->position_fraction *
        inverse_mass_rigid / inverse_mass_sum;
    positions[rigid].y += overlap.normal.y * overlap.depth *
        query->position_fraction *
        inverse_mass_rigid / inverse_mass_sum;

    Velocity rigid_point_velocity = physics_point_velocity_by_index_get(rigid,
        query->contact_position);
    relative_velocity = (Vec2D){
        rigid_point_velocity.x -
            (velocities[query->a].x * weight_a + velocities[query->b].x * weight_b),
        rigid_point_velocity.y -
            (velocities[query->a].y * weight_a + velocities[query->b].y * weight_b)
    };
    normal_velocity = math_dot_product(relative_velocity, overlap.normal);
    contact = (ContactInfo){
        .detected = true,
        .normal = overlap.normal,
        .depth = overlap.depth,
        .points = {{
            .position = query->contact_position,
            .relative_velocity = relative_velocity
        }},
        .point_count = 1
    };
    if(normal_velocity < 0.0f) {
        Vec2D rigid_offset = math_vector_subtract(query->contact_position,
            physics_com_world_by_index_get(rigid));
        Vec2D rigid_angular_velocity = {0};
        Vec2D edge_velocity;
        Vec2D tangent;
        float tangent_length;
        float inverse_inertia_rigid = physics_inverse_inertia_by_index_get(rigid);
        float edge_friction;
        float rigid_friction;
        float friction;
        float tangent_denominator;
        float tangent_impulse_magnitude;
        float maximum_friction;

        restitution = query->solved ? 0.0f : fminf(
            (restitutions_pool.used[query->a] ? restitutions[query->a] : 0.0f) *
                weight_a +
            (restitutions_pool.used[query->b] ? restitutions[query->b] : 0.0f) *
                weight_b,
            restitutions_pool.used[rigid] ? restitutions[rigid] : 0.0f);
        float normal_lever = math_cross_2d(rigid_offset, overlap.normal);
        impulse_magnitude = -(1.0f + restitution) * normal_velocity /
            (inverse_mass_sum + normal_lever * normal_lever * inverse_inertia_rigid);
        angular_velocities[rigid] -= math_radians_to_degrees(normal_lever * impulse_magnitude * inverse_inertia_rigid);
        contact.points[0].normal_impulse = (Vec2D){
            overlap.normal.x * impulse_magnitude,
            overlap.normal.y * impulse_magnitude
        };
        velocities[query->a].x -= contact.points[0].normal_impulse.x *
            weight_a * inverse_mass_a;
        velocities[query->a].y -= contact.points[0].normal_impulse.y *
            weight_a * inverse_mass_a;
        velocities[query->b].x -= contact.points[0].normal_impulse.x *
            weight_b * inverse_mass_b;
        velocities[query->b].y -= contact.points[0].normal_impulse.y *
            weight_b * inverse_mass_b;
        velocities[rigid].x += contact.points[0].normal_impulse.x * inverse_mass_rigid;
        velocities[rigid].y += contact.points[0].normal_impulse.y * inverse_mass_rigid;

        rigid_offset = math_vector_subtract(
            contact.points[0].position, physics_com_world_by_index_get(rigid));
        if(physics_entity_simulated_get(rigid) &&
                !entity_index_components_check(rigid, ROHR_PARTICLE) &&
                entity_index_components_check(rigid, ROHR_MASS | ROHR_HIT_BOX)) {
            inverse_inertia_rigid = physics_inverse_inertia_by_index_get(rigid);
            rigid_angular_velocity = math_angular_velocity_cross_vec(
                angular_velocities[rigid], rigid_offset);
        }
        edge_velocity = (Vec2D){
            velocities[query->a].x * weight_a +
                velocities[query->b].x * weight_b,
            velocities[query->a].y * weight_a +
                velocities[query->b].y * weight_b
        };
        relative_velocity = (Vec2D){
            velocities[rigid].x + rigid_angular_velocity.x - edge_velocity.x,
            velocities[rigid].y + rigid_angular_velocity.y - edge_velocity.y
        };
        {
            float along_normal = math_dot_product(
                relative_velocity, overlap.normal);
            tangent = (Vec2D){
                relative_velocity.x - overlap.normal.x * along_normal,
                relative_velocity.y - overlap.normal.y * along_normal
            };
        }
        tangent_length = math_vector_magnitude(tangent);
        edge_friction =
            (frictions_pool.used[query->a] ? frictions[query->a] : 0.0f) *
                weight_a +
            (frictions_pool.used[query->b] ? frictions[query->b] : 0.0f) *
                weight_b;
        rigid_friction = frictions_pool.used[rigid] ? frictions[rigid] : 0.0f;
        friction = sqrtf(edge_friction * rigid_friction);
        if(tangent_length > 0.0001f && friction > 0.0f) {
            float rigid_lever;

            tangent.x /= tangent_length;
            tangent.y /= tangent_length;
            rigid_lever = math_cross_2d(rigid_offset, tangent);
            tangent_denominator = inverse_mass_edge + inverse_mass_rigid +
                rigid_lever * rigid_lever * inverse_inertia_rigid;
            if(tangent_denominator > 0.0f) {
                tangent_impulse_magnitude = -math_dot_product(
                    relative_velocity, tangent) / tangent_denominator;
                maximum_friction = fabsf(impulse_magnitude) * friction;
                tangent_impulse_magnitude = fmaxf(-maximum_friction,
                    fminf(tangent_impulse_magnitude, maximum_friction));
                contact.points[0].friction_impulse = (Vec2D){
                    tangent.x * tangent_impulse_magnitude,
                    tangent.y * tangent_impulse_magnitude
                };
                velocities[query->a].x -= contact.points[0].friction_impulse.x *
                    weight_a * inverse_mass_a;
                velocities[query->a].y -= contact.points[0].friction_impulse.y *
                    weight_a * inverse_mass_a;
                velocities[query->b].x -= contact.points[0].friction_impulse.x *
                    weight_b * inverse_mass_b;
                velocities[query->b].y -= contact.points[0].friction_impulse.y *
                    weight_b * inverse_mass_b;
                velocities[rigid].x += contact.points[0].friction_impulse.x *
                    inverse_mass_rigid;
                velocities[rigid].y += contact.points[0].friction_impulse.y *
                    inverse_mass_rigid;
                angular_velocities[rigid] -= math_radians_to_degrees(math_cross_2d(
                    rigid_offset, contact.points[0].friction_impulse) *
                    inverse_inertia_rigid);
            }
        }
    }
    query->contact = contact;
    physics_rigid_contact_point_impulses_accumulate(
        &query->contact,
        &previous_contact);
    query->solved = true;
    physics_step_hitbox_dirty_add(query->a);
    physics_step_hitbox_dirty_add(query->b);
    physics_step_hitbox_dirty_add(rigid);
    return true;
}

static void system_soft_body_boundary_collisions_apply(void) {
    for(EntityIndex body_index = 0; body_index < soft_bodies_pool.capacity;
            body_index += 1) {
        SoftBody body;

        if(!soft_bodies_pool.used[body_index] || !entity_index_alive_check(body_index)) continue;
        body = soft_bodies[body_index];
        for(uint32_t i = 0; i < body.beam_count; i += 1) {
            EntityIndex beam_index;
            EntityIndex a;
            EntityIndex b;
            SoftBodyBeam beam;
            SystemSoftBoundaryQuery query;

            if(!entity_index_get(body.beams[i], &beam_index) ||
                    beam_index >= soft_body_beams_pool.capacity ||
                    !soft_body_beams_pool.used[beam_index]) continue;
            beam = soft_body_beams[beam_index];
            if(!beam.collision_enabled ||
                    !entity_index_get(beam.node_a, &a) || !entity_index_alive_check(a) ||
                    !entity_index_get(beam.node_b, &b) || !entity_index_alive_check(b) ||
                    !soft_body_nodes_pool.used[a] || !soft_body_nodes_pool.used[b]) continue;
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
            (void)aabb_tree_query(&physics_broadphase_tree,
                math_aabb_create(query.shape),
                system_soft_boundary_pair_apply, &query);
        }
    }
}

static bool system_soft_node_rigid_filter_allows(EntityIndex node, EntityIndex rigid) {
    CollisionFilterConfig rigid_filter = physics_collision_filter_config_default_get();

    if(entity_index_components_check(rigid, ROHR_COLLISION_FILTER) &&
            rigid < collision_filters_pool.capacity && collision_filters_pool.used[rigid]) {
        rigid_filter = collision_filters[rigid];
    }
    return (soft_body_nodes[node].collides_with & rigid_filter.category) != 0 &&
        (rigid_filter.collides_with & soft_body_nodes[node].category) != 0;
}

static void system_soft_body_node_rigid_collisions_apply(void) {
    for(EntityIndex node = 0; node < soft_body_nodes_pool.capacity; node += 1) {
        Shape node_shape;

        if(!soft_body_nodes_pool.used[node] || !entity_index_alive_check(node) ||
                !entity_index_components_check(node, ROHR_SOFT_BODY_NODE | ROHR_HIT_BOX)) continue;
        node_shape = physics_shape_world_translate(
            math_circle_create(soft_body_nodes[node].radius, 8),
            positions[node],
            0.0f
        );
        for(EntityIndex rigid = 0; rigid < world_hit_boxes_pool.capacity; rigid += 1) {
            OverlapInfo collision;
            float inverse_mass_node;
            float inverse_mass_rigid;
            float inverse_mass_sum;
            Vec2D relative_velocity;
            float normal_velocity;
            float impulse_magnitude;
            float restitution;
            ContactInfo contact_info;

            if(node == rigid || !entity_index_alive_check(rigid) ||
                    !entity_index_components_check(rigid, ROHR_HIT_BOX | ROHR_COLLISION) ||
                    entity_index_components_check(rigid, ROHR_SOFT_BODY_NODE) ||
                    !system_soft_node_rigid_filter_allows(node, rigid)) continue;
            collision = physics_sat_overlap_get(node_shape, world_hit_boxes[rigid]);
            if(!collision.detected) continue;
            Position contact_point = {positions[node].x + collision.normal.x * soft_body_nodes[node].radius,
                positions[node].y + collision.normal.y * soft_body_nodes[node].radius};
            Vec2D normal_offset = math_vector_subtract(contact_point,
                physics_com_world_by_index_get(rigid));
            float normal_inverse_inertia = physics_inverse_inertia_by_index_get(rigid);
            relative_velocity = math_vector_subtract(
                physics_point_velocity_by_index_get(rigid, contact_point),
                physics_point_velocity_by_index_get(node, contact_point));
            contact_info = (ContactInfo){
                .detected = true,
                .normal = collision.normal,
                .depth = collision.depth,
                .points = {{
                    .position = {
                        positions[node].x +
                            collision.normal.x * soft_body_nodes[node].radius,
                        positions[node].y +
                            collision.normal.y * soft_body_nodes[node].radius
                    },
                    .relative_velocity = relative_velocity
                }},
                .point_count = 1
            };
            physics_step_interaction_by_index_record(
                node,
                rigid,
                collision,
                contact_info,
                PHYSICS_INTERACTION_OVERLAP | PHYSICS_INTERACTION_CONTACT
            );
            inverse_mass_node = physics_entity_simulated_get(node) ? 1.0f / mass[node] : 0.0f;
            inverse_mass_rigid = physics_entity_simulated_get(rigid) ? 1.0f / mass[rigid] : 0.0f;
            inverse_mass_sum = inverse_mass_node + inverse_mass_rigid;
            if(inverse_mass_sum <= 0.0f) continue;
            positions[node].x -= collision.normal.x * collision.depth * inverse_mass_node / inverse_mass_sum;
            positions[node].y -= collision.normal.y * collision.depth * inverse_mass_node / inverse_mass_sum;
            positions[rigid].x += collision.normal.x * collision.depth * inverse_mass_rigid / inverse_mass_sum;
            positions[rigid].y += collision.normal.y * collision.depth * inverse_mass_rigid / inverse_mass_sum;
            normal_velocity = math_dot_product(relative_velocity, collision.normal);
            if(normal_velocity >= 0.0f) continue;
            restitution = fminf(
                restitutions_pool.used[node] ? restitutions[node] : 0.25f,
                restitutions_pool.used[rigid] ? restitutions[rigid] : 0.25f
            );
            float normal_lever = math_cross_2d(normal_offset, collision.normal);
            impulse_magnitude = -(1.0f + restitution) * normal_velocity /
                (inverse_mass_sum + normal_lever * normal_lever * normal_inverse_inertia);
            angular_velocities[rigid] -= math_radians_to_degrees(normal_lever * impulse_magnitude * normal_inverse_inertia);
            velocities[node].x -= collision.normal.x * impulse_magnitude * inverse_mass_node;
            velocities[node].y -= collision.normal.y * impulse_magnitude * inverse_mass_node;
            velocities[rigid].x += collision.normal.x * impulse_magnitude * inverse_mass_rigid;
            velocities[rigid].y += collision.normal.y * impulse_magnitude * inverse_mass_rigid;
            contact_info.points[0].normal_impulse = (Vec2D){
                collision.normal.x * impulse_magnitude,
                collision.normal.y * impulse_magnitude
            };
            physics_step_interaction_by_index_record(
                node,
                rigid,
                collision,
                contact_info,
                PHYSICS_INTERACTION_OVERLAP | PHYSICS_INTERACTION_CONTACT
            );
            {
                float node_friction = frictions_pool.used[node] ? frictions[node] : 0.0f;
                float rigid_friction = frictions_pool.used[rigid] ? frictions[rigid] : 0.0f;
                float coefficient = sqrtf(node_friction * rigid_friction);
                Vec2D tangent = {-collision.normal.y, collision.normal.x};
                Vec2D rigid_offset = {
                    positions[node].x + collision.normal.x * soft_body_nodes[node].radius - physics_com_world_by_index_get(rigid).x,
                    positions[node].y + collision.normal.y * soft_body_nodes[node].radius - physics_com_world_by_index_get(rigid).y
                };
                float inverse_inertia_rigid = 0.0f;
                float tangent_speed;
                float tangent_impulse;
                float maximum_friction;
                float denominator;

                if(physics_entity_simulated_get(rigid) &&
                        entity_index_components_check(rigid, ROHR_MASS | ROHR_HIT_BOX)) {
                    inverse_inertia_rigid = physics_inverse_inertia_by_index_get(rigid);
                }
                {
                    Vec2D angular_velocity = math_angular_velocity_cross_vec(
                        angular_velocities[rigid], rigid_offset);
                    relative_velocity = (Vec2D){
                        velocities[rigid].x + angular_velocity.x - velocities[node].x,
                        velocities[rigid].y + angular_velocity.y - velocities[node].y
                    };
                }
                tangent_speed = math_dot_product(relative_velocity, tangent);
                denominator = inverse_mass_sum +
                    powf(math_cross_2d(rigid_offset, tangent), 2.0f) * inverse_inertia_rigid;
                if(coefficient <= 0.0f || denominator <= 0.0f) continue;
                tangent_impulse = -tangent_speed / denominator;
                maximum_friction = fabsf(impulse_magnitude) * coefficient;
                tangent_impulse = fmaxf(-maximum_friction,
                    fminf(tangent_impulse, maximum_friction));
                velocities[node].x -= tangent.x * tangent_impulse * inverse_mass_node;
                velocities[node].y -= tangent.y * tangent_impulse * inverse_mass_node;
                velocities[rigid].x += tangent.x * tangent_impulse * inverse_mass_rigid;
                velocities[rigid].y += tangent.y * tangent_impulse * inverse_mass_rigid;
                angular_velocities[rigid] -= math_radians_to_degrees(math_cross_2d(
                    rigid_offset,
                    (Vec2D){tangent.x * tangent_impulse, tangent.y * tangent_impulse}
                ) * inverse_inertia_rigid);
                contact_info.points[0].friction_impulse = (Vec2D){
                    tangent.x * tangent_impulse,
                    tangent.y * tangent_impulse
                };
                physics_step_interaction_by_index_record(
                    node,
                    rigid,
                    collision,
                    contact_info,
                    PHYSICS_INTERACTION_OVERLAP | PHYSICS_INTERACTION_CONTACT
                );
            }
        }
    }
}

void physics_soft_body_beams_apply(void) {
    system_soft_body_beams_apply();
}

void physics_soft_body_constraints_gather(void) {
    system_soft_body_boundary_collisions_apply();
}

void physics_soft_body_constraint_solve(
    SystemContactConstraint *constraint,
    float position_fraction
) {
    if(constraint == NULL) return;
    constraint->value.soft.position_fraction = position_fraction;
    (void)system_soft_boundary_pair_apply(
        constraint->value.soft.rigid_entity,
        &constraint->value.soft);
}

void physics_soft_body_constraint_finalize(
    const SystemContactConstraint *constraint
) {
    const SystemSoftBoundaryQuery *soft;

    if(constraint == NULL) return;
    soft = &constraint->value.soft;
    physics_step_interaction_by_index_record(
        soft->beam_index, soft->rigid, soft->overlap, soft->contact,
        PHYSICS_INTERACTION_OVERLAP | PHYSICS_INTERACTION_CONTACT);
}
