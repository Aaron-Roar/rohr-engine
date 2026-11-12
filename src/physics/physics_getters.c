/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics.h"
#include "physics/physics_internal.h"

VelocityResult physics_velocity_get(Entity entity) {
    EntityIndex index;
    EngineResult valid = physics_live_index_get(entity, &index);
    if(error_check(valid)) return ERROR_RESULT_MAKE_ERROR(VelocityResult, valid.result.error);
    if(index >= velocities_pool.capacity || !velocities_pool.used[index])
        return ERROR_RESULT_MAKE_ERROR(VelocityResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(VelocityResult, velocities_pool.objects[index]);
}

AccelerationResult physics_acceleration_get(Entity entity) {
    EntityIndex index;
    EngineResult valid = physics_live_index_get(entity, &index);
    if(error_check(valid)) return ERROR_RESULT_MAKE_ERROR(AccelerationResult, valid.result.error);
    if(index >= accelerations_pool.capacity || !accelerations_pool.used[index])
        return ERROR_RESULT_MAKE_ERROR(AccelerationResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(AccelerationResult, accelerations_pool.objects[index]);
}

ScalarResult physics_orientation_get(Entity entity) {
    EntityIndex index;
    EngineResult valid = physics_live_index_get(entity, &index);
    if(error_check(valid)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, valid.result.error);
    if(index >= orientations_pool.capacity || !orientations_pool.used[index])
        return ERROR_RESULT_MAKE_ERROR(ScalarResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, orientations_pool.objects[index]);
}

ScalarResult physics_angular_acceleration_get(Entity entity) {
    EntityIndex index;
    EngineResult valid = physics_live_index_get(entity, &index);
    if(error_check(valid)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, valid.result.error);
    if(index >= angular_accelerations_pool.capacity || !angular_accelerations_pool.used[index])
        return ERROR_RESULT_MAKE_ERROR(ScalarResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, angular_accelerations_pool.objects[index]);
}

ScalarResult physics_friction_get(Entity entity) {
    EntityIndex index;
    EngineResult valid = physics_live_index_get(entity, &index);
    if(error_check(valid)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, valid.result.error);
    if(entity_index_components_check(index, ROHR_SOFT_BODY_BEAM)) {
        SoftBodyBeamResult beam = physics_soft_body_beam_get(entity);
        if(error_check(beam)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, beam.result.error);
        return ERROR_RESULT_MAKE_VALUE(ScalarResult, beam.result.value.friction);
    }
    if(index >= frictions_pool.capacity || !frictions_pool.used[index])
        return ERROR_RESULT_MAKE_ERROR(ScalarResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, frictions_pool.objects[index]);
}

ScalarResult physics_restitution_get(Entity entity) {
    EntityIndex index;
    EngineResult valid = physics_live_index_get(entity, &index);
    if(error_check(valid)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, valid.result.error);
    if(entity_index_components_check(index, ROHR_SOFT_BODY_BEAM)) {
        SoftBodyBeamResult beam = physics_soft_body_beam_get(entity);
        if(error_check(beam)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, beam.result.error);
        return ERROR_RESULT_MAKE_VALUE(ScalarResult, beam.result.value.restitution);
    }
    if(index >= restitutions_pool.capacity || !restitutions_pool.used[index])
        return ERROR_RESULT_MAKE_ERROR(ScalarResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, restitutions_pool.objects[index]);
}

AngleLockResult physics_angle_lock_get(Entity entity) {
    EntityIndex index;
    EngineResult valid = physics_live_index_get(entity, &index);
    if(error_check(valid)) return ERROR_RESULT_MAKE_ERROR(AngleLockResult, valid.result.error);
    if(index >= angle_locks_pool.capacity || !angle_locks_pool.used[index] ||
            !entity_index_components_check(index, ROHR_ANGLE_LOCK))
        return ERROR_RESULT_MAKE_ERROR(AngleLockResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(AngleLockResult, angle_locks_pool.objects[index]);
}

EntityResult physics_target_get(Entity entity) {
    EntityIndex index;
    EngineResult valid = physics_live_index_get(entity, &index);
    if(error_check(valid)) return ERROR_RESULT_MAKE_ERROR(EntityResult, valid.result.error);
    if(index >= targets_pool.capacity || !targets_pool.used[index] ||
            !entity_index_components_check(index, ROHR_TARGETABLE))
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(EntityResult, targets_pool.objects[index]);
}

ScalarResult physics_orientation_radians_get(Entity entity) {
    ScalarResult current = physics_orientation_get(entity);
    if(!error_check(current)) current.result.value *= (PI_F / 180.0);
    return current;
}

ScalarResult physics_angular_acceleration_radians_get(Entity entity) {
    ScalarResult current = physics_angular_acceleration_get(entity);
    if(!error_check(current)) current.result.value *= (PI_F / 180.0);
    return current;
}

ScalarResult physics_velocity_x_get(Entity entity) {
    VelocityResult current = physics_velocity_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.x);
}

ScalarResult physics_velocity_y_get(Entity entity) {
    VelocityResult current = physics_velocity_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.y);
}

ScalarResult physics_acceleration_x_get(Entity entity) {
    AccelerationResult current = physics_acceleration_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.x);
}

ScalarResult physics_acceleration_y_get(Entity entity) {
    AccelerationResult current = physics_acceleration_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.y);
}

ScalarResult physics_position_x_get(Entity entity) {
    PositionResult current = physics_position_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.x);
}

ScalarResult physics_position_y_get(Entity entity) {
    PositionResult current = physics_position_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.y);
}

ScalarResult physics_center_of_mass_local_position_x_get(Entity entity) {
    PositionResult current = physics_center_of_mass_local_position_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.x);
}

ScalarResult physics_center_of_mass_local_position_y_get(Entity entity) {
    PositionResult current = physics_center_of_mass_local_position_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.y);
}

ScalarResult physics_center_of_mass_world_position_x_get(Entity entity) {
    PositionResult current = physics_center_of_mass_world_position_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.x);
}

ScalarResult physics_center_of_mass_world_position_y_get(Entity entity) {
    PositionResult current = physics_center_of_mass_world_position_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.y);
}

ScalarResult physics_particle_origin_x_get(Entity entity) {
    PositionResult current = physics_particle_origin_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.x);
}

ScalarResult physics_particle_origin_y_get(Entity entity) {
    PositionResult current = physics_particle_origin_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.y);
}

ScalarResult physics_joint_anchor_local_position_x_get(JointAnchorId anchor) {
    JointAnchorPositionResult current = physics_joint_anchor_local_position_get(anchor);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.x);
}

ScalarResult physics_joint_anchor_local_position_y_get(JointAnchorId anchor) {
    JointAnchorPositionResult current = physics_joint_anchor_local_position_get(anchor);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.y);
}

ScalarResult physics_joint_anchor_world_position_x_get(JointAnchorId anchor) {
    JointAnchorPositionResult current = physics_joint_anchor_world_position_get(anchor);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.x);
}

ScalarResult physics_joint_anchor_world_position_y_get(JointAnchorId anchor) {
    JointAnchorPositionResult current = physics_joint_anchor_world_position_get(anchor);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.y);
}

ScalarResult physics_soft_body_node_local_position_x_get(Entity entity) {
    PositionResult current = physics_soft_body_node_local_position_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.x);
}

ScalarResult physics_soft_body_node_local_position_y_get(Entity entity) {
    PositionResult current = physics_soft_body_node_local_position_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.y);
}

float physics_gravity_x_get(void) {
    return physics_gravity_get().x;
}

float physics_gravity_y_get(void) {
    return physics_gravity_get().y;
}

CollisionCategoryResult physics_collision_category_get(Entity entity) {
    CollisionFilterDescriptionResult current = physics_collision_filter_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(CollisionCategoryResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(CollisionCategoryResult, current.result.value.category);
}

CollisionCategoryResult physics_collision_response_get(Entity entity) {
    CollisionFilterDescriptionResult current = physics_collision_filter_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(CollisionCategoryResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(CollisionCategoryResult, current.result.value.responds_to);
}

bool physics_dynamic_check(Entity entity) {
    return entity_components_check(entity, ROHR_DYNAMIC);
}

bool physics_static_check(Entity entity) {
    return entity_components_check(entity, ROHR_STATIC);
}

bool physics_entity_held_check(Entity entity) {
    return entity_components_check(entity, ROHR_HOLD);
}

bool physics_collision_check(Entity entity) {
    return entity_components_check(entity, ROHR_COLLISION);
}

AxisResult physics_axis_lock_axis_get(Entity entity) {
    AxisLockDescriptionResult current = physics_axis_lock_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(AxisResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(AxisResult, current.result.value.axis);
}

ScalarResult physics_axis_lock_axis_x_get(Entity entity) {
    AxisResult current = physics_axis_lock_axis_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.x);
}

ScalarResult physics_axis_lock_axis_y_get(Entity entity) {
    AxisResult current = physics_axis_lock_axis_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.y);
}

PositionResult physics_axis_lock_point_on_axis_get(Entity entity) {
    AxisLockDescriptionResult current = physics_axis_lock_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(PositionResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(PositionResult, current.result.value.point_on_axis);
}

ScalarResult physics_axis_lock_point_on_axis_x_get(Entity entity) {
    PositionResult current = physics_axis_lock_point_on_axis_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.x);
}

ScalarResult physics_axis_lock_point_on_axis_y_get(Entity entity) {
    PositionResult current = physics_axis_lock_point_on_axis_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.y);
}

EntityResult physics_transform_lock_driver_get(Entity entity) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(EntityResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(EntityResult, current.result.value.driver);
}

PositionResult physics_transform_lock_local_offset_get(Entity entity) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(PositionResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(PositionResult, current.result.value.local_offset);
}

ScalarResult physics_transform_lock_local_offset_x_get(Entity entity) {
    PositionResult current = physics_transform_lock_local_offset_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.x);
}

ScalarResult physics_transform_lock_local_offset_y_get(Entity entity) {
    PositionResult current = physics_transform_lock_local_offset_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.y);
}

ScalarResult physics_transform_lock_local_angle_get(Entity entity) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.local_angle);
}

ScalarResult physics_transform_lock_local_angle_radians_get(Entity entity) {
    ScalarResult current = physics_transform_lock_local_angle_get(entity);
    if(!error_check(current)) current.result.value *= (PI_F / 180.0);
    return current;
}

JointTypeResult physics_joint_type_get(Entity entity) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(JointTypeResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(JointTypeResult, current.result.value.type);
}

JointAnchorIdResult physics_joint_anchor_a_get(Entity entity) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(JointAnchorIdResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(JointAnchorIdResult, current.result.value.anchor_a);
}

JointAnchorIdResult physics_joint_anchor_b_get(Entity entity) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(JointAnchorIdResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(JointAnchorIdResult, current.result.value.anchor_b);
}

ScalarResult physics_joint_rest_length_get(Entity entity) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.rest_length);
}

ScalarResult physics_joint_stiffness_get(Entity entity) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.stiffness);
}

ScalarResult physics_joint_damping_get(Entity entity) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.damping);
}

ScalarResult physics_joint_rest_angle_get(Entity entity) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.rest_angle);
}

ScalarResult physics_joint_rest_angle_radians_get(Entity entity) {
    ScalarResult current = physics_joint_rest_angle_get(entity);
    if(!error_check(current)) current.result.value *= (PI_F / 180.0);
    return current;
}

ScalarResult physics_soft_body_node_radius_get(Entity entity) {
    SoftBodyNodeDescriptionResult current = physics_soft_body_node_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.radius);
}

EntityResult physics_soft_body_beam_node_a_get(Entity entity) {
    SoftBodyBeamDescriptionResult current = physics_soft_body_beam_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(EntityResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(EntityResult, current.result.value.node_a);
}

EntityResult physics_soft_body_beam_node_b_get(Entity entity) {
    SoftBodyBeamDescriptionResult current = physics_soft_body_beam_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(EntityResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(EntityResult, current.result.value.node_b);
}

ScalarResult physics_soft_body_beam_rest_length_get(Entity entity) {
    SoftBodyBeamDescriptionResult current = physics_soft_body_beam_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.rest_length);
}

ScalarResult physics_soft_body_beam_stiffness_get(Entity entity) {
    SoftBodyBeamDescriptionResult current = physics_soft_body_beam_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.stiffness);
}

ScalarResult physics_soft_body_beam_damping_get(Entity entity) {
    SoftBodyBeamDescriptionResult current = physics_soft_body_beam_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.damping);
}

PhysicsCountResult physics_particle_rigid_vertices_get(Entity entity) {
    ParticleDescriptionResult current = physics_particle_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(PhysicsCountResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(PhysicsCountResult, current.result.value.rigid_vertices);
}

bool physics_transform_lock_lock_position_check(Entity entity) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    return !error_check(current) && current.result.value.lock_position;
}

bool physics_transform_lock_lock_orientation_check(Entity entity) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    return !error_check(current) && current.result.value.lock_orientation;
}

bool physics_transform_lock_inherit_velocity_check(Entity entity) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    return !error_check(current) && current.result.value.inherit_velocity;
}

ScalarResult physics_angle_lock_min_get(Entity entity) {
    AngleLockResult current = physics_angle_lock_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.min);
}

ScalarResult physics_angle_lock_min_radians_get(Entity entity) {
    ScalarResult current = physics_angle_lock_min_get(entity);
    if(!error_check(current)) current.result.value *= (PI_F / 180.0);
    return current;
}

ScalarResult physics_angle_lock_max_get(Entity entity) {
    AngleLockResult current = physics_angle_lock_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.max);
}

ScalarResult physics_angle_lock_max_radians_get(Entity entity) {
    ScalarResult current = physics_angle_lock_max_get(entity);
    if(!error_check(current)) current.result.value *= (PI_F / 180.0);
    return current;
}

AngleLockResult physics_angle_lock_radians_get(Entity entity) {
    AngleLockResult current = physics_angle_lock_get(entity);
    if(!error_check(current)) {
        current.result.value.min *= (PI_F / 180.0);
        current.result.value.max *= (PI_F / 180.0);
    }
    return current;
}

ScalarResult physics_soft_body_beam_collision_thickness_get(Entity entity) {
    SoftBodyBeamCollisionDescriptionResult current = physics_soft_body_beam_collision_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value.thickness);
}

bool physics_soft_body_beam_collision_check(Entity entity) {
    SoftBodyBeamCollisionDescriptionResult current = physics_soft_body_beam_collision_description_get(entity);
    return !error_check(current) && current.result.value.enabled;
}

SoftBodyAreaGeometryResult physics_soft_body_area_geometry_get(Entity entity) {
    SoftBodyAreaDescriptionResult current = physics_soft_body_area_description_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(SoftBodyAreaGeometryResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(SoftBodyAreaGeometryResult, current.result.value.geometry);
}

PhysicsCountResult physics_soft_body_area_order_get(Entity entity) {
    SoftBodyAreaResult area = physics_soft_body_area_get(entity);
    if(error_check(area)) return ERROR_RESULT_MAKE_ERROR(PhysicsCountResult, area.result.error);
    SoftBodyResult body = physics_soft_body_get(area.result.value.soft_body);
    if(error_check(body)) return ERROR_RESULT_MAKE_ERROR(PhysicsCountResult, body.result.error);
    for(uint32_t i = 0; i < body.result.value.area_count; ++i)
        if(body.result.value.areas[i] == entity) return ERROR_RESULT_MAKE_VALUE(PhysicsCountResult, i);
    return ERROR_RESULT_MAKE_ERROR(PhysicsCountResult, ERROR_ENGINE_STATE_INVALID);
}

PhysicsCountResult physics_soft_body_node_count_get(Entity entity) {
    SoftBodyResult current = physics_soft_body_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(PhysicsCountResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(PhysicsCountResult, current.result.value.node_count);
}

PhysicsCountResult physics_soft_body_beam_count_get(Entity entity) {
    SoftBodyResult current = physics_soft_body_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(PhysicsCountResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(PhysicsCountResult, current.result.value.beam_count);
}

PhysicsCountResult physics_soft_body_area_count_get(Entity entity) {
    SoftBodyResult current = physics_soft_body_get(entity);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(PhysicsCountResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(PhysicsCountResult, current.result.value.area_count);
}

