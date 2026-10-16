/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics.h"
#include "entity/entity_internal.h"
#include "soft_body_area.h"

#include "physics/physics_internal.h"
#include "math2d.h"
#include "physics/physics_operation.h"

#include <math.h>
#include <float.h>

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
        if((velocities_pool.used[node_index] && !physics_vector_finite_check(math_vector_rotate(velocities[node_index], delta))) ||
                (accelerations_pool.used[node_index] && !physics_vector_finite_check(math_vector_rotate(accelerations[node_index], delta))))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
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

void physics_soft_body_member_release(Entity entity, EntityIndex index) {
    if(index < soft_body_nodes_pool.capacity && soft_body_nodes_pool.used[index])
        (void)physics_soft_body_node_owner_set(entity, ENTITY_INVALID);
    if(index < soft_body_beams_pool.capacity && soft_body_beams_pool.used[index]) {
        EntityIndex owner;
        if(entity_index_get(soft_body_beams[index].soft_body, &owner) && soft_bodies_pool.used[owner])
            physics_soft_body_entity_list_remove(soft_bodies[owner].beams, &soft_bodies[owner].beam_count, entity);
        soft_body_beams[index].soft_body = ENTITY_INVALID;
    }
    if(index < soft_body_area_states_pool.capacity && soft_body_area_states_pool.used[index]) {
        EntityIndex owner;
        if(entity_index_get(soft_body_area_states[index].value.soft_body, &owner) && soft_bodies_pool.used[owner])
            physics_soft_body_entity_list_remove(soft_bodies[owner].areas, &soft_bodies[owner].area_count, entity);
        soft_body_area_states[index].value.soft_body = ENTITY_INVALID;
    }
}

void physics_soft_body_entity_clear(Entity entity, EntityIndex index) {
    if(index < soft_bodies_pool.capacity && soft_bodies_pool.used[index]) {
        SoftBody body = soft_bodies[index];

        for(uint32_t i = 0; i < body.area_count; i += 1)
            if(entity_alive_check(body.areas[i]))
                (void)entity_delete(body.areas[i]);
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
            if(!soft_body_beams_pool.used[connected] ||
                    (soft_body_beams[connected].node_a != entity &&
                        soft_body_beams[connected].node_b != entity)) continue;
            if(soft_body_beams[connected].node_a == entity)
                soft_body_beams[connected].node_a = ENTITY_INVALID;
            if(soft_body_beams[connected].node_b == entity)
                soft_body_beams[connected].node_b = ENTITY_INVALID;
        }
        physics_soft_body_areas_node_clear(entity, owner);
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
    physics_soft_body_area_entity_clear(entity, index);
}

EntityResult physics_soft_body_create(void) {
    EntityResult result = entity_default_create("soft_body", ENTITY_INVALID);
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
    EntityIndex body_index = 0;
    EntityIndex node_index;
    EntityResult node;
    if((soft_body != ENTITY_INVALID &&
            (error_check(physics_live_index_get(soft_body, &body_index)) ||
             !entity_index_components_check(body_index, ROHR_SOFT_BODY) ||
             !soft_bodies_pool.used[body_index])) ||
            !isfinite(node_mass) || !isfinite(radius) ||
            node_mass <= 0.0f || radius <= 0.0f || !physics_world_position_check(position)) {
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    }
    if(soft_body != ENTITY_INVALID && soft_bodies[body_index].node_count >= SOFT_BODY_MAX_NODES) {
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_MAX_ENTITIES_EXCEEDED);
    }
    node = entity_default_create("node", soft_body);
    if(node.kind == ERROR_RESULT_ERROR) return node;
    if(!entity_index_get(node.result.value, &node_index) ||
            physics_position_set(node.result.value, position).kind == ERROR_RESULT_ERROR ||
            physics_orientation_set(node.result.value, 0).kind == ERROR_RESULT_ERROR ||
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
    if(soft_body != ENTITY_INVALID)
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

EngineResult physics_soft_body_node_owner_set(Entity node, Entity soft_body) {
    EntityIndex index, next = 0, prior;
    SoftBodyNodeResult value = physics_soft_body_node_get(node);
    if(error_check(value)) return error_result_error(value.result.error);
    if(value.result.value.soft_body == soft_body) return error_result_value(true);
    if(soft_body != ENTITY_INVALID &&
            (!entity_index_get(soft_body, &next) || !soft_bodies_pool.used[next] ||
             soft_bodies[next].node_count >= SOFT_BODY_MAX_NODES))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    entity_index_get(node, &index);
    Entity old = soft_body_nodes[index].soft_body;
    if(old != ENTITY_INVALID && entity_index_get(old, &prior) && soft_bodies_pool.used[prior]) {
        physics_soft_body_entity_list_remove(soft_bodies[prior].nodes, &soft_bodies[prior].node_count, node);
        for(size_t b = 0; b < soft_body_beams_pool.capacity; ++b) {
            if(!soft_body_beams_pool.used[b] || soft_body_beams[b].soft_body != old) continue;
            if(soft_body_beams[b].node_a == node) soft_body_beams[b].node_a = ENTITY_INVALID;
            if(soft_body_beams[b].node_b == node) soft_body_beams[b].node_b = ENTITY_INVALID;
        }
        for(size_t a = 0; a < soft_body_area_states_pool.capacity; ++a) {
            if(!soft_body_area_states_pool.used[a] || soft_body_area_states[a].value.soft_body != old) continue;
            SoftBodyAreaGeometry *geometry = &soft_body_area_states[a].value.geometry;
            for(uint32_t l = 0; l <= geometry->hole_count; ++l) {
                SoftBodyAreaLoop *loop = l ? &geometry->holes[l - 1] : &geometry->outer;
                for(uint32_t n = 0; n < loop->node_count; ++n)
                    if(loop->nodes[n] == node) loop->nodes[n] = ENTITY_INVALID;
            }
        }
    }
    soft_body_nodes[index].soft_body = soft_body;
    if(soft_body) soft_bodies[next].nodes[soft_bodies[next].node_count++] = node;
    return error_result_value(true);
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

EngineResult physics_soft_body_node_impulse_apply(Entity node, Vec2D impulse) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(node, &index);

    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(!entity_index_components_check(index, ROHR_SOFT_BODY_NODE)) {
        return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    }
    return physics_impulse_apply(node, impulse);
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
    joint = entity_default_create("joint", ENTITY_INVALID);
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
    EntityIndex body_index = 0;
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

    if(!isfinite(stiffness) || !isfinite(damping) || stiffness < 0 || damping < 0 ||
            (soft_body != ENTITY_INVALID &&
             (error_check(physics_live_index_get(soft_body, &body_index)) ||
              !entity_index_components_check(body_index, ROHR_SOFT_BODY))))
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    if(soft_body != ENTITY_INVALID && soft_bodies[body_index].beam_count >= SOFT_BODY_MAX_BEAMS)
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_MAX_ENTITIES_EXCEEDED);
    if(node_a == ENTITY_INVALID || node_b == ENTITY_INVALID) {
        if((node_a != ENTITY_INVALID && error_check(physics_soft_body_node_get(node_a))) ||
                (node_b != ENTITY_INVALID && error_check(physics_soft_body_node_get(node_b))))
            return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
        if(soft_body != ENTITY_INVALID &&
                ((node_a && physics_soft_body_node_get(node_a).result.value.soft_body != soft_body) ||
                 (node_b && physics_soft_body_node_get(node_b).result.value.soft_body != soft_body)))
            return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
        beam = entity_default_create("beam", soft_body);
        if(error_check(beam)) return beam;
        if(!entity_index_get(beam.result.value, &beam_index) ||
                error_check(SoftBodyBeamPool_store_at(&soft_body_beams_pool, beam_index,
                    (SoftBodyBeam){.soft_body = soft_body, .node_a = node_a, .node_b = node_b,
                        .stiffness = stiffness, .damping = damping, .restitution = 0.25f,
                        .collision_thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN}))) {
            (void)entity_delete(beam.result.value);
            return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
        }
        entity_mask[beam_index] |= ROHR_SOFT_BODY_BEAM;
        if(soft_body != ENTITY_INVALID)
            soft_bodies[body_index].beams[soft_bodies[body_index].beam_count++] = beam.result.value;
        return beam;
    }
    if(
            physics_live_index_get(node_a, &a_index).kind == ERROR_RESULT_ERROR ||
            physics_live_index_get(node_b, &b_index).kind == ERROR_RESULT_ERROR || node_a == node_b ||
            !entity_index_components_check(a_index, ROHR_SOFT_BODY_NODE) ||
            !entity_index_components_check(b_index, ROHR_SOFT_BODY_NODE) ||
            (soft_body != ENTITY_INVALID && (soft_body_nodes[a_index].soft_body != soft_body ||
                soft_body_nodes[b_index].soft_body != soft_body))) {
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
    if(soft_body != ENTITY_INVALID && soft_bodies[body_index].beam_count >= SOFT_BODY_MAX_BEAMS) {
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_MAX_ENTITIES_EXCEEDED);
    }
    beam = entity_default_create("beam", soft_body);
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
            .friction = 0.0f,
            .restitution = 0.25f,
            .collision_enabled = collision_enabled,
            .category = filter_a.category | filter_b.category,
            .collides_with = filter_a.collides_with | filter_b.collides_with
        }).kind == ERROR_RESULT_ERROR) {
        (void)entity_delete(beam.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    entity_mask[beam_index] |= ROHR_SOFT_BODY_BEAM;
    if(soft_body != ENTITY_INVALID)
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
    if(beam && maximum && (beam->node_a == ENTITY_INVALID || beam->node_b == ENTITY_INVALID)) {
        *maximum = FLT_MAX;
        return true;
    }
    if(beam == NULL || maximum == NULL ||
            !entity_index_get(beam->node_a, &a) || !entity_index_alive_check(a) ||
            !entity_index_get(beam->node_b, &b) || !entity_index_alive_check(b) ||
            !soft_body_nodes_pool.used[a] || !soft_body_nodes_pool.used[b]) return false;
    *maximum = 2.0f * fminf(
        soft_body_nodes[a].radius, soft_body_nodes[b].radius);
    return isfinite(*maximum) && *maximum > 0.0f;
}

EngineResult physics_soft_body_beam_endpoints_set(Entity beam, Entity node_a, Entity node_b) {
    SoftBodyBeamResult value = physics_soft_body_beam_get(beam);
    if(error_check(value)) return error_result_error(value.result.error);
    if(node_a != ENTITY_INVALID && node_a == node_b) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    Entity nodes[] = {node_a, node_b};
    for(size_t n = 0; n < 2; ++n) {
        if(!nodes[n]) continue;
        SoftBodyNodeResult node = physics_soft_body_node_get(nodes[n]);
        if(error_check(node) || (value.result.value.soft_body != ENTITY_INVALID &&
                node.result.value.soft_body != value.result.value.soft_body))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
    }
    SoftBodyBeam updated = value.result.value;
    updated.node_a = node_a; updated.node_b = node_b;
    float maximum;
    if(node_a && node_b && updated.collision_enabled &&
            (!physics_soft_body_beam_collision_limit_get(&updated, &maximum) || updated.collision_thickness > maximum))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    EntityIndex index; entity_index_get(beam, &index); soft_body_beams[index] = updated;
    return error_result_value(true);
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
