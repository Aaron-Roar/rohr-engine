/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EDITOR_AUTO_SHAPE_EDITOR_H
#define ROHR_EDITOR_AUTO_SHAPE_EDITOR_H

#include "editors/editor_mode_context.h"

typedef struct EditorAutoShapeEditor {
    FontAsset *font;
    TextAsset triangle_label;
    TextAsset rectangle_label;
    TextAsset circle_label;
    TextAsset equilateral_label;
    TextAsset isosceles_label;
    TextAsset scalene_label;
    TextAsset width_label;
    TextAsset height_label;
    TextAsset length_label;
    TextAsset radius_label;
    TextAsset apex_offset_label;
    TextAsset first_field;
    TextAsset second_field;
    TextAsset third_field;
    EditorAutoShapeConfig config;
    bool first_was_active;
    bool second_was_active;
    bool third_was_active;
    /* Borrowed open flag and layout recorded only for the current UI frame. */
    bool *picker_open;
    UIRect picker_button_bounds;
    UIRect picker_bounds;
    UIRect picker_clip;
    float picker_translation_y;
} EditorAutoShapeEditor;

bool editor_auto_shape_editor_create(EditorAutoShapeEditor *editor,
    FontAsset *font);
void editor_auto_shape_editor_destroy(EditorAutoShapeEditor *editor);
bool editor_auto_shape_editor_draw(EditorAutoShapeEditor *editor,
    const EditorModeContext *context);
bool editor_auto_shape_editor_apply(EditorAutoShapeEditor *editor,
    EditorProject *project, EditorViewportState *viewport,
    EditorViewportMode parent_mode);
size_t editor_auto_shape_hitbox_points_capture(EditorViewportState *viewport,
    const EditorObject *object, const EditorRigidBody *body,
    const EditorHitbox *hitbox);
size_t editor_auto_shape_soft_body_points_capture(EditorViewportState *viewport,
    const EditorObject *object, const EditorSoftBody *body);
void editor_auto_shape_picker_frame_begin(EditorAutoShapeEditor *editor,
    float translation_y, UIRect clip);
void editor_auto_shape_picker_frame_end(EditorAutoShapeEditor *editor,
    Position pointer, MouseButtonState primary, bool escape);
int editor_auto_shape_picker_draw(EditorAutoShapeEditor *editor,
    const char *id_prefix, UIRect button_bounds, UIRect bounds,
    size_t point_count, bool *open);

#endif
