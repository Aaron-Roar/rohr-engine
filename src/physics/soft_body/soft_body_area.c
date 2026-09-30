/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "soft_body_area.h"
#include "physics/physics_internal.h"

#include <string.h>
#include <stdlib.h>

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

/* Shared scratch is engine-owned; per-area caches own only poses and fill. */
static NodeLoopWorkspace *area_workspace;
_Static_assert(SOFT_BODY_MAX_NODES == NODE_LOOP_MAX_POINTS, "area node limit mismatch");
_Static_assert(SOFT_BODY_MAX_AREA_HOLES + 1 == NODE_LOOP_MAX_LOOPS, "area loop limit mismatch");

static const SoftBodyAreaLoop *area_loop_get(const SoftBodyAreaGeometry *geometry, uint32_t loop) {
    return loop == 0 ? &geometry->outer : &geometry->holes[loop-1];
}
static void area_cache_destroy(SoftBodyAreaCache *cache) {
    if(cache == NULL) return;
    node_loop_fill_destroy(&cache->fill); free(cache);
}
void physics_soft_body_area_tables_clear(void) {
    for(size_t i=0;i<soft_body_area_states_pool.capacity;i+=1)
        if(soft_body_area_states_pool.used[i]) area_cache_destroy(soft_body_area_states[i].cache);
    node_loop_workspace_destroy(area_workspace); area_workspace=NULL;
}
static EngineResult area_fill_build(const NodeLoopPoints *poses, size_t count, NodeLoopFill *fill) {
    if(area_workspace == NULL) area_workspace=node_loop_workspace_create();
    if(area_workspace == NULL) return error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    NodeLoopGeometryStatus status=node_loop_fill_build(poses,count,area_workspace,fill);
    if(status == NODE_LOOP_GEOMETRY_ALLOCATION_FAILED)
        return error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    if(status != NODE_LOOP_GEOMETRY_OK) return error_result_error(ERROR_ENGINE_INVALID_SHAPE);
    return error_result_value(true);
}
static EngineResult area_poses_get(const SoftBodyAreaGeometry *geometry, NodeLoopPoints *poses) {
    for(uint32_t l=0;l<=geometry->hole_count;l+=1) {
        const SoftBodyAreaLoop *loop=area_loop_get(geometry,l);
        poses[l].count=loop->node_count;
        for(uint32_t i=0;i<loop->node_count;i+=1) {
            PositionResult position=physics_position_get(loop->nodes[i]);
            if(error_check(position)) return error_result_error(position.result.error);
            poses[l].points[i]=position.result.value;
        }
    }
    return error_result_value(true);
}
static EngineResult physics_soft_body_area_prepare(Entity owner,
        const SoftBodyAreaGeometry *geometry, SoftBodyAreaState *state) {
    if(geometry->hole_count > SOFT_BODY_MAX_AREA_HOLES)
        return error_result_error(ERROR_ENGINE_MAX_AREA_HOLES_EXCEEDED);
    SoftBodyAreaGeometry normalized={.hole_count=geometry->hole_count};
    for(uint32_t l=0;l<=geometry->hole_count;l+=1) {
        const SoftBodyAreaLoop *loop=area_loop_get(geometry,l);
        if(loop->node_count<3 || loop->node_count>SOFT_BODY_MAX_NODES)
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        for(uint32_t i=0;i<loop->node_count;i+=1) {
            SoftBodyNodeResult node=physics_soft_body_node_get(loop->nodes[i]);
            if(error_check(node)) return error_result_error(node.result.error);
            if(node.result.value.soft_body!=owner) return error_result_error(ERROR_ENGINE_STATE_INVALID);
            for(uint32_t j=0;j<i;j+=1)
                if(loop->nodes[i]==loop->nodes[j]) return error_result_error(ERROR_ENGINE_STATE_INVALID);
        }
        SoftBodyAreaLoop *copy=l==0 ? &normalized.outer : &normalized.holes[l-1];
        copy->node_count=loop->node_count;
        memcpy(copy->nodes,loop->nodes,loop->node_count*sizeof(Entity));
    }
    SoftBodyAreaCache *cache=calloc(1,sizeof(*cache));
    if(cache==NULL) return error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    EngineResult result=area_poses_get(&normalized,cache->poses);
    if(!error_check(result)) result=area_fill_build(cache->poses,geometry->hole_count+1,&cache->fill);
    uint32_t required=(UINT32_C(1)<<(geometry->hole_count+1))-1;
    if(!error_check(result) && cache->fill.enclosed_loops!=required)
        result=error_result_error(ERROR_ENGINE_INVALID_SHAPE);
    if(error_check(result)) { area_cache_destroy(cache); return result; }
    state->value.soft_body=owner;
    state->value.geometry=normalized;
    state->cache=cache;
    return error_result_value(true);
}

EngineResult physics_soft_body_area_fill_get(Entity area, const NodeLoopFill **fill) {
    if(fill==NULL) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    EntityIndex index;
    EngineResult result=physics_soft_body_area_index_get(area,&index);
    if(error_check(result)) return result;
    SoftBodyAreaState *state=&soft_body_area_states[index];
    NodeLoopPoints poses[NODE_LOOP_MAX_LOOPS]={0};
    result=area_poses_get(&state->value.geometry,poses);
    if(error_check(result)) return result;
    if(memcmp(poses,state->cache->poses,sizeof(poses))!=0) {
        result=area_fill_build(poses,state->value.geometry.hole_count+1,&state->cache->fill);
        if(error_check(result)) return result;
        memcpy(state->cache->poses,poses,sizeof(poses));
    }
    *fill=&state->cache->fill;
    return error_result_value(true);
}

EntityResult physics_soft_body_area_create(Entity soft_body, SoftBodyAreaGeometry geometry) {
    EntityIndex body_index, area_index;
    SoftBodyResult body = physics_soft_body_get(soft_body);
    SoftBodyAreaState state = {.value = {.visible = true}};
    if(error_check(body)) return ERROR_RESULT_MAKE_ERROR(EntityResult, body.result.error);
    if(body.result.value.area_count >= SOFT_BODY_MAX_AREAS)
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_MAX_ENTITIES_EXCEEDED);
    EngineResult prepared = physics_soft_body_area_prepare(soft_body, &geometry, &state);
    if(error_check(prepared)) return ERROR_RESULT_MAKE_ERROR(EntityResult, prepared.result.error);
    if(!entity_index_get(soft_body, &body_index)) {
        area_cache_destroy(state.cache);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_INVALID_ENTITY);
    }
    EntityResult created = entity_add();
    if(error_check(created)) { area_cache_destroy(state.cache); return created; }
    if(!entity_index_get(created.result.value, &area_index) ||
            error_check(SoftBodyAreaStatePool_store_at(&soft_body_area_states_pool,
                area_index, state))) {
        area_cache_destroy(state.cache);
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

EngineResult physics_soft_body_area_geometry_set(Entity area, SoftBodyAreaGeometry geometry) {
    EntityIndex index;
    EngineResult result = physics_soft_body_area_index_get(area, &index);
    if(error_check(result)) return result;
    SoftBodyAreaState state = soft_body_area_states[index];
    state.cache = NULL;
    result = physics_soft_body_area_prepare(state.value.soft_body, &geometry, &state);
    if(error_check(result)) return result;
    area_cache_destroy(soft_body_area_states[index].cache);
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
    area_cache_destroy(soft_body_area_states[index].cache);
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
        const SoftBodyAreaGeometry *geometry=&soft_body_area_states[index].value.geometry;
        bool dependent=false;
        for(uint32_t l=0;l<=geometry->hole_count && !dependent;l+=1) {
            const SoftBodyAreaLoop *loop=area_loop_get(geometry,l);
            for(uint32_t j=0;j<loop->node_count;j+=1)
                if(loop->nodes[j]==node) { dependent=true; break; }
        }
        if(dependent) (void)entity_delete(area);
    }
}
