/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef EDITOR_CAMERA_H
#define EDITOR_CAMERA_H

#include "editors/editor_mode_context.h"

typedef struct EditorCameraEditor {
    FontAsset *font;
    TextAsset name_label, x_label, y_label, angle_label, width_label, height_label,
        zoom_label;
    TextAsset attachment_label, inherit_label, visible_label, none_label, delete_label;
    TextAsset name_values[EDITOR_CAMERA_MAX], target_names[256];
    TextAsset x_field, y_field, angle_field, width_field, height_field, zoom_field;
    char name_cache[EDITOR_CAMERA_MAX][EDITOR_OBJECT_NAME_MAX];
    char target_cache[256][EDITOR_OBJECT_NAME_MAX];
} EditorCameraEditor;

bool editor_camera_editor_create(EditorCameraEditor *editor, FontAsset *font);
void editor_camera_editor_destroy(EditorCameraEditor *editor);
bool editor_camera_editor_draw(EditorCameraEditor *editor,
    const EditorModeContext *context);

#endif
