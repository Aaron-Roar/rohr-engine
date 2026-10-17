/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef EDITOR_FORCE_GEOMETRY_H
#define EDITOR_FORCE_GEOMETRY_H
#include "editor_project.h"

/* Pure authored-scene queries. No runtime entities or allocations. */
bool editor_force_location_get(const EditorObject *object, ObjectForceLocation location, Position *position);
bool editor_force_orientation_get(const EditorObject *object, ObjectForceOrientation orientation, Orientation *angle);
bool editor_force_location_set(const EditorObject *object, ObjectForceLocation *location, Position position);
bool editor_force_orientation_set(const EditorObject *object, ObjectForceOrientation *orientation, Orientation angle);
bool editor_torque_target_position_get(const EditorObject *object, const EditorTorque *torque, Position *position);
Position editor_torque_marker_position_get(const EditorObject *object, const EditorTorque *torque);
enum { EDITOR_TORQUE_MARKER_STROKE_COUNT = 34 };
bool editor_torque_marker_strokes_get(const EditorObject *object,const EditorTorque *torque,float scale,
    Position strokes[EDITOR_TORQUE_MARKER_STROKE_COUNT][2]);
#endif
