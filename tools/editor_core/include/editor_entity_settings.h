/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_ENTITY_SETTINGS_H
#define ROHR_EDITOR_ENTITY_SETTINGS_H

/* One property list keeps the shared entity panel, commands and CLI aligned.
 * X(symbol, CLI name, display label, value kind, member) */
#define EDITOR_ENTITY_SETTING_PROPERTIES(X) \
    X(LIFETIME_TIME_ENABLED, "lifetime-time-enabled", "Seconds Enabled", BOOL, lifetime.deadline.time_enabled) \
    X(LIFETIME_TIME_ABSOLUTE, "lifetime-time-absolute", "Absolute Seconds", BOOL, lifetime.time_absolute) \
    X(LIFETIME_TIME, "lifetime-time", "Seconds", DOUBLE, lifetime.deadline.expiry_time) \
    X(LIFETIME_TICK_ENABLED, "lifetime-tick-enabled", "Ticks Enabled", BOOL, lifetime.deadline.tick_enabled) \
    X(LIFETIME_TICK_ABSOLUTE, "lifetime-tick-absolute", "Absolute Ticks", BOOL, lifetime.tick_absolute) \
    X(LIFETIME_TICK, "lifetime-tick", "Ticks", UINT64, lifetime.deadline.expiry_tick) \
    X(ANGULAR_ACCELERATION, "angular-acceleration", "Acceleration (deg/s2)", FLOAT, angular_acceleration) \
    X(ANGULAR_MAXIMUM_ENABLED, "angular-maximum-enabled", "Limit Angular Velocity", BOOL, angular_velocity_maximum_enabled) \
    X(ANGULAR_MAXIMUM, "angular-maximum", "Maximum (deg/s)", FLOAT, angular_velocity_maximum) \
    X(GAMEPLAY_TARGET, "gameplay-target", "Gameplay Target", REFERENCE, gameplay_target) \
    X(AXIS_LOCK_ENABLED, "axis-lock-enabled", "Axis Lock", BOOL, axis_lock_enabled) \
    X(AXIS_WORLD_FIXED, "axis-world-fixed", "World Fixed Axis", BOOL, axis_lock_world_fixed) \
    X(AXIS_X, "axis-x", "Axis X", FLOAT, axis_lock.axis.x) \
    X(AXIS_Y, "axis-y", "Axis Y", FLOAT, axis_lock.axis.y) \
    X(AXIS_POINT_X, "axis-point-x", "Shift X", FLOAT, axis_lock.point_on_axis.x) \
    X(AXIS_POINT_Y, "axis-point-y", "Shift Y", FLOAT, axis_lock.point_on_axis.y) \
    X(TRANSFORM_DRIVER, "transform-driver", "Transform Driver", REFERENCE, transform_driver) \
    X(TRANSFORM_OFFSET_X, "transform-offset-x", "Driver Offset X", FLOAT, transform_offset.x) \
    X(TRANSFORM_OFFSET_Y, "transform-offset-y", "Driver Offset Y", FLOAT, transform_offset.y) \
    X(TRANSFORM_ANGLE, "transform-angle", "Driver Angle (deg)", FLOAT, transform_angle) \
    X(TRANSFORM_POSITION, "transform-position", "Lock Position", BOOL, transform_position) \
    X(TRANSFORM_ORIENTATION, "transform-orientation", "Lock Orientation", BOOL, transform_orientation) \
    X(TRANSFORM_VELOCITY, "transform-velocity", "Inherit Velocity", BOOL, transform_inherit_velocity)

#endif
