/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics.h"

#include "physics/physics_internal.h"
#include "physics/physics_operation.h"
#include <math.h>

EngineResult physics_angle_lock_set(
        Entity entity, Orientation min, Orientation max) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);

    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(!isfinite(min) || !isfinite(max))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    result = entity_components_add(entity, ROHR_ANGLE_LOCK);
    if(result.kind == ERROR_RESULT_ERROR) return result;
    (void)AngleLockPool_store_at(&angle_locks_pool, index, (AngleLock){
        .min = min,
        .max = max
    });
    return error_result_value(true);
}

EngineResult physics_axis_lock_description_set(Entity entity, AxisLockDescription description) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    if(error_check(result)) return result;
    if(!physics_vector_finite_check(description.axis))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(!physics_world_position_check(description.point_on_axis))
        return error_result_error(ERROR_ENGINE_POSITION_OUT_OF_RANGE);
    /* Double precision avoids overflow/underflow while normalizing finite float axes. */
    double magnitude = hypot((double)description.axis.x, (double)description.axis.y);
    if(magnitude == 0) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    AxisLock lock = {
        .axis = {(float)(description.axis.x / magnitude), (float)(description.axis.y / magnitude)},
        .point_on_axis = description.point_on_axis
    };
    if(error_check(AxisLockPool_store_at(&axis_locks_pool, index, lock)))
        return error_result_error(ERROR_ENGINE_TABLE_EXPANSION_FAILED);
    entity_mask[index] |= ROHR_AXIS_LOCK;
    return error_result_value(true);
}

AxisLockDescriptionResult physics_axis_lock_description_get(Entity entity) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(AxisLockDescriptionResult, result.result.error);
    if(!entity_index_components_check(index, ROHR_AXIS_LOCK) ||
            index >= axis_locks_pool.capacity || !axis_locks_pool.used[index])
        return ERROR_RESULT_MAKE_ERROR(AxisLockDescriptionResult, ERROR_ENGINE_COMPONENT_MISSING);
    AxisLockDescription description = {.axis = axis_locks[index].axis,
        .point_on_axis = axis_locks[index].point_on_axis};
    return ERROR_RESULT_MAKE_VALUE(AxisLockDescriptionResult, description);
}

EngineResult physics_axis_lock_remove(Entity entity) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    if(error_check(result)) return result;
    entity_mask[index] &= ~ROHR_AXIS_LOCK;
    if(index < axis_locks_pool.capacity && axis_locks_pool.used[index])
        (void)AxisLockPool_release_at(&axis_locks_pool, index);
    return error_result_value(true);
}

EngineResult physics_transform_lock_description_set(Entity driven, TransformLockDescription description) {
    EntityIndex driven_index, driver_index;
    EngineResult result = physics_live_index_get(driven, &driven_index);
    if(error_check(result)) return result;
    result = physics_live_index_get(description.driver, &driver_index);
    if(error_check(result)) return result;
    if(!physics_vector_finite_check(description.local_offset) || !isfinite(description.local_angle))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    TransformLock lock = {
        .driver = description.driver, .local_offset = description.local_offset,
        .local_angle = description.local_angle, .lock_position = description.lock_position,
        .lock_orientation = description.lock_orientation, .inherit_velocity = description.inherit_velocity
    };
    if(error_check(TransformLockPool_store_at(&transform_locks_pool, driven_index, lock)))
        return error_result_error(ERROR_ENGINE_TABLE_EXPANSION_FAILED);
    entity_mask[driven_index] |= ROHR_TRANSFORM_LOCK;
    return error_result_value(true);
}

TransformLockDescriptionResult physics_transform_lock_description_get(Entity driven) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(driven, &index);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(TransformLockDescriptionResult, result.result.error);
    if(!entity_index_components_check(index, ROHR_TRANSFORM_LOCK) ||
            index >= transform_locks_pool.capacity || !transform_locks_pool.used[index])
        return ERROR_RESULT_MAKE_ERROR(TransformLockDescriptionResult, ERROR_ENGINE_COMPONENT_MISSING);
    TransformLock lock = transform_locks[index];
    TransformLockDescription description = {
        .driver = lock.driver, .local_offset = lock.local_offset, .local_angle = lock.local_angle,
        .lock_position = lock.lock_position, .lock_orientation = lock.lock_orientation,
        .inherit_velocity = lock.inherit_velocity
    };
    return ERROR_RESULT_MAKE_VALUE(TransformLockDescriptionResult, description);
}

EngineResult physics_transform_lock_remove(Entity entity) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);

    if(result.kind == ERROR_RESULT_ERROR) return result;
    result = entity_components_delete(entity, ROHR_TRANSFORM_LOCK);
    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(index < transform_locks_pool.capacity && transform_locks_pool.used[index])
        (void)TransformLockPool_release_at(&transform_locks_pool, index);
    return error_result_value(true);
}

EngineResult physics_transform_lock_current_transform_set(
        Entity driven,
        Entity driver,
        bool lock_position,
        bool lock_orientation,
        bool inherit_velocity) {
    EntityIndex driven_index;
    EntityIndex driver_index;
    Vec2D world_offset;
    Vec2D local_offset;
    Orientation local_angle;
    EngineResult result = physics_live_index_get(driven, &driven_index);

    if(result.kind == ERROR_RESULT_ERROR) return result;
    result = physics_live_index_get(driver, &driver_index);
    if(result.kind == ERROR_RESULT_ERROR) return result;
    world_offset = (Vec2D){
        .x = positions[driven_index].x - positions[driver_index].x,
        .y = positions[driven_index].y - positions[driver_index].y
    };
    local_offset = math_vector_rotate(
        world_offset, -orientations[driver_index]);
    local_angle = orientations[driven_index] - orientations[driver_index];
    return physics_transform_lock_description_set(driven, (TransformLockDescription){
        .driver = driver, .local_offset = local_offset, .local_angle = local_angle,
        .lock_position = lock_position, .lock_orientation = lock_orientation, .inherit_velocity = inherit_velocity});
}
