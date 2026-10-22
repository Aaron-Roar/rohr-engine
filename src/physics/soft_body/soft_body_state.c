/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics.h"
#include "entity/entity_internal.h"
#include "soft_body_area.h"
#include "soft_body_frame.h"

#include "physics/physics_internal.h"
#include "math2d.h"
#include "physics/physics_operation.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>

static Vec2D physics_soft_body_vector_add(Vec2D first, Vec2D second) {
    return (Vec2D){first.x + second.x, first.y + second.y};
}

static void physics_soft_body_entity_list_remove(
        Entity *values, uint32_t *count, Entity entity) {
    if(values == NULL || count == NULL) return;
    for(uint32_t i = 0; i < *count; i += 1) {
        if(values[i] != entity) continue;
        memmove(&values[i], &values[i + 1], (*count - i - 1) * sizeof(Entity));
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

EngineResult physics_soft_body_membership_validate(EntityIndex index, Entity owner) {
    EntityIndex next;
    if(!entity_index_get(owner, &next) || !soft_bodies_pool.used[next])
        return error_result_value(true);
    const SoftBody *body = &soft_bodies[next];
    if((soft_body_nodes_pool.used[index] && soft_body_nodes[index].soft_body != owner &&
            body->node_count >= SOFT_BODY_MAX_NODES) ||
       (soft_body_beams_pool.used[index] && soft_body_beams[index].soft_body != owner &&
            body->beam_count >= SOFT_BODY_MAX_BEAMS) ||
       (soft_body_area_states_pool.used[index] && soft_body_area_states[index].value.soft_body != owner &&
            body->area_count >= SOFT_BODY_MAX_AREAS))
        return error_result_error(ERROR_ENGINE_MAX_ENTITIES_EXCEEDED);
    return error_result_value(true);
}

void physics_soft_body_membership_update(Entity entity, EntityIndex index, Entity owner) {
    EntityIndex next = 0, prior;
    if(!entity_index_get(owner, &next) || !soft_bodies_pool.used[next]) owner = ENTITY_INVALID;
    Entity old_node_owner = soft_body_nodes_pool.used[index] ? soft_body_nodes[index].soft_body : ENTITY_INVALID;
    /* Fixed lists index immediate ownership; they never own deletion. */
#define UPDATE(pool, field, list, count) do { \
    if(pool.used[index] && field != owner) { \
        if(entity_index_get(field, &prior) && soft_bodies_pool.used[prior]) \
            physics_soft_body_entity_list_remove(soft_bodies[prior].list, &soft_bodies[prior].count, entity); \
        field = owner; \
        if(owner != ENTITY_INVALID) soft_bodies[next].list[soft_bodies[next].count++] = entity; \
    } \
} while(0)
    UPDATE(soft_body_nodes_pool, soft_body_nodes[index].soft_body, nodes, node_count);
    UPDATE(soft_body_beams_pool, soft_body_beams[index].soft_body, beams, beam_count);
    UPDATE(soft_body_area_states_pool, soft_body_area_states[index].value.soft_body, areas, area_count);
#undef UPDATE
    if(soft_body_nodes_pool.used[index] && old_node_owner != owner) {
        if(entity_index_get(old_node_owner, &prior) && soft_bodies_pool.used[prior])
            physics_soft_body_frame_reset(prior);
        if(owner != ENTITY_INVALID) physics_soft_body_frame_reset(next);
    }
}

void physics_soft_body_entity_clear(Entity entity, EntityIndex index) {
    if(index < soft_bodies_pool.capacity && soft_bodies_pool.used[index]) {
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
                soft_bodies_pool.used[body_index]) {
            physics_soft_body_entity_list_remove(
                soft_bodies[body_index].nodes,
                &soft_bodies[body_index].node_count,
                entity);
            physics_soft_body_frame_reset(body_index);
        }
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

EntityResult physics_soft_body_add(SoftBodyDescription description) {
    if((description.owner != ENTITY_INVALID && !entity_alive_check(description.owner)) ||
            !physics_world_position_check(description.position) || !isfinite(description.orientation))
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    EntityResult result = entity_default_create("soft_body", description.owner);
    EntityIndex index;

    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(!entity_index_get(result.result.value, &index) ||
            physics_position_set(result.result.value, description.position).kind ==
                ERROR_RESULT_ERROR ||
            physics_orientation_set(result.result.value, description.orientation).kind ==
                ERROR_RESULT_ERROR ||
            SoftBodyPool_store_at(&soft_bodies_pool, index,
                (SoftBody){.origin = result.result.value}).kind ==
                ERROR_RESULT_ERROR) {
        (void)entity_delete(result.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    entity_mask[index] |= ROHR_SOFT_BODY;
    EngineResult owned = entity_owner_set(result.result.value, description.owner);
    if(error_check(owned)) {
        (void)entity_delete(result.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, owned.result.error);
    }
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

SoftBodyDescription physics_soft_body_description_default_get(void) {
    return (SoftBodyDescription){0};
}

SoftBodyDescriptionResult physics_soft_body_description_get(Entity entity) {
    SoftBodyResult body = physics_soft_body_get(entity);
    if(error_check(body)) return ERROR_RESULT_MAKE_ERROR(SoftBodyDescriptionResult, body.result.error);
    EntityIndex index; (void)entity_index_get(entity, &index);
    return ERROR_RESULT_MAKE_VALUE(SoftBodyDescriptionResult, ((SoftBodyDescription){
        .owner = entity_owner_get(entity).result.value,
        .position = positions[index], .orientation = orientations[index]}));
}

EngineResult physics_soft_body_description_set(Entity entity, SoftBodyDescription description) {
    SoftBodyResult value = physics_soft_body_get(entity);
    if(error_check(value)) return error_result_error(value.result.error);
    if(!physics_world_position_check(description.position) || !isfinite(description.orientation))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    EntityIndex index; (void)entity_index_get(entity, &index);
    Orientation delta = description.orientation - orientations[index];
    if(!isfinite(delta)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    bool moved = description.position.x != positions[index].x || description.position.y != positions[index].y;
    bool rotated = delta != 0;
    Position next[SOFT_BODY_MAX_NODES];
    Velocity velocity[SOFT_BODY_MAX_NODES];
    Acceleration acceleration[SOFT_BODY_MAX_NODES];
    SoftBody *body = &soft_bodies[index];
    for(uint32_t i = 0; (moved || rotated) && i < body->node_count; ++i) {
        EntityIndex node;
        if(!entity_index_get(body->nodes[i], &node) || !positions_pool.used[node])
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        Vec2D offset = math_vector_subtract(positions[node], positions[index]);
        next[i] = rotated ? physics_soft_body_vector_add(description.position, math_vector_rotate(offset, delta)) :
            physics_soft_body_vector_add(positions[node], math_vector_subtract(description.position, positions[index]));
        velocity[i] = velocities_pool.used[node] ? velocities[node] : (Velocity){0};
        acceleration[i] = accelerations_pool.used[node] ? accelerations[node] : (Acceleration){0};
        if(rotated) {
            velocity[i] = math_vector_rotate(velocity[i], delta);
            acceleration[i] = math_vector_rotate(acceleration[i], delta);
        }
        if(!physics_world_position_check(next[i]) || !physics_vector_finite_check(velocity[i]) ||
                !physics_vector_finite_check(acceleration[i]))
            return error_result_error(ERROR_ENGINE_POSITION_OUT_OF_RANGE);
    }
    EngineResult result = entity_owner_set(entity, description.owner);
    if(error_check(result)) return result;
    for(uint32_t i = 0; (moved || rotated) && i < body->node_count; ++i) {
        EntityIndex node; (void)entity_index_get(body->nodes[i], &node);
        positions[node] = next[i];
        if(rotated && velocities_pool.used[node]) velocities[node] = velocity[i];
        if(rotated && accelerations_pool.used[node]) accelerations[node] = acceleration[i];
    }
    (void)PositionPool_store_at(&positions_pool, index, description.position);
    (void)OrientationPool_store_at(&orientations_pool, index, description.orientation);
    if(moved || rotated) physics_soft_body_frame_reset(index);
    return error_result_value(true);
}

SoftBodyNodeDescription physics_soft_body_node_description_default_get(void) {
    return (SoftBodyNodeDescription){.mass = 1, .radius = 1, .restitution = 0.25f,
        .collision_enabled = true, .collision_filter = {
            .category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}};
}

SoftBodyNodeDescriptionResult physics_soft_body_node_description_get(Entity entity) {
    SoftBodyNodeResult node = physics_soft_body_node_get(entity);
    if(error_check(node)) return ERROR_RESULT_MAKE_ERROR(SoftBodyNodeDescriptionResult, node.result.error);
    EntityIndex index; (void)entity_index_get(entity, &index);
    CollisionFilterDescriptionResult filter = physics_collision_filter_description_get(entity);
    if(error_check(filter)) return ERROR_RESULT_MAKE_ERROR(SoftBodyNodeDescriptionResult, filter.result.error);
    return ERROR_RESULT_MAKE_VALUE(SoftBodyNodeDescriptionResult, ((SoftBodyNodeDescription){
        .owner = entity_owner_get(entity).result.value,
        .position = positions[index], .mass = mass_pool.used[index] ? mass[index] : 0,
        .radius = node.result.value.radius,
        .velocity = velocities_pool.used[index] ? velocities[index] : (Velocity){0},
        .acceleration = accelerations_pool.used[index] ? accelerations[index] : (Acceleration){0},
        .friction = frictions_pool.used[index] ? frictions[index] : 0,
        .restitution = restitutions_pool.used[index] ? restitutions[index] : 0,
        .collision_filter = filter.result.value,
        .static_body = (entity_mask[index] & ROHR_STATIC) != 0,
        .gravity_enabled = (entity_mask[index] & ROHR_GRAVITY) != 0,
        .collision_enabled = (entity_mask[index] & ROHR_COLLISION) != 0,
        .draw_color = node.result.value.draw_color,
        .draw_color_overridden = node.result.value.draw_color_overridden}));
}

EngineResult physics_soft_body_node_description_set(Entity entity, SoftBodyNodeDescription description) {
    SoftBodyNodeResult node = physics_soft_body_node_get(entity);
    if(error_check(node)) return error_result_error(node.result.error);
    if(!physics_world_position_check(description.position) || !physics_vector_finite_check(description.velocity) ||
            !physics_vector_finite_check(description.acceleration) || !isfinite(description.mass) || description.mass < 0 ||
            !isfinite(description.radius) || description.radius <= 0 || !isfinite(description.friction) ||
            description.friction < 0 || !isfinite(description.restitution) || description.restitution < 0 || description.restitution > 1)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    EntityIndex index; (void)entity_index_get(entity, &index);
    Position origin = particle_geometries_pool.used[index] ? particle_geometries_pool.objects[index].local_origin : (Position){0};
    /* Nodes collide through their circle radius. Preserve their independently
     * editable polygon, as the focused node-radius setter has always done. */
    ParticleShapeChange shape = {0};
    EngineResult result = error_result_value(true);
    if(!entity_index_components_check(index, ROHR_HIT_BOX)) {
        result = physics_particle_shape_prepare(index, origin, description.radius, 8, &shape);
        if(error_check(result)) return result;
    }
    result = entity_owner_set(entity, description.owner);
    if(error_check(result)) { free(shape.allocated); return result; }
    physics_particle_shape_commit(index, origin, description.radius, &shape, false);
    (void)PositionPool_store_at(&positions_pool, index, description.position);
    (void)VelocityPool_store_at(&velocities_pool, index, description.velocity);
    (void)AccelerationPool_store_at(&accelerations_pool, index, description.acceleration);
    (void)MassPool_store_at(&mass_pool, index, description.mass);
    (void)FrictionPool_store_at(&frictions_pool, index, description.friction);
    (void)RestitutionPool_store_at(&restitutions_pool, index, description.restitution);
    (void)CollisionFilterDescriptionPool_store_at(&collision_filters_pool, index, description.collision_filter);
    soft_body_nodes[index].radius = description.radius;
    soft_body_nodes[index].category = description.collision_filter.category;
    soft_body_nodes[index].responds_to = description.collision_filter.responds_to;
    soft_body_nodes[index].draw_color = description.draw_color;
    soft_body_nodes[index].draw_color_overridden = description.draw_color_overridden;
    entity_mask[index] &= ~(ROHR_STATIC | ROHR_DYNAMIC | ROHR_GRAVITY | ROHR_COLLISION);
    entity_mask[index] |= ROHR_MASS | ROHR_COLLISION_FILTER | (description.static_body ? ROHR_STATIC : ROHR_DYNAMIC);
    if(description.gravity_enabled) entity_mask[index] |= ROHR_GRAVITY;
    if(description.collision_enabled) entity_mask[index] |= ROHR_COLLISION;
    physics_body_state_sync(index);
    if(node.result.value.soft_body != soft_body_nodes[index].soft_body) {
        EntityIndex body;
        if(entity_index_get(soft_body_nodes[index].soft_body, &body)) physics_soft_body_frame_reset(body);
    }
    return error_result_value(true);
}

EntityResult physics_soft_body_node_add(SoftBodyNodeDescription description) {
    /* Creation retains its positive-mass contract; replacement also accepts a
     * current zero mass established through the focused kinematic API. */
    if(!isfinite(description.mass) || description.mass <= 0)
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    EntityResult created = entity_default_create("node", description.owner);
    if(error_check(created)) return created;
    EntityIndex index; (void)entity_index_get(created.result.value, &index);
    (void)SoftBodyNodePool_store_at(&soft_body_nodes_pool, index, (SoftBodyNode){0});
    (void)OrientationPool_store_at(&orientations_pool, index, 0);
    entity_mask[index] |= ROHR_SOFT_BODY_NODE | ROHR_PARTICLE;
    EngineResult result = physics_soft_body_node_description_set(created.result.value, description);
    if(error_check(result)) {
        (void)entity_delete(created.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    }
    return created;
}

EntityResult physics_soft_body_node_local_add(SoftBodyNodeDescription description) {
    Entity soft_body = description.owner;
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
        math_vector_rotate(description.position, orientations[body_index]));
    description.position = world_position;
    return physics_soft_body_node_add(description);
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

EngineResult physics_soft_body_node_collision_filter_description_set(Entity node, CollisionFilterDescription description) {
    SoftBodyNodeResult value = physics_soft_body_node_get(node);
    if(error_check(value)) return error_result_error(value.result.error);
    EngineResult result = physics_collision_filter_description_set(node, description);
    if(error_check(result)) return result;
    EntityIndex index; (void)entity_index_get(node, &index);
    soft_body_nodes[index].category = description.category;
    soft_body_nodes[index].responds_to = description.responds_to;
    return result;
}

CollisionFilterDescriptionResult physics_soft_body_node_collision_filter_description_get(Entity node) {
    SoftBodyNodeResult value = physics_soft_body_node_get(node);
    if(error_check(value)) return ERROR_RESULT_MAKE_ERROR(CollisionFilterDescriptionResult, value.result.error);
    return physics_collision_filter_description_get(node);
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

SoftBodyNodeAnchorPinResult physics_soft_body_node_to_anchor_pin_add(SoftBodyNodeAnchorPinDescription description) {
    Entity node = description.node;
    JointAnchorId anchor = description.anchor;

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
    if(description.owner != ENTITY_INVALID && !entity_alive_check(description.owner))
        return ERROR_RESULT_MAKE_ERROR(SoftBodyNodeAnchorPinResult, ERROR_ENGINE_INVALID_ENTITY);
    node_anchor = physics_joint_anchor_add((JointAnchorDescription){.entity = node, .local_offset = (Vec2D){0.0f, 0.0f}});
    if(node_anchor.kind == ERROR_RESULT_ERROR) {
        return ERROR_RESULT_MAKE_ERROR(SoftBodyNodeAnchorPinResult, node_anchor.result.error);
    }
    joint = entity_default_create("joint", ENTITY_INVALID);
    if(joint.kind == ERROR_RESULT_ERROR) {
        (void)physics_joint_anchor_remove(node_anchor.result.value);
        return ERROR_RESULT_MAKE_ERROR(SoftBodyNodeAnchorPinResult, joint.result.error);
    }
    pin_result = physics_joint_description_set(joint.result.value, (JointDescription){
        .owner = description.owner, .type = JOINT_PIN,
        .anchor_a = node_anchor.result.value, .anchor_b = anchor});
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

SoftBodyBeamCollisionDescription physics_soft_body_beam_collision_description_default_get(void) {
    return (SoftBodyBeamCollisionDescription){.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
        .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}};
}

SoftBodyBeamDescription physics_soft_body_beam_description_default_get(void) {
    return (SoftBodyBeamDescription){.restitution = 0.25f,
        .collision = physics_soft_body_beam_collision_description_default_get()};
}

static bool beam_collision_description_check(SoftBodyBeamCollisionDescription description) {
    return isfinite(description.thickness) && description.thickness >= 0 &&
        (!description.enabled || description.thickness >= ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN);
}

static EngineResult beam_endpoints_check(Entity node_a, Entity node_b) {
    if(node_a != ENTITY_INVALID && node_a == node_b) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    Entity nodes[] = {node_a, node_b};
    for(size_t i = 0; i < 2; ++i) {
        if(nodes[i] == ENTITY_INVALID) continue;
        SoftBodyNodeResult node = physics_soft_body_node_get(nodes[i]);
        if(error_check(node)) return error_result_error(node.result.error);
    }
    return error_result_value(true);
}

SoftBodyBeamDescriptionResult physics_soft_body_beam_description_get(Entity entity) {
    SoftBodyBeamResult value = physics_soft_body_beam_get(entity);
    if(error_check(value)) return ERROR_RESULT_MAKE_ERROR(SoftBodyBeamDescriptionResult, value.result.error);
    SoftBodyBeam beam = value.result.value;
    return ERROR_RESULT_MAKE_VALUE(SoftBodyBeamDescriptionResult, ((SoftBodyBeamDescription){
        .owner = entity_owner_get(entity).result.value, .node_a = beam.node_a, .node_b = beam.node_b,
        .rest_length = beam.rest_length, .rest_length_explicit = true, .stiffness = beam.stiffness,
        .damping = beam.damping, .friction = beam.friction, .restitution = beam.restitution,
        .collision = {.enabled = beam.collision_enabled, .thickness = beam.collision_thickness,
            .filter = {.category = beam.category, .responds_to = beam.responds_to}},
        .draw_color = beam.draw_color, .draw_color_overridden = beam.draw_color_overridden}));
}

EngineResult physics_soft_body_beam_description_set(Entity entity, SoftBodyBeamDescription description) {
    SoftBodyBeamResult value = physics_soft_body_beam_get(entity);
    if(error_check(value)) return error_result_error(value.result.error);
    if(!isfinite(description.stiffness) || description.stiffness < 0 || !isfinite(description.damping) || description.damping < 0 ||
            !isfinite(description.rest_length) || description.rest_length < 0 || !isfinite(description.friction) || description.friction < 0 ||
            !isfinite(description.restitution) || description.restitution < 0 || description.restitution > 1 ||
            !beam_collision_description_check(description.collision))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    EngineResult result = beam_endpoints_check(description.node_a, description.node_b);
    if(error_check(result)) return result;
    float length = description.rest_length;
    if(!description.rest_length_explicit) {
        length = 0;
        if(description.node_a && description.node_b) {
            PositionResult a = physics_position_get(description.node_a), b = physics_position_get(description.node_b);
            if(error_check(a)) return error_result_error(a.result.error);
            if(error_check(b)) return error_result_error(b.result.error);
            double measured = hypot((double)b.result.value.x - a.result.value.x, (double)b.result.value.y - a.result.value.y);
            if(!isfinite(measured) || measured > FLT_MAX) return error_result_error(ERROR_ENGINE_STATE_INVALID);
            length = (float)measured;
        }
    }
    result = entity_owner_set(entity, description.owner);
    if(error_check(result)) return result;
    EntityIndex index; (void)entity_index_get(entity, &index);
    soft_body_beams[index] = (SoftBodyBeam){.soft_body = soft_body_beams[index].soft_body,
        .node_a = description.node_a, .node_b = description.node_b,
        .rest_length = length, .stiffness = description.stiffness, .damping = description.damping,
        .collision_thickness = description.collision.thickness, .collision_enabled = description.collision.enabled,
        .category = description.collision.filter.category, .responds_to = description.collision.filter.responds_to,
        .friction = description.friction, .restitution = description.restitution,
        .draw_color = description.draw_color, .draw_color_overridden = description.draw_color_overridden};
    return error_result_value(true);
}

EntityResult physics_soft_body_beam_add(SoftBodyBeamDescription description) {
    EntityResult created = entity_default_create("beam", description.owner);
    if(error_check(created)) return created;
    EntityIndex index; (void)entity_index_get(created.result.value, &index);
    (void)SoftBodyBeamPool_store_at(&soft_body_beams_pool, index, (SoftBodyBeam){0});
    entity_mask[index] |= ROHR_SOFT_BODY_BEAM;
    EngineResult result = physics_soft_body_beam_description_set(created.result.value, description);
    if(error_check(result)) {
        (void)entity_delete(created.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    }
    return created;
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

EngineResult physics_soft_body_beam_endpoints_set(Entity beam, Entity node_a, Entity node_b) {
    SoftBodyBeamResult value = physics_soft_body_beam_get(beam);
    if(error_check(value)) return error_result_error(value.result.error);
    EngineResult result = beam_endpoints_check(node_a, node_b);
    if(error_check(result)) return result;
    EntityIndex index; (void)entity_index_get(beam, &index);
    soft_body_beams[index].node_a = node_a;
    soft_body_beams[index].node_b = node_b;
    return error_result_value(true);
}

EngineResult physics_soft_body_beam_collision_description_set(Entity beam, SoftBodyBeamCollisionDescription description) {
    SoftBodyBeamResult value = physics_soft_body_beam_get(beam);
    if(error_check(value)) return error_result_error(value.result.error);
    if(!beam_collision_description_check(description)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    EntityIndex index; (void)entity_index_get(beam, &index);
    soft_body_beams[index].collision_enabled = description.enabled;
    soft_body_beams[index].collision_thickness = description.thickness;
    soft_body_beams[index].category = description.filter.category;
    soft_body_beams[index].responds_to = description.filter.responds_to;
    return error_result_value(true);
}

SoftBodyBeamCollisionDescriptionResult physics_soft_body_beam_collision_description_get(Entity entity) {
    SoftBodyBeamDescriptionResult description = physics_soft_body_beam_description_get(entity);
    if(error_check(description)) return ERROR_RESULT_MAKE_ERROR(SoftBodyBeamCollisionDescriptionResult, description.result.error);
    return ERROR_RESULT_MAKE_VALUE(SoftBodyBeamCollisionDescriptionResult, description.result.value.collision);
}

EngineResult physics_soft_body_beam_collision_enable(Entity entity) {
    SoftBodyBeamCollisionDescriptionResult description = physics_soft_body_beam_collision_description_get(entity);
    if(error_check(description)) return error_result_error(description.result.error);
    description.result.value.enabled = true;
    return physics_soft_body_beam_collision_description_set(entity, description.result.value);
}

EngineResult physics_soft_body_beam_collision_disable(Entity entity) {
    SoftBodyBeamCollisionDescriptionResult description = physics_soft_body_beam_collision_description_get(entity);
    if(error_check(description)) return error_result_error(description.result.error);
    description.result.value.enabled = false;
    return physics_soft_body_beam_collision_description_set(entity, description.result.value);
}

EngineResult physics_soft_body_beam_collision_thickness_set(Entity entity, float thickness) {
    SoftBodyBeamCollisionDescriptionResult description = physics_soft_body_beam_collision_description_get(entity);
    if(error_check(description)) return error_result_error(description.result.error);
    description.result.value.thickness = thickness;
    return physics_soft_body_beam_collision_description_set(entity, description.result.value);
}

EngineResult physics_soft_body_beam_collision_filter_description_set(Entity beam, CollisionFilterDescription description) {
    SoftBodyBeamResult value = physics_soft_body_beam_get(beam);
    if(error_check(value)) return error_result_error(value.result.error);
    EntityIndex index; (void)entity_index_get(beam, &index);
    soft_body_beams[index].category = description.category;
    soft_body_beams[index].responds_to = description.responds_to;
    return error_result_value(true);
}

CollisionFilterDescriptionResult physics_soft_body_beam_collision_filter_description_get(Entity entity) {
    SoftBodyBeamCollisionDescriptionResult description = physics_soft_body_beam_collision_description_get(entity);
    if(error_check(description)) return ERROR_RESULT_MAKE_ERROR(CollisionFilterDescriptionResult, description.result.error);
    return ERROR_RESULT_MAKE_VALUE(CollisionFilterDescriptionResult, description.result.value.filter);
}
