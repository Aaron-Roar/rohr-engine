/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics.h"
#include "entity/entity_internal.h"

#include "console.h"
#include "physics/physics_internal.h"
#include "physics/physics_operation.h"
#include "physics/physics_step_internal.h"

#include <float.h>
#include <math.h>

static Acceleration physics_gravity = ROHR_PHYSICS_GRAVITY_DEFAULT;

void physics_force_state_init(void) {
    physics_gravity = ROHR_PHYSICS_GRAVITY_DEFAULT;
}

EngineResult physics_impulse_apply(Entity entity, Vec2D impulse) {
    EntityIndex index;
    Velocity velocity;
    EngineResult result = physics_live_index_get(entity, &index);

    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(!physics_vector_finite_check(impulse)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(!physics_entity_simulated_get(index)) return error_result_value(true);
    if(!entity_index_components_check(index, ROHR_MASS))
        return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    if(mass[index] == 0.0f) return error_result_value(true);
    velocity = (Velocity){
        .x = velocities[index].x + impulse.x / mass[index],
        .y = velocities[index].y + impulse.y / mass[index]
    };
    if(!physics_vector_finite_check(velocity)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    (void)VelocityPool_store_at(&velocities_pool, index, velocity);
    console_debug_write(
        LOG_ENGINE,
        "Apply Entity: %d Impulse: {x: %f, y: %f} Velocity: {x: %f, y: %f}\n",
        entity, impulse.x, impulse.y, velocity.x, velocity.y);
    return error_result_value(true);
}

EngineResult physics_angular_impulse_apply(Entity entity, float impulse) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    float inverse_inertia;
    double velocity;

    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(!isfinite(impulse))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    inverse_inertia = physics_inverse_inertia_by_index_get(index);
    if(inverse_inertia == 0.0f) return error_result_value(true);
    velocity = (double)angular_velocities[index] +
        (double)impulse * inverse_inertia * (180.0 / (double)PI_F);
    if(!isfinite(velocity) || fabs(velocity) > FLT_MAX)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(AngularVelocityPool_store_at(
            &angular_velocities_pool, index, (AngularVelocity)velocity).kind
            == ERROR_RESULT_ERROR)
        return error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    return error_result_value(true);
}

EngineResult physics_acceleration_set(
        Entity entity, Acceleration acceleration) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);

    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(!physics_vector_finite_check(acceleration)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    (void)AccelerationPool_store_at(
        &accelerations_pool, index, acceleration);
    console_debug_write(
        LOG_ENGINE,
        "Set Entity: %d Acceleration: {x: %f, y: %f}\n",
        entity, acceleration.x, acceleration.y);
    return error_result_value(true);
}

EngineResult physics_acceleration_toward_position_set(
        Entity entity,
        float acceleration_magnitude,
        Position position) {
    EntityIndex index;
    Vec2D direction;
    Acceleration acceleration;
    EngineResult result = physics_live_index_get(entity, &index);

    if(result.kind == ERROR_RESULT_ERROR) return result;
    if(!isfinite(acceleration_magnitude)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(!physics_world_position_check(position))
        return error_result_error(ERROR_ENGINE_POSITION_OUT_OF_RANGE);
    direction = physics_direction_between_positions(positions[index], position);
    acceleration = (Acceleration){
        .x = direction.x * acceleration_magnitude,
        .y = direction.y * acceleration_magnitude
    };
    if(!physics_vector_finite_check(acceleration)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    (void)AccelerationPool_store_at(
        &accelerations_pool, index, acceleration);
    console_debug_write(
        LOG_ENGINE,
        "Set Entity: %d Acceleration Toward Position: {x: %f, y: %f} Magnitude: %f Acceleration: {x: %f, y: %f}\n",
        entity, position.x, position.y, acceleration_magnitude,
        acceleration.x, acceleration.y);
    return error_result_value(true);
}

EngineResult physics_acceleration_toward_entity_set(
        Entity entity, float acceleration_magnitude, Entity target) {
    EntityIndex target_index;
    EngineResult result = physics_live_index_get(target, &target_index);

    if(result.kind == ERROR_RESULT_ERROR) return result;
    return physics_acceleration_toward_position_set(
        entity, acceleration_magnitude, positions[target_index]);
}

EngineResult physics_acceleration_away_from_position_set(
        Entity entity, float acceleration_magnitude, Position position) {
    return physics_acceleration_toward_position_set(
        entity, -acceleration_magnitude, position);
}

EngineResult physics_acceleration_away_from_entity_set(
        Entity entity, float acceleration_magnitude, Entity target) {
    return physics_acceleration_toward_entity_set(
        entity, -acceleration_magnitude, target);
}

EngineResult physics_group_acceleration_toward_entity_set(
        GroupId group, float acceleration_magnitude, Entity target) {
    return physics_group_entity_target_apply(
        group, acceleration_magnitude, target,
        physics_acceleration_toward_entity_set);
}

EngineResult physics_group_acceleration_away_from_entity_set(
        GroupId group, float acceleration_magnitude, Entity target) {
    return physics_group_entity_target_apply(
        group, acceleration_magnitude, target,
        physics_acceleration_away_from_entity_set);
}

EngineResult physics_gravity_set(Acceleration gravity) {
    if(!isfinite(gravity.x) || !isfinite(gravity.y))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    physics_gravity = gravity;
    return error_result_value(true);
}

Acceleration physics_gravity_get(void) {
    return physics_gravity;
}

EngineResult physics_gravity_enable(Entity entity) {
    return entity_components_add(entity, ROHR_GRAVITY);
}

EngineResult physics_gravity_disable(Entity entity) {
    return entity_components_delete(entity, ROHR_GRAVITY);
}

bool physics_gravity_check(Entity entity) {
    return entity_components_check(entity, ROHR_GRAVITY);
}

/* Configuration validation never resolves geometry. Resolution is read-only and
 * shared by inspection and the scheduled load stage. */
static EngineResult source_reference_get(Entity entity, RohrComponentMask mask,
        bool position, bool orientation, EntityIndex *index) {
    EngineResult result = physics_live_index_get(entity, index);
    if(error_check(result)) return result;
    if((mask && !entity_index_components_check(*index, mask)) ||
            (mask == ROHR_SOFT_BODY_BEAM && !soft_body_beams_pool.used[*index]) ||
            (position && !positions_pool.used[*index]) ||
            (orientation && !orientations_pool.used[*index]))
        return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    return error_result_value(true);
}

static EngineResult location_validate(ForceLocation location) {
    EntityIndex index;
    if(!physics_vector_finite_check(location.offset))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    switch(location.reference.kind) {
    case ROHR_FORCE_WORLD_POSITION: return error_result_value(true);
    case ROHR_FORCE_COM_POSITION: {
        EngineResult result = source_reference_get(location.reference.entity, 0, true, true, &index);
        if(error_check(result)) return result;
        PositionResult com = physics_center_of_mass_local_position_get(location.reference.entity);
        if(error_check(com) && com.result.error == ERROR_ENGINE_COMPONENT_MISSING)
            return error_result_error(com.result.error);
        return error_result_value(true);
    }
    case ROHR_FORCE_ORIGIN_POSITION:
        return source_reference_get(location.reference.entity, 0, true, true, &index);
    case ROHR_FORCE_BEAM_POSITION:
        if(location.reference.beam.anchor < ROHR_FORCE_BEAM_START ||
                location.reference.beam.anchor > ROHR_FORCE_BEAM_END ||
                !isfinite(location.reference.beam.fraction)) break;
        return source_reference_get(location.reference.beam.entity,
            ROHR_SOFT_BODY_BEAM, false, false, &index);
    default: break;
    }
    return error_result_error(ERROR_ENGINE_STATE_INVALID);
}

static EngineResult orientation_validate(ForceOrientation orientation) {
    EntityIndex index;
    if(!isfinite(orientation.offset))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    switch(orientation.reference.kind) {
    case ROHR_FORCE_WORLD_ORIENTATION: return error_result_value(true);
    case ROHR_FORCE_BODY_ORIENTATION:
        return source_reference_get(orientation.reference.entity, 0, false, true, &index);
    case ROHR_FORCE_BEAM_ORIENTATION:
        return source_reference_get(orientation.reference.entity,
            ROHR_SOFT_BODY_BEAM, false, false, &index);
    case ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION: {
        EngineResult result = location_validate(orientation.reference.between.from);
        if(error_check(result)) return result;
        return location_validate(orientation.reference.between.to);
    }
    default: return error_result_error(ERROR_ENGINE_STATE_INVALID);
    }
}

static EngineResult source_target_validate(Entity target) {
    EntityIndex index;
    return target == ENTITY_INVALID ? error_result_value(true) :
        physics_live_index_get(target, &index);
}
static EngineResult force_validate(ForceDescription description) {
    if(!isfinite(description.magnitude)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    EngineResult result = source_target_validate(description.target);
    if(error_check(result)) return result;
    result = location_validate(description.location);
    if(error_check(result)) return result;
    return orientation_validate(description.orientation);
}
static EngineResult torque_validate(TorqueDescription description) {
    if(!isfinite(description.magnitude)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    return source_target_validate(description.target);
}

static EngineResult beam_geometry_get(Entity beam, Position *start, Vec2D *axis,
        double *length) {
    EntityIndex index, a, b;
    EngineResult result = source_reference_get(beam, ROHR_SOFT_BODY_BEAM,
        false, false, &index);
    if(error_check(result)) return result;
    if(!soft_body_beams_pool.used[index]) return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    SoftBodyBeam value = soft_body_beams[index];
    result = source_reference_get(value.node_a, ROHR_SOFT_BODY_NODE, true, false, &a);
    if(error_check(result)) return result;
    result = source_reference_get(value.node_b, ROHR_SOFT_BODY_NODE, true, false, &b);
    if(error_check(result)) return result;
    *start = physics_com_world_by_index_get(a);
    Position end = physics_com_world_by_index_get(b);
    double x = (double)end.x - start->x, y = (double)end.y - start->y;
    *length = hypot(x, y);
    if(!physics_vector_finite_check(*start) || !physics_vector_finite_check(end) ||
            !isfinite(*length) || *length <= ROHR_PHYSICS_BEAM_MIN_LENGTH)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    *axis = (Vec2D){x / *length, y / *length};
    return error_result_value(true);
}
/* Keep the COM relative to a node: large world translations must not erase
 * small radii or introduce turning into a force at the body's own COM. */
typedef struct SoftLoadGeometry {
    EntityIndex nodes[SOFT_BODY_MAX_NODES];
    uint32_t count;
    Position base;
    double total_mass, x, y, inertia;
} SoftLoadGeometry;

static EngineResult soft_load_geometry_get(Entity target, bool positive_masses,
        SoftLoadGeometry *geometry) {
    EntityIndex body;
    if(!entity_index_get(target, &body) || !entity_index_alive_check(body) ||
            body >= soft_bodies_pool.capacity || !soft_bodies_pool.used[body])
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    *geometry = (SoftLoadGeometry){.count = soft_bodies[body].node_count};
    if(geometry->count == 0 || geometry->count > SOFT_BODY_MAX_NODES)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    for(uint32_t n = 0; n < geometry->count; ++n) {
        EntityIndex node;
        if(!entity_index_get(soft_bodies[body].nodes[n], &node) ||
                !entity_index_alive_check(node) ||
                !entity_index_components_check(node, ROHR_SOFT_BODY_NODE | ROHR_MASS) ||
                node >= soft_body_nodes_pool.capacity || !soft_body_nodes_pool.used[node] ||
                soft_body_nodes[node].soft_body != target ||
                node >= mass_pool.capacity || !mass_pool.used[node] ||
                node >= positions_pool.capacity || !positions_pool.used[node] ||
                !isfinite(mass[node]) || mass[node] < 0 || (positive_masses && mass[node] == 0) ||
                !physics_vector_finite_check(positions[node]))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        for(uint32_t prior = 0; prior < n; ++prior)
            if(geometry->nodes[prior] == node)
                return error_result_error(ERROR_ENGINE_STATE_INVALID);
        geometry->nodes[n] = node;
        if(n == 0) geometry->base = positions[node];
        geometry->total_mass += mass[node];
        geometry->x += mass[node] * ((double)positions[node].x - geometry->base.x);
        geometry->y += mass[node] * ((double)positions[node].y - geometry->base.y);
    }
    if(geometry->total_mass <= 0) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    geometry->x /= geometry->total_mass;
    geometry->y /= geometry->total_mass;
    for(uint32_t n = 0; n < geometry->count; ++n) {
        EntityIndex node = geometry->nodes[n];
        double x = ((double)positions[node].x - geometry->base.x) - geometry->x;
        double y = ((double)positions[node].y - geometry->base.y) - geometry->y;
        geometry->inertia += mass[node] * (x * x + y * y);
    }
    return error_result_value(true);
}

static EngineResult location_resolve(ForceLocation location, Position *point) {
    EngineResult result = location_validate(location);
    EntityIndex index;
    if(error_check(result)) return result;
    double x = location.offset.x, y = location.offset.y;
    switch(location.reference.kind) {
    case ROHR_FORCE_WORLD_POSITION: break;
    case ROHR_FORCE_COM_POSITION: {
        (void)entity_index_get(location.reference.entity, &index);
        if(entity_index_components_check(index, ROHR_SOFT_BODY)) {
            SoftLoadGeometry geometry;
            result = soft_load_geometry_get(location.reference.entity, false, &geometry);
            if(error_check(result)) return result;
            x += (double)geometry.base.x + geometry.x;
            y += (double)geometry.base.y + geometry.y;
            break;
        }
        PositionResult com = physics_center_of_mass_world_position_get(location.reference.entity);
        if(error_check(com)) return error_result_error(com.result.error);
        x += com.result.value.x; y += com.result.value.y;
        break;
    }
    case ROHR_FORCE_ORIGIN_POSITION: {
        (void)entity_index_get(location.reference.entity, &index);
        if(!orientations_pool.used[index] || !isfinite(orientations[index]))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        Vec2D offset = math_vector_rotate(location.offset, orientations[index]);
        x = (double)positions[index].x + offset.x;
        y = (double)positions[index].y + offset.y;
        break;
    }
    case ROHR_FORCE_BEAM_POSITION: {
        Position start; Vec2D axis; double length;
        result = beam_geometry_get(location.reference.beam.entity, &start, &axis, &length);
        if(error_check(result)) return result;
        double alpha = 0.5 * location.reference.beam.anchor + location.reference.beam.fraction;
        double along = alpha * length + x;
        x = start.x + along * axis.x - y * axis.y;
        y = start.y + along * axis.y + location.offset.y * axis.x;
        break;
    }
    }
    if(!isfinite(x) || !isfinite(y) || fabs(x) > FLT_MAX || fabs(y) > FLT_MAX)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    *point = (Position){x, y};
    return error_result_value(true);
}
static EngineResult orientation_resolve(ForceOrientation orientation, Vec2D *direction) {
    EngineResult result = orientation_validate(orientation);
    if(error_check(result)) return result;
    double angle = orientation.offset;
    switch(orientation.reference.kind) {
    case ROHR_FORCE_WORLD_ORIENTATION: break;
    case ROHR_FORCE_BODY_ORIENTATION: {
        EntityIndex index;
        (void)entity_index_get(orientation.reference.entity, &index);
        if(!isfinite(orientations[index])) return error_result_error(ERROR_ENGINE_STATE_INVALID);
        angle += orientations[index]; break;
    }
    case ROHR_FORCE_BEAM_ORIENTATION: {
        Position start; Vec2D axis; double length;
        result = beam_geometry_get(orientation.reference.entity, &start, &axis, &length);
        if(error_check(result)) return result;
        angle += atan2((double)axis.x, axis.y) * (180.0 / PI_F); break;
    }
    case ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION: {
        Position from, to;
        result = location_resolve(orientation.reference.between.from, &from);
        if(error_check(result)) return result;
        result = location_resolve(orientation.reference.between.to, &to);
        if(error_check(result)) return result;
        double x = (double)to.x - from.x, y = (double)to.y - from.y;
        if(x == 0 && y == 0) return error_result_error(ERROR_ENGINE_STATE_INVALID);
        angle += atan2(x, y) * (180.0 / PI_F); break;
    }
    }
    /* Reduce before conversion to keep all finite degree offsets usable. */
    double radians = remainder(angle, 360.0) * (PI_F / 180.0);
    *direction = (Vec2D){sin(radians), cos(radians)};
    return error_result_value(true);
}

ForceDescription physics_force_vector_description(Entity target, Force vector) {
    return (ForceDescription){.target = target,
        .location.reference = {.kind = ROHR_FORCE_COM_POSITION, .entity = target},
        .orientation.offset = vector.x == 0 && vector.y == 0 ? 0 :
            math_radians_to_degrees(atan2f(vector.x, vector.y)),
        .magnitude = hypotf(vector.x, vector.y)};
}


EngineResult physics_force_description_set(Entity source, ForceDescription description) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(source, &index);
    if(error_check(result)) return result;
    result = force_validate(description);
    if(error_check(result)) return result;
    if(ForcePool_store_at(&forces_pool, index, description).kind == ERROR_RESULT_ERROR)
        return error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    entity_mask[index] |= ROHR_FORCE;
    return error_result_value(true);
}
ForceDescriptionResult physics_force_description_get(Entity source) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(source, &index);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForceDescriptionResult, result.result.error);
    if(!entity_index_components_check(index, ROHR_FORCE) || !forces_pool.used[index])
        return ERROR_RESULT_MAKE_ERROR(ForceDescriptionResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(ForceDescriptionResult, forces[index]);
}
EntityResult physics_force_add(ForceDescription description) {
    EngineResult result = force_validate(description);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    EntityResult source = entity_default_create("force", ENTITY_INVALID);
    if(error_check(source)) return source;
    result = physics_force_description_set(source.result.value, description);
    if(error_check(result)) {
        (void)entity_delete(source.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    }
    return source;
}
EngineResult physics_force_apply(ForceDescription description) {
    EntityResult source = physics_force_add(description);
    if(error_check(source)) return error_result_error(source.result.error);
    EngineResult result = entity_life_time_set(source.result.value, 0.0, engine_tick_get() + 1);
    if(error_check(result)) (void)entity_delete(source.result.value);
    return result;
}

EngineResult physics_force_target_set(Entity source, Entity value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    result.result.value.target = value;
    return physics_force_description_set(source, result.result.value);
}
EntityResult physics_force_target_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    return ERROR_RESULT_MAKE_VALUE(EntityResult, result.result.value.target);
}

EngineResult physics_force_magnitude_set(Entity source, float value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    result.result.value.magnitude = value;
    return physics_force_description_set(source, result.result.value);
}
ForceMagnitudeResult physics_force_magnitude_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForceMagnitudeResult, result.result.error);
    return ERROR_RESULT_MAKE_VALUE(ForceMagnitudeResult, result.result.value.magnitude);
}

EngineResult physics_force_location_set(Entity source, ForceLocation value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    result.result.value.location = value;
    return physics_force_description_set(source, result.result.value);
}
ForceLocationResult physics_force_location_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForceLocationResult, result.result.error);
    return ERROR_RESULT_MAKE_VALUE(ForceLocationResult, result.result.value.location);
}

EngineResult physics_force_orientation_set(Entity source, ForceOrientation value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    result.result.value.orientation = value;
    return physics_force_description_set(source, result.result.value);
}
ForceOrientationResult physics_force_orientation_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForceOrientationResult, result.result.error);
    return ERROR_RESULT_MAKE_VALUE(ForceOrientationResult, result.result.value.orientation);
}

EngineResult physics_torque_description_set(Entity source, TorqueDescription description) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(source, &index);
    if(error_check(result)) return result;
    result = torque_validate(description);
    if(error_check(result)) return result;
    if(TorquePool_store_at(&torques_pool, index, description).kind == ERROR_RESULT_ERROR)
        return error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    entity_mask[index] |= ROHR_TORQUE;
    return error_result_value(true);
}
TorqueDescriptionResult physics_torque_description_get(Entity source) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(source, &index);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(TorqueDescriptionResult, result.result.error);
    if(!entity_index_components_check(index, ROHR_TORQUE) || !torques_pool.used[index])
        return ERROR_RESULT_MAKE_ERROR(TorqueDescriptionResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(TorqueDescriptionResult, torques[index]);
}
EntityResult physics_torque_add(TorqueDescription description) {
    EngineResult result = torque_validate(description);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    EntityResult source = entity_default_create("torque", ENTITY_INVALID);
    if(error_check(source)) return source;
    result = physics_torque_description_set(source.result.value, description);
    if(error_check(result)) {
        (void)entity_delete(source.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    }
    return source;
}
EngineResult physics_torque_apply(TorqueDescription description) {
    EntityResult source = physics_torque_add(description);
    if(error_check(source)) return error_result_error(source.result.error);
    EngineResult result = entity_life_time_set(source.result.value, 0.0, engine_tick_get() + 1);
    if(error_check(result)) (void)entity_delete(source.result.value);
    return result;
}

EngineResult physics_torque_target_set(Entity source, Entity value) {
    TorqueDescriptionResult result = physics_torque_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    result.result.value.target = value;
    return physics_torque_description_set(source, result.result.value);
}
EntityResult physics_torque_target_get(Entity source) {
    TorqueDescriptionResult result = physics_torque_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    return ERROR_RESULT_MAKE_VALUE(EntityResult, result.result.value.target);
}

EngineResult physics_torque_magnitude_set(Entity source, Torque value) {
    TorqueDescriptionResult result = physics_torque_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    result.result.value.magnitude = value;
    return physics_torque_description_set(source, result.result.value);
}
TorqueResult physics_torque_magnitude_get(Entity source) {
    TorqueDescriptionResult result = physics_torque_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(TorqueResult, result.result.error);
    return ERROR_RESULT_MAKE_VALUE(TorqueResult, result.result.value.magnitude);
}

/* Geometry determines the load split; reference representation never does.
 * Fixed endpoints absorb their assigned loads without redistribution. */
static EngineResult beam_source_changes_get(Entity target, bool torque, double magnitude,
        Position point, Vec2D direction, PhysicsSourceChanges *changes, bool *active) {
    Position start; Vec2D axis; double length;
    EngineResult result = beam_geometry_get(target, &start, &axis, &length);
    if(error_check(result)) return result;
    EntityIndex beam_index, endpoints[2];
    (void)entity_index_get(target, &beam_index);
    SoftBodyBeam beam = soft_body_beams[beam_index];
    (void)entity_index_get(beam.node_a, &endpoints[0]);
    (void)entity_index_get(beam.node_b, &endpoints[1]);
    double fx = torque ? 0 : magnitude * direction.x;
    double fy = torque ? 0 : magnitude * direction.y;
    double along = ((double)point.x - start.x) * axis.x +
        ((double)point.y - start.y) * axis.y;
    double height = -((double)point.x - start.x) * axis.y +
        ((double)point.y - start.y) * axis.x;
    double parallel = fx * axis.x + fy * axis.y;
    double perpendicular = -fx * axis.y + fy * axis.x;
    double axial_b = torque ? 0 : (along / length) * parallel;
    double normal_b = torque ? -magnitude / length :
        (along * perpendicular - height * parallel) / length;
    double bx = axial_b * axis.x - normal_b * axis.y;
    double by = axial_b * axis.y + normal_b * axis.x;
    double loads[2][2] = {{fx - bx, fy - by}, {bx, by}};
    for(size_t n = 0; n < 2; ++n) {
        /* Validate even absorbed shares: never hide an invalid force couple. */
        if(!isfinite(loads[n][0]) || !isfinite(loads[n][1]) ||
                fabs(loads[n][0]) > FLT_MAX || fabs(loads[n][1]) > FLT_MAX)
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        EntityIndex index = endpoints[n];
        if(!physics_entity_simulated_get(index)) continue;
        if(!isfinite(mass[index])) return error_result_error(ERROR_ENGINE_STATE_INVALID);
        PhysicsChange change = physics_change_get(index, PHYSICS_CHANGE_FORCE_ACCELERATION);
        double ax = (double)change.force_acceleration.x + loads[n][0] / mass[index];
        double ay = (double)change.force_acceleration.y + loads[n][1] / mass[index];
        if(!isfinite(ax) || !isfinite(ay) || fabs(ax) > FLT_MAX || fabs(ay) > FLT_MAX)
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        change.force_acceleration = (Acceleration){ax, ay};
        changes->values[changes->count++] = change;
    }
    *active = changes->count != 0;
    return error_result_value(true);
}

/* Every assigned share is checked, including shares absorbed by constraints.
 * Acceleration snapshots are committed only after the complete source succeeds. */
static EngineResult soft_source_changes_get(Entity target, bool torque, double magnitude,
        Position point, Vec2D direction, const ForceLocation *location,
        PhysicsSourceChanges *changes, bool *active) {
    SoftLoadGeometry geometry;
    EngineResult result = soft_load_geometry_get(target, true, &geometry);
    if(error_check(result)) return result;
    double fx = torque ? 0 : magnitude * direction.x;
    double fy = torque ? 0 : magnitude * direction.y;
    double px = ((double)point.x - geometry.base.x) - geometry.x;
    double py = ((double)point.y - geometry.base.y) - geometry.y;
    if(location && location->reference.kind == ROHR_FORCE_COM_POSITION &&
            location->reference.entity == target) {
        px = location->offset.x; py = location->offset.y;
    }
    double moment = torque ? magnitude : py * fx - px * fy;
    if(moment != 0 && geometry.inertia <= 0)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    for(uint32_t n = 0; n < geometry.count; ++n) {
        EntityIndex node = geometry.nodes[n];
        double x = ((double)positions[node].x - geometry.base.x) - geometry.x;
        double y = ((double)positions[node].y - geometry.base.y) - geometry.y;
        double scale = moment == 0 ? 0 : mass[node] * moment / geometry.inertia;
        double load_x = (mass[node] / geometry.total_mass) * fx + scale * y;
        double load_y = (mass[node] / geometry.total_mass) * fy - scale * x;
        if(!isfinite(load_x) || !isfinite(load_y) ||
                fabs(load_x) > FLT_MAX || fabs(load_y) > FLT_MAX)
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        if(!physics_entity_simulated_get(node)) continue;
        PhysicsChange change = physics_change_get(node, PHYSICS_CHANGE_FORCE_ACCELERATION);
        double ax = (double)change.force_acceleration.x + load_x / mass[node];
        double ay = (double)change.force_acceleration.y + load_y / mass[node];
        if(!isfinite(ax) || !isfinite(ay) || fabs(ax) > FLT_MAX || fabs(ay) > FLT_MAX)
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        change.force_acceleration = (Acceleration){ax, ay};
        changes->values[changes->count++] = change;
    }
    *active = changes->count != 0;
    return error_result_value(true);
}

/* Prepare a complete contribution without modifying source, target, or reports. */
EngineResult physics_source_change_get(Entity source, bool torque,
        PhysicsSourceChanges *changes, bool *active) {
    changes->count = 0;
    PhysicsChange *change = &changes->values[0];
    EntityIndex index, target_index;
    EngineResult result = physics_live_index_get(source, &index);
    *active = false;
    if(error_check(result)) return result;
    Entity target;
    double magnitude;
    Position point = {0}; Vec2D direction = {0};
    ForceLocation location = {0};
    if(torque) {
        TorqueDescriptionResult stored = physics_torque_description_get(source);
        if(error_check(stored)) return error_result_error(stored.result.error);
        result = torque_validate(stored.result.value);
        target = stored.result.value.target; magnitude = stored.result.value.magnitude;
    } else {
        ForceDescriptionResult stored = physics_force_description_get(source);
        if(error_check(stored)) return error_result_error(stored.result.error);
        ForceDescription description = stored.result.value;
        location = description.location;
        target = description.target; magnitude = description.magnitude;
        result = force_validate(description);
        if(!error_check(result)) result = location_resolve(description.location, &point);
        if(!error_check(result)) result = orientation_resolve(description.orientation, &direction);
    }
    if(error_check(result)) return result;
    if(target == ENTITY_INVALID) return error_result_value(true);
    result = physics_live_index_get(target, &target_index);
    if(error_check(result)) return result;
    if(entity_index_components_check(target_index, ROHR_SOFT_BODY)) {
        if(magnitude == 0 || physics_entity_held_get(index)) return error_result_value(true);
        return soft_source_changes_get(target, torque, magnitude, point, direction,
            torque ? NULL : &location, changes, active);
    }
    if(entity_index_components_check(target_index, ROHR_SOFT_BODY_BEAM)) {
        if(magnitude == 0 || physics_entity_held_get(index)) return error_result_value(true);
        return beam_source_changes_get(target, torque, magnitude, point, direction, changes, active);
    }
    if(magnitude == 0 || physics_entity_held_get(index) ||
            !physics_entity_simulated_get(target_index)) return error_result_value(true);
    double inertia = physics_inverse_inertia_by_index_get(target_index);
    if(torque && inertia == 0) return error_result_value(true);
    unsigned fields = torque ? PHYSICS_CHANGE_TORQUE_ACCELERATION :
        PHYSICS_CHANGE_FORCE_ACCELERATION | PHYSICS_CHANGE_TORQUE_ACCELERATION;
    *change = physics_change_get(target_index, fields);
    double ax = change->force_acceleration.x, ay = change->force_acceleration.y;
    double angular = change->torque_acceleration;
    if(torque) angular += magnitude * inertia * (180.0 / PI_F);
    else {
        double fx = magnitude * direction.x, fy = magnitude * direction.y;
        ax += fx / mass[target_index]; ay += fy / mass[target_index];
        if(inertia != 0) {
            Position com = physics_com_world_by_index_get(target_index);
            double moment = ((double)point.y - com.y) * fx - ((double)point.x - com.x) * fy;
            angular += moment * inertia * (180.0 / PI_F);
        }
    }
    if(!isfinite(ax) || !isfinite(ay) || !isfinite(angular) ||
            fabs(ax) > FLT_MAX || fabs(ay) > FLT_MAX || fabs(angular) > FLT_MAX)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    change->force_acceleration = (Acceleration){ax, ay};
    change->torque_acceleration = angular;
    changes->count = 1;
    *active = true;
    return error_result_value(true);
}

void physics_sources_apply(Report *report, bool torque) {
    for(uint32_t n = 0; n < entity_alive_count_get(); ++n) {
        EntityIndex index;
        if(!physics_step_alive_index_at(n, &index) ||
                !entity_index_components_check(index, torque ? ROHR_TORQUE : ROHR_FORCE)) continue;
        Entity source = physics_operation_entity_get(index);
        Entity target = torque ? torques[index].target : forces[index].target;
        PhysicsSourceChanges change; bool active;
        EngineResult result = physics_source_change_get(source, torque, &change, &active);
        PhysicsReportStage stage = torque ? ROHR_PHYSICS_TORQUE_CONTRIBUTION : ROHR_PHYSICS_FORCE_CONTRIBUTION;
        if(error_check(result)) {
            physics_operation_reject(report, stage, result.result.error, source, target);
            physics_report_counter_add(report, ROHR_REPORT_SOURCE_SUSPENDED_COUNT, 1);
        } else if(!active) physics_report_counter_add(report, ROHR_REPORT_SOURCE_INACTIVE_COUNT, 1);
        else if(physics_changes_commit(change.values, change.count, report, stage, source, target))
            physics_report_counter_add(report, ROHR_REPORT_SOURCE_CONTRIBUTING_COUNT, 1);
        else physics_report_counter_add(report, ROHR_REPORT_SOURCE_SUSPENDED_COUNT, 1);
    }
}
