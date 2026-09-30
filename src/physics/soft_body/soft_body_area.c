/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "soft_body_area.h"
#include "physics/physics_internal.h"

#include <string.h>

EngineResult physics_soft_body_area_index_get(Entity area, EntityIndex *index) {
    if(index == NULL) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    EngineResult result = physics_live_index_get(area, index);
    if(error_check(result)) return result;
    if(!entity_index_components_check(*index, ROHR_SOFT_BODY_AREA) ||
            *index >= soft_body_area_states_pool.capacity ||
            !soft_body_area_states_pool.used[*index])
        return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    return error_result_value(true);
}

static EngineResult physics_soft_body_area_prepare(Entity owner,
        SoftBodyAreaLoop loop, SoftBodyAreaState *state) {
    Position reference[SOFT_BODY_MAX_NODES] = {0};
    if(loop.node_count < 3 || loop.node_count > SOFT_BODY_MAX_NODES)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    for(uint32_t i = 0; i < loop.node_count; i += 1) {
        SoftBodyNodeResult node = physics_soft_body_node_get(loop.nodes[i]);
        if(error_check(node)) return error_result_error(node.result.error);
        if(node.result.value.soft_body != owner)
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        for(uint32_t j = 0; j < i; j += 1)
            if(loop.nodes[i] == loop.nodes[j])
                return error_result_error(ERROR_ENGINE_STATE_INVALID);
        PositionResult position = physics_soft_body_node_local_position_get(loop.nodes[i]);
        if(error_check(position)) return error_result_error(position.result.error);
        reference[i] = position.result.value;
    }
    NodeLoopTriangulation mesh;
    if(node_loop_triangulation_get(reference, loop.node_count, &mesh) != NODE_LOOP_GEOMETRY_OK)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    state->value.soft_body = owner;
    state->value.loop = (SoftBodyAreaLoop){.node_count = loop.node_count};
    memcpy(state->value.loop.nodes, loop.nodes, loop.node_count * sizeof(Entity));
    memcpy(state->value.reference_positions, reference, sizeof(reference));
    state->triangulation = mesh;
    return error_result_value(true);
}

EntityResult physics_soft_body_area_create(Entity soft_body, SoftBodyAreaLoop loop) {
    EntityIndex body_index, area_index;
    SoftBodyResult body = physics_soft_body_get(soft_body);
    SoftBodyAreaState state = {.value = {.visible = true}};
    if(error_check(body)) return ERROR_RESULT_MAKE_ERROR(EntityResult, body.result.error);
    if(body.result.value.area_count >= SOFT_BODY_MAX_AREAS)
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_MAX_ENTITIES_EXCEEDED);
    EngineResult prepared = physics_soft_body_area_prepare(soft_body, loop, &state);
    if(error_check(prepared)) return ERROR_RESULT_MAKE_ERROR(EntityResult, prepared.result.error);
    if(!entity_index_get(soft_body, &body_index))
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_INVALID_ENTITY);
    EntityResult created = entity_add();
    if(error_check(created)) return created;
    if(!entity_index_get(created.result.value, &area_index) ||
            error_check(SoftBodyAreaStatePool_store_at(&soft_body_area_states_pool,
                area_index, state))) {
        (void)entity_delete(created.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    entity_mask[area_index] |= ROHR_SOFT_BODY_AREA;
    soft_bodies[body_index].areas[soft_bodies[body_index].area_count++] = created.result.value;
    return created;
}

SoftBodyAreaResult physics_soft_body_area_get(Entity area) {
    EntityIndex index;
    EngineResult result = physics_soft_body_area_index_get(area, &index);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(SoftBodyAreaResult, result.result.error);
    return ERROR_RESULT_MAKE_VALUE(SoftBodyAreaResult, soft_body_area_states[index].value);
}

EngineResult physics_soft_body_area_nodes_set(Entity area, SoftBodyAreaLoop loop) {
    EntityIndex index;
    EngineResult result = physics_soft_body_area_index_get(area, &index);
    if(error_check(result)) return result;
    SoftBodyAreaState state = soft_body_area_states[index];
    result = physics_soft_body_area_prepare(state.value.soft_body, loop, &state);
    if(error_check(result)) return result;
    soft_body_area_states[index] = state;
    return error_result_value(true);
}

EngineResult physics_soft_body_area_order_set(Entity area, uint32_t index) {
    EntityIndex area_index, body_index;
    EngineResult result = physics_soft_body_area_index_get(area, &area_index);
    if(error_check(result)) return result;
    if(!entity_index_get(soft_body_area_states[area_index].value.soft_body, &body_index))
        return error_result_error(ERROR_ENGINE_INVALID_ENTITY);
    SoftBody *body = &soft_bodies[body_index];
    if(index >= body->area_count) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    uint32_t previous = 0;
    while(previous < body->area_count && body->areas[previous] != area) previous += 1;
    if(previous == body->area_count) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(previous < index)
        memmove(&body->areas[previous], &body->areas[previous + 1],
            (index - previous) * sizeof(Entity));
    else if(previous > index)
        memmove(&body->areas[index + 1], &body->areas[index],
            (previous - index) * sizeof(Entity));
    body->areas[index] = area;
    return error_result_value(true);
}

void physics_soft_body_area_entity_clear(Entity area, EntityIndex index) {
    if(index >= soft_body_area_states_pool.capacity || !soft_body_area_states_pool.used[index])
        return;
    EntityIndex owner;
    if(entity_index_get(soft_body_area_states[index].value.soft_body, &owner) &&
            owner < soft_bodies_pool.capacity && soft_bodies_pool.used[owner]) {
        SoftBody *body = &soft_bodies[owner];
        for(uint32_t i = 0; i < body->area_count; i += 1) {
            if(body->areas[i] != area) continue;
            memmove(&body->areas[i], &body->areas[i + 1],
                (body->area_count - i - 1) * sizeof(Entity));
            body->areas[--body->area_count] = ENTITY_INVALID;
            break;
        }
    }
    (void)SoftBodyAreaStatePool_release_at(&soft_body_area_states_pool, index);
}

void physics_soft_body_areas_node_clear(Entity node, Entity owner) {
    EntityIndex body_index;
    if(!entity_index_get(owner, &body_index) || body_index >= soft_bodies_pool.capacity ||
            !soft_bodies_pool.used[body_index]) return;
    for(uint32_t i = soft_bodies[body_index].area_count; i > 0; i -= 1) {
        Entity area = soft_bodies[body_index].areas[i - 1];
        EntityIndex index;
        if(error_check(physics_soft_body_area_index_get(area, &index))) continue;
        const SoftBodyAreaLoop *loop = &soft_body_area_states[index].value.loop;
        for(uint32_t j = 0; j < loop->node_count; j += 1) {
            if(loop->nodes[j] != node) continue;
            (void)entity_delete(area);
            break;
        }
    }
}
