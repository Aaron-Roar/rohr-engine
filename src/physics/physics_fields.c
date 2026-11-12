/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics.h"

/* Copy the current description before replacing one field. Joint and beam
 * getters return resolved rest values with explicit flags, so these edits do
 * not measure a new rest state from a deformed or disconnected configuration. */

EngineResult physics_angle_lock_min_set(Entity entity, Orientation value) {
    AngleLockResult current = physics_angle_lock_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.min = value;
    return physics_angle_lock_set(entity, current.result.value.min, current.result.value.max);
}

EngineResult physics_angle_lock_min_radians_set(Entity entity, Orientation value) {
    return physics_angle_lock_min_set(entity, math_radians_to_degrees(value));
}

EngineResult physics_angle_lock_max_set(Entity entity, Orientation value) {
    AngleLockResult current = physics_angle_lock_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.max = value;
    return physics_angle_lock_set(entity, current.result.value.min, current.result.value.max);
}

EngineResult physics_angle_lock_max_radians_set(Entity entity, Orientation value) {
    return physics_angle_lock_max_set(entity, math_radians_to_degrees(value));
}

EngineResult physics_axis_lock_axis_set(Entity entity, Axis value) {
    AxisLockDescriptionResult current = physics_axis_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.axis = value;
    return physics_axis_lock_description_set(entity, current.result.value);
}

EngineResult physics_axis_lock_axis_x_set(Entity entity, float value) {
    AxisLockDescriptionResult current = physics_axis_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.axis.x = value;
    return physics_axis_lock_description_set(entity, current.result.value);
}

EngineResult physics_axis_lock_axis_y_set(Entity entity, float value) {
    AxisLockDescriptionResult current = physics_axis_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.axis.y = value;
    return physics_axis_lock_description_set(entity, current.result.value);
}

EngineResult physics_axis_lock_point_on_axis_set(Entity entity, Position value) {
    AxisLockDescriptionResult current = physics_axis_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.point_on_axis = value;
    return physics_axis_lock_description_set(entity, current.result.value);
}

EngineResult physics_axis_lock_point_on_axis_x_set(Entity entity, float value) {
    AxisLockDescriptionResult current = physics_axis_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.point_on_axis.x = value;
    return physics_axis_lock_description_set(entity, current.result.value);
}

EngineResult physics_axis_lock_point_on_axis_y_set(Entity entity, float value) {
    AxisLockDescriptionResult current = physics_axis_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.point_on_axis.y = value;
    return physics_axis_lock_description_set(entity, current.result.value);
}

EngineResult physics_transform_lock_driver_set(Entity entity, Entity value) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.driver = value;
    return physics_transform_lock_description_set(entity, current.result.value);
}

EngineResult physics_transform_lock_local_offset_set(Entity entity, Position value) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.local_offset = value;
    return physics_transform_lock_description_set(entity, current.result.value);
}

EngineResult physics_transform_lock_local_offset_x_set(Entity entity, float value) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.local_offset.x = value;
    return physics_transform_lock_description_set(entity, current.result.value);
}

EngineResult physics_transform_lock_local_offset_y_set(Entity entity, float value) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.local_offset.y = value;
    return physics_transform_lock_description_set(entity, current.result.value);
}

EngineResult physics_transform_lock_local_angle_set(Entity entity, Orientation value) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.local_angle = value;
    return physics_transform_lock_description_set(entity, current.result.value);
}

EngineResult physics_transform_lock_lock_position_set(Entity entity, bool value) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.lock_position = value;
    return physics_transform_lock_description_set(entity, current.result.value);
}

EngineResult physics_transform_lock_lock_orientation_set(Entity entity, bool value) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.lock_orientation = value;
    return physics_transform_lock_description_set(entity, current.result.value);
}

EngineResult physics_transform_lock_inherit_velocity_set(Entity entity, bool value) {
    TransformLockDescriptionResult current = physics_transform_lock_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.inherit_velocity = value;
    return physics_transform_lock_description_set(entity, current.result.value);
}

EngineResult physics_transform_lock_local_angle_radians_set(Entity entity, Orientation value) {
    return physics_transform_lock_local_angle_set(entity, math_radians_to_degrees(value));
}

EngineResult physics_joint_type_set(Entity entity, JointType value) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.type = value;
    return physics_joint_description_set(entity, current.result.value);
}

EngineResult physics_joint_anchor_a_set(Entity entity, JointAnchorId value) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.anchor_a = value;
    return physics_joint_description_set(entity, current.result.value);
}

EngineResult physics_joint_anchor_b_set(Entity entity, JointAnchorId value) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.anchor_b = value;
    return physics_joint_description_set(entity, current.result.value);
}

EngineResult physics_joint_rest_length_set(Entity entity, float value) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.rest_length = value;
    return physics_joint_description_set(entity, current.result.value);
}

EngineResult physics_joint_stiffness_set(Entity entity, float value) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.stiffness = value;
    return physics_joint_description_set(entity, current.result.value);
}

EngineResult physics_joint_damping_set(Entity entity, float value) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.damping = value;
    return physics_joint_description_set(entity, current.result.value);
}

EngineResult physics_joint_rest_angle_set(Entity entity, Orientation value) {
    JointDescriptionResult current = physics_joint_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.rest_angle = value;
    return physics_joint_description_set(entity, current.result.value);
}

EngineResult physics_joint_rest_angle_radians_set(Entity entity, Orientation value) {
    return physics_joint_rest_angle_set(entity, math_radians_to_degrees(value));
}

EngineResult physics_joint_anchor_entity_set(JointAnchorId anchor, Entity value) {
    JointAnchorDescriptionResult current = physics_joint_anchor_description_get(anchor);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.entity = value;
    return physics_joint_anchor_description_set(anchor, current.result.value);
}

EntityResult physics_joint_anchor_entity_get(JointAnchorId anchor) {
    JointAnchorDescriptionResult current = physics_joint_anchor_description_get(anchor);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(EntityResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(EntityResult, current.result.value.entity);
}

EngineResult physics_soft_body_beam_node_a_set(Entity entity, Entity value) {
    SoftBodyBeamResult current = physics_soft_body_beam_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    return physics_soft_body_beam_endpoints_set(entity, value, current.result.value.node_b);
}

EngineResult physics_soft_body_beam_node_b_set(Entity entity, Entity value) {
    SoftBodyBeamResult current = physics_soft_body_beam_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    return physics_soft_body_beam_endpoints_set(entity, current.result.value.node_a, value);
}

EngineResult physics_soft_body_beam_rest_length_set(Entity entity, float value) {
    SoftBodyBeamDescriptionResult current = physics_soft_body_beam_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.rest_length = value;
    return physics_soft_body_beam_description_set(entity, current.result.value);
}

EngineResult physics_soft_body_beam_stiffness_set(Entity entity, float value) {
    SoftBodyBeamDescriptionResult current = physics_soft_body_beam_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.stiffness = value;
    return physics_soft_body_beam_description_set(entity, current.result.value);
}

EngineResult physics_soft_body_beam_damping_set(Entity entity, float value) {
    SoftBodyBeamDescriptionResult current = physics_soft_body_beam_description_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    current.result.value.damping = value;
    return physics_soft_body_beam_description_set(entity, current.result.value);
}

EngineResult physics_soft_body_node_radius_set(Entity entity, float value) {
    SoftBodyNodeResult current = physics_soft_body_node_get(entity);
    if(error_check(current)) return error_result_error(current.result.error);
    return physics_particle_radius_set(entity, value);
}
