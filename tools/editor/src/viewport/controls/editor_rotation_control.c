/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_rotation_control.h"

#include <math.h>

Position editor_rotation_control_position_get(Position center,
        Orientation orientation,
        float arm_length) {
    return (Position){
        center.x + sinf(orientation) * arm_length,
        center.y - cosf(orientation) * arm_length
    };
}

bool editor_rotation_control_hit_check(Position pointer,
        Position control,
        float radius) {
    float x = pointer.x - control.x;
    float y = pointer.y - control.y;
    return x * x + y * y <= radius * radius;
}

bool editor_rotation_control_begin(Position center, Orientation orientation,
        float arm_length, Position pointer, float radius,
        float *pointer_offset) {
    Position control;
    if(pointer_offset == NULL) return false;
    control = editor_rotation_control_position_get(center, orientation,
        arm_length);
    if(!editor_rotation_control_hit_check(pointer, control, radius)) return false;
    *pointer_offset = orientation -
        atan2f(pointer.y - center.y, pointer.x - center.x);
    return true;
}

Orientation editor_rotation_control_orientation_get(Position center,
        Position pointer, float pointer_offset) {
    return atan2f(pointer.y - center.y, pointer.x - center.x) + pointer_offset;
}
