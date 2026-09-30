/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics.h"

#include "physics/physics_internal.h"
#include "math2d.h"

#include <math.h>

static Vec2D physics_soft_body_vector_add(Vec2D first, Vec2D second) {
    return (Vec2D){first.x + second.x, first.y + second.y};
}

static void physics_soft_body_entity_list_remove(
        Entity *values, uint32_t *count, Entity entity) {
    if(values == NULL || count == NULL) return;
    for(uint32_t i = 0; i < *count; i += 1) {
        if(values[i] != entity) continue;
        values[i] = values[*count - 1];
        values[*count - 1] = ENTITY_INVALID;
        *count -= 1;
        return;
    }
}

EngineResult physics_soft_body_origin_position_set(
        EntityIndex body_index, Position position) {
    Vec2D translation;
    SoftBody *body;
    if(body_index >= soft_bodies_pool.capacity ||
            !soft_bodies_pool.used[body_index] ||
            !positions_pool.used[body_index]) return error_result_value(true);
    body = &soft_bodies[body_index];
    translation = math_vector_subtract(position, positions[body_index]);
    for(uint32_t i = 0; i < body->node_count; i += 1) {
        EntityIndex node_index;
        Position moved;
        if(!entity_index_get(body->nodes[i], &node_index) ||
                !positions_pool.used[node_index]) continue;
        moved = physics_soft_body_vector_add(positions[node_index], translation);
        if(!physics_world_position_check(moved))
            return error_result_error(ERROR_ENGINE_POSITION_OUT_OF_RANGE);
    }
    for(uint32_t i = 0; i < body->node_count; i += 1) {
        EntityIndex node_index;
        if(!entity_index_get(body->nodes[i], &node_index) ||
                !positions_pool.used[node_index]) continue;
        positions[node_index] = physics_soft_body_vector_add(
            positions[node_index], translation);
    }
    return error_result_value(true);
}

EngineResult physics_soft_body_origin_orientation_set(
        EntityIndex body_index, Orientation orientation) {
    Orientation prior;
    Orientation delta;
    SoftBody *body;
    if(body_index >= soft_bodies_pool.capacity ||
            !soft_bodies_pool.used[body_index] ||
            !positions_pool.used[body_index] ||
            !orientations_pool.used[body_index]) return error_result_value(true);
    body = &soft_bodies[body_index];
    prior = orientations[body_index];
    delta = orientation - prior;
    for(uint32_t i = 0; i < body->node_count; i += 1) {
        EntityIndex node_index;
        Vec2D offset;
        Position rotated;
        if(!entity_index_get(body->nodes[i], &node_index) ||
                !positions_pool.used[node_index]) continue;
        offset = math_vector_subtract(positions[node_index], positions[body_index]);
        offset = math_vector_rotate(offset, delta);
        rotated = physics_soft_body_vector_add(positions[body_index], offset);
        if(!physics_world_position_check(rotated))
            return error_result_error(ERROR_ENGINE_POSITION_OUT_OF_RANGE);
    }
    for(uint32_t i = 0; i < body->node_count; i += 1) {
        EntityIndex node_index;
        Vec2D offset;
        if(!entity_index_get(body->nodes[i], &node_index) ||
                !positions_pool.used[node_index]) continue;
        offset = math_vector_subtract(positions[node_index], positions[body_index]);
        positions[node_index] = physics_soft_body_vector_add(positions[body_index],
            math_vector_rotate(offset, delta));
        if(velocities_pool.used[node_index])
            velocities[node_index] = math_vector_rotate(velocities[node_index], delta);
        if(accelerations_pool.used[node_index])
            accelerations[node_index] =
                math_vector_rotate(accelerations[node_index], delta);
    }
    return error_result_value(true);
}

void physics_soft_body_entity_clear(Entity entity, EntityIndex index) {
    /* Deleting referenced geometry invalidates its surfaces, never retargets IDs. */
    for(EntityIndex i=0;i<soft_body_areas_pool.capacity;i++) {
        if(!soft_body_areas_pool.used[i]) continue;
        const SoftBodyArea *area=&soft_body_areas[i]; bool referenced=false;
        for(size_t k=0;k<area->boundary_count;k++) {
            const AreaBoundaryPoint *p=&area->boundary[k];
            for(size_t n=0;n<(p->beams[0]?4:1);n++) referenced|=p->nodes[n]==entity;

        }
        if(referenced) {
            EntityResult id=entity_from_index_get(i);
            if(id.kind==ERROR_RESULT_VALUE) (void)entity_delete(id.result.value);
        }
    }
    if(index<soft_body_areas_pool.capacity && soft_body_areas_pool.used[index]) {
        EntityIndex owner;
        if(entity_index_get(soft_body_areas[index].soft_body,&owner) && soft_bodies_pool.used[owner])
            physics_soft_body_entity_list_remove(soft_bodies[owner].areas,
                &soft_bodies[owner].area_count,entity);
        free((void*)soft_body_areas[index].boundary);
        (void)SoftBodyAreaPool_release_at(&soft_body_areas_pool,index);
    }

    if(index < soft_bodies_pool.capacity && soft_bodies_pool.used[index]) {
        SoftBody body = soft_bodies[index];

        for(uint32_t i = 0; i < body.area_count; i++)
            if(entity_alive_check(body.areas[i])) (void)entity_delete(body.areas[i]);
        for(uint32_t i = 0; i < body.triangle_count; i += 1)
            if(entity_alive_check(body.triangles[i]))
                (void)entity_delete(body.triangles[i]);
        for(uint32_t i = 0; i < body.beam_count; i += 1)
            if(entity_alive_check(body.beams[i]))
                (void)entity_delete(body.beams[i]);
        for(uint32_t i = 0; i < body.node_count; i += 1)
            if(entity_alive_check(body.nodes[i]))
                (void)entity_delete(body.nodes[i]);
        (void)SoftBodyPool_release_at(&soft_bodies_pool, index);
    }
    if(index < soft_body_nodes_pool.capacity &&
            soft_body_nodes_pool.used[index]) {
        EntityIndex body_index;
        Entity owner = soft_body_nodes[index].soft_body;

        for(EntityIndex connected = 0;
                connected < soft_body_beams_pool.capacity;
                connected += 1) {
            EntityResult connected_entity;

            if(!soft_body_beams_pool.used[connected] ||
                    (soft_body_beams[connected].node_a != entity &&
                        soft_body_beams[connected].node_b != entity)) continue;
            connected_entity = entity_from_index_get(connected);
            if(connected_entity.kind == ERROR_RESULT_VALUE)
                (void)entity_delete(connected_entity.result.value);
        }
        for(EntityIndex connected = 0;
                connected < soft_body_triangles_pool.capacity;
                connected += 1) {
            SoftBodyTriangle triangle;
            EntityResult connected_entity;

            if(!soft_body_triangles_pool.used[connected]) continue;
            triangle = soft_body_triangles[connected];
            if(triangle.node_a != entity &&
                    triangle.node_b != entity &&
                    triangle.node_c != entity) continue;
            connected_entity = entity_from_index_get(connected);
            if(connected_entity.kind == ERROR_RESULT_VALUE)
                (void)entity_delete(connected_entity.result.value);
        }
        if(entity_index_get(owner, &body_index) &&
                body_index < soft_bodies_pool.capacity &&
                soft_bodies_pool.used[body_index])
            physics_soft_body_entity_list_remove(
                soft_bodies[body_index].nodes,
                &soft_bodies[body_index].node_count,
                entity);
        (void)SoftBodyNodePool_release_at(&soft_body_nodes_pool, index);
    }
    if(index < soft_body_beams_pool.capacity &&
            soft_body_beams_pool.used[index]) {
        EntityIndex body_index;
        Entity owner = soft_body_beams[index].soft_body;

        if(entity_index_get(owner, &body_index) &&
                body_index < soft_bodies_pool.capacity &&
                soft_bodies_pool.used[body_index])
            physics_soft_body_entity_list_remove(
                soft_bodies[body_index].beams,
                &soft_bodies[body_index].beam_count,
                entity);
        (void)SoftBodyBeamPool_release_at(&soft_body_beams_pool, index);
    }
    if(index < soft_body_triangles_pool.capacity &&
            soft_body_triangles_pool.used[index]) {
        EntityIndex body_index;
        Entity owner = soft_body_triangles[index].soft_body;

        if(entity_index_get(owner, &body_index) &&
                body_index < soft_bodies_pool.capacity &&
                soft_bodies_pool.used[body_index])
            physics_soft_body_entity_list_remove(
                soft_bodies[body_index].triangles,
                &soft_bodies[body_index].triangle_count,
                entity);
        (void)SoftBodyTrianglePool_release_at(
            &soft_body_triangles_pool, index);
    }
}

EntityResult physics_soft_body_create(void) {
    EntityResult result = entity_add();
    EntityIndex index;

    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(!entity_index_get(result.result.value, &index) ||
            physics_position_set(result.result.value, (Position){0}).kind ==
                ERROR_RESULT_ERROR ||
            physics_orientation_set(result.result.value, 0.0f).kind ==
                ERROR_RESULT_ERROR ||
            SoftBodyPool_store_at(&soft_bodies_pool, index,
                (SoftBody){.origin = result.result.value}).kind ==
                ERROR_RESULT_ERROR) {
        (void)entity_delete(result.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    entity_mask[index] |= ROHR_SOFT_BODY;
    return result;
}

SoftBodyResult physics_soft_body_get(Entity soft_body) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(soft_body, &index);

    if(result.kind == ERROR_RESULT_ERROR) return ERROR_RESULT_MAKE_ERROR(SoftBodyResult, result.result.error);
    if(!entity_index_components_check(index, ROHR_SOFT_BODY) || !soft_bodies_pool.used[index]) {
        return ERROR_RESULT_MAKE_ERROR(SoftBodyResult, ERROR_ENGINE_COMPONENT_MISSING);
    }
    return ERROR_RESULT_MAKE_VALUE(SoftBodyResult, soft_bodies[index]);
}

EntityResult physics_soft_body_node_create(Entity soft_body, Position position,
        Mass node_mass, float radius) {
    EntityIndex body_index;
    EntityIndex node_index;
    EntityResult node;
    EngineResult result = physics_live_index_get(soft_body, &body_index);

    if(result.kind == ERROR_RESULT_ERROR || !entity_index_components_check(body_index, ROHR_SOFT_BODY) ||
            !soft_bodies_pool.used[body_index] || node_mass <= 0.0f || radius <= 0.0f) {
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    }
    if(soft_bodies[body_index].node_count >= SOFT_BODY_MAX_NODES) {
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_MAX_ENTITIES_EXCEEDED);
    }
    node = entity_add();
    if(node.kind == ERROR_RESULT_ERROR) return node;
    if(!entity_index_get(node.result.value, &node_index) ||
            physics_position_set(node.result.value, position).kind == ERROR_RESULT_ERROR ||
            physics_mass_set(node.result.value, node_mass).kind == ERROR_RESULT_ERROR ||
            physics_velocity_set(node.result.value, (Velocity){0}).kind == ERROR_RESULT_ERROR ||
            physics_acceleration_set(node.result.value, (Acceleration){0}).kind == ERROR_RESULT_ERROR ||
            physics_dynamic_set(node.result.value).kind == ERROR_RESULT_ERROR ||
            physics_hitbox_set(node.result.value,
                math_circle_create(radius, 8)).kind == ERROR_RESULT_ERROR ||
            physics_restitution_set(node.result.value, 0.25f).kind == ERROR_RESULT_ERROR ||
            physics_friction_set(node.result.value, 0.0f).kind == ERROR_RESULT_ERROR ||
            physics_collision_filter_set(node.result.value, (CollisionFilterConfig){
                .category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .collides_with = ROHR_COLLISION_CATEGORY_ALL &
                    ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE
            }).kind == ERROR_RESULT_ERROR ||
            SoftBodyNodePool_store_at(&soft_body_nodes_pool, node_index, (SoftBodyNode){
                .soft_body = soft_body,
                .radius = radius,
                .category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .collides_with = ROHR_COLLISION_CATEGORY_ALL &
                    ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE
            }).kind == ERROR_RESULT_ERROR) {
        (void)entity_delete(node.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    entity_mask[node_index] |= ROHR_SOFT_BODY_NODE | ROHR_PARTICLE;
    soft_bodies[body_index].nodes[soft_bodies[body_index].node_count++] = node.result.value;
    return node;
}

EntityResult physics_soft_body_node_local_create(Entity soft_body,
        Position local_position, Mass node_mass, float radius) {
    EntityIndex body_index;
    EngineResult result = physics_live_index_get(soft_body, &body_index);
    Position world_position;
    if(result.kind == ERROR_RESULT_ERROR ||
            !entity_index_components_check(body_index, ROHR_SOFT_BODY) ||
            !soft_bodies_pool.used[body_index] ||
            !positions_pool.used[body_index] ||
            !orientations_pool.used[body_index])
        return ERROR_RESULT_MAKE_ERROR(EntityResult,
            ERROR_ENGINE_COMPONENT_MISSING);
    world_position = physics_soft_body_vector_add(positions[body_index],
        math_vector_rotate(local_position, orientations[body_index]));
    return physics_soft_body_node_create(
        soft_body, world_position, node_mass, radius);
}

SoftBodyNodeResult physics_soft_body_node_get(Entity node) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(node, &index);

    if(result.kind == ERROR_RESULT_ERROR) return ERROR_RESULT_MAKE_ERROR(SoftBodyNodeResult, result.result.error);
    if(!entity_index_components_check(index, ROHR_SOFT_BODY_NODE) || !soft_body_nodes_pool.used[index]) {
        return ERROR_RESULT_MAKE_ERROR(SoftBodyNodeResult, ERROR_ENGINE_COMPONENT_MISSING);
    }
    return ERROR_RESULT_MAKE_VALUE(SoftBodyNodeResult, soft_body_nodes[index]);
}

PositionResult physics_soft_body_node_local_position_get(Entity node) {
    EntityIndex node_index;
    EntityIndex body_index;
    EngineResult result = physics_live_index_get(node, &node_index);
    Vec2D offset;
    if(result.kind == ERROR_RESULT_ERROR)
        return ERROR_RESULT_MAKE_ERROR(PositionResult, result.result.error);
    if(!entity_index_components_check(node_index, ROHR_SOFT_BODY_NODE) ||
            !soft_body_nodes_pool.used[node_index] ||
            !positions_pool.used[node_index] ||
            !entity_index_get(soft_body_nodes[node_index].soft_body, &body_index) ||
            !positions_pool.used[body_index] ||
            !orientations_pool.used[body_index])
        return ERROR_RESULT_MAKE_ERROR(PositionResult,
            ERROR_ENGINE_COMPONENT_MISSING);
    offset = math_vector_subtract(positions[node_index], positions[body_index]);
    return ERROR_RESULT_MAKE_VALUE(PositionResult,
        math_vector_rotate(offset, -orientations[body_index]));
}

EngineResult physics_soft_body_node_local_position_set(
        Entity node, Position local_position) {
    EntityIndex node_index;
    EntityIndex body_index;
    EngineResult result = physics_live_index_get(node, &node_index);
    Position world_position;
    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(!entity_index_components_check(node_index, ROHR_SOFT_BODY_NODE) ||
            !soft_body_nodes_pool.used[node_index] ||
            !entity_index_get(soft_body_nodes[node_index].soft_body, &body_index) ||
            !positions_pool.used[body_index] ||
            !orientations_pool.used[body_index])
        return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    world_position = physics_soft_body_vector_add(positions[body_index],
        math_vector_rotate(local_position, orientations[body_index]));
    return physics_position_set(node, world_position);
}

EngineResult physics_soft_body_node_collision_filter_set(Entity node,
        RohrCollisionCategoryMask category, RohrCollisionCategoryMask collides_with) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(node, &index);

    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(!entity_index_components_check(index, ROHR_SOFT_BODY_NODE) || !soft_body_nodes_pool.used[index]) {
        return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    }
    result = physics_collision_filter_set(node, (CollisionFilterConfig){
        .category = category,
        .collides_with = collides_with
    });
    if(result.kind == ERROR_RESULT_ERROR) return result;
    soft_body_nodes[index].category = category;
    soft_body_nodes[index].collides_with = collides_with;
    return result;
}

EngineResult physics_soft_body_node_force_apply(Entity node, Force force) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(node, &index);

    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(!entity_index_components_check(index, ROHR_SOFT_BODY_NODE)) {
        return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    }
    return physics_force_apply(node, force);
}

EngineResult physics_soft_body_node_impulse_apply(Entity node, Vec2D impulse) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(node, &index);

    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(!entity_index_components_check(index, ROHR_SOFT_BODY_NODE)) {
        return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    }
    return physics_impulse_apply(node, impulse);
}

EngineResult physics_soft_body_force_apply(Entity soft_body, Force force) {
    SoftBodyResult body_result = physics_soft_body_get(soft_body);
    SoftBody body;
    float total_mass = 0.0f;

    if(body_result.kind == ERROR_RESULT_ERROR) return error_result_error(body_result.result.error);
    body = body_result.result.value;
    for(uint32_t i = 0; i < body.node_count; i += 1) {
        EntityIndex index;
        if(!entity_index_get(body.nodes[i], &index) || !entity_index_alive_check(index) || mass[index] <= 0.0f) {
            return error_result_error(ERROR_ENGINE_ENTITY_NOT_FOUND);
        }
        total_mass += mass[index];
    }
    if(total_mass <= 0.0f) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    for(uint32_t i = 0; i < body.node_count; i += 1) {
        EntityIndex index;
        EngineResult result;
        (void)entity_index_get(body.nodes[i], &index);
        result = physics_soft_body_node_force_apply(body.nodes[i], (Force){
            .x = force.x * mass[index] / total_mass,
            .y = force.y * mass[index] / total_mass
        });
        if(result.kind == ERROR_RESULT_ERROR) return result;
    }
    return error_result_value(true);
}

EngineResult physics_soft_body_torque_apply(Entity soft_body, Torque torque) {
    SoftBodyResult body_result = physics_soft_body_get(soft_body);
    SoftBody body;
    Position center = {0};
    float total_mass = 0.0f;
    float weighted_radius_squared = 0.0f;

    if(body_result.kind == ERROR_RESULT_ERROR) return error_result_error(body_result.result.error);
    body = body_result.result.value;
    for(uint32_t i = 0; i < body.node_count; i += 1) {
        EntityIndex index;
        if(!entity_index_get(body.nodes[i], &index) || !entity_index_alive_check(index) || mass[index] <= 0.0f) {
            return error_result_error(ERROR_ENGINE_ENTITY_NOT_FOUND);
        }
        center.x += positions[index].x * mass[index];
        center.y += positions[index].y * mass[index];
        total_mass += mass[index];
    }
    if(total_mass <= 0.0f) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    center.x /= total_mass;
    center.y /= total_mass;
    for(uint32_t i = 0; i < body.node_count; i += 1) {
        EntityIndex index;
        Vec2D offset;
        (void)entity_index_get(body.nodes[i], &index);
        offset = math_vector_subtract(positions[index], center);
        weighted_radius_squared += mass[index] * math_dot_product(offset, offset);
    }
    if(weighted_radius_squared <= 0.0001f) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    for(uint32_t i = 0; i < body.node_count; i += 1) {
        EntityIndex index;
        Vec2D offset;
        float scale;
        EngineResult result;
        (void)entity_index_get(body.nodes[i], &index);
        offset = math_vector_subtract(positions[index], center);
        scale = torque * mass[index] / weighted_radius_squared;
        result = physics_soft_body_node_force_apply(body.nodes[i], (Force){
            .x = offset.y * scale,
            .y = -offset.x * scale
        });
        if(result.kind == ERROR_RESULT_ERROR) return result;
    }
    return error_result_value(true);
}

SoftBodyNodeAnchorPinResult physics_soft_body_node_to_anchor_pin_create(
        Entity node, JointAnchorId anchor) {
    EntityIndex node_index;
    JointAnchorIdResult node_anchor;
    EntityResult joint;
    EngineResult pin_result;

    if(physics_live_index_get(node, &node_index).kind == ERROR_RESULT_ERROR ||
            !entity_index_components_check(node_index, ROHR_SOFT_BODY_NODE) ||
            !soft_body_nodes_pool.used[node_index]) {
        return ERROR_RESULT_MAKE_ERROR(SoftBodyNodeAnchorPinResult, ERROR_ENGINE_COMPONENT_MISSING);
    }
    if(physics_joint_anchor_world_position_get(anchor).kind == ERROR_RESULT_ERROR) {
        return ERROR_RESULT_MAKE_ERROR(SoftBodyNodeAnchorPinResult, ERROR_ENGINE_ENTITY_NOT_FOUND);
    }
    node_anchor = physics_joint_anchor_create(node, (Vec2D){0.0f, 0.0f});
    if(node_anchor.kind == ERROR_RESULT_ERROR) {
        return ERROR_RESULT_MAKE_ERROR(SoftBodyNodeAnchorPinResult, node_anchor.result.error);
    }
    joint = entity_add();
    if(joint.kind == ERROR_RESULT_ERROR) {
        (void)physics_joint_anchor_remove(node_anchor.result.value);
        return ERROR_RESULT_MAKE_ERROR(SoftBodyNodeAnchorPinResult, joint.result.error);
    }
    pin_result = physics_joint_pin_set(joint.result.value, node_anchor.result.value, anchor);
    if(pin_result.kind == ERROR_RESULT_ERROR) {
        (void)entity_delete(joint.result.value);
        (void)physics_joint_anchor_remove(node_anchor.result.value);
        return ERROR_RESULT_MAKE_ERROR(SoftBodyNodeAnchorPinResult, pin_result.result.error);
    }
    return ERROR_RESULT_MAKE_VALUE(SoftBodyNodeAnchorPinResult, ((SoftBodyNodeAnchorPin){
        .joint = joint.result.value,
        .node_anchor = node_anchor.result.value
    }));
}

EntityResult physics_soft_body_beam_create(Entity soft_body, Entity node_a, Entity node_b,
        float stiffness, float damping) {
    EntityIndex body_index;
    EntityIndex a_index;
    EntityIndex b_index;
    EntityIndex beam_index;
    EntityResult beam;
    Vec2D delta;
    float collision_thickness;
    CollisionFilterConfig filter_a;
    CollisionFilterConfig filter_b;
    CollisionFilterConfigResult filter_result;
    bool collision_enabled;

    if(physics_live_index_get(soft_body, &body_index).kind == ERROR_RESULT_ERROR ||
            physics_live_index_get(node_a, &a_index).kind == ERROR_RESULT_ERROR ||
            physics_live_index_get(node_b, &b_index).kind == ERROR_RESULT_ERROR || node_a == node_b ||
            !entity_index_components_check(body_index, ROHR_SOFT_BODY) ||
            !entity_index_components_check(a_index, ROHR_SOFT_BODY_NODE) ||
            !entity_index_components_check(b_index, ROHR_SOFT_BODY_NODE) ||
            soft_body_nodes[a_index].soft_body != soft_body ||
            soft_body_nodes[b_index].soft_body != soft_body || stiffness < 0.0f || damping < 0.0f) {
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    }
    collision_thickness = 2.0f * fminf(
        soft_body_nodes[a_index].radius, soft_body_nodes[b_index].radius);
    if(!isfinite(collision_thickness) || collision_thickness <= 0.0f)
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    filter_result = physics_collision_filter_get(node_a);
    if(error_check(filter_result))
        return ERROR_RESULT_MAKE_ERROR(EntityResult, filter_result.result.error);
    filter_a = filter_result.result.value;
    filter_result = physics_collision_filter_get(node_b);
    if(error_check(filter_result))
        return ERROR_RESULT_MAKE_ERROR(EntityResult, filter_result.result.error);
    filter_b = filter_result.result.value;
    collision_enabled = entity_index_components_check(a_index, ROHR_COLLISION) &&
        entity_index_components_check(b_index, ROHR_COLLISION) &&
        filter_a.category != ROHR_COLLISION_CATEGORY_NONE &&
        filter_a.collides_with != ROHR_COLLISION_CATEGORY_NONE &&
        filter_b.category != ROHR_COLLISION_CATEGORY_NONE &&
        filter_b.collides_with != ROHR_COLLISION_CATEGORY_NONE &&
        collision_thickness >= ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN;
    collision_thickness = fmaxf(
        collision_thickness, ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN);
    if(soft_bodies[body_index].beam_count >= SOFT_BODY_MAX_BEAMS) {
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_MAX_ENTITIES_EXCEEDED);
    }
    beam = entity_add();
    if(beam.kind == ERROR_RESULT_ERROR) return beam;
    if(!entity_index_get(beam.result.value, &beam_index)) {
        (void)entity_delete(beam.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_ENTITY_NOT_FOUND);
    }
    delta = math_vector_subtract(positions[b_index], positions[a_index]);
    if(SoftBodyBeamPool_store_at(&soft_body_beams_pool, beam_index, (SoftBodyBeam){
            .soft_body = soft_body,
            .node_a = node_a,
            .node_b = node_b,
            .rest_length = math_vector_magnitude(delta),
            .stiffness = stiffness,
            .damping = damping,
            .collision_thickness = collision_thickness,
            .collision_enabled = collision_enabled,
            .category = filter_a.category | filter_b.category,
            .collides_with = filter_a.collides_with | filter_b.collides_with
        }).kind == ERROR_RESULT_ERROR) {
        (void)entity_delete(beam.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    entity_mask[beam_index] |= ROHR_SOFT_BODY_BEAM;
    soft_bodies[body_index].beams[soft_bodies[body_index].beam_count++] = beam.result.value;
    return beam;
}

SoftBodyBeamResult physics_soft_body_beam_get(Entity beam) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(beam, &index);
    if(result.kind == ERROR_RESULT_ERROR) return ERROR_RESULT_MAKE_ERROR(SoftBodyBeamResult, result.result.error);
    if(!entity_index_components_check(index, ROHR_SOFT_BODY_BEAM) || !soft_body_beams_pool.used[index]) {
        return ERROR_RESULT_MAKE_ERROR(SoftBodyBeamResult, ERROR_ENGINE_COMPONENT_MISSING);
    }
    return ERROR_RESULT_MAKE_VALUE(SoftBodyBeamResult, soft_body_beams[index]);
}

static bool physics_soft_body_beam_collision_limit_get(
        const SoftBodyBeam *beam, float *maximum) {
    EntityIndex a;
    EntityIndex b;
    if(beam == NULL || maximum == NULL ||
            !entity_index_get(beam->node_a, &a) || !entity_index_alive_check(a) ||
            !entity_index_get(beam->node_b, &b) || !entity_index_alive_check(b) ||
            !soft_body_nodes_pool.used[a] || !soft_body_nodes_pool.used[b]) return false;
    *maximum = 2.0f * fminf(
        soft_body_nodes[a].radius, soft_body_nodes[b].radius);
    return isfinite(*maximum) && *maximum > 0.0f;
}

EngineResult physics_soft_body_beam_collision_config_set(
        Entity beam, SoftBodyBeamCollisionConfig config) {
    EntityIndex index;
    float maximum;
    SoftBodyBeam resolved;
    EngineResult result = physics_live_index_get(beam, &index);

    if(error_check(result)) return result;
    if(!entity_index_components_check(index, ROHR_SOFT_BODY_BEAM) ||
            !soft_body_beams_pool.used[index])
        return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    if(!isfinite(config.thickness) || config.thickness <= 0.0f ||
            !physics_soft_body_beam_collision_limit_get(
                &soft_body_beams[index], &maximum) ||
            (config.enabled &&
                maximum < ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    resolved = soft_body_beams[index];
    resolved.collision_thickness =
        maximum < ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN ?
            ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN :
            fminf(maximum, fmaxf(
                ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
                config.thickness));
    resolved.collision_enabled = config.enabled;
    resolved.category = config.filter.category;
    resolved.collides_with = config.filter.collides_with;
    soft_body_beams[index] = resolved;
    return error_result_value(true);
}

EngineResult physics_soft_body_beam_collision_enable(Entity beam) {
    EntityIndex index;
    float maximum;
    EngineResult result = physics_live_index_get(beam, &index);
    if(error_check(result)) return result;
    if(!entity_index_components_check(index, ROHR_SOFT_BODY_BEAM) ||
            !soft_body_beams_pool.used[index] ||
            !physics_soft_body_beam_collision_limit_get(
                &soft_body_beams[index], &maximum) ||
            maximum < ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(soft_body_beams[index].collision_thickness > maximum)
        soft_body_beams[index].collision_thickness = maximum;
    else if(soft_body_beams[index].collision_thickness <
            ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN)
        soft_body_beams[index].collision_thickness =
            ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN;
    soft_body_beams[index].collision_enabled = true;
    return error_result_value(true);
}

EngineResult physics_soft_body_beam_collision_disable(Entity beam) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(beam, &index);
    if(error_check(result)) return result;
    if(!entity_index_components_check(index, ROHR_SOFT_BODY_BEAM) ||
            !soft_body_beams_pool.used[index])
        return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    soft_body_beams[index].collision_enabled = false;
    return error_result_value(true);
}

EngineResult physics_soft_body_beam_collision_thickness_set(
        Entity beam, float thickness) {
    EntityIndex index;
    float maximum;
    EngineResult result = physics_live_index_get(beam, &index);
    if(error_check(result)) return result;
    if(!isfinite(thickness) || thickness <= 0.0f ||
            !entity_index_components_check(index, ROHR_SOFT_BODY_BEAM) ||
            !soft_body_beams_pool.used[index] ||
            !physics_soft_body_beam_collision_limit_get(
                &soft_body_beams[index], &maximum) ||
            maximum < ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    soft_body_beams[index].collision_thickness = fminf(maximum,
        fmaxf(ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN, thickness));
    return error_result_value(true);
}

EngineResult physics_soft_body_beam_collision_filter_set(Entity beam,
        RohrCollisionCategoryMask category,
        RohrCollisionCategoryMask collides_with) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(beam, &index);
    if(error_check(result)) return result;
    if(!entity_index_components_check(index, ROHR_SOFT_BODY_BEAM) ||
            !soft_body_beams_pool.used[index])
        return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    soft_body_beams[index].category = category;
    soft_body_beams[index].collides_with = collides_with;
    return error_result_value(true);
}

EntityResult physics_soft_body_triangle_create(Entity soft_body, Entity node_a, Entity node_b, Entity node_c) {
    EntityIndex body_index;
    EntityIndex indices[3];
    Entity nodes_to_check[3] = {node_a, node_b, node_c};
    EntityIndex triangle_index;
    EntityResult triangle;

    if(physics_live_index_get(soft_body, &body_index).kind == ERROR_RESULT_ERROR ||
            !entity_index_components_check(body_index, ROHR_SOFT_BODY) ||
            node_a == node_b || node_b == node_c || node_a == node_c) {
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    }
    for(uint32_t i = 0; i < 3; i += 1) {
        if(physics_live_index_get(nodes_to_check[i], &indices[i]).kind == ERROR_RESULT_ERROR ||
                !entity_index_components_check(indices[i], ROHR_SOFT_BODY_NODE) ||
                soft_body_nodes[indices[i]].soft_body != soft_body) {
            return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
        }
    }
    if(soft_bodies[body_index].triangle_count >= SOFT_BODY_MAX_TRIANGLES) {
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_MAX_ENTITIES_EXCEEDED);
    }
    triangle = entity_add();
    if(triangle.kind == ERROR_RESULT_ERROR) return triangle;
    if(!entity_index_get(triangle.result.value, &triangle_index) ||
            SoftBodyTrianglePool_store_at(&soft_body_triangles_pool, triangle_index, (SoftBodyTriangle){
                .soft_body = soft_body,
                .node_a = node_a,
                .node_b = node_b,
                .node_c = node_c
            }).kind == ERROR_RESULT_ERROR) {
        (void)entity_delete(triangle.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    entity_mask[triangle_index] |= ROHR_SOFT_BODY_TRIANGLE;
    soft_bodies[body_index].triangles[soft_bodies[body_index].triangle_count++] = triangle.result.value;
    return triangle;
}

SoftBodyTriangleResult physics_soft_body_triangle_get(Entity triangle) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(triangle, &index);
    if(result.kind == ERROR_RESULT_ERROR) return ERROR_RESULT_MAKE_ERROR(SoftBodyTriangleResult, result.result.error);
    if(!entity_index_components_check(index, ROHR_SOFT_BODY_TRIANGLE) || !soft_body_triangles_pool.used[index]) {
        return ERROR_RESULT_MAKE_ERROR(SoftBodyTriangleResult, ERROR_ENGINE_COMPONENT_MISSING);
    }
    return ERROR_RESULT_MAKE_VALUE(SoftBodyTriangleResult, soft_body_triangles[index]);
}

EntityResult physics_soft_body_area_create(Entity body,
        const AreaBoundaryPoint *boundary, size_t count) {
    EntityIndex owner;
    if(!boundary || count<3 || !entity_index_get(body,&owner) ||
        !entity_index_components_check(owner,ROHR_SOFT_BODY) ||
        soft_bodies[owner].area_count>=SOFT_BODY_MAX_TRIANGLES ||
        count>SIZE_MAX/sizeof(*boundary))
        return ERROR_RESULT_MAKE_ERROR(EntityResult,ERROR_ENGINE_STATE_INVALID);
    for(size_t i=0;i<count;i++) {
        const AreaBoundaryPoint *p=&boundary[i];
        for(size_t k=0;k<(p->beams[0]?4:1);k++) {
            SoftBodyNodeResult node=physics_soft_body_node_get(p->nodes[k]);
            if(node.kind==ERROR_RESULT_ERROR || node.result.value.soft_body!=body)
                return ERROR_RESULT_MAKE_ERROR(EntityResult,ERROR_ENGINE_STATE_INVALID);
        }
        for(size_t k=0;k<3;k++) {
            Entity id=k==2?p->edge:p->beams[k];
            if(id==0 && (k==2 || p->beams[0]==0)) continue;
            SoftBodyBeamResult beam=physics_soft_body_beam_get(id);
            if(beam.kind==ERROR_RESULT_ERROR || beam.result.value.soft_body!=body ||
                (k<2 && (!isfinite(p->fractions[k]) || p->fractions[k]<0 || p->fractions[k]>1 ||
                 beam.result.value.node_a!=p->nodes[k*2] || beam.result.value.node_b!=p->nodes[k*2+1])))
                return ERROR_RESULT_MAKE_ERROR(EntityResult,ERROR_ENGINE_STATE_INVALID);
        }
    }
    AreaBoundaryPoint *copy=malloc(count*sizeof(*copy));
    if(!copy) return ERROR_RESULT_MAKE_ERROR(EntityResult,ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    memcpy(copy,boundary,count*sizeof(*copy));
    EntityResult created=entity_add(); EntityIndex index;
    if(created.kind==ERROR_RESULT_ERROR) { free(copy); return created; }
    if(!entity_index_get(created.result.value,&index) ||
        SoftBodyAreaPool_store_at(&soft_body_areas_pool,index,(SoftBodyArea){
            .soft_body=body,.boundary=copy,.boundary_count=count,.visible=true,.surface_enabled=true}).kind==ERROR_RESULT_ERROR) {
        free(copy); (void)entity_delete(created.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult,ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    entity_mask[index]|=ROHR_SOFT_BODY_AREA;
    soft_bodies[owner].areas[soft_bodies[owner].area_count++]=created.result.value;
    return created;
}
SoftBodyAreaResult physics_soft_body_area_get(Entity area) {
    EntityIndex index;
    if(!entity_index_get(area,&index) || !entity_index_components_check(index,ROHR_SOFT_BODY_AREA) ||
        !soft_body_areas_pool.used[index])
        return ERROR_RESULT_MAKE_ERROR(SoftBodyAreaResult,ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(SoftBodyAreaResult,soft_body_areas[index]);
}
EngineResult physics_soft_body_area_style_set(Entity area,Color color,
        bool override_color,bool visible,bool surface_enabled) {
    SoftBodyAreaResult result=physics_soft_body_area_get(area); EntityIndex index;
    if(result.kind==ERROR_RESULT_ERROR || !entity_index_get(area,&index))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    soft_body_areas[index].draw_color=color;
    soft_body_areas[index].draw_color_overridden=override_color;
    soft_body_areas[index].visible=visible;
    soft_body_areas[index].surface_enabled=surface_enabled;
    return error_result_value(true);
}
bool physics_soft_body_area_mesh_create(Entity area,AreaMesh *out) {
    if(!out) return false;
    *out=(AreaMesh){0}; SoftBodyAreaResult result=physics_soft_body_area_get(area);
    if(result.kind==ERROR_RESULT_ERROR) return false;
    SoftBodyArea value=result.result.value;
    Position *points=malloc(value.boundary_count*sizeof(*points));
    if(!points) return false;
    for(size_t i=0;i<value.boundary_count;i++) {
        const AreaBoundaryPoint *p=&value.boundary[i]; Position nodes[4];
        for(size_t k=0;k<(p->beams[0]?4:1);k++) {
            EntityIndex index;
            if(!entity_index_get(p->nodes[k],&index) || !positions_pool.used[index]) { free(points); return false; }
            nodes[k]=positions[index];
        }
        points[i]=nodes[0];
        if(p->beams[0]) points[i]=(Position){
            .5f*(nodes[0].x+(nodes[1].x-nodes[0].x)*p->fractions[0]+nodes[2].x+(nodes[3].x-nodes[2].x)*p->fractions[1]),
            .5f*(nodes[0].y+(nodes[1].y-nodes[0].y)*p->fractions[0]+nodes[2].y+(nodes[3].y-nodes[2].y)*p->fractions[1])};
    }
    bool ok=area_mesh_create(points,value.boundary_count,out); free(points); return ok;
}

static bool physics_area_boundary_equal_check(const SoftBodyArea *area,const AreaFace *face) {
    if(area->boundary_count!=face->count) return false;
    for(size_t start=0;start<face->count;start++) for(size_t reverse=0;reverse<2;reverse++) {
        bool equal=true;
        for(size_t i=0;i<face->count;i++) {
            size_t index=(start+(reverse?face->count-i:i))%face->count;
            const AreaBoundaryPoint *a=&area->boundary[index],*b=&face->points[i];
            size_t edge=reverse?(index+face->count-1)%face->count:index;
            if(area->boundary[edge].edge!=b->edge ||
                a->beams[0]!=b->beams[0] || a->beams[1]!=b->beams[1] ||
                memcmp(a->nodes,b->nodes,sizeof(a->nodes))!=0) { equal=false; break; }
        }
        if(equal) return true;
    }
    return false;
}

EngineResult physics_soft_body_areas_rebuild(Entity body,
        EngineResult (*style_copy)(Entity from, Entity to)) {
    SoftBodyResult queried=physics_soft_body_get(body);
    if(queried.kind==ERROR_RESULT_ERROR) return error_result_error(queried.result.error);
    SoftBody previous=queried.result.value;
    AreaSegment segments[SOFT_BODY_MAX_BEAMS]; size_t segment_count=0;
    for(size_t i=0;i<previous.beam_count;i++) {
        SoftBodyBeamResult beam=physics_soft_body_beam_get(previous.beams[i]);
        EntityIndex a,b;
        if(beam.kind==ERROR_RESULT_ERROR || !entity_index_get(beam.result.value.node_a,&a) ||
            !entity_index_get(beam.result.value.node_b,&b)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
        segments[segment_count++]=(AreaSegment){previous.beams[i],beam.result.value.node_a,
            beam.result.value.node_b,positions[a],positions[b]};
    }
    AreaFaces faces={0};
    if(!area_faces_create(segments,segment_count,&faces)) return error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    if(faces.count>SOFT_BODY_MAX_TRIANGLES) { area_faces_destroy(&faces); return error_result_error(ERROR_ENGINE_MAX_ENTITIES_EXCEEDED); }
    Entity retained[SOFT_BODY_MAX_TRIANGLES]={0},created[SOFT_BODY_MAX_TRIANGLES]={0};
    size_t created_count=0; EngineResult result=error_result_value(true);
    for(size_t i=0;i<faces.count;i++) {
        for(size_t j=0;j<previous.area_count;j++) {
            SoftBodyAreaResult area=physics_soft_body_area_get(previous.areas[j]);
            if(area.kind==ERROR_RESULT_VALUE && physics_area_boundary_equal_check(&area.result.value,&faces.items[i])) {
                retained[i]=previous.areas[j]; break;
            }
        }
        if(retained[i]) continue;
        EntityResult next=physics_soft_body_area_create(body,faces.items[i].points,faces.items[i].count);
        if(next.kind==ERROR_RESULT_ERROR) { result=error_result_error(next.result.error); goto fail; }
        retained[i]=created[created_count++]=next.result.value;
        AreaMesh mesh={0};
        if(!physics_soft_body_area_mesh_create(next.result.value,&mesh)) { result=error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED); goto fail; }
        double overlap=0; Entity style_id=0; SoftBodyArea style={0};
        for(size_t j=0;j<previous.area_count;j++) {
            AreaMesh old={0};
            SoftBodyAreaResult old_area=physics_soft_body_area_get(previous.areas[j]);
            if(old_area.kind==ERROR_RESULT_ERROR || !physics_soft_body_area_mesh_create(previous.areas[j],&old)) continue;
            double amount=area_mesh_overlap_get(&mesh,&old); area_mesh_destroy(&old);
            if(amount>overlap+0.000001 || (amount>0 && fabs(amount-overlap)<=0.000001 &&
                (style_id==0 || previous.areas[j]<style_id))) {
                overlap=amount; style=old_area.result.value; style_id=previous.areas[j];
            }
        }
        area_mesh_destroy(&mesh);
        if(style_id) {
            (void)physics_soft_body_area_style_set(next.result.value,style.draw_color,
                style.draw_color_overridden,style.visible,style.surface_enabled);
            if(style_copy) {
                result=style_copy(style_id,next.result.value);
                if(result.kind==ERROR_RESULT_ERROR) goto fail;
            }
        }
    }
    for(size_t j=0;j<previous.area_count;j++) {
        bool keep=false;
        for(size_t i=0;i<faces.count;i++) keep|=retained[i]==previous.areas[j];
        if(!keep) (void)entity_delete(previous.areas[j]);
    }
    area_faces_destroy(&faces); return result;
fail:
    for(size_t i=0;i<created_count;i++) (void)entity_delete(created[i]);
    area_faces_destroy(&faces); return result;
}
