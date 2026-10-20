/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "object_internal.h"
#include <math.h>

bool object_entity_settings_check(ObjectEntitySettings v) {
    if(!isfinite(v.lifetime.deadline.expiry_time) || v.lifetime.deadline.expiry_time < 0 ||
            !isfinite(v.angular_acceleration) || !isfinite(v.angular_velocity_maximum) ||
            v.angular_velocity_maximum < 0 || !isfinite(v.transform_offset.x) ||
            !isfinite(v.transform_offset.y) || !isfinite(v.transform_angle)) return false;
    if(v.gameplay_target.id && (v.gameplay_target.kind < OBJECT_CONTAINER ||
            v.gameplay_target.kind > OBJECT_TORQUE)) return false;
    if(v.transform_driver.id && (v.transform_driver.kind < OBJECT_CONTAINER ||
            v.transform_driver.kind > OBJECT_TORQUE)) return false;
    if(v.axis_lock_enabled && (!isfinite(v.axis_lock.axis.x) || !isfinite(v.axis_lock.axis.y) ||
            (v.axis_lock.axis.x == 0 && v.axis_lock.axis.y == 0) ||
            !physics_world_position_check(v.axis_lock.point_on_axis))) return false;
    return true;
}
