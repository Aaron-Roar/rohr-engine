/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_rotation_control.h"

#include <math.h>

enum { EDITOR_ROTATION_ARC_STEPS = 6 };
static const float editor_rotation_arc_start = 4.0f;
static const float editor_rotation_arc_end = 22.0f;
static const float editor_rotation_arc_head = 7.0f;

void editor_rotation_control_arcs_get(Position center, Orientation orientation,
        float arm_length, float scale,
        Position strokes[EDITOR_ROTATION_ARC_STROKE_COUNT][2]) {
    if(strokes == NULL || scale <= 0.0f) return;
    size_t index = 0;
    for(int sign = -1; sign <= 1; sign += 2) {
        Position previous = editor_rotation_control_position_get(center,
            orientation + sign * editor_rotation_arc_start, arm_length);
        for(int step = 1; step <= EDITOR_ROTATION_ARC_STEPS; ++step) {
            float angle = orientation + sign * (editor_rotation_arc_start +
                (editor_rotation_arc_end - editor_rotation_arc_start) *
                step / EDITOR_ROTATION_ARC_STEPS);
            Position next = editor_rotation_control_position_get(center, angle, arm_length);
            strokes[index][0] = previous; strokes[index++][1] = next;
            previous = next;
        }
        float tangent = orientation + sign * (editor_rotation_arc_end + 90.0f);
        for(int side = -1; side <= 1; side += 2) {
            strokes[index][0] = previous;
            strokes[index++][1] = editor_rotation_control_position_get(previous,
                tangent + 180.0f + side * 30.0f, editor_rotation_arc_head / scale);
        }
    }
}

/* Rectangular stroke with flat ends; coordinates and width use the same units. */
bool editor_stroke_hit_check(Position pointer, Position start,
        Position end, float width) {
    Vec2D delta = {end.x - start.x, end.y - start.y};
    Vec2D offset = {pointer.x - start.x, pointer.y - start.y};
    float length_squared = delta.x * delta.x + delta.y * delta.y;
    float along = offset.x * delta.x + offset.y * delta.y;
    float across = offset.x * delta.y - offset.y * delta.x;
    return length_squared > 0.0f && along >= 0.0f && along <= length_squared &&
        fabsf(across) <= width * 0.5f * sqrtf(length_squared);
}

bool editor_rotation_control_arcs_begin(Position center, Orientation orientation,
        float arm_length, float scale, Position pointer, float radius,
        float *pointer_offset) {
    if(pointer_offset == NULL || scale <= 0.0f) return false;
    Position strokes[EDITOR_ROTATION_ARC_STROKE_COUNT][2];
    editor_rotation_control_arcs_get(center, orientation, arm_length, scale, strokes);
    for(size_t i = 0; i < EDITOR_ROTATION_ARC_STROKE_COUNT; ++i) {
        Position a = strokes[i][0], b = strokes[i][1];
        float dx = b.x - a.x, dy = b.y - a.y;
        float length_squared = dx * dx + dy * dy;
        float t = length_squared > 0.0f ?
            ((pointer.x - a.x) * dx + (pointer.y - a.y) * dy) / length_squared : 0.0f;
        t = fmaxf(0.0f, fminf(1.0f, t));
        if(editor_rotation_control_hit_check(pointer,
                (Position){a.x + t * dx, a.y + t * dy}, radius)) {
            *pointer_offset = orientation - editor_rotation_control_pointer_angle_get(center, pointer);
            return true;
        }
    }
    return false;
}

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
