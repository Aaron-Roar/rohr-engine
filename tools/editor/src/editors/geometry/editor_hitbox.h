/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef EDITOR_HITBOX_H
#define EDITOR_HITBOX_H

#include "editors/editor_mode_controls.h"
#include "editors/geometry/editor_auto_shape_editor.h"

typedef struct EditorHitboxEditor {
    FontAsset *font;
    TextAsset name_label;
    TextAsset auto_shape_label;
    TextAsset vertices_label;
    TextAsset lines_label;
    TextAsset visible_label;
    TextAsset hidden_label;
    TextAsset delete_label;
    TextAsset hitbox_names[EDITOR_BODY_HITBOX_MAX];
    EditorModeTextCache vertex_names;
    EditorModeTextCache line_names;
    char hitbox_cache[EDITOR_BODY_HITBOX_MAX][EDITOR_OBJECT_NAME_MAX];
    bool auto_shape_picker_open;
} EditorHitboxEditor;

bool editor_hitbox_editor_create(EditorHitboxEditor *editor, FontAsset *font);
void editor_hitbox_editor_destroy(EditorHitboxEditor *editor);
bool editor_hitbox_editor_draw(EditorHitboxEditor *editor,
    EditorAutoShapeEditor *auto_shape, const EditorModeContext *context);

#endif
