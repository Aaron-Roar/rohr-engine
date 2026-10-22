/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics.h"

#include "physics/physics_internal.h"

CollisionFilterDescription physics_collision_filter_description_default_get(void) {
    return (CollisionFilterDescription){
        .category = ROHR_COLLISION_CATEGORY_DEFAULT,
        .responds_to = ROHR_COLLISION_CATEGORY_ALL
    };
}

EngineResult physics_collision_filter_description_set(Entity entity,
        CollisionFilterDescription config) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);

    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(entity_index_components_check(index, ROHR_SOFT_BODY_BEAM))
        return physics_soft_body_beam_collision_filter_description_set(entity, config);
    if(CollisionFilterDescriptionPool_store_at(
            &collision_filters_pool, index, config).kind == ERROR_RESULT_ERROR)
        return error_result_error(ERROR_ENGINE_TABLE_EXPANSION_FAILED);
    entity_mask[index] |= ROHR_COLLISION_FILTER;
    return error_result_value(true);
}

CollisionFilterDescriptionResult physics_collision_filter_description_get(Entity entity) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);

    if(result.kind == ERROR_RESULT_ERROR)
        return ERROR_RESULT_MAKE_ERROR(
            CollisionFilterDescriptionResult, result.result.error);
    if(entity_index_components_check(index, ROHR_SOFT_BODY_BEAM))
        return physics_soft_body_beam_collision_filter_description_get(entity);
    if(!entity_index_components_check(index, ROHR_COLLISION_FILTER))
        return ERROR_RESULT_MAKE_VALUE(CollisionFilterDescriptionResult,
            physics_collision_filter_description_default_get());
    return ERROR_RESULT_MAKE_VALUE(
        CollisionFilterDescriptionResult, collision_filters[index]);
}

EngineResult physics_collision_category_set(Entity entity,
        RohrCollisionCategoryMask category) {
    CollisionFilterDescriptionResult result = physics_collision_filter_description_get(entity);

    if(result.kind == ERROR_RESULT_ERROR)
        return error_result_error(result.result.error);
    result.result.value.category = category;
    return physics_collision_filter_description_set(entity, result.result.value);
}

EngineResult physics_collision_response_set(Entity entity,
        RohrCollisionCategoryMask categories) {
    CollisionFilterDescriptionResult result = physics_collision_filter_description_get(entity);

    if(result.kind == ERROR_RESULT_ERROR)
        return error_result_error(result.result.error);
    result.result.value.responds_to = categories;
    return physics_collision_filter_description_set(entity, result.result.value);
}

EngineResult physics_collision_response_all_set(Entity entity) {
    return physics_collision_response_set(entity, ROHR_COLLISION_CATEGORY_ALL);
}

EngineResult physics_collision_response_none_set(Entity entity) {
    return physics_collision_response_set(entity, ROHR_COLLISION_CATEGORY_NONE);
}

bool physics_collision_response_to_check(Entity entity_1, Entity entity_2) {
    CollisionFilterDescriptionResult first = physics_collision_filter_description_get(entity_1);
    CollisionFilterDescriptionResult second = physics_collision_filter_description_get(entity_2);

    if(first.kind == ERROR_RESULT_ERROR || second.kind == ERROR_RESULT_ERROR)
        return false;
    return (first.result.value.responds_to & second.result.value.category) != 0;
}

bool physics_collision_response_between_check(Entity entity_1, Entity entity_2) {
    return physics_collision_response_to_check(entity_1, entity_2) ||
        physics_collision_response_to_check(entity_2, entity_1);
}
