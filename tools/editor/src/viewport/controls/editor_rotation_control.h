/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EDITOR_ROTATION_CONTROL_H
#define ROHR_EDITOR_ROTATION_CONTROL_H

#include "rohr.h"

Position editor_rotation_control_position_get(Position center,
    Orientation orientation, float arm_length);
bool editor_rotation_control_hit_check(Position pointer, Position control,
    float radius);
bool editor_rotation_control_begin(Position center, Orientation orientation,
    float arm_length, Position pointer, float radius, float *pointer_offset);
Orientation editor_rotation_control_orientation_get(Position center,
    Position pointer, float pointer_offset, Orientation previous);
Orientation editor_rotation_control_pointer_angle_get(Position center, Position pointer);
Position editor_rotation_control_screen_position_get(Position center,
    Orientation orientation, float arm_length);
bool editor_rotation_control_screen_begin(Position center, Orientation orientation,
    float arm_length, Position pointer, float radius, float *pointer_offset);

#endif
