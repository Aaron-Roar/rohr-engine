/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics/physics_internal.h"
#include "physics/physics_step_internal.h"
#include <float.h>
#include <math.h>

typedef struct BodyMassProperties {
    Position explicit_com;
    Position centroid;
    double unit_inertia;
    bool explicit_mode;
    bool geometry_present;
} BodyMassProperties;

MEMORY_DECLARE_OBJECT_POOL(BodyMassPropertiesPool, BodyMassProperties);
MEMORY_DEFINE_OBJECT_POOL(BodyMassPropertiesPool, BodyMassProperties)
static BodyMassPropertiesPool properties = {0};

EngineResult physics_mass_properties_reserve(size_t capacity) {
    if(capacity <= properties.capacity) return error_result_value(true);
    BodyMassPropertiesPoolResult result = BodyMassPropertiesPool_expand(
        &properties, capacity - properties.capacity);
    return result.kind == ERROR_RESULT_ERROR ?
        error_result_error(result.result.error) : error_result_value(true);
}

void physics_mass_properties_destroy(void) {
    (void)BodyMassPropertiesPool_destroy(&properties);
}

void physics_mass_properties_clear(EntityIndex index) {
    if(index < properties.capacity) {
        properties.objects[index] = (BodyMassProperties){0};
        if(properties.used[index])
            (void)BodyMassPropertiesPool_release_at(&properties, index);
    }
}

/* Shift to a vertex before accumulating to avoid cancellation for polygons
 * authored far from the origin. Signed sums support either winding and concavity. */
bool physics_shape_mass_properties_get(const Shape *shape,
        Position *centroid, double *unit_inertia) {
    if(shape == NULL || centroid == NULL || unit_inertia == NULL ||
            shape->amount_of_vertices < 3 ||
            shape->amount_of_vertices > MAX_VERTICIES) return false;
    double x0 = shape->vertices[0].x, y0 = shape->vertices[0].y;
    double cross_sum = 0, cx = 0, cy = 0, moment = 0;
    for(size_t i = 0; i < shape->amount_of_vertices; i += 1) {
        Position a = shape->vertices[i];
        Position b = shape->vertices[(i + 1) % shape->amount_of_vertices];
        if(!physics_world_position_check(a)) return false;
        double ax = a.x - x0, ay = a.y - y0;
        double bx = b.x - x0, by = b.y - y0;
        double cross = ax * by - bx * ay;
        cross_sum += cross;
        cx += (ax + bx) * cross;
        cy += (ay + by) * cross;
    }
    if(!isfinite(cross_sum) || fabs(cross_sum) <= 0.00002) return false;
    cx /= 3.0 * cross_sum;
    cy /= 3.0 * cross_sum;
    /* Accumulate inertia directly about the centroid, avoiding subtraction of
     * two large moments for small shapes with large local offsets. */
    for(size_t i = 0; i < shape->amount_of_vertices; i += 1) {
        Position a = shape->vertices[i];
        Position b = shape->vertices[(i + 1) % shape->amount_of_vertices];
        double ax = a.x - x0 - cx, ay = a.y - y0 - cy;
        double bx = b.x - x0 - cx, by = b.y - y0 - cy;
        moment += (ax * by - bx * ay) *
            (ax * ax + ax * bx + bx * bx + ay * ay + ay * by + by * by);
    }
    *centroid = (Position){(float)(x0 + cx), (float)(y0 + cy)};
    *unit_inertia = moment / (6.0 * cross_sum);
    return physics_world_position_check(*centroid) &&
        isfinite(*unit_inertia) && *unit_inertia > 0;
}

void physics_mass_properties_geometry_set(EntityIndex index, const Shape *shape) {
    BodyMassProperties *value = &properties.objects[index];
    value->geometry_present = shape != NULL &&
        physics_shape_mass_properties_get(shape, &value->centroid, &value->unit_inertia);
    (void)BodyMassPropertiesPool_store_at(&properties, index, *value);
}

static bool geometry_present_check(EntityIndex index) {
    return index < properties.capacity && properties.objects[index].geometry_present &&
        entity_index_components_check(index, ROHR_HIT_BOX);
}

Position physics_com_local_by_index_get(EntityIndex index) {
    if(index >= properties.capacity) return (Position){0};
    const BodyMassProperties *value = &properties.objects[index];
    if(index < particle_geometries_pool.capacity && particle_geometries_pool.used[index] &&
            particle_geometries_pool.objects[index].standalone)
        return particle_geometries_pool.objects[index].local_origin;
    if(value->explicit_mode) return value->explicit_com;
    return geometry_present_check(index) ? value->centroid : (Position){0};
}

Position physics_com_world_by_index_get(EntityIndex index) {
    Vec2D offset = math_vector_rotate(physics_com_local_by_index_get(index),
        orientations[index]);
    return (Position){positions[index].x + offset.x, positions[index].y + offset.y};
}

Velocity physics_point_velocity_by_index_get(EntityIndex index, Position point) {
    if(!physics_entity_movable_get(index)) return (Velocity){0};
    Vec2D lever = math_vector_subtract(point, physics_com_world_by_index_get(index));
    Vec2D angular = math_angular_velocity_cross_vec(
        entity_index_components_check(index, ROHR_PARTICLE) ? 0 : angular_velocities[index],
        lever);
    return (Velocity){velocities[index].x + angular.x, velocities[index].y + angular.y};
}

static double inertia_get(EntityIndex index, Position com) {
    const BodyMassProperties *value = &properties.objects[index];
    double dx = (double)value->centroid.x - com.x;
    double dy = (double)value->centroid.y - com.y;
    return (double)mass[index] * (value->unit_inertia + dx * dx + dy * dy);
}

float physics_inverse_inertia_by_index_get(EntityIndex index) {
    if(!physics_entity_simulated_get(index) ||
            entity_index_components_check(index, ROHR_PARTICLE) ||
            !geometry_present_check(index)) return 0;
    if(entity_index_components_check(index, ROHR_ANGLE_LOCK) &&
            angle_locks[index].min == angle_locks[index].max) return 0;
    double inertia = inertia_get(index, physics_com_local_by_index_get(index));
    if(!isfinite(inertia) || inertia <= 0 || inertia > FLT_MAX ||
            1.0 / inertia > FLT_MAX) return 0;
    return (float)(1.0 / inertia);
}

/* Simulation and solver angle changes hold COM fixed, unlike the public
 * orientation setter, which intentionally holds the authored origin fixed. */
void physics_com_orientation_set(EntityIndex index, float orientation) {
    if(orientations[index] == orientation) return;
    Position center = physics_com_world_by_index_get(index);
    Vec2D offset = math_vector_rotate(physics_com_local_by_index_get(index), orientation);
    orientations[index] = orientation;
    positions[index] = (Position){center.x - offset.x, center.y - offset.y};
    physics_step_hitbox_dirty_add(index);
}

EngineResult physics_center_of_mass_local_position_set(Entity entity, Position offset) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(index < particle_geometries_pool.capacity && particle_geometries_pool.used[index] &&
            particle_geometries_pool.objects[index].standalone)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(!physics_world_position_check(offset))
        return error_result_error(ERROR_ENGINE_POSITION_OUT_OF_RANGE);
    result = physics_mass_properties_reserve((size_t)index + 1);
    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(geometry_present_check(index) && entity_index_components_check(index, ROHR_MASS) &&
            (!isfinite(inertia_get(index, offset)) || inertia_get(index, offset) > FLT_MAX))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    properties.objects[index].explicit_com = offset;
    properties.objects[index].explicit_mode = true;
    (void)BodyMassPropertiesPool_store_at(&properties, index, properties.objects[index]);
    return error_result_value(true);
}

EngineResult physics_center_of_mass_automatic_set(Entity entity) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(index < properties.capacity) {
        properties.objects[index].explicit_mode = false;
        properties.objects[index].explicit_com = (Position){0};
    }
    return error_result_value(true);
}

BoolResult physics_center_of_mass_automatic_check(Entity entity) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    if(result.kind == ERROR_RESULT_ERROR)
        return ERROR_RESULT_MAKE_ERROR(BoolResult, result.result.error);
    return ERROR_RESULT_MAKE_VALUE(BoolResult, index >= properties.capacity ||
        !properties.objects[index].explicit_mode);
}

PositionResult physics_center_of_mass_local_position_get(Entity entity) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    if(result.kind == ERROR_RESULT_ERROR)
        return ERROR_RESULT_MAKE_ERROR(PositionResult, result.result.error);
    if((index >= properties.capacity || !properties.objects[index].explicit_mode) &&
            !geometry_present_check(index))
        return ERROR_RESULT_MAKE_ERROR(PositionResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(PositionResult, physics_com_local_by_index_get(index));
}

PositionResult physics_center_of_mass_world_position_get(Entity entity) {
    PositionResult result = physics_center_of_mass_local_position_get(entity);
    EntityIndex index;
    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(!entity_index_get(entity, &index) || !positions_pool.used[index] ||
            !orientations_pool.used[index])
        return ERROR_RESULT_MAKE_ERROR(PositionResult, ERROR_ENGINE_COMPONENT_MISSING);
    Position center = physics_com_world_by_index_get(index);
    if(!isfinite(center.x) || !isfinite(center.y))
        return ERROR_RESULT_MAKE_ERROR(PositionResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(PositionResult, center);
}

MomentOfInertiaResult physics_moment_of_inertia_get(Entity entity) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    if(result.kind == ERROR_RESULT_ERROR)
        return ERROR_RESULT_MAKE_ERROR(MomentOfInertiaResult, result.result.error);
    if(!geometry_present_check(index) || !entity_index_components_check(index, ROHR_MASS))
        return ERROR_RESULT_MAKE_ERROR(MomentOfInertiaResult, ERROR_ENGINE_COMPONENT_MISSING);
    double inertia = inertia_get(index, physics_com_local_by_index_get(index));
    if(!isfinite(inertia) || inertia < 0 || inertia > FLT_MAX ||
            (inertia > 0 && (float)inertia == 0))
        return ERROR_RESULT_MAKE_ERROR(MomentOfInertiaResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(MomentOfInertiaResult, (float)inertia);
}
