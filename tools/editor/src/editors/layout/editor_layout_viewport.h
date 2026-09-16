/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef EDITOR_LAYOUT_VIEWPORT_H
#define EDITOR_LAYOUT_VIEWPORT_H

#include "editors/editor_mode_context.h"

typedef struct EditorLayoutViewportEditor {
    FontAsset *font;
    TextAsset name_label, x_label, y_label, width_label, height_label;
    TextAsset enabled_label, cameras_label, add_label, delete_label, remove_label;
    TextAsset layer_label, visible_label;
    TextAsset visible_icon, hidden_icon;
    TextAsset border_label, border_type_label, border_line_label;
    TextAsset border_hashed_label, border_thickness_label;
    TextAsset hash_spacing_label, corner_radius_label, border_color_label;
    TextAsset fill_color_label;
    TextAsset hover_border_color_label, hover_fill_color_label;
    TextAsset click_border_color_label, click_fill_color_label;
    TextAsset add_shape_label, add_text_label, button_label;
    TextAsset text_label, font_file_label, font_color_label;
    TextAsset default_font_label, load_font_label;
    TextAsset add_vertex_label, length_label;
    TextAsset width_scale_label, height_scale_label;
    TextAsset name_field, x_field, y_field, width_field, height_field, layer_field;
    TextAsset text_field, font_file_field, width_scale_field, height_scale_field;
    TextAsset length_field;
    TextAsset border_thickness_field, hash_spacing_field, corner_radius_field;
    TextAsset camera_names[EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX];
    char camera_cache[EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX][EDITOR_OBJECT_NAME_MAX];
    TextAsset ui_names[EDITOR_LAYOUT_VIEWPORT_UI_MAX];
    char ui_cache[EDITOR_LAYOUT_VIEWPORT_UI_MAX][EDITOR_OBJECT_NAME_MAX];
    TextAsset font_names[EDITOR_UI_FONT_MAX];
    char font_cache[EDITOR_UI_FONT_MAX][EDITOR_OBJECT_NAME_MAX];
} EditorLayoutViewportEditor;

bool editor_layout_viewport_editor_create(EditorLayoutViewportEditor *editor,
    FontAsset *font);
void editor_layout_viewport_editor_destroy(EditorLayoutViewportEditor *editor);
bool editor_layout_viewport_editor_draw(EditorLayoutViewportEditor *editor,
    const EditorModeContext *context);
bool editor_ui_shape_editor_draw(EditorLayoutViewportEditor *editor,
    const EditorModeContext *context);
bool editor_ui_text_editor_draw(EditorLayoutViewportEditor *editor,
    const EditorModeContext *context);
bool editor_ui_vertex_editor_draw(EditorLayoutViewportEditor *editor,
    const EditorModeContext *context);
bool editor_ui_line_editor_draw(EditorLayoutViewportEditor *editor,
    const EditorModeContext *context);

#endif
