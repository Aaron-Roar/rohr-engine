/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_rotation_control.h"

#include <math.h>

Position editor_rotation_control_position_get(Position center,
        Orientation orientation,
        float arm_length) {
    return (Position){
        center.x + sinf(math_degrees_to_radians(orientation)) * arm_length,
        center.y + cosf(math_degrees_to_radians(orientation)) * arm_length
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
    *pointer_offset = orientation - editor_rotation_control_pointer_angle_get(center, pointer);
    return true;
}

Orientation editor_rotation_control_orientation_get(Position center,
        Position pointer, float pointer_offset, Orientation previous) {
    if(pointer.x == center.x && pointer.y == center.y) return previous;
    float candidate = editor_rotation_control_pointer_angle_get(center, pointer) + pointer_offset;
    return previous + remainderf(candidate - previous, 360.0f);
}

Orientation editor_rotation_control_pointer_angle_get(Position center, Position pointer) {
    return math_radians_to_degrees(atan2f(pointer.x - center.x, pointer.y - center.y));
}

Position editor_rotation_control_screen_position_get(Position center,
        Orientation orientation, float arm_length) {
    Position point = editor_rotation_control_position_get(
        (Position){center.x, -center.y}, orientation, arm_length);
    return (Position){point.x, -point.y};
}

bool editor_rotation_control_screen_begin(Position center, Orientation orientation,
        float arm_length, Position pointer, float radius, float *pointer_offset) {
    return editor_rotation_control_begin((Position){center.x, -center.y}, orientation,
        arm_length, (Position){pointer.x, -pointer.y}, radius, pointer_offset);
}
